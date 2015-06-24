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
PlaylistWorker::PlaylistWorker(const QFileInfo &source_info, const Utils::TranscoderConfiguration& configuration)
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
    fileNames << file.absoluteFilePath();
  }

  fileNames.sort();

  return fileNames;
}

//-----------------------------------------------------------------
void PlaylistWorker::generate_playlist()
{
  av_register_all();

  auto files = get_file_names();

  if(files.empty())
  {
    emit information_message(QString("There aren't MP3 files in folder '%1'.").arg(m_source_info.absoluteFilePath()));
    return;
  }

  auto playlistname = m_source_info.absoluteFilePath().split('/').last();
  auto baseName = Utils::formatString(playlistname, m_configuration.formatConfiguration(), false);
  QFile playlist(m_source_info.absoluteFilePath() + QDir::separator() + baseName + PLAYLIST_EXTENSION);

  if(!playlist.open(QFile::WriteOnly|QFile::Truncate))
  {
    emit error_message(QString("Couldn't create playlist file in folder '%1'.").arg(m_source_info.absoluteFilePath()));
    m_fail = true;
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

    auto basename = file.split('/').last();

    contents.append(QString("#EXTINF:") +
                    QString().number(duration) +
                    QString(",") +
                    basename.split('.').first() +
                    newline);

    contents.append(basename + newline);
  }

  playlist.write(contents);
  playlist.close();
}

//-----------------------------------------------------------------
bool PlaylistWorker::get_song_duration(const QString &file_name, long long &duration)
{
  QFile input_file(file_name);
  if(!input_file.open(QIODevice::ReadOnly))
  {
    emit error_message(QString("Couldn't open input file '%1'.").arg(file_name));
    return false;
  }

  unsigned char *ioBuffer = reinterpret_cast<unsigned char *>(av_malloc(s_io_buffer_size)); // can get freed with av_free() by libav
  AVIOContext *avioContext = avio_alloc_context(ioBuffer, s_io_buffer_size - FF_INPUT_BUFFER_PADDING_SIZE, 0, reinterpret_cast<void*>(&input_file), &custom_IO_read, nullptr, &custom_IO_seek);
  avioContext->seekable = 0;
  avioContext->write_flag = 0;

  m_libav_context = avformat_alloc_context();
  m_libav_context->pb = avioContext;

  auto value = avformat_open_input(&m_libav_context, "dummy", nullptr, nullptr);
  if (value < 0)
  {
    emit error_message(QString("Couldn't open file: '%1' with libav. Error is \"%2\"").arg(file_name).arg(av_error_string(value)));
    m_fail = true;
    return false;
  }

  // avoids a warning message from libav when the duration can't be calculated accurately. this increases the lookup frames.
  m_libav_context->max_analyze_duration *= 1000;

  value = avformat_find_stream_info(m_libav_context, nullptr);
  if(value < 0)
  {
    emit error_message(QString("Couldn't get the information of '%1'. Error is \"%2\".").arg(file_name).arg(av_error_string(value)));
    deinit_libav();
    m_fail = true;
    return false;
  }

  AVCodec *dummy_decoder = nullptr;
  auto stream_id = av_find_best_stream(m_libav_context, AVMEDIA_TYPE_AUDIO, -1, -1, &dummy_decoder, 0);
  if (stream_id < 0)
  {
    emit error_message(QString("Couldn't find any audio stream in '%1'. Error is \"%2\".").arg(file_name).arg(av_error_string(stream_id)));
    deinit_libav();
    m_fail = true;
    return false;
  }

  auto stream = m_libav_context->streams[stream_id];
  duration = (stream->duration * stream->time_base.num) / stream->time_base.den;

  deinit_libav();

  input_file.close();

  return true;
}
