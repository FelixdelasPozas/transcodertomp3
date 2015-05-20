/*
 File: Worker.cpp
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
#include "Worker.h"

// C++
#include <cstring>

//-----------------------------------------------------------------
Worker::Worker(const QFileInfo source_info, const Utils::TranscoderConfiguration &configuration)
: m_source_info  {source_info}
, m_source_path  {m_source_info.absoluteFilePath().remove(m_source_info.absoluteFilePath().split('/').last())}
, m_configuration(configuration)
, m_num_tracks   {0}
, m_stop         {false}
, m_fail         {false}
, m_gfp          {nullptr}
{
}

//-----------------------------------------------------------------
Worker::~Worker()
{
  if((has_been_cancelled() && m_configuration.deleteOutputOnCancellation()) || m_fail)
  {
    if(m_mp3_file_stream.isOpen())
    {
      m_mp3_file_stream.close();
    }

    auto mp3_file = m_source_path + m_destinations.first().name;
    QFile::remove(mp3_file);
  }

  if(!has_been_cancelled() && !m_fail && m_configuration.renameInputOnSuccess() && !Utils::isMP3File(m_source_info) && !m_source_info.isDir())
  {
    auto file_name = m_source_info.absoluteFilePath();
    QFile file(file_name);
    file.rename(file_name + QString(".") + m_configuration.renamedInputFilesExtension());
  }
}

//-----------------------------------------------------------------
void Worker::stop()
{
  emit information_message(QString("Transcoder for '%1' has been cancelled.").arg(m_source_info.absoluteFilePath()));
  m_stop = true;
}

//-----------------------------------------------------------------
bool Worker::has_been_cancelled()
{
  return m_stop;
}

//-----------------------------------------------------------------
void Worker::run()
{
  if(check_input_file_permissions() && check_output_file_permissions())
  {
    run_implementation();
  }

  emit progress(100);
}

//-----------------------------------------------------------------
int Worker::init_lame()
{
  Q_ASSERT(m_information.init);

  m_gfp = lame_init();

  lame_set_num_channels (m_gfp, m_information.num_channels);
  lame_set_in_samplerate(m_gfp, m_information.samplerate);
  lame_set_brate        (m_gfp, m_configuration.bitrate());
  lame_set_quality      (m_gfp, m_configuration.quality());
  lame_set_mode         (m_gfp, m_information.num_channels == 2 ? MPEG_mode_e::STEREO : MPEG_mode_e::MONO);
  lame_set_bWriteVbrTag (m_gfp, 0);
  lame_set_copyright    (m_gfp, 0);
  lame_set_original     (m_gfp, 0);

  return lame_init_params(m_gfp);
}

//-----------------------------------------------------------------
void Worker::deinit_lame()
{
  lame_close(m_gfp);
}

//-----------------------------------------------------------------
void Worker::lame_encoder_flush()
{
  auto flush_bytes = lame_encode_flush(m_gfp, m_mp3_buffer, MP3_BUFFER_SIZE);
  if (flush_bytes != 0)
  {
    m_mp3_file_stream.write(reinterpret_cast<char *>(&m_mp3_buffer), flush_bytes);
  }
}

//-----------------------------------------------------------------
bool Worker::lame_encode_internal_buffer(unsigned int buffer_start, unsigned int buffer_length, unsigned char *buffer_L, unsigned char *buffer_R)
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
    default:
    case Sample_format::UNSIGNED_8:
    case Sample_format::UNSIGNED_8_PLANAR:
    case Sample_format::SIGNED_32:
      m_fail = true;
      return false;
      break;
  }

  if (output_bytes < 0)
  {
    switch (output_bytes)
    {
      case -1:
        emit error_message(QString("Error in LAME code stage, mp3 buffer was too small."));
        m_fail = true;
        break;
      case -2:
        emit error_message(QString("Error in LAME code stage, malloc() problem."));
        m_fail = true;
        break;
      case -3:
        emit error_message(QString("Error in LAME code stage, lame_init_params() not called."));
        m_fail = true;
        break;
      case -4:
        emit error_message(QString("Error in LAME code stage, psycho acoustic problems."));
        m_fail = true;
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
bool Worker::encode(unsigned int buffer_start, unsigned int buffer_length, unsigned char *buffer1, unsigned char *buffer2)
{
  if (!lame_encode_internal_buffer(buffer_start, buffer_length, buffer1, buffer2))
  {
    auto source_file = m_source_info.absoluteFilePath();
    emit error_message(QString("Error in encode phase for file '%1'. Unknown sample format, format is '%2'").arg(source_file).arg(sample_format_string()));
    m_fail = true;
    return false;
  }

  return true;
}

//-----------------------------------------------------------------
bool Worker::open_next_destination_file()
{
  Q_ASSERT(!m_mp3_file_stream.isOpen());

  if(m_destinations.empty())
  {
    m_destinations = compute_destinations();
    m_num_tracks   = m_destinations.size();
  }

  std::memset(m_mp3_buffer, 0, MP3_BUFFER_SIZE);

  auto music_file = m_source_info.absoluteFilePath().replace('/',QDir::separator());
  if(0 != init_lame())
  {
    emit error_message(QString("Error in LAME library init stage for '%1'").arg(music_file));
    m_fail = true;
    return false;
  }

  auto destination = m_destinations.first();
  auto mp3_file = m_source_path + destination.name;
  m_mp3_file_stream.setFileName(mp3_file);
  m_mp3_file_stream.open(QIODevice::WriteOnly|QIODevice::Truncate);

  if(!m_mp3_file_stream.isOpen())
  {
    emit error_message(QString("Couldn't open destination file: %1. Error is: %2.").arg(mp3_file).arg(m_mp3_file_stream.error()));
    m_fail = true;
    return false;
  }

  auto source_name = m_source_info.absoluteFilePath().split('/').last();

  if(m_num_tracks != 1)
  {
    emit information_message(QString("Extracting '%1' from '%2'.").arg(destination.name).arg(source_name));
  }
  else
  {
    emit information_message(QString("Transcoding '%1' from '%2'.").arg(destination.name).arg(source_name));
  }

  return true;
}

//-----------------------------------------------------------------
void Worker::close_destination_file()
{
  m_destinations.removeFirst();

  m_mp3_file_stream.flush();
  m_mp3_file_stream.close();

  deinit_lame();
}

//-----------------------------------------------------------------
Worker::Destination &Worker::destination()
{
  return m_destinations.first();
}

//-----------------------------------------------------------------
const int Worker::number_of_tracks() const
{
  return m_num_tracks;
}

//-----------------------------------------------------------------
bool Worker::check_input_file_permissions()
{
  if(m_source_info.isDir())
  {
    return true;
  }

  QFile file(m_source_info.absoluteFilePath());
  if(file.exists() && !file.open(QFile::ReadOnly))
  {
    emit error_message(QString("Can't open file '%1' but it exists, check for permissions.").arg(m_source_info.absoluteFilePath()));
    m_fail = true;
    return false;
  }

  file.close();
  return true;
}

//-----------------------------------------------------------------
bool Worker::check_output_file_permissions()
{
  auto temp_file = QString(m_source_info.absoluteFilePath()) + QString(".TranscoderTemporalFile");
  QFile file(temp_file);
  if(!file.open(QFile::WriteOnly|QFile::Truncate))
  {
    emit error_message(QString("Can't create files in '%1' path, check for permissions.").arg(m_source_info.absoluteFilePath()));
    m_fail = true;
    return false;
  }

  file.close();
  file.remove();
  return true;
}

//-----------------------------------------------------------------
int Worker::bytes_per_sample() const
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
QString Worker::sample_format_string() const
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
Worker::Destinations Worker::compute_destinations()
{
  Destinations destinations;

  QString file_name;

  destinations << Destination(Utils::formatString(m_source_info.absoluteFilePath(), m_configuration.formatConfiguration()), 0);

  return destinations;
}
