/*
 File: ModuleWorker.cpp
 Created on: 10/5/2015
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
#include "ModuleWorker.h"

// libopenmpt
#include <libopenmpt/libopenmpt.hpp>

// C++
#include <fstream>
#include <cstring>

//-----------------------------------------------------------------
ModuleWorker::ModuleWorker(const QFileInfo &source_info, const Utils::TranscoderConfiguration &configuration)
: Worker{source_info, configuration}
{
  std::memset(&m_left_buffer, 0, BUFFER_SIZE * sizeof(short int));
  std::memset(&m_right_buffer, 0, BUFFER_SIZE * sizeof(short int));
}

//-----------------------------------------------------------------
void ModuleWorker::run_implementation()
{
  if(!init())
  {
    m_fail = true;
    return;
  }

  process_module();
}

//-----------------------------------------------------------------
bool ModuleWorker::init()
{
  m_information.init         = true;
  m_information.format       = Sample_format::SIGNED_16_PLANAR;
  m_information.mode         = MPEG_mode_e::STEREO;
  m_information.num_channels = 2;
  m_information.samplerate   = 48000;

  auto file_name = m_source_info.absoluteFilePath().replace('/',QDir::separator());
  std::ifstream file(file_name.toStdString().c_str(), std::ios::binary );

  if(!file.is_open())
  {
    emit error_message(QString("Couldn't open source file: %1").arg(file_name));
    return false;
  }

  openmpt::module mod(file);
  file.close();

  if(m_configuration.useMetadataToRenameOutput())
  {
    auto title = mod.get_metadata("title");
    auto artist = mod.get_metadata("artist");

    if(!artist.empty())
    {
      m_module_file_name = tr(artist.c_str());

      if(!title.empty())
      {
        m_module_file_name += tr(" - ");
      }
    }

    if(!title.empty())
    {
      // mod files usually have unallowed characters in the title.
      auto qTitle = tr(title.c_str());
      qTitle = qTitle.replace('/','-');
      m_module_file_name = qTitle.replace(QDir::separator(),'-');
    }
  }

  if(m_module_file_name.isEmpty())
  {
    m_module_file_name = file_name;
  }

  return true;
}

//-----------------------------------------------------------------
void ModuleWorker::process_module()
{
  if(!open_next_destination_file())
  {
    return;
  }

  auto file_name = m_source_info.absoluteFilePath().replace('/',QDir::separator());
  std::ifstream file(file_name.toStdString().c_str(), std::ios::binary );

  if(!file.is_open())
  {
    emit error_message(QString("Couldn't open source file: %1").arg(file_name));
    m_fail = true;
    return;
  }

  openmpt::module mod(file);
  file.close();

  mod.select_subsong(-1);  // play all songs
  mod.set_repeat_count(0); // do not loop

  auto duration = mod.get_duration_seconds();

  bool finished = false;
  int progressVal = 0;
  while (!has_been_cancelled() && !finished)
  {
    std::size_t count = mod.read(m_information.samplerate, BUFFER_SIZE, reinterpret_cast<short *>(&m_left_buffer[0]), reinterpret_cast<short *>(&m_right_buffer[0]));

    if (count == 0)
    {
      finished = true;
    }
    else
    {
      auto position = mod.get_position_seconds();
      if(duration != 0)
      {
        const int currentProgress = (position * 100) / duration;
        if(progressVal != currentProgress)
        {
          progressVal = currentProgress;
          emit progress(progressVal);
        }
      }

      encode(0, count, reinterpret_cast<unsigned char *>(&m_left_buffer), reinterpret_cast<unsigned char *>(&m_right_buffer));
    }
  }

  if(has_been_cancelled())
  {
    return;
  }

  close_destination_file();
}

//-----------------------------------------------------------------
Worker::Destinations ModuleWorker::compute_destinations()
{
  Destinations destinations;

  destinations << Destination(Utils::formatString(m_module_file_name, m_configuration.formatConfiguration()), 0);

  return destinations;
}
