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
ConverterThread::ConverterThread(const QFileInfo &origin_info, const QString &destination)
: m_gfp            {nullptr}
, m_originInfo     {origin_info}
, m_destination    {destination}
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

  return lame_init_params(m_gfp);
}

//-----------------------------------------------------------------
void ConverterThread::run()
{
  m_mp3File.open(m_destination.toStdString().c_str(), std::ios::trunc|std::ios::binary);
  if(!m_mp3File.is_open())
  {
    emit error_message(QString("Couldn't open destination file: %1") + m_destination);
    return;
  }

  auto musicFile = m_originInfo.absoluteFilePath().replace('/','\\');
  m_originFile.open(musicFile.toStdString().c_str(), std::ios::binary);
  if(!m_originFile.is_open())
  {
    emit error_message(QString("Couldn't open source file: %1") + musicFile);
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
  if(!correct)
  {
    emit error_message(QString("Error in LAME init stage, code: %1").arg(correct));
    return;
  }

  const auto total = m_originInfo.size();
  auto source_bytes = total;

  auto source_name = m_originInfo.absoluteFilePath().split('/').last();
  auto mp3_name = m_destination.split('/').last();
  emit information_message(QString("%1 -> %2").arg(source_name).arg(mp3_name));

  while(source_bytes > 0)
  {
    auto bytes_read = read_data();

    if(bytes_read)
    {
      if(properties.format == PCM_FORMAT::INTERLEAVED)
      {
        lame_encode_buffer_interleaved(m_gfp, m_pcm_interleaved, 1024, m_mp3_buffer, 8480);
      }
      else
      {
        lame_encode_buffer(m_gfp, m_pcm_left, m_pcm_right, 1024, m_mp3_buffer, 8480);
      }

      m_mp3File.write(reinterpret_cast<char *>(&m_mp3_buffer), bytes_read);

      emit progress(((total-source_bytes) * 100) / total);
    }

    source_bytes -= bytes_read;
  }

  lame_close(m_gfp);
  m_mp3File.close();
  m_originFile.close();
}
