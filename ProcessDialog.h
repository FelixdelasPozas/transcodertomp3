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
    explicit ProcessDialog(const QList<QFileInfo> &files, const int threadsNum);
    virtual ~ProcessDialog();

  private slots:
    void stop();
    void log_error(const QString &message);
    void log_information(const QString &message);
    void increment_global_progress();
    void exit_dialog();

  private:
    void create_threads();
    void register_av_lock_manager();
    void unregister_av_lock_manager();

    static int lock_manager(void **mutex, enum AVLockOp op);

    QList<QFileInfo>          m_mp3_files;
    QList<QFileInfo>          m_music_files;
    int                       m_max_workers;
    int                       m_num_workers;

    QMutex m_mutex;
    QMap<QProgressBar *, ConverterThread *> m_progress_bars;
};

#endif // PROCESSDIALOG_H_
