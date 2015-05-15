/*
 File: AudioConverterThread.h
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

#ifndef AUDIO_CONVERTER_THREAD_H_
#define AUDIO_CONVERTER_THREAD_H_

// Project
#include "ConverterThread.h"

// Qt
#include <QMutex>

// libav
extern "C"
{
#include <libavformat/avformat.h>
}

class AudioConverter
: public ConverterThread
{
  public:
    /** \brief ConverterThread class constructor.
     * \param[in] source_info QFileInfo struct of the source file.
     * \param[in] configuration configuration struct reference.
     *
     */
    explicit AudioConverter(const QFileInfo source_info, const Utils::TranscoderConfiguration &configuration);

    /** \brief ConverterThread class virtual destructor.
     *
     */
    virtual ~AudioConverter();

  protected:
    virtual void run() override;

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

    /** \brief Extracts the cover picture and dumps it to the disk. An additional transcode
     *         process might be needed if the source isn't already in jpeg format.
     */
    bool extract_cover_picture() const;

    AVFormatContext   *m_libav_context;
    AVPacket           m_packet;
    int                m_cover_stream_id;

  private:
    /** \brief Helper method to send the buffers to encode. Returns the value of the lame library buffer
     *         encoding method called.
     *
     */
    bool encode_buffers(unsigned int buffer_start, unsigned int buffer_length);

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

    /** \brief Helper method to get a user-friendly description of a libav error code.
     *
     */
    QString av_error_string(const int error_number) const;

    virtual Destinations compute_destinations() override final;

    static const int   CD_FRAMES_PER_SECOND = 75;

    AVCodec           *m_audio_decoder;
    AVCodecContext    *m_audio_decoder_context;
    AVCodec           *m_cover_encoder;
    AVCodecContext    *m_cover_encoder_context;
    AVCodec           *m_cover_decoder;
    AVCodecContext    *m_cover_decoder_context;
    AVPacket           m_cover_packet;
    AVFrame           *m_frame;
    AVFrame           *m_cover_frame;
    int                m_audio_stream_id;

    static QMutex      s_mutex;
};

#endif // AUDIO_CONVERTER_THREAD_H_
