/*
 File: OggConverter.cpp
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
#include "OggConverter.h"

// Vorbis lib
#include <vorbis/vorbisfile.h>

//-----------------------------------------------------------------
OGGConverter::OGGConverter(const QFileInfo origin_info)
: ConverterThread{origin_info}
, m_init         {false}
{
}

//-----------------------------------------------------------------
OGGConverter::~OGGConverter()
{
  if(m_init)
  {
    ov_clear(&m_vorbis_file);
  }
}

//-----------------------------------------------------------------
bool OGGConverter::open_source_file()
{
  auto file_name = m_origin_info.absoluteFilePath();
  auto transformed_name = file_name.replace('/', '\\');

  int result = ov_fopen(transformed_name.toStdString().c_str(), &m_vorbis_file);
  switch(result)
  {
    case OV_EREAD:
      emit error_message(QString("OGG Init: A read from media returned an error (%1).").arg(file_name));
      break;
    case OV_ENOTVORBIS:
      emit error_message(QString("OGG Init: Bitstream does not contain any Vorbis data (%1).").arg(file_name));
      break;
    case OV_EVERSION:
      emit error_message(QString("OGG Init: Vorbis version mismatch (%1).").arg(file_name));
      break;
    case OV_EBADHEADER:
      emit error_message(QString("OGG Init: Invalid Vorbis bitstream header (%1).").arg(file_name));
      break;
    case OV_EFAULT:
      emit error_message(QString("OGG Init: Internal logic fault, a bug or heap/stack corruption (%1).").arg(file_name));
      break;
    case 0:
    default:
      break;
  }

  m_init = (result == 0);

  return m_init;
}

//-----------------------------------------------------------------
long int OGGConverter::read_data()
{
  if(!m_init) Q_ASSERT(false);

  int unused = 0;
  return ov_read(&m_vorbis_file, reinterpret_cast<char *>(m_pcm_interleaved), 4096*2, 0, 2, 1, &unused);
}

//-----------------------------------------------------------------
void OGGConverter::get_source_properties(Source_Info &information)
{
  auto info = ov_info(&m_vorbis_file, -1);

  information.num_channels = info->channels;
  information.samplerate   = info->rate;
  information.format       = PCM_FORMAT::INTERLEAVED;
  information.mode         = (info->channels == 2) ? MPEG_mode_e::STEREO : MPEG_mode_e::MONO;
  information.num_samples  = ov_pcm_total(&m_vorbis_file, -1);

  auto file_name = this->m_origin_info.absoluteFilePath().replace('/','\\').toStdString().c_str();

  if(info->channels > 2)
  {
    emit error_message(QString("Source is not mono or stereo (%1).").arg(file_name));
  }
}

