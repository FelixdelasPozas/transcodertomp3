/*
 File: ConverterThread.cpp
 Created on: 7/5/2015
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
#include <cstring>

//-----------------------------------------------------------------
ConverterThread::ConverterThread(const QFileInfo source_info)
: m_source_info{source_info}
, m_source_path{m_source_info.absoluteFilePath().remove(m_source_info.absoluteFilePath().split('/').last())}
, m_num_tracks {0}
, m_stop       {false}
, m_gfp        {nullptr}
{
}

//-----------------------------------------------------------------
ConverterThread::~ConverterThread()
{
  if(has_been_cancelled())
  {
    auto mp3_file = m_source_path + m_destinations.first().name;
    QFile::remove(mp3_file);
  }
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
void ConverterThread::set_format_configuration(const Utils::FormatConfiguration format)
{
  m_format_configuration = format;
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
void ConverterThread::lame_encoder_flush()
{
  auto flush_bytes = lame_encode_flush(m_gfp, m_mp3_buffer, MP3_BUFFER_SIZE);
  if (flush_bytes != 0)
  {
    m_mp3_file_stream.write(reinterpret_cast<char *>(&m_mp3_buffer), flush_bytes);
  }
}

//-----------------------------------------------------------------
bool ConverterThread::lame_encode_internal_buffer(unsigned int buffer_start, unsigned int buffer_length, unsigned char *buffer_L, unsigned char *buffer_R)
{
  buffer_start *= bytes_per_sample();

  int output_bytes = 0;
  auto buffer_pointer_L = buffer_L + (buffer_start);
  auto buffer_pointer_R = buffer_R + (buffer_start);

  switch(m_information.format)
  {
    case Sample_format::SIGNED_16:
      output_bytes = lame_encode_buffer_interleaved(m_gfp, reinterpret_cast<short int *>(buffer_pointer_L), buffer_length, m_mp3_buffer, MP3_BUFFER_SIZE);
      break;
    case Sample_format::FLOAT:
      output_bytes = lame_encode_buffer_interleaved_ieee_float(m_gfp, reinterpret_cast<const float *>(buffer_pointer_L), buffer_length, m_mp3_buffer, MP3_BUFFER_SIZE);
      break;
    case Sample_format::DOUBLE:
      output_bytes = lame_encode_buffer_interleaved_ieee_double(m_gfp, reinterpret_cast<const double *>(buffer_pointer_L), buffer_length, m_mp3_buffer, MP3_BUFFER_SIZE);
      break;
    case Sample_format::SIGNED_16_PLANAR:
      output_bytes = lame_encode_buffer(m_gfp, reinterpret_cast<const short int *>(buffer_pointer_L), reinterpret_cast<const short int *>(buffer_pointer_R), buffer_length, m_mp3_buffer, MP3_BUFFER_SIZE);
      break;
    case Sample_format::SIGNED_32_PLANAR:
      output_bytes = lame_encode_buffer_long2(m_gfp, reinterpret_cast<const long int *>(buffer_pointer_L), reinterpret_cast<const long int *>(buffer_pointer_R), buffer_length, m_mp3_buffer, MP3_BUFFER_SIZE);
      break;
    case Sample_format::FLOAT_PLANAR:
      output_bytes = lame_encode_buffer_ieee_float(m_gfp, reinterpret_cast<const float *>(buffer_pointer_L), reinterpret_cast<const float *>(buffer_pointer_R), buffer_length, m_mp3_buffer, MP3_BUFFER_SIZE);
      break;
    case Sample_format::DOUBLE_PLANAR:
      output_bytes = lame_encode_buffer_ieee_double(m_gfp, reinterpret_cast<const double *>(buffer_pointer_L), reinterpret_cast<const double *>(buffer_pointer_R), buffer_length, m_mp3_buffer, MP3_BUFFER_SIZE);
      break;
      // Unsupported formats
    case Sample_format::UNSIGNED_8:
    case Sample_format::UNSIGNED_8_PLANAR:
    case Sample_format::SIGNED_32:
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
bool ConverterThread::encode(unsigned int buffer_start, unsigned int buffer_length, unsigned char *buffer1, unsigned char *buffer2)
{
  if (!lame_encode_internal_buffer(buffer_start, buffer_length, buffer1, buffer2))
  {
    auto source_file = m_source_info.absoluteFilePath();
    emit error_message(QString("Error in encode phase for file '%1'. Unknown sample format, format is '%2'").arg(source_file).arg(sample_format_string()));
    return false;
  }

  return true;
}

//-----------------------------------------------------------------
bool ConverterThread::open_next_destination_file()
{
  Q_ASSERT(!m_mp3_file_stream.is_open());

  if(m_destinations.empty())
  {
    m_destinations = compute_destinations();
    m_num_tracks   = m_destinations.size();
  }

  std::memset(m_mp3_buffer, 0, MP3_BUFFER_SIZE);

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
    emit error_message(QString("Couldn't open destination file: %1").arg(mp3_file));
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

  return true;
}

//-----------------------------------------------------------------
void ConverterThread::close_destination_file()
{
  m_destinations.removeFirst();

  deinit_lame();
  m_mp3_file_stream.close();
}

//-----------------------------------------------------------------
ConverterThread::Destination &ConverterThread::destination()
{
  return m_destinations.first();
}

//-----------------------------------------------------------------
const int ConverterThread::number_of_tracks() const
{
  return m_num_tracks;
}


//-----------------------------------------------------------------
int ConverterThread::bytes_per_sample() const
{
  switch(m_information.format)
  {
    case Sample_format::UNSIGNED_8:
    case Sample_format::UNSIGNED_8_PLANAR: return 1;
    case Sample_format::SIGNED_16:
    case Sample_format::SIGNED_16_PLANAR:  return 2;
    case Sample_format::FLOAT_PLANAR:
    case Sample_format::FLOAT:             return sizeof(float);
    case Sample_format::DOUBLE_PLANAR:
    case Sample_format::DOUBLE:            return sizeof(double);
    case Sample_format::SIGNED_32:
    case Sample_format::SIGNED_32_PLANAR:  return 4;
    case Sample_format::UNDEFINED:
    default:
      break;
  }

  return -1;
}
//-----------------------------------------------------------------
QString ConverterThread::sample_format_string() const
{
  switch(m_information.format)
  {
    case Sample_format::SIGNED_16:         return QString("signed 16 bits");
    case Sample_format::FLOAT:             return QString("float");
    case Sample_format::DOUBLE:            return QString("double");
    case Sample_format::SIGNED_16_PLANAR:  return QString("signed 16 bits planar");
    case Sample_format::SIGNED_32_PLANAR:  return QString("signed 32 bits planar");
    case Sample_format::FLOAT_PLANAR:      return QString("float planar");
    case Sample_format::DOUBLE_PLANAR:     return QString("double planar");
    case Sample_format::UNSIGNED_8:        return QString("unsigned 8 bits");
    case Sample_format::UNSIGNED_8_PLANAR: return QString("unsigned 8 bits planar");
    case Sample_format::SIGNED_32:         return QString("signed 32 bits");
    default:
    break;
  };

  return QString("undefined");
}

//-----------------------------------------------------------------
ConverterThread::Destinations ConverterThread::compute_destinations()
{
  Destinations destinations;

  destinations << Destination(Utils::formatString(m_source_info.absoluteFilePath(), m_format_configuration), 0);

  return destinations;
}
