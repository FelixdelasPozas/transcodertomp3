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

//-----------------------------------------------------------------
MP3Worker::MP3Worker(const QFileInfo &source_info, const Utils::TranscoderConfiguration &configuration)
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
  auto file_name = m_source_info.absoluteFilePath().replace('/', QDir::separator());
  QString track_title;

  ID3_Tag file_id3_tag(file_name.toStdString().c_str());

  if(file_id3_tag.HasV1Tag() || file_id3_tag.HasV2Tag())
  {
    track_title = parse_metadata(file_id3_tag);

    emit progress(25);

    if(m_configuration.extractMetadataCoverPicture())
    {
      extract_cover(file_id3_tag);
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
  emit information_message(QString("Renaming '%1' from '%2'.").arg(track_title).arg(source_name));

  auto final_name = m_source_path + track_title;

  // we need to do the rename in two steps as Qt won't rename a file if the only difference is the case of the letters,
  // but usually we need to do just that.
  if(m_source_info.absoluteFilePath().compare(final_name) != 0)
  {
    QString temporal_string("rename");

    if(!QFile::rename(m_source_info.absoluteFilePath(), m_source_info.absoluteFilePath() + temporal_string) ||
       !QFile::rename(m_source_info.absoluteFilePath() + temporal_string, final_name))
    {
      emit error_message(QString("Couldn't rename '%1' file to '%2'.").arg(m_source_info.absoluteFilePath()).arg(final_name));
    }
  }
}

//-----------------------------------------------------------------
void MP3Worker::extract_cover(const ID3_Tag &file_tag)
{
  auto cover_frame = file_tag.Find(ID3FID_PICTURE);

  if(cover_frame)
  {
    bool adquired = false;

    auto format  = ID3_GetString(cover_frame, ID3FN_IMAGEFORMAT);
    auto extension = QString(format).toLower();
    delete [] format;

    if(extension.isEmpty())
    {
      auto mimeType = ID3_GetString(cover_frame, ID3FN_MIMETYPE);
      auto qmimeType = QString(mimeType);
      delete [] mimeType;

      if(!qmimeType.isEmpty())
      {
        extension = qmimeType.split('/').last().toLower();
      }
      else
      {
        extension = QString("unknown");
      }
    }

    auto cover_name = m_source_path + m_configuration.coverPictureName() + QString(".") + extension;

    s_mutex.lock();
    if (!QFile::exists(cover_name))
    {
      // if there are several files with the same cover I just need one of the workers to dump the cover, not all of them.
      QFile file(cover_name);
      if (!file.open(QIODevice::WriteOnly | QIODevice::Append))
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

    if (adquired)
    {
      auto data_size = cover_frame->GetField(ID3FN_DATA)->Size();

      auto field = cover_frame->GetField(ID3FN_DATA);
      auto picture = reinterpret_cast<const char *>(field->GetRawBinary());

      QFile file(cover_name);
      file.open(QIODevice::WriteOnly | QIODevice::Append);
      file.write(picture, data_size);
      file.flush();
      file.close();
    }
  }
}

//-----------------------------------------------------------------
QString MP3Worker::parse_metadata(const ID3_Tag& file_tag)
{
  QString track_title;

  if(m_configuration.useMetadataToRenameOutput())
  {
    // CD number
    auto disc_frame = file_tag.Find(ID3FID_PARTINSET);
    if(disc_frame)
    {
      auto charString = ID3_GetString(disc_frame, ID3FN_TEXT);
      auto cd_number = QString(charString);

      delete [] charString;

      if (!cd_number.isEmpty() && !Utils::isSpaces(cd_number) && Utils::isANumber(cd_number))
      {
        track_title += cd_number + QString("-");
      }
    }

    // track number
    auto num_frame = file_tag.Find(ID3FID_TRACKNUM);
    if (num_frame)
    {
      auto track_number = ID3_GetTrackNum(&file_tag);
      auto number_string = QString().number(track_number);

      if(Utils::isANumber(number_string))
      {
        while (m_configuration.formatConfiguration().number_of_digits > number_string.length())
        {
          number_string = "0" + number_string;
        }

        track_title += QString().number(track_number) + QString(" - ");
      }
    }

    // track title
    auto title_frame = file_tag.Find(ID3FID_TITLE);
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

  return track_title;
}
