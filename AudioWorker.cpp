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

// Qt
#include <QStringList>

// libav
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

// taglib
#include <fileref.h>

// Qt
#include <QUuid>
#include <QTemporaryFile>

// libcue
extern "C"
{
#include <libcue/libcue.h>
}

QMutex AudioWorker::s_mutex;

//-----------------------------------------------------------------
AudioWorker::AudioWorker(const QFileInfo &origin_info, const Utils::TranscoderConfiguration &configuration)
: Worker{origin_info, configuration}
, m_libav_context        {nullptr}
, m_cover_stream_id      {-1}
, m_audio_decoder        {nullptr}
, m_audio_decoder_context{nullptr}
, m_frame                {nullptr}
, m_audio_stream_id      {-1}
{
}

//-----------------------------------------------------------------
AudioWorker::~AudioWorker()
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

  unsigned char *ioBuffer = reinterpret_cast<unsigned char *>(av_malloc(s_io_buffer_size)); // can get freed with av_free() by libav
  if(nullptr == ioBuffer)
  {
    emit error_message(QString("Couldn't allocate buffer for custom libav IO for file: '%1'.").arg(source_name));
    return false;
  }

  AVIOContext *avioContext = avio_alloc_context(ioBuffer, s_io_buffer_size - FF_INPUT_BUFFER_PADDING_SIZE, 0, reinterpret_cast<void*>(&m_input_file), &custom_IO_read, nullptr, &custom_IO_seek);
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

  int value = 0;

  value = avformat_open_input(&m_libav_context, "dummy", nullptr, nullptr);
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
    // try to get the decoder using other ways
    m_audio_decoder = avcodec_find_decoder(m_libav_context->streams[m_audio_stream_id]->codec->codec_id);
    if (!m_audio_decoder)
    {
      emit error_message(QString("Couldn't find decoder for '%1'.").arg(source_name));
      return false;
    }
  }

  m_audio_decoder_context = m_libav_context->streams[m_audio_stream_id]->codec;
  m_audio_decoder_context->codec = m_audio_decoder;

  if(m_libav_context->nb_streams != 1 && !Utils::isVideoFile(m_source_info) && m_configuration.extractMetadataCoverPicture())
  {
    init_libav_cover_extraction();
  }

  value = avcodec_open2(m_audio_decoder_context, m_audio_decoder, nullptr);
  if (value < 0)
  {
    emit error_message(QString("Couldn't open decoder for '%1'. Error is \"%2\"").arg(source_name).arg(av_error_string(value)));
    return false;
  }

  av_init_packet(&m_packet);
  m_frame = av_frame_alloc();

  m_information.init         = true;
  m_information.samplerate   = m_audio_decoder_context->sample_rate;
  m_information.num_channels = m_audio_decoder_context->channels;
  m_information.format       = Sample_format::UNDEFINED;

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
  m_cover_stream_id = av_find_best_stream(m_libav_context, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
  if(m_cover_stream_id > 0)
  {
    auto codec_id = m_libav_context->streams[m_cover_stream_id]->codec->codec_id;
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
        m_cover_extension = QString(".tiff");
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

  if(m_frame)
  {
    av_frame_free(&m_frame);
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
  while(0 == (value = av_read_frame(m_libav_context, &m_packet)))
  {
    emit progress(m_libav_context->pb->pos * 100 / m_source_info.size());

    // decode audio and encode it to mp3
    if (m_packet.stream_index == m_audio_stream_id)
    {
      // Audio packets can have multiple audio frames in a single packet
      while (m_packet.size > 0)
      {
        if(!process_audio_packet())
        {
          m_fail = true;
          return;
        }
      }
    }

    // dump the cover
    if(m_packet.stream_index == m_cover_stream_id)
    {
      if(!extract_cover_picture())
      {
        emit error_message(QString("Error extracting cover picture for file '%1.").arg(m_source_info.absoluteFilePath()));
      }
    }

    // You *must* call av_free_packet() after each call to av_read_frame() or else you'll leak memory
    av_free_packet(&m_packet);

    // stop and return if user has aborted the conversion.
    if(has_been_cancelled()) return;
  }

  if(value < 0 && value != AVERROR_EOF)
  {
    emit error_message(QString("Error reading input file '%1. %2").arg(m_source_info.absoluteFilePath()).arg(av_error_string(value)));
    return;
  }

  // flush buffered frames from the decoder
  if (m_audio_decoder->capabilities & CODEC_CAP_DELAY)
  {
    av_init_packet(&m_packet);

    // Decode all the remaining frames in the buffer, until the end is reached
    int gotFrame = 0;
    while ((avcodec_decode_audio4(m_audio_decoder_context, m_frame, &gotFrame, &m_packet) >= 0) && gotFrame)
    {
      if(!encode_buffers(0, m_frame->nb_samples))
      {
        return;
      }
    }

    av_free_packet(&m_packet);
  }

  close_destination_file();
}

//-----------------------------------------------------------------
bool AudioWorker::process_audio_packet()
{
  // Try to decode the packet into a frame. Some frames rely on multiple packets, so we have to
  // make sure the frame is finished before we can use it.
  int gotFrame = 0;
  auto result = avcodec_decode_audio4(m_audio_decoder_context, m_frame, &gotFrame, &m_packet);

  if (result >= 0 && gotFrame)
  {
    m_packet.size -= result;
    m_packet.data += result;

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
  else
  {
    m_packet.size = 0;
    m_packet.data = nullptr;
  }

  return true;
}

//-----------------------------------------------------------------
bool AudioWorker::extract_cover_picture() const
{
  auto cover_name = m_source_path + m_configuration.coverPictureName() + m_cover_extension;

  QFile file(cover_name);
  if(!file.open(QIODevice::WriteOnly|QIODevice::Append))
  {
    emit error_message(QString("Couldn't create cover picture file for '%1', check file permissions.").arg(m_source_info.absoluteFilePath()));
    return false;
  }

  file.write(reinterpret_cast<const char *>(m_packet.data), static_cast<long long int>(m_packet.size));
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
  auto source_name = m_source_info.absoluteFilePath().split('/').last();
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
          auto track_length = track_get_length(track);

          // convert to number of samples.
          track_length = m_information.samplerate * (track_length / CD_FRAMES_PER_SECOND);

          destinations << Destination(track_clean_name, track_length);
        }
      }
    }
  }

  if(destinations.empty())
  {
    QString file_name;
    auto file_extension = m_source_info.absoluteFilePath().split('.').last();
    auto file_metadata = TagLib::FileRef(m_source_info.absoluteFilePath().toStdString().c_str());

    // try the hard way
    if(file_metadata.isNull())
    {
      auto id = QUuid::createUuid();
      QTemporaryFile temp_file(id.toString());
      QFile original_file(m_source_info.absoluteFilePath());

      if(temp_file.open() && original_file.open(QFile::ReadOnly))
      {
        temp_file.write(original_file.readAll());
        original_file.close();
        temp_file.flush();
        temp_file.close();
        temp_file.rename(temp_file.fileName() + file_extension);

        file_metadata = TagLib::FileRef(temp_file.fileName().toStdString().c_str());
        file_name = parse_metadata(file_metadata.tag());
      }
      original_file.close();
      temp_file.remove();
    }
    else
    {
      file_name = parse_metadata(file_metadata.tag());
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
QString AudioWorker::parse_metadata(const TagLib::Tag *tags)
{
  QString track_title;

  // track number
  auto track_number = tags->track();
  if (track_number != 0)
  {
    auto number_string = QString().number(track_number);

    track_title += QString().number(track_number) + QString(" - ");
  }

  // track title
  TagLib::String temp;
  auto title = QString::fromStdWString(tags->title().toWString());

  if (!title.isEmpty() && !Utils::isSpaces(title))
  {
    title.replace(QDir::separator(), QChar('-'));
    title.replace(QChar('/'), QChar('-'));
    track_title += title;
  }
  else
  {
    track_title.clear();
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
