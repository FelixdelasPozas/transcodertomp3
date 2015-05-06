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
    /** \brief ConverterThread class constructor.
     * \param[in] source_info QFileInfo struct of the source file.
     *
     */
    explicit ConverterThread(const QFileInfo source_info);

    /** \brief ConverterThread class virtual destructor.
     *
     */
    virtual ~ConverterThread();

    /** \brief Aborts the conversion process.
     *
     */
    void stop();

    /** \brief Returns true if the process has been aborted and false otherwise.
     *
     */
    bool has_been_cancelled();

  signals:
    /** \brief Emits a error message signal.
     * \param[in] message error message.
     *
     */
    void error_message(const QString message);

    /** \brief Emits an information message signal.
     * \param[in] message information message.
     *
     */
    void information_message(const QString message);

    /** \brief Emits a progress signal.
     * \param[in] value progress in [0-100].
     *
     */
    void progress(int value);

  protected:
      virtual void run() override final;

  private:
      // information of the source file needed to init the mp3 encoder.
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
          long long int duration; // there are 75 frames per second in a cue sheet duration.
                                  // 0 indicates not to worry about duration and encode until the end of the source file.

          Destination(QString destination_name, long int destination_duration): name{destination_name}, duration{destination_duration} {};
      };
      using Destinations = QList<Destination>;


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
       * \param[in] buffer_start starting position in the data buffer to convert.
       * \param[in] buffer_length number of samples per channel in the data buffer.
       *
       */
      bool lame_encode(unsigned int buffer_start, unsigned int buffer_length);

      /** \brief Encodes the data using the lame_encode() method and emits a message
       *         in case of error.
       * \param[in] buffer_start starting position in the data buffer to convert.
       * \param[in] buffer_length number of samples per channel in the data buffer.
       *
       */
      bool lame_encode_frame(unsigned int buffer_start, unsigned int buffer_length);

      /** \brief Decodes the source file and encodes the resulting pcm data with the mp3
       *         codec into the destination files.
       *
       */
      void transcode();

      /** \brief Process an unique audio packet and encodes it to mp3 taking into account the duration
       *         of the tracks. Opens and closes destinations if the packet crosses it's boundaries.
       *
       */
      bool process_audio_packet();

      /** \brief Extracts the cover picture and dumps it to the disk. An additional transcode
       *         process might be needed if the source isn't already in jpeg format.
       */
      bool extract_cover_picture() const;

      /** \brief Helper method to get a user-friendly description of a libav error code.
       *
       */
      QString av_error_string(const int error_number) const;

      /** \brief Computes the file or files that will be created in case there is a cue file. If there
       * is no cue file in the same directory as the source file only the formatted file will be
       * the only destination. Formats the output file names.
       *
       */
      QList<Destination> compute_destinations();

      /** \brief Opens the next destination file, initializes the lame context and computes the number
       *         of samples of duration if specified by the cue file.
       *
       */
      bool open_next_destination_file();

      /** \brief Closes the destination file and de-initializes the lame context.
       *
       */
      void close_destination_file();

      const QFileInfo m_source_info;
      const QString   m_source_path;
      Source_Info     m_information;
      Destinations    m_destinations;
      int             m_num_tracks;

      static const int   s_mp3_buffer_size = 33920;
      static const int   s_cd_frames_per_second = 75;
      lame_global_flags *m_gfp;
      unsigned char      m_mp3_buffer[s_mp3_buffer_size];
      bool               m_stop;

      const Utils::FormatConfiguration m_format_configuration;

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
