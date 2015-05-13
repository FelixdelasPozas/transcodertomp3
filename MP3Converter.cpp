/*
 File: MP3Converter.cpp
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
#include <MP3Converter.h>

// id3lib
#include <id3/tag.h>
#include <id3/utils.h>
#include <id3/misc_support.h>

QMutex MP3Converter::s_mutex;

QString MP3Converter::MP3_EXTENSION = ".mp3";
QString MP3Converter::COVER_MIME_TYPE = "image/jpeg";

//-----------------------------------------------------------------
MP3Converter::MP3Converter(const QFileInfo source_info)
: AudioConverter(source_info)
{
}

//-----------------------------------------------------------------
MP3Converter::~MP3Converter()
{
}

//-----------------------------------------------------------------
void MP3Converter::run()
{
  auto file_name = m_source_info.absoluteFilePath().replace('/','\\');
  QString track_title;

  ID3_Tag file_id3_tag(file_name.toStdString().c_str());

  if(file_id3_tag.HasV1Tag() || !file_id3_tag.HasV2Tag())
  {
    auto num_frame = file_id3_tag.Find(ID3FID_TRACKNUM);
    if (num_frame)
    {
      auto field = num_frame->GetField(ID3FN_TEXT);
      auto text = QString(field->GetRawText());
      auto number = text.split('/').first();

      if(!number.isEmpty())
      {
        track_title += number + QString(" - ");
      }
    }

    auto title_frame = file_id3_tag.Find(ID3FID_TITLE);
    if(title_frame)
    {
      auto charString = ID3_GetString(title_frame, ID3FN_TEXT);
      auto title = QString(charString);

      delete [] charString;

      if(!title.isEmpty())
      {
        track_title += title;
      }
    }

    extract_cover(file_id3_tag);

    file_id3_tag.Strip(ID3TT_ALL);
    file_id3_tag.Clear();
  }

  emit progress(50);

  if(track_title.isEmpty())
  {
    track_title = file_name.split('\\').last().remove(MP3_EXTENSION);
  }

  track_title = Utils::formatString(track_title, m_format_configuration);

  auto source_name = m_source_info.absoluteFilePath().split('/').last();
  emit information_message(QString("%1: processing to %2").arg(source_name).arg(track_title));

  QFile::rename(m_source_info.absoluteFilePath(), m_source_path + Utils::formatString(track_title, m_format_configuration));

  emit progress(100);
}

//-----------------------------------------------------------------
void MP3Converter::extract_cover(const ID3_Tag &file_tag)
{
  auto cover_frame = file_tag.Find(ID3FID_PICTURE);
  if(cover_frame)
  {
    bool adquired = false;
    auto cover_name = m_source_path + QString("Frontal.jpg");

    s_mutex.lock();
    if(!QFile::exists(cover_name))
    {
      // if there are several files with the same cover I just need one of the converters to dump the cover, not all of them.
      QFile file(cover_name);
      file.open(QIODevice::WriteOnly|QIODevice::Append);
      file.close();
      adquired = true;
    }
    s_mutex.unlock();

    if(!adquired)
    {
      return;
    }

    auto mime_type   = ID3_GetString(cover_frame, ID3FN_MIMETYPE);
    auto data_size   = cover_frame->GetField(ID3FN_DATA)->Size();

    if(QString(mime_type).compare(COVER_MIME_TYPE) == 0)
    {
      auto field = cover_frame->GetField(ID3FN_DATA);
      auto picture = reinterpret_cast<const char *>(field->GetRawBinary());

      QFile file(cover_name);
      file.open(QIODevice::WriteOnly | QIODevice::Append);
      file.write(picture, data_size);
      file.close();
    }
    else
    {
      // damn, call the artillery
      init_libav();

      open_next_destination_file();

      while(0 == av_read_frame(m_libav_context, &m_packet))
      {
        // dump the cover if the format is jpeg, if not a decoding-encoding phase must be applied.
        if(m_packet.stream_index == m_cover_stream_id)
        {
          if(!extract_cover_picture())
          {
            emit error_message(QString("Error encoding cover picture for file '%1.").arg(m_source_info.absoluteFilePath()));
            return;
          }

          av_free_packet(&m_packet);
          break;
        }

        // You *must* call av_free_packet() after each call to av_read_frame() or else you'll leak memory
        av_free_packet(&m_packet);

        // stop and return if user has aborted the conversion.
        if(has_been_cancelled()) return;
      }

      close_destination_file();

      deinit_libav();
    }

    delete [] mime_type;
  }
}
