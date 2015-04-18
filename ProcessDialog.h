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
#include "Utils.h"

// Qt
#include <QList>

class QProgressBar;
class QFileInfo;

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
    void log(const QString &);
    void progress(int value, int converter);

  private:
    QList<std::shared_ptr<QProgressBar>> m_progressGUI;
    QList<QFileInfo> m_mp3Files;
    QList<QFileInfo> m_musicFiles;
    int m_numThreads;
    Utils::CleanConfiguration cleanConfiguration;
};

#endif // PROCESSDIALOG_H_
