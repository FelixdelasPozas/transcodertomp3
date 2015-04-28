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

  if(0 != init_lame())
  {
    emit error_message(QString("Error in LAME library init stage for '%1'").arg(music_file));
    return;
  }

  for(auto destiny: compute_destinations())
  {
    auto mp3_file = m_source_path + destiny.name;
    m_mp3_file_stream.open(mp3_file.toStdString().c_str(), std::ios::trunc|std::ios::binary);

    if(!m_mp3_file_stream.is_open())
    {
      emit error_message(QString("Couldn't open destination file: %1") + mp3_file);
      return;
    }

    auto source_name = m_source_info.absoluteFilePath().split('/').last();
    emit information_message(QString("%1 -> %2").arg(source_name).arg(destiny.name));

    transcode();
  }


  deinit_libav();
  deinit_lame();
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
    // try to get it using other ways
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

  // try to guess if the other stream is the audio cover picture.
  m_cover_stream_id = av_find_best_stream(m_libav_context, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
  if(m_cover_stream_id > 0)
  {
    auto cover_name = m_source_path + QString("Frontal.jpg");

    s_mutex.lock();
    if(!QFile::exists(cover_name))
    {
      // if there are several files with the same cover i just need one of the coverters to dump the cover, not all of them.
      QFile file(cover_name);
      file.open(QIODevice::WriteOnly|QIODevice::Append);
      file.close();
    }
    else
    {
      // for the rest of converters there is no cover stream.
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
  m_mp3_file_stream.close();

  if(m_stop)
  {
    for(auto destiny: compute_destinations())
    {
      auto mp3_file = m_source_path + destiny.name;
      QFile::remove(mp3_file);
    }
  }
}

//-----------------------------------------------------------------
bool ConverterThread::lame_encode()
{
  int output_bytes = 0;
  switch(m_frame->format)
  {
    case AV_SAMPLE_FMT_S16:         // signed 16 bits
      output_bytes = lame_encode_buffer_interleaved(m_gfp, reinterpret_cast<short int *>(m_frame->data[0]), m_frame->nb_samples, m_mp3_buffer, 8480);
      break;
    case AV_SAMPLE_FMT_FLT:         // float
      output_bytes = lame_encode_buffer_interleaved_ieee_float(m_gfp, reinterpret_cast<const float *>(m_frame->data[0]), m_frame->nb_samples, m_mp3_buffer, 8480);
      break;
    case AV_SAMPLE_FMT_DBL:         // double
      output_bytes = lame_encode_buffer_interleaved_ieee_double(m_gfp, reinterpret_cast<const double *>(m_frame->data[0]), m_frame->nb_samples, m_mp3_buffer, 8480);
      break;
    case AV_SAMPLE_FMT_S16P:        // signed 16 bits, planar
      output_bytes = lame_encode_buffer(m_gfp, reinterpret_cast<const short int *>(m_frame->extended_data[0]), reinterpret_cast<const short int *>(m_frame->extended_data[1]), m_frame->nb_samples, m_mp3_buffer, 8480);
      break;
    case AV_SAMPLE_FMT_S32P:        // signed 32 bits, planar
      output_bytes = lame_encode_buffer_long2(m_gfp, reinterpret_cast<const long int *>(m_frame->extended_data[0]), reinterpret_cast<const long int *>(m_frame->extended_data[1]), m_frame->nb_samples, m_mp3_buffer, 8480);
      break;
    case AV_SAMPLE_FMT_FLTP:        // float, planar
      output_bytes = lame_encode_buffer_ieee_float(m_gfp, reinterpret_cast<const float *>(m_frame->extended_data[0]), reinterpret_cast<const float *>(m_frame->extended_data[1]), m_frame->nb_samples, m_mp3_buffer, 8480);
      break;
    case AV_SAMPLE_FMT_DBLP:        // double, planar
      output_bytes = lame_encode_buffer_ieee_double(m_gfp, reinterpret_cast<const double *>(m_frame->extended_data[0]), reinterpret_cast<const double *>(m_frame->extended_data[1]), m_frame->nb_samples, m_mp3_buffer, 8480);
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
  // TODO: handle multiple files.

  auto source_file = m_source_info.absoluteFilePath();
  int total_duration = m_libav_context->streams[m_audio_stream_id]->duration;
  int partial_duration = 0;

  while(0 == av_read_frame(m_libav_context, &m_packet))
  {
    // decode audio and encode it to mp3
    if (m_packet.stream_index == m_audio_stream_id)
    {
      // Audio packets can have multiple audio frames in a single packet
      while (m_packet.size > 0)
      {
        // Try to decode the packet into a frame. Some frames rely on multiple packets, so we have to
        // make sure the frame is finished before we can use it.
        int gotFrame = 0;
        int result = avcodec_decode_audio4(m_audio_decoder_context, m_frame, &gotFrame, &m_packet);

        if (result >= 0 && gotFrame)
        {
          m_packet.size -= result;
          m_packet.data += result;

          if(!lame_encode())
          {
            emit error_message(QString("Error in encode phase for file '%1'. Unknown sample format, format is '%2'").arg(source_file).arg(QString(av_get_sample_fmt_name(m_audio_decoder_context->sample_fmt))));
            return;
          }
          else
          {
            if(total_duration != 0)
            {
              partial_duration += m_packet.duration;
              emit progress(partial_duration*100/total_duration);
            }
          }
        }
        else
        {
          m_packet.size = 0;
          m_packet.data = nullptr;
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
      if(!lame_encode())
      {
        emit error_message(QString("Error in encode phase for file '%1'. Unknown sample format, format is '%2'").arg(source_file).arg(QString(av_get_sample_fmt_name(m_audio_decoder_context->sample_fmt))));
        return;
      }
    }

    av_free_packet(&m_packet);
  }

  auto flush_bytes = lame_encode_flush(m_gfp, m_mp3_buffer, 8480);
  if (flush_bytes != 0)
  {
    m_mp3_file_stream.write(reinterpret_cast<char *>(&m_mp3_buffer), flush_bytes);
  }

  avcodec_close(m_audio_decoder_context);
  av_frame_free(&m_frame);

  if(m_cover_decoder_context)
  {
    avcodec_close(m_cover_decoder_context);
    avcodec_close(m_cover_encoder_context);
    av_frame_free(&m_cover_frame);
  }
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
  auto source_basename = source_name.remove(source_name.split('.').last());
  auto source_cue = m_source_path + source_basename + QString("cue");

  if(!files.empty() && QFile::exists(source_cue))
  {
    QFile cue_file(source_cue);
    if(!cue_file.open(QIODevice::ReadOnly))
    {
      emit error_message(QString("Error opening cue file '%1'.").arg(source_cue));
    }
    else
    {
      auto content = cue_file.readAll();
      auto cd = cue_parse_string(content.data());
      if(cd == nullptr)
      {
        emit error_message(QString("Error parsing cue file '%1'.").arg(source_cue));
      }
      else
      {
        for(int i = 0; i < cd_get_ntrack(cd); ++i)
        {
          // MAGIC
        }
      }
    }
  }
//  else
  {
    destinations << Destination(Utils::cleanName(m_source_info.absoluteFilePath(), m_clean_configuration), 0);
  }

  return destinations;
}
