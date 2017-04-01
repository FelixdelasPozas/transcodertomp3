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
#include "Utils.h"
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
class Worker;

// C++
#include <memory>

/** \class ProcessDialog
 * \brief Implements the processing dialog.
 *
 */
class ProcessDialog
: public QDialog
, public Ui_ProcessDialog
{
    Q_OBJECT
  public:
    /** \brief ProcessDialog class constructor.
     * \param[in] files files to transcode.
     * \param[in] folders to create playlists.
     * \param[in] threadNum number of threads to use in the transcoding process.
     *
     */
    explicit ProcessDialog(const QList<QFileInfo> &files,
                           const QList<QFileInfo> &folders,
                           const Utils::TranscoderConfiguration &configuration,
                           QWidget *parent = nullptr,
                           Qt::WindowFlags flags = Qt::WindowFlags());

    /** \brief Process dialog class constructor.
     *
     */
    virtual ~ProcessDialog();

    virtual bool event(QEvent * e);

  protected:
    virtual void closeEvent(QCloseEvent *e) override final;

  private slots:
    /** \brief Stops the process.
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

    /** \brief When completed a worker increments the counter of completely processed files,
     *         updates the GUI and launches the next worker thread.
     *
     */
    void increment_global_progress();

    /** \brief Closes the dialog.
     *
     */
    void exit_dialog();

    /** \brief Copies the contents of the log to the clipboard.
     *
     */
    void onClipboardPressed() const;

  private:
    /** \brief Creates and launches the workers' threads.
     *
     */
    void create_threads();

    /** \brief Creates and launches the transcoders threads.
     *
     */
    void create_transcoder();

    /** \brief Assigns a worker thread to the bar that will show it's progress.
     * \param[in] worker worker pointer to assign.
     * \param[in] message message to show in the bar.
     *
     */
    void assign_bar_to_worker(Worker *worker, const QString &message);

    /** \brief Creates and launches the playlist generators threads.
     *
     */
    void create_playlistWorker();

    /** \brief Registers the lock manager for the libav library.
     *
     */
    void register_av_lock_manager();

    /** \brief Unregisters the lock manager for the libav library.
     *
     */
    void unregister_av_lock_manager();

    /** \brief Lock manager that complies with the specifications required
     *         by libav. Needed for concurrent file transcoding.
     *  \param[inout] mutex custom mutex object (QMutex in this case).
     *  \param[in] op manager operation to perform.
     *
     */
    static int lock_manager(void **mutex, enum AVLockOp op);

    QList<QFileInfo>                      m_music_files;          /** list of file informations.                 */
    QList<QFileInfo>                      m_music_folders;        /** list of folder informations.               */
    int                                   m_max_workers;          /** max number of simultaneous threads.        */
    int                                   m_num_workers;          /** current number of simultaneous threads.    */
    const Utils::TranscoderConfiguration &m_configuration;        /** application configuration struct.          */
    int                                   m_errorsCount;          /** number of errors that have ocurred.        */
    bool                                  m_finished_transcoding; /** true if process finished, false otherwise. */
    QMutex                                m_mutex;                /** protects internal data and writes to log.  */
    QMap<QProgressBar *, Worker *>        m_progress_bars;        /** maps worker<->progress bar.                */
};

#endif // PROCESSDIALOG_H_
