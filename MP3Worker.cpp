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

// tagparser
#include <tagparser/diagnostics.h>
#include <tagparser/mediafileinfo.h>
#include <tagparser/progressfeedback.h>
#include <tagparser/tag.h>

// Qt
#include <QTemporaryFile>
#include <QUuid>
#include <QDebug>

// C++
#include <memory>

QMutex MP3Worker::s_mutex;

QString MP3Worker::MP3_EXTENSION = ".mp3";

//-----------------------------------------------------------------
MP3Worker::MP3Worker(const QFileInfo &source_info, const Utils::TranscoderConfiguration &configuration)
: AudioWorker(source_info, configuration)
{
}

//-----------------------------------------------------------------
void MP3Worker::run_implementation()
{
  const auto file_name = QDir::fromNativeSeparators(m_source_info.absoluteFilePath());
  const auto shortName = Utils::shortFileName(QDir::toNativeSeparators(file_name));

  QString track_title;

  TagParser::MediaFileInfo fileInfo(shortName);
  fileInfo.setForceFullParse(true);

  TagParser::Diagnostics diag;
  TagParser::AbortableProgressFeedback progressFeedback;
  TagParser::Tag *tags = nullptr;

  try
  {
    fileInfo.open();
    if(fileInfo.isOpen())
    {
      fileInfo.parseEverything(diag, progressFeedback);

      for(auto t: fileInfo.tags())
      {
        if(!tags)
        {
          tags = t;
        }
        else
        {
          if(tags->type() < t->type()) tags = t;
        }
      }

      if(tags) tags->ensureTextValuesAreProperlyEncoded();
    }
    else
    {
      emit error_message(tr("Unable to parse tags from: %1.").arg(m_source_info.absoluteFilePath()));
    }
  }
  catch(...)
  {
    for(auto error: diag)
    {
      const auto message = error.context() + " -> " + error.message();
      emit error_message(tr("Error processing tags from file: %1 (%2)").arg(m_source_info.absoluteFilePath()).arg(QString::fromStdString(message)));
    }
  }

  if(tags && m_configuration.extractMetadataCoverPicture())
  {
    extract_cover(tags);
  }

  emit progress(25);

  if(tags && m_configuration.useMetadataToRenameOutput())
  {
    track_title = parse_metadata(tags);
  }

  emit progress(50);

  if(tags && m_configuration.stripTagsFromMp3())
  {
    TagParser::AbortableProgressFeedback::Callback nullCallback;;

    fileInfo.removeAllTags();
    TagParser::AbortableProgressFeedback progress(nullCallback, nullCallback);
    try
    {
      fileInfo.applyChanges(diag, progress);
      QFile::remove(QString::fromStdString(shortName) + ".bak");
    }
    catch(...)
    {
      emit error_message(tr("Unable to remove tags from %1.").arg(m_source_info.absoluteFilePath()));
    }
  }

  if(fileInfo.isOpen()) fileInfo.close();

  emit progress(75);

  if(track_title.isEmpty())
  {
    track_title = file_name.split('/').last().remove(MP3_EXTENSION);
  }
  track_title = Utils::formatString(track_title, m_configuration.formatConfiguration());

  auto source_name = file_name.split('/').last();
  if(track_title.compare(source_name, Qt::CaseSensitive) != 0)
  {
    emit information_message(QString("Renaming '%1' from '%2'.").arg(track_title).arg(source_name));

    const auto final_name = m_source_path + track_title;

    if(!QFile::rename(m_source_info.absoluteFilePath(), final_name))
    {
      emit error_message(QString("Couldn't rename file '%1' to '%2'.").arg(m_source_info.absoluteFilePath()).arg(final_name));
    }
  }
  else
  {
    emit information_message(QString("Renaming not needed for '%1'.").arg(track_title));
  }

  emit progress(100);
}

//-----------------------------------------------------------------
void MP3Worker::extract_cover(const TagParser::Tag *tag)
{
  if(!tag || !tag->hasField(TagParser::KnownField::Cover)) return;

  const auto cover = tag->value(TagParser::KnownField::Cover);

  const auto name = m_configuration.coverPictureName();
  QString extension = "picture_unknown_format";

  auto mime = QString::fromStdString(cover.mimeType());

  if(mime.contains("jpg") || mime.contains("jpeg"))
  {
    extension = ".jpg";
  }
  else if(mime.contains("png"))
  {
    extension = ".png";
  }
  else if(mime.contains("bmp"))
  {
    extension = ".bmp";
  }
  else if(mime.contains("tiff"))
  {
    extension = ".tif";
  }

  const auto cover_name = m_source_path + name + extension;
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
        file.write(cover.dataPointer(), cover.dataSize());
        file.waitForBytesWritten(-1);
        file.flush();
        file.close();
      }
    }
  }
}
