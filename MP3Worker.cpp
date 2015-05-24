/*
 File: MP3Worker.cpp
 Created on: 9/5/2015
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
#include "MP3Worker.h"

// id3lib
#include <id3/tag.h>
#include <id3/utils.h>
#include <id3/misc_support.h>

QMutex MP3Worker::s_mutex;

QString MP3Worker::MP3_EXTENSION = ".mp3";
QString MP3Worker::COVER_MIME_TYPE_1 = "image/jpeg";
QString MP3Worker::COVER_MIME_TYPE_2 = "image/jpg";

//-----------------------------------------------------------------
MP3Worker::MP3Worker(const QFileInfo source_info, const Utils::TranscoderConfiguration &configuration)
: AudioWorker(source_info, configuration)
{
}

//-----------------------------------------------------------------
MP3Worker::~MP3Worker()
{
}

//-----------------------------------------------------------------
void MP3Worker::run_implementation()
{
  if(!Utils::renameFile(m_source_info, m_working_filename))
  {
    emit error_message(QString("Couldn't rename '%1'.").arg(m_source_info.absoluteFilePath()));
    return;
  }

  auto file_name = m_working_filename.replace('/', QDir::separator());
  QString track_title;

  ID3_Tag file_id3_tag(file_name.toStdString().c_str());

  if(file_id3_tag.HasV1Tag() || file_id3_tag.HasV2Tag())
  {
    if(m_configuration.useMetadataToRenameOutput())
    {
      // CD number
      auto disc_frame = file_id3_tag.Find(ID3FID_PARTINSET);
      if(disc_frame)
      {
        auto charString = ID3_GetString(disc_frame, ID3FN_TEXT);
        auto cd_number = QString(charString);

        delete [] charString;

        if (!cd_number.isEmpty() && !Utils::isSpaces(cd_number))
        {
          track_title += cd_number + QString("-");
        }
      }

      // track number
      auto num_frame = file_id3_tag.Find(ID3FID_TRACKNUM);
      if (num_frame)
      {
        auto track_number = ID3_GetTrackNum(&file_id3_tag);

        track_title += QString().number(track_number) + QString(" - ");
      }

      // track title
      auto title_frame = file_id3_tag.Find(ID3FID_TITLE);
      if (title_frame)
      {
        auto charString = ID3_GetString(title_frame, ID3FN_TEXT);
        auto title = QString(charString);

        delete[] charString;

        if (!title.isEmpty() && !Utils::isSpaces(title))
        {
          title.replace(QDir::separator(), QChar('-'));
          title.replace(QChar('/'),QChar('-'));
          track_title += title;
        }
        else
        {
          track_title.clear();
        }
      }
      else
      {
        track_title.clear();
      }
    }

    emit progress(25);

    if(m_configuration.extractMetadataCoverPicture())
    {
      if (!extract_cover(file_id3_tag))
      {
        return;
      }
    }

    emit progress(50);

    if(m_configuration.stripTagsFromMp3())
    {
      file_id3_tag.Strip(ID3TT_ALL);
      file_id3_tag.Clear();
    }
  }

  emit progress(75);

  if(track_title.isEmpty())
  {
    track_title = m_source_info.absoluteFilePath().split('/').last().remove(MP3_EXTENSION);
  }

  track_title = Utils::formatString(track_title, m_configuration.formatConfiguration());

  auto source_name = m_source_info.absoluteFilePath().split('/').last();
  emit information_message(QString("%1: processing to %2").arg(source_name).arg(track_title));

  auto final_name = m_source_path + track_title;

  if(m_source_info.absoluteFilePath().compare(final_name) != 0)
  {
    if (!QFile::rename(m_working_filename, final_name))
    {
      emit error_message(QString("Couldn't rename '%1' file to '%2'.").arg(m_source_info.absoluteFilePath()).arg(final_name));
      QFile::rename(m_working_filename, m_source_info.absoluteFilePath());
    }
  }
  else
  {
    QFile::rename(m_working_filename, m_source_info.absoluteFilePath());
  }
}

//-----------------------------------------------------------------
bool MP3Worker::extract_cover(const ID3_Tag &file_tag)
{
  auto cover_frame = file_tag.Find(ID3FID_PICTURE);
  if(cover_frame)
  {
    bool adquired = false;
    auto cover_name = m_source_path + m_configuration.coverPictureName() + QString(".jpg");

    s_mutex.lock();
    if(!QFile::exists(cover_name))
    {
      // if there are several files with the same cover I just need one of the workers to dump the cover, not all of them.
      QFile file(cover_name);
      if(!file.open(QIODevice::WriteOnly|QIODevice::Append))
      {
        emit error_message(QString("Couldn't create cover picture file for '%1', check for file permissions.").arg(m_source_info.absoluteFilePath()));
      }
      else
      {
        file.close();
        adquired = true;
      }
    }
    s_mutex.unlock();

    if(!adquired)
    {
      return true;
    }

    auto mime_type   = ID3_GetString(cover_frame, ID3FN_MIMETYPE);
    auto qmime_type  = QString(mime_type);
    delete [] mime_type;
    auto data_size   = cover_frame->GetField(ID3FN_DATA)->Size();

    if(qmime_type.compare(COVER_MIME_TYPE_1) == 0 || qmime_type.compare(COVER_MIME_TYPE_2) == 0)
    {
      auto field = cover_frame->GetField(ID3FN_DATA);
      auto picture = reinterpret_cast<const char *>(field->GetRawBinary());

      QFile file(cover_name);
      file.open(QIODevice::WriteOnly | QIODevice::Append);
      file.write(picture, data_size);
      file.flush();
      file.close();
    }
    else
    {
      // damn, call the artillery
      if(!init_libav())
      {
        return false;
      }

      while(0 == av_read_frame(m_libav_context, &m_packet))
      {
        // dump the cover if the format is jpeg, if not a transcoding phase must be applied.
        if(m_packet.stream_index == m_cover_stream_id)
        {
          if(!extract_cover_picture())
          {
            emit error_message(QString("Error encoding cover picture for file '%1.").arg(m_source_info.absoluteFilePath()));
            return false;
          }

          av_free_packet(&m_packet);
          break;
        }

        // You *must* call av_free_packet() after each call to av_read_frame() or else you'll leak memory
        av_free_packet(&m_packet);

        // stop and return if user has aborted the conversion.
        if(has_been_cancelled())
        {
          return false;
        }
      }

      deinit_libav();
    }
  }

  return true;
}
