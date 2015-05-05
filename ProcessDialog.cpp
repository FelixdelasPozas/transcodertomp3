/*
 File: ProcessDialog.cpp
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

// Application
#include <ProcessDialog.h>
#include "ConverterThread.h"

// Qt
#include <QLayout>
#include <QFileInfo>
#include <QProgressBar>
#include <QMutexLocker>
#include <QDebug>
#include <string>
#include <pthread.h>

// libav
extern "C"
{
  #include <libavformat/avformat.h>
}

//-----------------------------------------------------------------
ProcessDialog::ProcessDialog(const QList<QFileInfo> &files, const int threads_num)
: m_music_files{files}
, m_max_workers{threads_num}
{
  setupUi(this);

  av_register_all();
  register_av_lock_manager();

  connect(m_cancelButton, SIGNAL(clicked()), this, SLOT(stop()));

  setWindowFlags(windowFlags() & ~(Qt::WindowContextHelpButtonHint) & Qt::WindowMaximizeButtonHint);

  m_globalProgress->setMinimum(0);
  m_globalProgress->setMaximum(m_music_files.size());

  auto boxLayout = new QVBoxLayout();
  m_converters->setLayout(boxLayout);

  auto bars_num = std::min(m_max_workers, m_music_files.size());

  for(int i = 0; i < bars_num; ++i)
  {
    auto bar = new QProgressBar();
    bar->setAlignment(Qt::AlignCenter);
    bar->setMaximum(0);
    bar->setMaximum(100);
    bar->setValue(0);
    bar->setEnabled(false);

    m_progress_bars[bar] = nullptr;

    boxLayout->addWidget(bar);
  }

  create_threads();
}

//-----------------------------------------------------------------
ProcessDialog::~ProcessDialog()
{
  m_progress_bars.clear();
  unregister_av_lock_manager();
}

void ProcessDialog::closeEvent(QCloseEvent *e)
{
  stop();

  QDialog::closeEvent(e);
}

//-----------------------------------------------------------------
void ProcessDialog::log_error(const QString &message)
{
  QMutexLocker lock(&m_mutex);
  m_log->setTextColor(Qt::red);
  m_log->append(QString("ERROR:") + message);
}

//-----------------------------------------------------------------
void ProcessDialog::log_information(const QString &message)
{
  QMutexLocker lock(&m_mutex);
  m_log->setTextColor(Qt::black);
  m_log->append(message);
}

//-----------------------------------------------------------------
void ProcessDialog::stop()
{
  for(auto converter: m_progress_bars.values())
  {
    if(converter != nullptr)
    {
      converter->stop();
      converter->wait();
    }
  }
}

//-----------------------------------------------------------------
void ProcessDialog::increment_global_progress()
{
  m_mutex.lock();

  auto converter = qobject_cast<ConverterThread *>(sender());
  disconnect(converter, SIGNAL(error_message(const QString &)), this, SLOT(log_error(const QString &)));
  disconnect(converter, SIGNAL(information_message(const QString &)), this, SLOT(log_information(const QString &)));
  disconnect(converter, SIGNAL(finished()), this, SLOT(increment_global_progress()));

  if(!converter->has_been_cancelled())
  {
    auto value = m_globalProgress->value();
    m_globalProgress->setValue(++value);
  }

  --m_num_workers;

  auto bar = m_progress_bars.key(converter);
  Q_ASSERT(bar);

  disconnect(converter, SIGNAL(progress(int)), bar, SLOT(setValue(int)));

  m_progress_bars[bar] = nullptr;
  bar->setEnabled(false);
  bar->setFormat("Idle");

  auto cancelled = converter->has_been_cancelled();
  delete converter;

  if((m_globalProgress->maximum() == m_globalProgress->value()) || cancelled)
  {
    disconnect(m_cancelButton, SIGNAL(clicked()), this, SLOT(stop()));
    connect(m_cancelButton, SIGNAL(clicked()), this, SLOT(exit_dialog()));
    m_cancelButton->setText("Exit");
  }

  m_mutex.unlock();

  if(!cancelled)
  {
    create_threads();
  }
}

//-----------------------------------------------------------------
void ProcessDialog::create_threads()
{
  QMutexLocker lock(&m_mutex);

  while(m_num_workers < m_max_workers && m_music_files.size() > 0)
  {
    ++m_num_workers;
    auto music_file = m_music_files.first();
    m_music_files.removeFirst();

    auto converter = new ConverterThread(music_file);
    connect(converter, SIGNAL(error_message(const QString &)), this, SLOT(log_error(const QString &)));
    connect(converter, SIGNAL(information_message(const QString &)), this, SLOT(log_information(const QString &)));
    connect(converter, SIGNAL(finished()), this, SLOT(increment_global_progress()));

    for(auto bar: m_progress_bars.keys())
    {
      if(m_progress_bars[bar] == nullptr)
      {
        m_progress_bars[bar] = converter;
        bar->setEnabled(true);
        bar->setFormat(QString("%1").arg(music_file.baseName()));
        connect(converter, SIGNAL(progress(int)), bar, SLOT(setValue(int)));

        break;
      }
    }

    converter->start();
  }
}

//-----------------------------------------------------------------
int ProcessDialog::lock_manager(void **mutex, AVLockOp operation)
{
  QMutex *passed_mutex;
  switch (operation)
  {
    case AV_LOCK_CREATE:
      *mutex =  new QMutex();
      return 0;
    case AV_LOCK_OBTAIN:
      passed_mutex = reinterpret_cast<QMutex *>(*mutex);
      passed_mutex->lock();
      return 0;
    case AV_LOCK_RELEASE:
      passed_mutex = reinterpret_cast<QMutex *>(*mutex);
      passed_mutex->unlock();
      return 0;
    case AV_LOCK_DESTROY:
      passed_mutex = reinterpret_cast<QMutex *>(*mutex);
      delete passed_mutex;
      return 0;
  }
  return 1;
}

//-----------------------------------------------------------------
void ProcessDialog::register_av_lock_manager()
{
  av_lockmgr_register(ProcessDialog::lock_manager);
}

//-----------------------------------------------------------------
void ProcessDialog::unregister_av_lock_manager()
{
  av_lockmgr_register(nullptr);
}

//-----------------------------------------------------------------
void ProcessDialog::exit_dialog()
{
  close();
}
