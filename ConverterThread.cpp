/*
 File: ConverterThread.cpp
 Created on: 18/4/2015
 Author: Felix

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

// Qt
#include <QStringList>

//-----------------------------------------------------------------
ConverterThread::ConverterThread(const QFileInfo origin_info)
: m_origin_info{origin_info}
, m_gfp        {nullptr}
, m_stop       {false}
{
}

//-----------------------------------------------------------------
ConverterThread::~ConverterThread()
{
}

//-----------------------------------------------------------------
int ConverterThread::init_LAME_codec(const Source_Info &information)
{
  m_gfp = lame_init();

  lame_set_num_channels(m_gfp, information.num_channels);
  lame_set_in_samplerate(m_gfp, information.samplerate);
  lame_set_brate(m_gfp, 320);
  lame_set_mode(m_gfp, information.mode);
  lame_set_quality(m_gfp, 0);
  lame_set_bWriteVbrTag(m_gfp, 0);
  lame_set_copyright(m_gfp, 0);
  lame_set_original(m_gfp, 0);
  lame_set_preset(m_gfp, preset_mode_e::ABR_320);

  return lame_init_params(m_gfp);
}

//-----------------------------------------------------------------
void ConverterThread::run()
{
  auto destination = Utils::cleanName(m_origin_info.absoluteFilePath(), m_clean_configuration);
  auto path = m_origin_info.absoluteFilePath().remove(m_origin_info.absoluteFilePath().split('/').last());
  auto mp3_file = path + destination;

  std::ofstream mp3_file_stream;
  mp3_file_stream.open(mp3_file.toStdString().c_str(), std::ios::trunc|std::ios::binary);
  if(!mp3_file_stream.is_open())
  {
    emit error_message(QString("Couldn't open destination file: %1") + mp3_file);
    return;
  }

  if(!open_source_file())
  {
    auto music_file = m_origin_info.absoluteFilePath().replace('/','\\');
    emit error_message(QString("Couldn't open source file: %1").arg(music_file));
    return;
  }

  Source_Info properties;
  get_source_properties(properties);

  if(properties.format == PCM_FORMAT::UNSUPPORTED)
  {
    emit error_message(QString("Unsupported PCM format."));
    return;
  }

  auto correct = init_LAME_codec(properties);
  if(0 != correct)
  {
    emit error_message(QString("Error in LAME init stage, code: %1").arg(correct));
    return;
  }

  const auto total_samples = properties.num_samples;
  auto source_bytes = total_samples;

  auto source_name = m_origin_info.absoluteFilePath().split('/').last();
  emit information_message(QString("%1 -> %2").arg(source_name).arg(destination));

  int old_progress_value = 0;
  int bytes_read = 1;
  while(bytes_read > 0 && !m_stop)
  {
    bytes_read = read_data();
    int output_bytes = 0;

    if(bytes_read)
    {
      if(properties.format == PCM_FORMAT::INTERLEAVED)
      {
        output_bytes = lame_encode_buffer_interleaved(m_gfp, m_pcm_interleaved, bytes_read/4, m_mp3_buffer, 8480);
      }
      else
      {
        output_bytes = lame_encode_buffer(m_gfp, m_pcm_left, m_pcm_right, bytes_read/4, m_mp3_buffer, 8480);
      }

      if(output_bytes < 0)
      {
        switch(output_bytes)
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

      if(output_bytes > 0)
      {
        mp3_file_stream.write(reinterpret_cast<char *>(&m_mp3_buffer), output_bytes);
      }

      auto progress_value = ((total_samples-source_bytes) * 100) / total_samples;
      if(progress_value != old_progress_value)
      {
        emit progress(progress_value);
        old_progress_value = progress_value;
      }
    }

    source_bytes -= bytes_read/4;
  }

  auto flush_bytes = lame_encode_flush(m_gfp, m_mp3_buffer, 8480);
  if(flush_bytes != 0)
  {
    mp3_file_stream.write(reinterpret_cast<char *>(&m_mp3_buffer), flush_bytes);
  }

  emit progress(100);

  lame_close(m_gfp);
  mp3_file_stream.close();

  if(m_stop)
  {
    QFile::remove(mp3_file);
  }
}

//-----------------------------------------------------------------
void ConverterThread::stop()
{
  auto source_name = m_origin_info.absoluteFilePath().split('/').last();
  emit information_message(QString("Converter for '%1' has been cancelled.").arg(source_name));
  m_stop = true;
}

//-----------------------------------------------------------------
bool ConverterThread::has_been_cancelled()
{
  return m_stop;
}
