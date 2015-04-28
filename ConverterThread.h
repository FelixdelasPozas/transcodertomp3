/*
 File: ConverterThread.h
 Created on: 18/4/2015
 Author: Felix de las Pozas Alvarez

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

    /** \brief Aborts the coversion process.
     *
     */
    void stop();

    /** \brief Returns true if the process has been aborted.
     *
     */
    bool has_been_cancelled();

  signals:
    void error_message(const QString message);
    void information_message(const QString message);
    void progress(int value);

  protected:
      virtual void run() override final;

  private:
      // information of the PCM data the decoding of the source file will produce.
      struct Source_Info
      {
        bool        init;
        int         num_channels;
        long        samplerate;
        MPEG_mode_e mode;
      };

      // information of a destination file that will be created.
      struct Destination
      {
          QString  name;
          long int duration;

          Destination(QString destiny_name, long int destiny_duration): name{destiny_name}, duration{destiny_duration} {};
      };


      /** \brief Initializes libav library structures and data to decode the source file to
       *         pcm data.
       *
       */
      bool init_libav();

      /** \brief Initializes additional libav structures to decode the video stream
       *         containing the cover picture of the source file if the stream is not
       *         an jpg codec, and also structures to encode the cover to jpg format.
       *         If the cover is already in jpg format this method doesn't need
       *         to be called, as the picture will be just dumped to disk.
       */
      void init_libav_cover_transcoding();

      /** \brief Frees the structures allocated in the init stages of libav library.
       *
       */
      void deinit_libav();

      /** \brief Initializes lame library structures and data to encode the pcm data.
       *
       */
      int init_lame();

      /** \brief Frees the structures allocated in the init stages of lame library.
       *
       */
      void deinit_lame();

      /** \brief Encodes the pcm data in the libav packet to the mp3 buffer and writes it
       *         to disk.
       */
      bool lame_encode();

      /** \brief Decodes the source file and encodes the resulting pcm data with the mp3 codec.
       *
       */
      void transcode();

      /** \brief Extracts the cover picture and dumps it to the disk. An additional transcode
       *         process might be needed if the source isn't already in jpeg format.
       */
      bool extract_cover_picture() const;

      /** \brief Helper method to print the libav errors.
       *
       */
      QString av_error_string(const int error_number) const;

      /** \brief Computes the file or files that will be created.
       *
       */
      QList<Destination> compute_destinations();

      const QFileInfo m_source_info;
      const QString   m_source_path;
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
