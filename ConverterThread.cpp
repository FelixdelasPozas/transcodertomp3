/*
 File: ConverterThread.cpp
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
#include "ConverterThread.h"

// C++
#include <fstream>
#include <iostream>
#include <cstring>

// Qt
#include <QStringList>
#include <QDebug>

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


QMutex ConverterThread::s_mutex;

//-----------------------------------------------------------------
ConverterThread::ConverterThread(const QFileInfo origin_info)
: m_source_info          {origin_info}
, m_source_path          {m_source_info.absoluteFilePath().remove(m_source_info.absoluteFilePath().split('/').last())}
, m_num_tracks           {0}
, m_gfp                  {nullptr}
, m_stop                 {false}
, m_libav_context        {nullptr}
, m_audio_decoder        {nullptr}
, m_audio_decoder_context{nullptr}
, m_cover_encoder        {nullptr}
, m_cover_encoder_context{nullptr}
, m_cover_decoder        {nullptr}
, m_cover_decoder_context{nullptr}
, m_frame                {nullptr}
, m_cover_frame          {nullptr}
, m_audio_stream_id      {-1}
, m_cover_stream_id      {-1}
{
}

//-----------------------------------------------------------------
ConverterThread::~ConverterThread()
{
}

//-----------------------------------------------------------------
void ConverterThread::stop()
{
  emit information_message(QString("Converter for '%1' has been cancelled.").arg(m_source_info.absoluteFilePath()));
  m_stop = true;
}

//-----------------------------------------------------------------
bool ConverterThread::has_been_cancelled()
{
  return m_stop;
}

//-----------------------------------------------------------------
void ConverterThread::run()
{
  auto music_file = m_source_info.absoluteFilePath().replace('/','\\');

  if(!init_libav())
  {
    emit error_message(QString("Error in LibAV init stage for '%1'").arg(music_file));
    return;
  }

  m_destinations = compute_destinations();
  m_num_tracks   = m_destinations.size();

  transcode();

  if(m_stop)
  {
    auto mp3_file = m_source_path + m_destinations.first().name;
    QFile::remove(mp3_file);
  }

  deinit_libav();
}

//-----------------------------------------------------------------
bool ConverterThread::init_libav()
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

  if(m_libav_context->nb_streams != 1)
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

  return true;
}

//-----------------------------------------------------------------
void ConverterThread::init_libav_cover_transcoding()
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
void ConverterThread::deinit_libav()
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
QString ConverterThread::av_error_string(const int error_num) const
{
  char buffer[255];
  av_strerror(error_num, buffer, sizeof(buffer));
  return QString(buffer);
}

//-----------------------------------------------------------------
int ConverterThread::init_lame()
{
  Q_ASSERT(m_information.init);

  m_gfp = lame_init();

  lame_set_num_channels(m_gfp, m_information.num_channels);
  lame_set_in_samplerate(m_gfp, m_information.samplerate);
  lame_set_brate(m_gfp, 320);
  lame_set_quality(m_gfp, 0);
  lame_set_mode(m_gfp, m_information.num_channels == 2 ? MPEG_mode_e::STEREO : MPEG_mode_e::MONO);
  lame_set_bWriteVbrTag(m_gfp, 0);
  lame_set_copyright(m_gfp, 0);
  lame_set_original(m_gfp, 0);
  lame_set_preset(m_gfp, preset_mode_e::INSANE);

  return lame_init_params(m_gfp);
}

//-----------------------------------------------------------------
void ConverterThread::deinit_lame()
{
  lame_close(m_gfp);
}

//-----------------------------------------------------------------
bool ConverterThread::lame_encode(unsigned int buffer_start, unsigned int buffer_length)
{
  auto bytes_per_sample = av_get_bytes_per_sample(m_audio_decoder_context->sample_fmt);
  buffer_start *= bytes_per_sample;

  int output_bytes = 0;
  auto data_pointer = m_frame->data[0] + (buffer_start);
  auto extended_data_pointer0 = m_frame->extended_data[0] + (buffer_start);
  auto extended_data_pointer1 = m_frame->extended_data[1] + (buffer_start);

  switch(m_frame->format)
  {
    case AV_SAMPLE_FMT_S16:         // signed 16 bits
      output_bytes = lame_encode_buffer_interleaved(m_gfp, reinterpret_cast<short int *>(data_pointer), buffer_length, m_mp3_buffer, s_mp3_buffer_size);
      break;
    case AV_SAMPLE_FMT_FLT:         // float
      output_bytes = lame_encode_buffer_interleaved_ieee_float(m_gfp, reinterpret_cast<const float *>(data_pointer), buffer_length, m_mp3_buffer, s_mp3_buffer_size);
      break;
    case AV_SAMPLE_FMT_DBL:         // double
      output_bytes = lame_encode_buffer_interleaved_ieee_double(m_gfp, reinterpret_cast<const double *>(data_pointer), buffer_length, m_mp3_buffer, s_mp3_buffer_size);
      break;
    case AV_SAMPLE_FMT_S16P:        // signed 16 bits, planar
      output_bytes = lame_encode_buffer(m_gfp, reinterpret_cast<const short int *>(extended_data_pointer0), reinterpret_cast<const short int *>(extended_data_pointer1), buffer_length, m_mp3_buffer, s_mp3_buffer_size);
      break;
    case AV_SAMPLE_FMT_S32P:        // signed 32 bits, planar
      output_bytes = lame_encode_buffer_long2(m_gfp, reinterpret_cast<const long int *>(extended_data_pointer0), reinterpret_cast<const long int *>(extended_data_pointer1), buffer_length, m_mp3_buffer, s_mp3_buffer_size);
      break;
    case AV_SAMPLE_FMT_FLTP:        // float, planar
      output_bytes = lame_encode_buffer_ieee_float(m_gfp, reinterpret_cast<const float *>(extended_data_pointer0), reinterpret_cast<const float *>(extended_data_pointer1), buffer_length, m_mp3_buffer, s_mp3_buffer_size);
      break;
    case AV_SAMPLE_FMT_DBLP:        // double, planar
      output_bytes = lame_encode_buffer_ieee_double(m_gfp, reinterpret_cast<const double *>(extended_data_pointer0), reinterpret_cast<const double *>(extended_data_pointer1), buffer_length, m_mp3_buffer, s_mp3_buffer_size);
      break;
      // Unsupported formats
    case AV_SAMPLE_FMT_U8:          // unsigned 8 bits
    case AV_SAMPLE_FMT_U8P:         // unsigned 8 bits, planar
    case AV_SAMPLE_FMT_S32:         // signed 32 bits
    default:
      return false;
      break;
  }

  if (output_bytes < 0)
  {
    switch (output_bytes)
    {
      case -1:
        emit error_message(QString("Error in LAME code stage, mp3 buffer was too small."));
        break;
      case -2:
        emit error_message(QString("Error in LAME code stage, malloc() problem."));
        break;
      case -3:
        emit error_message(QString("Error in LAME code stage, lame_init_params() not called."));
        break;
      case -4:
        emit error_message(QString("Error in LAME code stage, psycho acoustic problems."));
        break;
      default:
        Q_ASSERT(false);
    }
  }

  if (output_bytes > 0)
  {
    m_mp3_file_stream.write(reinterpret_cast<char *>(&m_mp3_buffer), output_bytes);
  }

  return true;
}

//-----------------------------------------------------------------
void ConverterThread::transcode()
{
  auto destination = m_destinations.first();

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
    if(m_stop) return;
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
      if(!lame_encode_frame(0, m_frame->nb_samples))
      {
        return;
      }
    }

    av_free_packet(&m_packet);
  }

  auto flush_bytes = lame_encode_flush(m_gfp, m_mp3_buffer, s_mp3_buffer_size);
  if (flush_bytes != 0)
  {
    m_mp3_file_stream.write(reinterpret_cast<char *>(&m_mp3_buffer), flush_bytes);
  }

  emit progress(100);

  close_destination_file();
}

//-----------------------------------------------------------------
bool ConverterThread::process_audio_packet()
{
  // Try to decode the packet into a frame. Some frames rely on multiple packets, so we have to
  // make sure the frame is finished before we can use it.
  int gotFrame = 0;
  auto result = avcodec_decode_audio4(m_audio_decoder_context, m_frame, &gotFrame, &m_packet);

  if (result >= 0 && gotFrame)
  {
    m_packet.size -= result;
    m_packet.data += result;

    if(m_destinations.first().duration == 0)
    {
      if (!lame_encode_frame(0, m_frame->nb_samples))
      {
        return false;
      }
    }
    else
    {
      if (m_destinations.first().duration <= m_frame->nb_samples)
      {
        auto remaining_samples = m_destinations.first().duration;

        if (!lame_encode_frame(0, remaining_samples))
        {
          return false;
        }

        auto flush_bytes = lame_encode_flush(m_gfp, m_mp3_buffer, s_mp3_buffer_size);
        if (flush_bytes != 0)
        {
          m_mp3_file_stream.write(reinterpret_cast<char *>(&m_mp3_buffer), flush_bytes);
        }

        close_destination_file();

        open_next_destination_file();

        if(m_destinations.first().duration != 0)
        {
          m_destinations.first().duration -= m_frame->nb_samples - remaining_samples;
        }

        if (!lame_encode_frame(remaining_samples, m_frame->nb_samples - remaining_samples))
        {
          return false;
        }
      }
      else
      {
        m_destinations.first().duration -= m_frame->nb_samples;

        if (!lame_encode_frame(0, m_frame->nb_samples))
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
bool ConverterThread::lame_encode_frame(unsigned int buffer_start, unsigned int buffer_length)
{
  if (!lame_encode(buffer_start, buffer_length))
  {
    auto source_file = m_source_info.absoluteFilePath();
    emit error_message(QString("Error in encode phase for file '%1'. Unknown sample format, format is '%2'").arg(source_file).arg(QString(av_get_sample_fmt_name(m_audio_decoder_context->sample_fmt))));
    return false;
  }

  return true;
}

//-----------------------------------------------------------------
bool ConverterThread::extract_cover_picture() const
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
QList<ConverterThread::Destination> ConverterThread::compute_destinations()
{
  QList<Destination> destinations;

  QStringList extensions;
  extensions << "*.cue";

  auto files = Utils::findFiles(QDir(m_source_path), extensions);
  auto source_name = m_source_info.absoluteFilePath().split('/').last();
  auto source_cue_name1 = m_source_path + source_name + QString(".cue");
  auto source_basename = source_name.remove(source_name.split('.').last());
  auto source_cue_name2 = m_source_path + source_basename + QString("cue");

  if(!files.empty() && (QFile::exists(source_cue_name1) || QFile::exists(source_cue_name2)))
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
          auto track_clean_name = Utils::formatString(track_name, m_format_configuration);
          auto track_length = track_get_length(track);
          auto number_prefix = QString().number(i);

          while(number_prefix.length() < m_format_configuration.number_of_digits)
          {
            number_prefix = "0" + number_prefix;
          }

          auto final_name = number_prefix + QString(" ") + QString(m_format_configuration.number_and_name_separator) + QString(" ") + track_clean_name;

          destinations << Destination(final_name, track_length);
        }
      }
    }
  }
  else
  {
    destinations << Destination(Utils::formatString(m_source_info.absoluteFilePath(), m_format_configuration), 0);
  }

  return destinations;
}

//-----------------------------------------------------------------
bool ConverterThread::open_next_destination_file()
{
  Q_ASSERT(!m_mp3_file_stream.is_open() && !m_destinations.empty());

  std::memset(m_mp3_buffer, 0, s_mp3_buffer_size);

  auto destination = m_destinations.first();
  auto music_file = m_source_info.absoluteFilePath().replace('/','\\');
  if(0 != init_lame())
  {
    emit error_message(QString("Error in LAME library init stage for '%1'").arg(music_file));
    return false;
  }

  auto mp3_file = m_source_path + destination.name;
  m_mp3_file_stream.open(mp3_file.toStdString().c_str(), std::ios::trunc|std::ios::binary);

  if(!m_mp3_file_stream.is_open())
  {
    emit error_message(QString("Couldn't open destination file: %1") + mp3_file);
    return false;
  }

  auto source_name = m_source_info.absoluteFilePath().split('/').last();

  if(m_num_tracks != 1)
  {
    emit information_message(QString("%1: extracting %2").arg(source_name).arg(destination.name));
  }
  else
  {
    emit information_message(QString("%1: converting to %2").arg(source_name).arg(destination.name));
  }

  if(destination.duration != 0)
  {
    // convert cd frames to number of audio samples.
    m_destinations.first().duration = m_information.samplerate * (destination.duration / s_cd_frames_per_second);
  }

  return true;
}

//-----------------------------------------------------------------
void ConverterThread::close_destination_file()
{
  auto destination = m_destinations.first();
  auto mp3_file = m_source_path + destination.name;

  m_destinations.removeFirst();

  deinit_lame();
  m_mp3_file_stream.close();
  return;
}
