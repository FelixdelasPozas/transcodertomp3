/*
 File: AudioWorker.cpp
 Created on: 18/4/2015
 Author: Felix de las Pozas Alvarez

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// Project
#include "AudioWorker.h"

// C++
#include <iostream>
#include <bitset>

// Qt
#include <QStringList>

// libav
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

// tagparser
#include <tagparser/mediafileinfo.h>
#include <tagparser/diagnostics.h>
#include <tagparser/tag.h>
#include <tagparser/tagvalue.h>
#include <tagparser/progressfeedback.h>

// Qt
#include <QUuid>
#include <QTemporaryFile>

// libcue
extern "C"
{
#include <libcue.h>
}

QMutex AudioWorker::s_mutex;

//-----------------------------------------------------------------
AudioWorker::AudioWorker(const QFileInfo &origin_info, const Utils::TranscoderConfiguration &configuration)
: Worker(origin_info, configuration)
, m_libav_context        {nullptr}
, m_packet               {nullptr}
, m_cover_stream_id      {-1}
, m_audio_decoder        {nullptr}
, m_audio_decoder_context{nullptr}
, m_frame                {nullptr}
, m_audio_stream_id      {-1}
{
}

//-----------------------------------------------------------------
void AudioWorker::run_implementation()
{
  if(init_libav())
  {
    transcode();
  }
  else
  {
    m_fail = true;
  }

  deinit_libav();
}

//-----------------------------------------------------------------
bool AudioWorker::init_libav()
{
  QMutexLocker lock(&s_mutex);

  av_register_all();

  auto source_name = m_source_info.absoluteFilePath();
  m_input_file.setFileName(source_name);
  if(!m_input_file.open(QIODevice::ReadOnly))
  {
    emit error_message(QString("Couldn't open input file '%1'.").arg(source_name));
    return false;
  }

  auto ioBuffer = reinterpret_cast<unsigned char *>(av_malloc(s_io_buffer_size)); // can get freed with av_free() by libav
  if(nullptr == ioBuffer)
  {
    emit error_message(QString("Couldn't allocate buffer for custom libav IO for file: '%1'.").arg(source_name));
    return false;
  }

  auto avioContext = avio_alloc_context(ioBuffer, s_io_buffer_size - AV_INPUT_BUFFER_PADDING_SIZE, 0, reinterpret_cast<void*>(&m_input_file), &custom_IO_read, nullptr, &custom_IO_seek);
  if(nullptr == avioContext)
  {
    emit error_message(QString("Couldn't allocate context for custom libav IO for file: '%1'.").arg(source_name));
    return false;
  }

  avioContext->seekable = 0;
  avioContext->write_flag = 0;

  m_libav_context = avformat_alloc_context();
  m_libav_context->pb = avioContext;
  m_libav_context->flags |= AVFMT_FLAG_CUSTOM_IO;

  auto value = avformat_open_input(&m_libav_context, "dummy", nullptr, nullptr);
  if (value < 0)
  {
    emit error_message(QString("Couldn't open file: '%1' with libav. Error is \"%2\"").arg(source_name).arg(av_error_string(value)));
    return false;
  }

  // avoids a warning message from libav when the duration can't be calculated accurately. this increases the lookup frames.
  m_libav_context->max_analyze_duration *= 1000;

  value = avformat_find_stream_info(m_libav_context, nullptr);
  if(value < 0)
  {
    emit error_message(QString("Couldn't get the information of '%1'. Error is \"%2\".").arg(source_name).arg(av_error_string(value)));
    return false;
  }

  m_audio_stream_id = av_find_best_stream(m_libav_context, AVMEDIA_TYPE_AUDIO, -1, -1, &m_audio_decoder, 0);
  if (m_audio_stream_id < 0)
  {
    emit error_message(QString("Couldn't find any audio stream in '%1'. Error is \"%2\".").arg(source_name).arg(av_error_string(m_audio_stream_id)));
    return false;
  }

  if(!m_audio_decoder)
  {
    emit error_message(QString("Couldn't find decoder for '%1'.").arg(source_name));
    return false;
  }

  if(m_libav_context->nb_streams != 1 && !Utils::isVideoFile(m_source_info) && m_configuration.extractMetadataCoverPicture())
  {
    init_libav_cover_extraction();
  }

  // NOTE: the use of streams is deprecated but I couldn't find another way to correctly init a
  // decoder context with the correct parameters found during av_format_find_info(). The method:
  // avcodec_alloc_context3(const AVCodec *codec) returns an uninitalized context that fails in
  // the send_packet() API. Also, in the examples it's done this way. Why if it's being deprecated?
  m_audio_decoder_context = m_libav_context->streams[m_audio_stream_id]->codec;

  value = avcodec_open2(m_audio_decoder_context, m_audio_decoder, nullptr);
  if (value < 0 || !avcodec_is_open(m_audio_decoder_context))
  {
    emit error_message(QString("Couldn't open decoder for '%1'. Error is \"%2\"").arg(source_name).arg(av_error_string(value)));
    return false;
  }

  m_packet = av_packet_alloc();
  m_frame  = av_frame_alloc();

  m_information.init         = true;
  m_information.samplerate   = m_audio_decoder_context->sample_rate;
  m_information.num_channels = m_audio_decoder_context->channels;
  m_information.format       = Sample_format::UNDEFINED;
  m_information.isFlac       = m_source_info.fileName().endsWith("flac", Qt::CaseInsensitive);

  switch(m_audio_decoder_context->sample_fmt)
  {
    case AV_SAMPLE_FMT_S16:
      m_information.format = Sample_format::SIGNED_16;
      break;
    case AV_SAMPLE_FMT_S32:
      m_information.format = Sample_format::SIGNED_32;
      break;
    case AV_SAMPLE_FMT_FLT:
      m_information.format = Sample_format::FLOAT;
      break;
    case AV_SAMPLE_FMT_DBL:
      m_information.format = Sample_format::DOUBLE;
      break;
    case AV_SAMPLE_FMT_S16P:
      m_information.format = Sample_format::SIGNED_16_PLANAR;
      break;
    case AV_SAMPLE_FMT_S32P:
      m_information.format = Sample_format::SIGNED_32_PLANAR;
      break;
    case AV_SAMPLE_FMT_FLTP:
      m_information.format = Sample_format::FLOAT_PLANAR;
      break;
    case AV_SAMPLE_FMT_DBLP:
      m_information.format = Sample_format::DOUBLE_PLANAR;
      break;
    default: // unsupported sample formats.
    case AV_SAMPLE_FMT_U8P:
    case AV_SAMPLE_FMT_U8:
      emit error_message(QString("Couldn't transcode '%1', because it has an unsupported sample format (8 bits).").arg(source_name));
      return false;
      break;
  }

  return true;
}

//-----------------------------------------------------------------
void AudioWorker::init_libav_cover_extraction()
{
  // try to guess if the other stream is the album cover picture.
  AVCodec *picture_codec = nullptr;
  m_cover_stream_id = av_find_best_stream(m_libav_context, AVMEDIA_TYPE_VIDEO, -1, -1, &picture_codec, 0);
  if(m_cover_stream_id > 0)
  {
    auto codec_id = picture_codec->id;
    switch(codec_id)
    {
      case AV_CODEC_ID_MJPEG:
        m_cover_extension = QString(".jpg");
        break;
      case AV_CODEC_ID_PNG:
        m_cover_extension = QString(".png");
        break;
      case AV_CODEC_ID_BMP:
        m_cover_extension = QString(".bmp");
        break;
      case AV_CODEC_ID_TIFF:
        m_cover_extension = QString(".tif");
        break;
      default:
        m_cover_extension = QString(".picture_unknown_format");
        break;
    }

    auto cover_name = m_source_path + m_configuration.coverPictureName() + m_cover_extension;

    if(!QFile::exists(cover_name))
    {
      // if there are several files with the same cover I just need one of the workers to dump the cover, not all of them.
      QFile file(cover_name);
      file.open(QIODevice::WriteOnly|QIODevice::Append);
      file.close();
    }
    else
    {
      // the rest of workers will ignore the cover stream.
      m_cover_stream_id = -1;
    }
  }
}

//-----------------------------------------------------------------
void AudioWorker::deinit_libav()
{
  if(m_input_file.isOpen())
  {
    m_input_file.close();
  }

  if(m_audio_decoder_context)
  {
    avcodec_close(m_audio_decoder_context);
  }

  if(m_frame)
  {
    av_frame_free(&m_frame);
  }

  if(m_libav_context)
  {
    av_free(m_libav_context->pb->buffer);
    avformat_close_input(&m_libav_context);
  }

  if(m_packet)
  {
    av_packet_free(&m_packet);
  }
}

//-----------------------------------------------------------------
QString AudioWorker::av_error_string(const int error_num) const
{
  char buffer[255];
  av_strerror(error_num, buffer, sizeof(buffer));
  return QString(buffer);
}

//-----------------------------------------------------------------
bool AudioWorker::encode_buffers(unsigned int buffer_start, unsigned int buffer_length)
{
  unsigned char *buffer1 = nullptr;
  unsigned char *buffer2 = nullptr;

  switch(m_audio_decoder_context->sample_fmt)
  {
    case AV_SAMPLE_FMT_S16:
    case AV_SAMPLE_FMT_S32:
    case AV_SAMPLE_FMT_FLT:
    case AV_SAMPLE_FMT_DBL:
      buffer1 = m_frame->data[0];
      break;
    case AV_SAMPLE_FMT_S16P:
    case AV_SAMPLE_FMT_FLTP:
    case AV_SAMPLE_FMT_DBLP:
      buffer1 = m_frame->extended_data[0];
      buffer2 = m_frame->extended_data[1];
      break;
    default:
      break;
  }

  return encode(buffer_start, buffer_length, buffer1, buffer2);
}

//-----------------------------------------------------------------
void AudioWorker::transcode()
{
  if(!open_next_destination_file())
  {
    m_fail = true;
    return;
  }

  int value;
  int progressVal = 0;
  while(0 == (value = av_read_frame(m_libav_context, m_packet)))
  {
    const int currentProgress = m_libav_context->pb->pos * 100 / m_source_info.size();
    if(progressVal != currentProgress)
    {
      progressVal = currentProgress;
      emit progress(progressVal);
    }

    // decode audio and encode it to mp3
    if (m_packet->stream_index == m_audio_stream_id)
    {
      // Audio packets can have multiple audio frames in a single packet
      while (m_packet->size > 0)
      {
        if(!process_audio_packet())
        {
          m_fail = true;
          return;
        }
      }
    }

    // dump the cover
    if(m_packet->stream_index == m_cover_stream_id)
    {
      if(!extract_cover_picture())
      {
        emit error_message(QString("Error extracting cover picture for file '%1.").arg(m_source_info.absoluteFilePath()));
      }
    }

    m_packet->size = 0;
    m_packet->data = nullptr;

    // stop and return if user has aborted the conversion.
    if(has_been_cancelled()) return;
  }

  if(value < 0 && value != AVERROR_EOF)
  {
    emit error_message(QString("Error reading input file '%1. %2").arg(m_source_info.absoluteFilePath()).arg(av_error_string(value)));
    return;
  }

  // flush buffered frames from the decoder
  if (m_audio_decoder->capabilities & AV_CODEC_CAP_DELAY)
  {
    m_packet = av_packet_alloc();

    if(!process_audio_packet())
    {
      m_fail = true;
      return;
    }

    m_packet->data = nullptr;
    m_packet->size = 0;
  }

  close_destination_file();
}

//-----------------------------------------------------------------
bool AudioWorker::process_audio_packet()
{
  if(m_information.isFlac && m_packet && m_packet->data)
  {
    // apparently flac metadata is passed as audio stream info. discard those frames.
    const auto temp = reinterpret_cast<uint8_t *>(m_packet->data)[0];
    if(temp != 0xFF)
    {
      m_packet->size = 0;
      m_packet->data = nullptr;
      return true;
    }
  }

  // Try to decode the packet into a frame/multiple frames.
  auto result = avcodec_send_packet(m_audio_decoder_context, m_packet);

  if(result != 0)
  {
    if(result == AVERROR(EAGAIN)) return true;

    emit error_message(QString("Error sending packet to audio decoder. Input file '%1. %2").arg(m_source_info.absoluteFilePath()).arg(av_error_string(result)));
    return false;
  }

  while(0 == (result = avcodec_receive_frame(m_audio_decoder_context, m_frame)))
  {
    m_packet->size -= result;
    m_packet->data += result;

    if(destination().duration == 0)
    {
      if (!encode_buffers(0, m_frame->nb_samples))
      {
        return false;
      }
    }
    else
    {
      if (destination().duration <= m_frame->nb_samples)
      {
        auto remaining_samples = destination().duration;

        if (!encode_buffers(0, remaining_samples))
        {
          return false;
        }

        close_destination_file();

        if(!open_next_destination_file())
        {
          return false;
        }

        if(destination().duration != 0)
        {
          destination().duration -= m_frame->nb_samples - remaining_samples;
        }

        if (!encode_buffers(remaining_samples, m_frame->nb_samples - remaining_samples))
        {
          return false;
        }
      }
      else
      {
        destination().duration -= m_frame->nb_samples;

        if (!encode_buffers(0, m_frame->nb_samples))
        {
          return false;
        }
      }
    }
  }

  if(result == AVERROR_EOF || result == AVERROR(EAGAIN))
  {
    m_packet->size = 0;
    m_packet->data = nullptr;
  }
  else
  {
    emit error_message(QString("Error reading input file '%1. %2").arg(m_source_info.absoluteFilePath()).arg(av_error_string(result)));
    return false;
  }

  return true;
}

//-----------------------------------------------------------------
bool AudioWorker::extract_cover_picture() const
{
  auto cover_name = m_source_path + m_configuration.coverPictureName() + m_cover_extension;

  QFile file(cover_name);
  if(!file.open(QIODevice::WriteOnly|QIODevice::Unbuffered|QIODevice::Append))
  {
    emit error_message(QString("Couldn't create cover picture file for '%1', check file permissions.").arg(m_source_info.absoluteFilePath()));
    return false;
  }

  file.write(reinterpret_cast<const char *>(m_packet->data), static_cast<long long int>(m_packet->size));
  file.flush();
  file.close();

  return true;
}

//-----------------------------------------------------------------
QList<AudioWorker::Destination> AudioWorker::compute_destinations()
{
  QList<Destination> destinations;

  QStringList filter;
  filter << "*.cue";

  auto files = Utils::findFiles(QDir(m_source_path), filter);
  auto source_name = QDir::fromNativeSeparators(m_source_info.absoluteFilePath()).split('/').last();
  auto source_cue_name1 = m_source_path + source_name + QString(".cue");
  auto source_basename = source_name.remove(source_name.split('.').last());
  auto source_cue_name2 = m_source_path + source_basename + QString("cue");

  if(m_configuration.useCueToSplit() && !files.empty() && (QFile::exists(source_cue_name1) || QFile::exists(source_cue_name2)))
  {
    auto name = source_cue_name1;
    if(!QFile::exists(source_cue_name1))
    {
      name = source_cue_name2;
    }

    QFile cue_file(name);

    if(!cue_file.open(QIODevice::ReadOnly))
    {
      emit error_message(QString("Error opening cue file '%1'.").arg(name));
    }
    else
    {
      auto content = cue_file.readAll();
      auto cd = cue_parse_string(content.constData());
      if(cd == nullptr)
      {
        emit error_message(QString("Error parsing cue file '%1'.").arg(name));
      }
      else
      {
        auto num_tracks = cd_get_ntrack(cd);

        for(int i = 1; i < num_tracks + 1; ++i)
        {
          auto track = cd_get_track(cd, i);

          if(track_get_mode(track) != MODE_AUDIO)
          {
            continue;
          }

          auto cdtext = track_get_cdtext(track);
          auto track_name = QString(cdtext_get(PTI_TITLE, cdtext));
          track_name.replace(QChar('/'), QChar('-'));
          track_name.replace(QDir::separator(), QChar('-'));
          auto track_clean_name = Utils::formatString(QString().number(i) + QString(" ") + track_name, m_configuration.formatConfiguration());
          auto pregap = track_get_zero_pre(track);
          auto postgap = track_get_zero_post(track);
          auto track_length = track_get_length(track) + (pregap != -1 ? pregap:0) + (postgap != -1 ? postgap:0);

          // convert to number of samples.
          track_length = m_information.samplerate * (track_length / CD_FRAMES_PER_SECOND);

          if(i == num_tracks) track_length = 0; // encode last track till the end of data.

          destinations << Destination(track_clean_name, track_length);
        }
      }
    }
  }

  if(destinations.empty())
  {
    QString file_name;

    if(m_configuration.useMetadataToRenameOutput())
    {
      const auto shortName = Utils::shortFileName(QDir::toNativeSeparators(m_source_info.absoluteFilePath()));
      TagParser::MediaFileInfo fileInfo(shortName);
      fileInfo.setForceFullParse(true);

      TagParser::Diagnostics diag;
      TagParser::AbortableProgressFeedback progressFeedback;

      try
      {
        fileInfo.open();
        if(fileInfo.isOpen())
        {
          fileInfo.parseEverything(diag, progressFeedback);

          if(!fileInfo.tags().empty())
          {
            auto tags = fileInfo.tags().at(0);
            tags->ensureTextValuesAreProperlyEncoded();

            file_name = parse_metadata(tags);
          }
        }
        else
        {
          emit error_message(tr("Unable to parse tags from: %1.").arg(QDir::toNativeSeparators(m_source_info.absoluteFilePath())));
        }
      }
      catch(...)
      {
        for(auto error: diag)
        {
          const auto message = error.context() + " -> " + error.message();
          emit error_message(tr("Error processing file: %1 (%2)").arg(m_source_info.absoluteFilePath()).arg(QString::fromStdString(message)));
        }
      }

      if(fileInfo.isOpen()) fileInfo.close();
    }

    if(file_name.isEmpty())
    {
      file_name = m_source_info.absoluteFilePath();
    }

    destinations << Destination(Utils::formatString(file_name, m_configuration.formatConfiguration()), 0);
  }

  return destinations;
}

//-----------------------------------------------------------------
QString AudioWorker::parse_metadata(const TagParser::Tag *tags)
{
  QString track_title;

  if(!tags) return track_title;

  if(m_configuration.formatConfiguration().prefix_disk_num && tags->hasField(TagParser::KnownField::DiskPosition))
  {
    try
    {
      const auto disc_num = tags->value(TagParser::KnownField::DiskPosition).toPositionInSet().position();
      if(disc_num != 0)
      {
        track_title += QString::number(disc_num) + QString("-");
      }
    }
    catch(const TagParser::Failure &e)
    {
      emit error_message(tr("Error processing tags from file: %1 (%2)").arg(m_source_info.absoluteFilePath()).arg(QString::fromLocal8Bit(e.what())));
    }
    catch(...)
    {
      emit error_message(tr("Error processing tags from file: %1").arg(m_source_info.absoluteFilePath()));
    }
  }

  // track number
  if (tags->hasField(TagParser::KnownField::TrackPosition) || tags->hasField(TagParser::KnownField::PartNumber))
  {
    try
    {
      int track_number{0};
      if(tags->hasField(TagParser::KnownField::TrackPosition))
      {
        track_number = tags->value(TagParser::KnownField::TrackPosition).toInteger();
      }
      else
      {
        track_number = tags->value(TagParser::KnownField::PartNumber).toInteger();
      }

      if (track_number != 0)
      {
        auto number_string = QString::number(track_number);
        while (m_configuration.formatConfiguration().number_of_digits > number_string.length())
        {
          number_string = "0" + number_string;
        }

        track_title += number_string + QString(" - ");
      }
    }
    catch(const TagParser::Failure &e)
    {
      emit error_message(tr("Error processing tags from file: %1 (%2)").arg(m_source_info.absoluteFilePath()).arg(QString::fromLocal8Bit(e.what())));
    }
    catch(...)
    {
      emit error_message(tr("Error processing tags from file: %1").arg(m_source_info.absoluteFilePath()));
    }
  }
  else
  {
    if (tags->hasField(TagParser::KnownField::Artist))
    {
      try
      {
        const auto artist = tags->value(TagParser::KnownField::Artist).toString(TagParser::TagTextEncoding::Utf8);
        track_title += QString::fromStdString(artist) + QString(" - ");
      }
      catch(const TagParser::Failure &e)
      {
        emit error_message(tr("Error processing tags from file: %1 (%2)").arg(m_source_info.absoluteFilePath()).arg(QString::fromLocal8Bit(e.what())));
      }
      catch(...)
      {
        emit error_message(tr("Error processing tags from file: %1").arg(m_source_info.absoluteFilePath()));
      }
    }
  }

  // track title
  try
  {
    const auto title = tags->value(TagParser::KnownField::Title).toString(TagParser::TagTextEncoding::Utf8);
    auto qTitle = QString::fromStdString(title);

    if (!qTitle.isEmpty() && !Utils::isSpaces(qTitle))
    {
      qTitle.replace(QDir::separator(), QChar('-'));
      qTitle.replace(QChar('/'), QChar('-'));
      track_title += qTitle;
    }
    else
    {
      track_title.clear();
    }
  }
  catch(const TagParser::Failure &e)
  {
    emit error_message(tr("Error processing tags from file: %1 (%2)").arg(m_source_info.absoluteFilePath()).arg(QString::fromLocal8Bit(e.what())));
  }
  catch(...)
  {
    emit error_message(tr("Error processing tags from file: %1").arg(m_source_info.absoluteFilePath()));
  }

  return track_title;
}

//-----------------------------------------------------------------
int AudioWorker::custom_IO_read(void* opaque, unsigned char* buffer, int buffer_size)
{
  auto reader = reinterpret_cast<QFile *>(opaque);
  return reader->read(reinterpret_cast<char *>(buffer), buffer_size);
}

//-----------------------------------------------------------------
long long int AudioWorker::custom_IO_seek(void* opaque, long long int offset, int whence)
{
  auto reader = reinterpret_cast<QFile *>(opaque);
  switch(whence)
  {
    case AVSEEK_SIZE:
      return reader->size();
    case SEEK_SET:
      return reader->seek(offset);
    case SEEK_END:
      return reader->seek(reader->size());
    case SEEK_CUR:
      return reader->pos();
      break;
    default:
      Q_ASSERT(false);
  }
  return 0;
}
