/*
 File: Worker.h
 Created on: 7/5/2015
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

#ifndef WORKER_H_
#define WORKER_H_

// Project
#include "Utils.h"

// Qt
#include <QThread>
#include <QFileInfo>

// Lame
#include <lame.h>

// C++
#include <fstream>

class Worker
: public QThread
{
    Q_OBJECT
  public:
    /** \brief Worker class constructor.
     * \param[in] source_info QFileInfo struct of the source file.
     * \param[in] configuration configuration struct reference.
     *
     */
    explicit Worker(const QFileInfo source_info, const Utils::TranscoderConfiguration &configuration);

    /** \brief Worker class virtual destructor.
     *
     */
    virtual ~Worker();

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
    void error_message(const QString message) const;

    /** \brief Emits an information message signal.
     * \param[in] message information message.
     *
     */
    void information_message(const QString message) const;

    /** \brief Emits a progress signal.
     * \param[in] value progress in [0-100].
     *
     */
    void progress(int value) const;

  protected:
    virtual void run() override final;

    /** \brief Conversion implementation code.
     *
     */
    virtual void run_implementation() = 0;

    /** \brief Initializes lame library structures and data to encode the pcm data.
     *
     */
    int init_lame();

    /** \brief Frees the structures allocated in the init stages of lame library.
     *
     */
    void deinit_lame();

    /** \brief Flushes the encoder to write the last bytes of the encoder.
     *
     */
    void lame_encoder_flush();

    /** \brief Encodes the pcm data in the libav packet to the mp3 buffer and writes it
     *         to disk.
     * \param[in] buffer_start starting position in the data buffer to convert.
     * \param[in] buffer_length number of samples per channel in the data buffer.
     * \param[in] buffer1 pointer to first buffer (main or left).
     * \param[in] buffer2 pointer to second buffer (unused or right).
     *
     */
    bool lame_encode_internal_buffer(unsigned int buffer_start, unsigned int buffer_length, unsigned char *buffer1, unsigned char *buffer2);

    /** \brief Encodes the data using the lame_encode() method and emits a message
     *         in case of error.
     * \param[in] buffer_start starting position in the data buffers to convert.
     * \param[in] buffer_length number of samples per channel in the data buffers.
     * \param[in] buffer1 pointer to first buffer (main or left).
     * \param[in] buffer2 pointer to second buffer (unused or right).
     *
     */
    bool encode(unsigned int buffer_start, unsigned int buffer_length, unsigned char *buffer1, unsigned char *buffer2);


    /** \brief Opens the next destination file, initializes the lame context and computes the number
     *         of samples of duration if specified by the cue file.
     *
     */
    bool open_next_destination_file();

    /** \brief Closes the destination file and de-initializes the lame context.
     *
     */
    void close_destination_file();

    // sample formats, not all supported.
    enum class Sample_format: unsigned char { UNDEFINED, SIGNED_16, FLOAT, DOUBLE, SIGNED_16_PLANAR, SIGNED_32_PLANAR, FLOAT_PLANAR, DOUBLE_PLANAR, UNSIGNED_8, UNSIGNED_8_PLANAR, SIGNED_32 };

    // information of the source audio file
    struct Source_Info
    {
      bool          init;
      int           num_channels;
      long          samplerate;
      MPEG_mode_e   mode;
      Sample_format format;
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

    /** \brief Returns the actual destination file.
     *
     */
    Destination &destination();

    /** \brief Returns the total number of destinations.
     *
     */
    const int number_of_tracks() const;


    const QFileInfo                m_source_info;
    const QString                  m_source_path;
    Utils::TranscoderConfiguration m_configuration;
    Source_Info                    m_information;

  private:
    /** \brief Returns true if the input file can be read and false otherwise.
     *
     */
    bool check_input_file_permissions() const;

    /** \brief Returns true if the program can write in the output directory and false otherwise.
     *
     */
    bool check_output_file_permissions() const;

    /** \brief Returns the bytes per sample of the source audio.
     *
     */
    int bytes_per_sample() const;

    /** \brief Returns the string for the sample format.
     *
     */
    QString sample_format_string() const;

    /** \brief Computes the file or files that will be created with the file names already formatted.
     *
     */
    virtual Destinations compute_destinations();

    static const int MP3_BUFFER_SIZE = 33920;

    Destinations       m_destinations;
    int                m_num_tracks;
    bool               m_stop;
    lame_global_flags *m_gfp;
    unsigned char      m_mp3_buffer[MP3_BUFFER_SIZE];
    std::ofstream      m_mp3_file_stream;
};

#endif // WORKER_H_
