/*
 File: ConverterThread.h
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

#ifndef CONVERTER_THREAD_H_
#define CONVERTER_THREAD_H_

// Project
#include "Utils.h"

// Qt
#include <QThread>
#include <QFileInfo>
#include <QMutex>

// Lame
#include <lame.h>

// libav
extern "C"
{
#include <libavformat/avformat.h>
}

// C++
#include <fstream>

class ConverterThread
: public QThread
{
    Q_OBJECT
  public:
    explicit ConverterThread(const QFileInfo origin_info);
    virtual ~ConverterThread();

    void stop();
    bool has_been_cancelled();

  signals:
    void error_message(const QString message);
    void information_message(const QString message);
    void progress(int value);

  protected:
      virtual void run() override final;

  private:
      // the derived classes will write the raw PCM data into these buffers.
      // what buffer will be used to encode will be given by the "format"
      // field in the Source_Info of the source file.
      short int m_pcm_interleaved[4096 + FF_INPUT_BUFFER_PADDING_SIZE];
      short int m_pcm_right[4096];
      short int m_pcm_left[4096];

      enum class MODE: unsigned char { MONO, STEREO, INTERLEAVED, UNSUPPORTED };

      // information of the PCM data the decoding of the source file will produce.
      struct Source_Info
      {
        bool        init;
        int         num_channels;
        long        samplerate;
        MPEG_mode_e mode;
      };

      bool init_decoder();
      void deinit_decoder();

      int init_encoder();
      bool encode();

      void transcode();
      bool extract_cover_picture() const;

      QString av_error_string(const int error_number) const;

      const QFileInfo m_origin_info;
      Source_Info     m_information;

      lame_global_flags *m_gfp;
      unsigned char      m_mp3_buffer[8480];
      bool               m_stop;

      const Utils::CleanConfiguration m_clean_configuration;

      AVFormatContext *m_libav_context;
      AVCodec         *m_audio_decoder;
      AVCodecContext  *m_audio_decoder_context;
      AVCodec         *m_cover_encoder;
      AVCodecContext  *m_cover_encoder_context;
      AVCodec         *m_cover_decoder;
      AVCodecContext  *m_cover_decoder_context;
      AVPacket         m_packet;
      AVPacket         m_cover_packet;
      AVFrame         *m_frame;
      AVFrame         *m_cover_frame;
      int              m_audio_stream_id;
      int              m_cover_stream_id;

      std::ofstream    m_mp3_file_stream;
      static QMutex    s_mutex;
};

#endif // CONVERTER_THREAD_H_
