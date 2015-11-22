/*
 File: AudioWorker.h
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

#ifndef AUDIO_WORKER_H_
#define AUDIO_WORKER_H_

// Project
#include "Worker.h"

// taglib
#include <tag.h>

// Qt
#include <QMutex>

// libav
extern "C"
{
#include <libavformat/avformat.h>
}

class AudioWorker
: public Worker
{
  public:
    /** \brief AudioWorker class constructor.
     * \param[in] source_info QFileInfo struct of the source file.
     * \param[in] configuration configuration struct reference.
     *
     */
    explicit AudioWorker(const QFileInfo &source_info, const Utils::TranscoderConfiguration &configuration);

    /** \brief AudioWorker class virtual destructor.
     *
     */
    virtual ~AudioWorker();

  protected:
    virtual void run_implementation() override;

    /** \brief Builds the file name based on the metadata.
     * \param[in] tags TagLib::tags metadata.
     *
     */
    QString parse_metadata(const TagLib::Tag *tags);

    /** \brief Initializes libav library structures and data to decode the source file to
     *         pcm data.
     *
     */
    bool init_libav();

    /** \brief Frees the structures allocated in the init stages of libav library.
     *
     */
    void deinit_libav();

    /** \brief Extracts the cover picture and dumps it to the disk.
     *
     */
    bool extract_cover_picture() const;

    /** \brief Helper method to get a user-friendly description of a libav error code.
     *
     */
    QString av_error_string(const int error_number) const;

    /** \brief Custom I/O read for libav, using a QFile.
     * \param[in] opaque pointer to the reader.
     * \param[in] buffer buffer to fill
     * \param[in] buffer_size buffer size.
     *
     */
    static int custom_IO_read(void *opaque, unsigned char *buffer, int buffer_size);

    /** \brief Custom I/O seek for libav, using a QFile.
     * \param[in] opaque pointer to the reader.
     * \param[in] offset seek value.
     * \param[in] whence seek direction.
     *
     */
    static long long int custom_IO_seek(void *opaque, long long int offset, int whence);

    AVFormatContext   *m_libav_context;
    AVPacket           m_packet;
    int                m_cover_stream_id;
    QString            m_cover_extension;

    static const int   s_io_buffer_size = 16384+FF_INPUT_BUFFER_PADDING_SIZE;

  private:
    /** \brief Initializes additional libav structures to decode the video stream
     *         containing the cover picture of the source file.
     */
    void init_libav_cover_extraction();

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

    QFile m_input_file;

    virtual Destinations compute_destinations() override final;

    static constexpr double CD_FRAMES_PER_SECOND = 75.0;

    AVCodec           *m_audio_decoder;
    AVCodecContext    *m_audio_decoder_context;
    AVFrame           *m_frame;
    int                m_audio_stream_id;

    static QMutex      s_mutex;
};

#endif // AUDIO_WORKER_H_
