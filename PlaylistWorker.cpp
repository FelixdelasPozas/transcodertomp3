/*
 File: PlaylistWorker.cpp
 Created on: 17/5/2015
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
#include "PlaylistWorker.h"
#include "Utils.h"

// libav
extern "C"
{
#include <libavformat/avformat.h>
}

const QString PlaylistWorker::PLAYLIST_EXTENSION = QString(".m3u");

//-----------------------------------------------------------------
PlaylistWorker::PlaylistWorker(const QFileInfo source_info, const Utils::TranscoderConfiguration& configuration)
: AudioWorker(source_info, configuration)
{
}

//-----------------------------------------------------------------
PlaylistWorker::~PlaylistWorker()
{
}

//-----------------------------------------------------------------
void PlaylistWorker::run_implementation()
{
  generate_playlist();
}

//-----------------------------------------------------------------
QStringList PlaylistWorker::get_file_names() const
{
  QStringList fileNames, filter;
  filter << QObject::tr("*.mp3");

  auto files = Utils::findFiles(QDir(m_source_info.absoluteFilePath()), filter, false);

  for(auto file: files)
  {
    fileNames << file.absoluteFilePath().split('/').last();
  }

  fileNames.sort();

  return fileNames;
}

//-----------------------------------------------------------------
void PlaylistWorker::generate_playlist()
{
  auto files = get_file_names();

  if(files.empty())
  {
    return;
  }

  auto baseName = Utils::formatString(m_source_info.baseName(), m_configuration.formatConfiguration(), false);
  QFile playlist(m_source_info.absoluteFilePath() + QDir::separator() + baseName + PLAYLIST_EXTENSION);

  if(!playlist.open(QFile::WriteOnly|QFile::Truncate))
  {
    emit error_message(QString("Couldn't create playlist file in folder '%1'.").arg(m_source_info.absoluteFilePath()));
    return;
  }

  emit information_message(QString("Creating playlist for %1%2").arg(m_source_info.absoluteFilePath().replace('/',QDir::separator())).arg(QDir::separator()));

  auto newline = QString("\n");
  QByteArray contents;
  contents.append(QString("#EXTM3U") + newline);

  for(auto file: files)
  {
    emit progress(((files.indexOf(file) + 1) * 100) / files.size());

    long long duration = 0;
    if(has_been_cancelled() || !get_song_duration(file, duration))
    {
      playlist.close();
      playlist.remove();
      return;
    }

    contents.append(QString("#EXTINF:") +
                    QString().number(duration) +
                    QString(",") +
                    file.split('.').first() +
                    newline);

    contents.append(file + newline);
  }

  playlist.write(contents);
  playlist.close();
}

//-----------------------------------------------------------------
bool PlaylistWorker::get_song_duration(const QString &file_name, long long &duration)
{
  auto complete_name = QString(m_source_info.absoluteFilePath() + "/" + file_name).replace('/',QDir::separator());

  auto value = avformat_open_input(&m_libav_context, complete_name.toStdString().c_str(), nullptr, nullptr);
  if (value < 0)
  {
    emit error_message(QString("Couldn't open file: '%1'. Error is \"%2\".").arg(complete_name).arg(av_error_string(value)));
    return false;
  }

  // avoids a warning message from libav when the duration can't be calculated accurately. this increases the lookup frames.
  m_libav_context->max_analyze_duration *= 1000;

  value = avformat_find_stream_info(m_libav_context, nullptr);
  if(value < 0)
  {
    emit error_message(QString("Couldn't get the information of '%1'. Error is \"%2\".").arg(complete_name).arg(av_error_string(value)));
    deinit_libav();
    return false;
  }

  AVCodec *dummy_decoder = nullptr;
  auto stream_id = av_find_best_stream(m_libav_context, AVMEDIA_TYPE_AUDIO, -1, -1, &dummy_decoder, 0);
  if (stream_id < 0)
  {
    emit error_message(QString("Couldn't find any audio stream in '%1'. Error is \"%2\".").arg(complete_name).arg(av_error_string(stream_id)));
    deinit_libav();
    return false;
  }

  auto stream = m_libav_context->streams[stream_id];
  duration = (stream->duration * stream->time_base.num) / stream->time_base.den;

  deinit_libav();

  return true;
}
