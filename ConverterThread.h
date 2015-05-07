/*
 File: ConverterThread.h
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

#ifndef CONVERTER_THREAD_H_
#define CONVERTER_THREAD_H_

// Project
#include "Utils.h"

// Qt
#include <QThread>
#include <QFileInfo>

// Lame
#include <lame.h>

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

    /** \brief Sets the format configuration for this thread.
     *
     */
    void set_format_configuration(const Utils::FormatConfiguration format);

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
      virtual void run() = 0;

  protected:
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

    const QFileInfo m_source_info;
    const QString   m_source_path;
    Source_Info     m_information;
    Destinations    m_destinations;
    bool            m_stop;

    Utils::FormatConfiguration m_format_configuration;
};

#endif // CONVERTER_THREAD_H_
