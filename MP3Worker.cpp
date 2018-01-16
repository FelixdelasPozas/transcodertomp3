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

// taglib
#include <tag.h>
#include <id3v1tag.h>
#include <id3v2tag.h>
#include <id3v2frame.h>
#include <id3v2header.h>
#include <id3v2extendedheader.h>
#include <id3v2framefactory.h>
#include <attachedpictureframe.h>
#include <textidentificationframe.h>
#include <mpegfile.h>
#include <tstring.h>
#include <tpropertymap.h>

// Qt
#include <QTemporaryFile>
#include <QUuid>
#include <QDebug>

// C++
#include <memory>

QMutex MP3Worker::s_mutex;

QString MP3Worker::MP3_EXTENSION = ".mp3";
QString TEMP_EXTENSION           = ".temp";

//-----------------------------------------------------------------
MP3Worker::MP3Worker(const QFileInfo &source_info, const Utils::TranscoderConfiguration &configuration)
: AudioWorker(source_info, configuration)
{
}

//-----------------------------------------------------------------
void MP3Worker::run_implementation()
{
  auto file_name = m_source_info.absoluteFilePath().replace('/', QDir::separator());

  // QTemporaryFile and QThreads are not playing fine together, the temp names are being reused
  // and taglib fails upon reading the file. This approach seems to work.
  auto id = QUuid::createUuid();
  QTemporaryFile temp_file(id.toString());
  if(!temp_file.open())
  {
    emit error_message(QString("Couldn't open temporary file for file '%1'.").arg(file_name));
    return;
  }

  QFile original_file(file_name);
  if(!original_file.open(QFile::ReadOnly))
  {
    emit error_message(QString("Couldn't open file '%1'.").arg(file_name));
    return;
  }

  // taglib wouldn't open files with unicode names.
  temp_file.write(original_file.readAll());
  temp_file.waitForBytesWritten(-1);
  temp_file.flush();
  temp_file.close();

  auto temp_name = temp_file.fileName() + MP3_EXTENSION;
  temp_file.rename(temp_name); // taglib won't open a file if it doesn't have the correct extension. ¿¿??
  original_file.close();

  QString track_title;

  { // ensure TagLib File object is destroyed at the end of scope.
    TagLib::MPEG::File file_metadata(temp_name.toStdString().c_str());

    if(file_metadata.hasID3v1Tag() || file_metadata.hasID3v2Tag())
    {
      if(m_configuration.useMetadataToRenameOutput())
      {
        if(!file_metadata.hasID3v2Tag())
        {
          track_title = parse_metadata(file_metadata.tag());
        }
        else
        {
          track_title = parse_metadata_id3v2(file_metadata.ID3v2Tag());
        }
      }

      emit progress(25);

      if(m_configuration.extractMetadataCoverPicture() && file_metadata.hasID3v2Tag())
      {
        extract_cover(file_metadata.ID3v2Tag());
      }

      emit progress(50);

      if(m_configuration.stripTagsFromMp3())
      {
        file_metadata.strip();
        file_metadata.save();
      }
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

  original_file.rename(original_file.fileName() + TEMP_EXTENSION);

  if(!QFile::copy(temp_file.fileName(), final_name))
  {
    emit error_message(QString("Couldn't copy file '%1' to '%2'.").arg(m_source_info.absoluteFilePath()).arg(final_name));
    original_file.rename(original_file.fileName().remove(TEMP_EXTENSION));
  }
  else
  {
    original_file.remove();
  }
}

//-----------------------------------------------------------------
void MP3Worker::extract_cover(const TagLib::ID3v2::Tag *tags)
{
  auto picture_frames = tags->frameList("APIC");
  int i = 0;
  for(auto it = picture_frames.begin(); it != picture_frames.end(); ++it)
  {
    bool adquired = false;
    QString name = m_configuration.coverPictureName();
    QString extension = "picture_unknown_format";
    if(picture_frames.size() > 1)
    {
      name = "Image_" + QString::number(i);
    }

    auto picture = reinterpret_cast<TagLib::ID3v2::AttachedPictureFrame *>(*it);
    auto mime = QString::fromStdWString(picture->mimeType().toWString());

    if(mime.contains("jpg") || mime.contains("jpeg"))
    {
      extension = ".jpg";
    }

    if(mime.contains("png"))
    {
      extension = ".png";
    }

    if(mime.contains("bmp"))
    {
      extension = ".bmp";
    }

    if(mime.contains("tiff"))
    {
      extension = ".tif";
    }

    auto cover_name = m_source_path + name + extension;

    {
      // if there are several files with the same cover I just need one of the workers to dump the cover, not all of them.
      QMutexLocker lock(&s_mutex);
      if (!QFile::exists(cover_name))
      {
        QFile file(cover_name);
        if (!file.open(QIODevice::WriteOnly|QIODevice::Append))
        {
          emit error_message(QString("Couldn't create cover picture file for '%1', check for file permissions.").arg(m_source_info.absoluteFilePath()));
        }
        else
        {
          file.close();
          adquired = true;
        }
      }
    }

    if (adquired)
    {
      QFile file(cover_name);
      file.open(QIODevice::WriteOnly|QIODevice::Append);
      file.write(picture->picture().data(), picture->picture().size());
      file.waitForBytesWritten(-1);
      file.flush();
      file.close();
    }
  }
}

//-----------------------------------------------------------------
QString MP3Worker::parse_metadata_id3v2(const TagLib::ID3v2::Tag *tags)
{
  QString track_title;

  // disc in set
  auto frames = tags->frameList();

  for(auto it = frames.begin(); it != frames.end(); ++it)
  {
    auto frameId = QString::fromStdWString(TagLib::String((*it)->frameID()).toWString());
    if(frameId.compare("TPOS") == 0)
    {
      auto tposFrame = static_cast<TagLib::ID3v2::TextIdentificationFrame *>(*it);
      if(tposFrame)
      {
        auto disc_num = tposFrame->toString().toInt();

        if(disc_num != 0)
        {
          track_title += QString::number(tposFrame->toString().toInt()) + QString("-");
        }
      }

    }
  }

  // track number
  auto track_number = tags->track();
  if (track_number != 0)
  {
    auto number_string = QString().number(track_number);

    while (m_configuration.formatConfiguration().number_of_digits > number_string.length())
    {
      number_string = "0" + number_string;
    }

    track_title += QString().number(track_number) + QString(" - ");
  }

  // track title
  TagLib::String temp;
  auto title = QString::fromStdWString(tags->title().toWString());

  if (!title.isEmpty() && !Utils::isSpaces(title))
  {
    title.replace(QDir::separator(), QChar('-'));
    title.replace(QChar('/'), QChar('-'));
    track_title += title;
  }
  else
  {
    track_title.clear();
  }

  return track_title;
}
