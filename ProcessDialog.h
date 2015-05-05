/*
 File: ProcessDialog.h
 Created on: 16/4/2015
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

#ifndef PROCESSDIALOG_H_
#define PROCESSDIALOG_H_

// Application
#include "ui_ProcessDialog.h"

// Qt
#include <QList>
#include <QMap>
#include <QMutex>

// libav
extern "C"
{
#include <libavcodec/avcodec.h>
}

class QProgressBar;
class QFileInfo;
class ConverterThread;

// C++
#include <memory>

class ProcessDialog
: public QDialog
, public Ui_ProcessDialog
{
    Q_OBJECT
  public:
    /** \brief ProcessDialog class constructor.
     * \param[in] files files to convert.
     * \param[in] threadNum number of threads to use in the conversion process.
     *
     */
    explicit ProcessDialog(const QList<QFileInfo> &files, const int threadsNum);

    /** \brief Process dialog class constructor.
     *
     */
    virtual ~ProcessDialog();

  private slots:
    /** \brief Stops the conversion process.
     *
     */
    void stop();

    /** \brief Adds a error message to the log.
     * \param[in] message message string.
     *
     */
    void log_error(const QString &message);

    /** \brief Adds an information message to the log.
     * \param[in] message message string.
     *
     */
    void log_information(const QString &message);

    /** \brief When completed a conversion increments the counter of completely processed files,
     *         updates the GUI and launches the next converter thread.
     *
     */
    void increment_global_progress();

    /** \brief Closes the dialog.
     *
     */
    void exit_dialog();

  private:
    /** \brief Creates and launches the converter threads.
     *
     */
    void create_threads();

    /** \brief Registers the lock manager for the libav library.
     *
     */
    void register_av_lock_manager();

    /** \brief Unregisters the lock manager for the libav library.
     *
     */
    void unregister_av_lock_manager();

    /** \brief Lock manager that complies with the specifications required
     *         by libav. Needed for concurrent file conversion.
     *  \param[inout] mutex custom mutex object (QMutex in this case).
     *  \param[in] op manager operation to perform.
     *
     */
    static int lock_manager(void **mutex, enum AVLockOp op);

    QList<QFileInfo>          m_music_files;
    int                       m_max_workers;
    int                       m_num_workers;

    QMutex m_mutex;
    QMap<QProgressBar *, ConverterThread *> m_progress_bars;
};

#endif // PROCESSDIALOG_H_
