/*
 File: AudioConverterThread.cpp
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
#include "AudioConverter.h"

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

// libcue
extern "C"
{
#include <libcue/libcue.h>
}

QMutex AudioConverter::s_mutex;

//-----------------------------------------------------------------
AudioConverter::AudioConverter(const QFileInfo origin_info, const Utils::TranscoderConfiguration &configuration)
: ConverterThread{origin_info, configuration}
, m_libav_context        {nullptr}
, m_cover_stream_id      {-1}
, m_audio_decoder        {nullptr}
, m_audio_decoder_context{nullptr}
, m_cover_encoder        {nullptr}
, m_cover_encoder_context{nullptr}
, m_cover_decoder        {nullptr}
, m_cover_decoder_context{nullptr}
, m_frame                {nullptr}
, m_cover_frame          {nullptr}
, m_audio_stream_id      {-1}
{
}

//-----------------------------------------------------------------
AudioConverter::~AudioConverter()
{
}

//-----------------------------------------------------------------
void AudioConverter::run()
{
  auto music_file = m_source_info.absoluteFilePath().replace('/','\\');
  if(!init_libav())
  {
    emit error_message(QString("Error in LibAV init stage for '%1'").arg(music_file));
    return;
  }

  transcode();

  deinit_libav();
}

//-----------------------------------------------------------------
bool AudioConverter::init_libav()
{
  auto source_name = m_source_info.absoluteFilePath();
  auto source_name_string = source_name.replace('/','\\');

  int value = 0;

  value = avformat_open_input(&m_libav_context, source_name_string.toStdString().c_str(), nullptr, nullptr);
  if (value < 0)
  {
    emit error_message(QString("Couldn't open file: '%1'. Error is \"%2\".").arg(source_name).arg(av_error_string(value)));
    return false;
  }

  // avoids a warning message from libav when the duration can't be calculated accurately. this increases the lookup frames.
  m_libav_context->max_analyze_duration *= 1000;

  value = avformat_find_stream_info(m_libav_context, nullptr);
  if(value < 0)
  {
    emit error_message(QString("Couldn't get the information of '%1'. Error is \"%2\".").arg(source_name).arg(av_error_string(value)));
    deinit_libav();
    return false;
  }

  m_audio_stream_id = av_find_best_stream(m_libav_context, AVMEDIA_TYPE_AUDIO, -1, -1, &m_audio_decoder, 0);
  if (m_audio_stream_id < 0)
  {
    emit error_message(QString("Couldn't find any audio stream in '%1'. Error is \"%2\".").arg(source_name).arg(av_error_string(m_audio_stream_id)));
    deinit_libav();
    return false;
  }

  if(!m_audio_decoder)
  {
    // try to get the decoder using other ways
    m_audio_decoder = avcodec_find_decoder(m_libav_context->streams[m_audio_stream_id]->codec->codec_id);
    if (!m_audio_decoder)
    {
      emit error_message(QString("Couldn't find decoder for '%1'.").arg(source_name));
      deinit_libav();
      return false;
    }
  }

  m_audio_decoder_context = m_libav_context->streams[m_audio_stream_id]->codec;
  m_audio_decoder_context->codec = m_audio_decoder;

  if(m_libav_context->nb_streams != 1 && !Utils::isVideoFile(m_source_info) && m_configuration.extractMetadataCoverPicture())
  {
    init_libav_cover_transcoding();
  }

  value = avcodec_open2(m_audio_decoder_context, m_audio_decoder, nullptr);
  if (value < 0)
  {
    emit error_message(QString("Couldn't open decoder for '%1'. Error is \"%2\"").arg(source_name).arg(av_error_string(value)));
    deinit_libav();
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
    case AV_SAMPLE_FMT_U8:
      m_information.format = Sample_format::UNSIGNED_8;
      break;
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
    case AV_SAMPLE_FMT_U8P:
      m_information.format = Sample_format::UNSIGNED_8_PLANAR;
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
    default:
      break;
  }

  return true;
}

//-----------------------------------------------------------------
void AudioConverter::init_libav_cover_transcoding()
{
  auto source_name = m_source_info.absoluteFilePath();
  int value = 0;

  // try to guess if the other stream is the album cover picture.
  m_cover_stream_id = av_find_best_stream(m_libav_context, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
  if(m_cover_stream_id > 0)
  {
    auto cover_name = m_source_path + QString("Frontal.jpg");

    s_mutex.lock();
    if(!QFile::exists(cover_name))
    {
      // if there are several files with the same cover I just need one of the converters to dump the cover, not all of them.
      QFile file(cover_name);
      file.open(QIODevice::WriteOnly|QIODevice::Append);
      file.close();
    }
    else
    {
      // the rest of converters will ignore the cover stream.
      m_cover_stream_id = -1;
    }
    s_mutex.unlock();

    if(m_cover_stream_id > 0)
    {
      auto cover_codec_id = m_libav_context->streams[m_cover_stream_id]->codec->codec_id;
      if(cover_codec_id != AV_CODEC_ID_MJPEG)
      {
        av_init_packet(&m_cover_packet);
        m_cover_frame = av_frame_alloc();

        m_cover_decoder = avcodec_find_decoder(cover_codec_id);
        m_cover_decoder_context = m_libav_context->streams[m_cover_stream_id]->codec;

        if (!m_cover_decoder)
        {
          emit error_message(QString("Couldn't find decoder for cover in '%1'.").arg(source_name));
          m_cover_stream_id = -1;
        }
        else
        {
          value = avcodec_open2(m_cover_decoder_context, m_cover_decoder, nullptr);
          if (value < 0)
          {
            emit error_message(QString("Couldn't open decoder for cover in '%1'. Error is \"%2\"").arg(source_name).arg(av_error_string(value)));
            m_cover_stream_id = -1;
          }
        }

        m_cover_encoder = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
        m_cover_encoder_context = avcodec_alloc_context3(m_cover_encoder);

        if (!m_cover_encoder)
        {
          emit error_message(QString("Couldn't find encoder for cover in '%1'.").arg(source_name));
          m_cover_stream_id = -1;
        }
        else
        {
          value = avcodec_open2(m_cover_encoder_context, m_cover_encoder, nullptr);
          if (value < 0)
          {
            emit error_message(QString("Couldn't open encoder for '%1'. Error is \"%2\"").arg(source_name).arg(av_error_string(value)));
            m_cover_stream_id = -1;
          }
        }
      }
    }
  }
}

//-----------------------------------------------------------------
void AudioConverter::deinit_libav()
{
  avcodec_close(m_audio_decoder_context);
  av_frame_free(&m_frame);

  if(m_cover_decoder_context)
  {
    avcodec_close(m_cover_decoder_context);
    avcodec_close(m_cover_encoder_context);
    av_frame_free(&m_cover_frame);
  }

  if(m_libav_context)
  {
    avformat_close_input(&m_libav_context);
  }

  if(m_frame)
  {
    av_frame_free(&m_frame);
  }

  if(m_cover_frame)
  {
    av_frame_free(&m_cover_frame);
  }

  if(m_cover_encoder_context)
  {
    avcodec_free_context(&m_cover_encoder_context);
  }

  if(m_cover_decoder_context)
  {
    avcodec_free_context(&m_cover_decoder_context);
  }
}

//-----------------------------------------------------------------
QString AudioConverter::av_error_string(const int error_num) const
{
  char buffer[255];
  av_strerror(error_num, buffer, sizeof(buffer));
  return QString(buffer);
}

//-----------------------------------------------------------------
bool AudioConverter::encode_buffers(unsigned int buffer_start, unsigned int buffer_length)
{
  unsigned char *buffer1 = nullptr;
  unsigned char *buffer2 = nullptr;

  switch(m_audio_decoder_context->sample_fmt)
  {
    case AV_SAMPLE_FMT_S16:
    case AV_SAMPLE_FMT_FLT:
    case AV_SAMPLE_FMT_DBL:
      buffer1 = m_frame->data[0];
      break;
    case AV_SAMPLE_FMT_S16P:
    case AV_SAMPLE_FMT_S32:
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
void AudioConverter::transcode()
{
  open_next_destination_file();

  while(0 == av_read_frame(m_libav_context, &m_packet))
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
          return;
        }
      }
    }

    // dump the cover if the format is jpeg, if not a decoding-encoding phase must be applied.
    if(m_packet.stream_index == m_cover_stream_id)
    {
      if(!extract_cover_picture())
      {
        emit error_message(QString("Error encoding cover picture for file '%1.").arg(m_source_info.absoluteFilePath()));
        return;
      }
    }

    // You *must* call av_free_packet() after each call to av_read_frame() or else you'll leak memory
    av_free_packet(&m_packet);

    // stop and return if user has aborted the conversion.
    if(has_been_cancelled()) return;
  }

  // flush buffered frames from the decoder
  if (m_audio_decoder->capabilities & CODEC_CAP_DELAY)
  {
    av_init_packet(&m_packet);

    // Decode all the remaining frames in the buffer, until the end is reached
    int gotFrame = 0;
    int result = 0;
    while ((result = avcodec_decode_audio4(m_audio_decoder_context, m_frame, &gotFrame, &m_packet) >= 0) && gotFrame)
    {
      if(!encode_buffers(0, m_frame->nb_samples))
      {
        return;
      }
    }

    av_free_packet(&m_packet);
  }

  lame_encoder_flush();

  emit progress(100);

  close_destination_file();
}

//-----------------------------------------------------------------
bool AudioConverter::process_audio_packet()
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

        lame_encoder_flush();

        close_destination_file();

        open_next_destination_file();

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
bool AudioConverter::extract_cover_picture() const
{
  auto cover_name = m_source_path + QString("Frontal.jpg");

  QFile file(cover_name);
  file.open(QIODevice::WriteOnly|QIODevice::Append);

  if(m_cover_encoder)
  {
    AVPacket *packet = const_cast<AVPacket *>(&m_packet);             // remove the constness of the reference.
    AVPacket *cover_packet = const_cast<AVPacket *>(&m_cover_packet); // remove the constness of the reference.

    while(packet->size > 0)
    {
      int gotFrame = 0;
      int result = avcodec_decode_video2(m_cover_decoder_context, m_cover_frame, &gotFrame, packet);

      if (result >= 0 && gotFrame)
      {
        packet->size -= result;
        packet->data += result;

        int gotFrame2 = 0;
        int result2 = avcodec_encode_video2(m_cover_encoder_context, cover_packet, m_cover_frame, &gotFrame);

        if(result2 >= 0 && gotFrame2)
        {
          file.write(reinterpret_cast<const char *>(cover_packet->data), static_cast<long long int>(cover_packet->size));
        }
      }
    }
  }
  else
  {
    file.write(reinterpret_cast<const char *>(m_packet.data), static_cast<long long int>(m_packet.size));
  }

  file.close();

  return true;
}

//-----------------------------------------------------------------
QList<AudioConverter::Destination> AudioConverter::compute_destinations()
{
  QList<Destination> destinations;

  QStringList extensions;
  extensions << "*.cue";

  auto files = Utils::findFiles(QDir(m_source_path), extensions);
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
          auto track_clean_name = Utils::formatString(track_name, m_configuration.formatConfiguration());
          auto track_length = track_get_length(track);
          // convert to number of samples.
          track_length = m_information.samplerate * (track_length / CD_FRAMES_PER_SECOND);
          auto number_prefix = QString().number(i);

          while(number_prefix.length() < m_configuration.formatConfiguration().number_of_digits)
          {
            number_prefix = "0" + number_prefix;
          }

          auto final_name = number_prefix + QString(" ") + QString(m_configuration.formatConfiguration().number_and_name_separator) + QString(" ") + track_clean_name;

          destinations << Destination(final_name, track_length);
        }
      }
    }
  }
  else
  {
    destinations << Destination(Utils::formatString(m_source_info.absoluteFilePath(), m_configuration.formatConfiguration()), 0);
  }

  return destinations;
}
