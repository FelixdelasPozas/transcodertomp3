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
#include "ProcessDialog.h"
#include "MusicTranscoder.h"
#include "AudioConverter.h"
#include "MP3Converter.h"
#include "ModuleConverter.h"
#include "PlaylistGenerator.h"

// Qt
#include <QLayout>
#include <QFileInfo>
#include <QProgressBar>
#include <QMutexLocker>

// libav
extern "C"
{
  #include <libavformat/avformat.h>
}

//-----------------------------------------------------------------
ProcessDialog::ProcessDialog(const QList<QFileInfo> &files, const QList<QFileInfo> &folders, const Utils::TranscoderConfiguration &configuration)
: m_music_files         {files}
, m_music_folders       {folders}
, m_configuration       (configuration)
, m_errorsCount         {0}
{
  setupUi(this);

  av_register_all();
  register_av_lock_manager();

  connect(m_cancelButton, SIGNAL(clicked()),
          this,           SLOT(stop()));

  connect(m_clipboard,    SIGNAL(pressed()),
          this,           SLOT(onClipboardPressed()));

  setWindowFlags(windowFlags() & ~(Qt::WindowContextHelpButtonHint) & Qt::WindowMaximizeButtonHint);

  m_max_workers = m_configuration.numberOfThreads();
  auto total_jobs = m_music_files.size() + m_music_folders.size();

  m_globalProgress->setMinimum(0);
  m_globalProgress->setMaximum(total_jobs);

  auto boxLayout = new QVBoxLayout();
  m_converters->setLayout(boxLayout);

  auto bars_num = std::min(m_max_workers, total_jobs);

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

  m_finished_transcoding = (files.size() == 0);

  create_threads();
}

//-----------------------------------------------------------------
ProcessDialog::~ProcessDialog()
{
  m_progress_bars.clear();
  unregister_av_lock_manager();
}

//-----------------------------------------------------------------
void ProcessDialog::closeEvent(QCloseEvent *e)
{
  stop();

  QDialog::closeEvent(e);
}

//-----------------------------------------------------------------
void ProcessDialog::log_error(const QString &message)
{
  QMutexLocker lock(&m_mutex);

  ++m_errorsCount;
  m_log->setTextColor(Qt::red);
  m_log->append(QString("ERROR: ") + message);

  m_errorsLabel->setStyleSheet("QLabel { color: rgb(255, 0, 0); };");
  m_errorsCountLabel->setStyleSheet("QLabel { color: rgb(255, 0, 0); };");
  m_errorsCountLabel->setText(QString().number(m_errorsCount));
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

  disconnect(converter, SIGNAL(error_message(const QString &)),
             this,      SLOT(log_error(const QString &)));

  disconnect(converter, SIGNAL(information_message(const QString &)),
             this,      SLOT(log_information(const QString &)));

  disconnect(converter, SIGNAL(finished()),
             this,      SLOT(increment_global_progress()));

  if(!converter->has_been_cancelled())
  {
    auto value = m_globalProgress->value();
    m_globalProgress->setValue(++value);
  }

  --m_num_workers;

  auto bar = m_progress_bars.key(converter);
  Q_ASSERT(bar);

  disconnect(converter, SIGNAL(progress(int)),
             bar,       SLOT(setValue(int)));

  m_progress_bars[bar] = nullptr;
  bar->setEnabled(false);
  bar->setFormat("Idle");

  auto cancelled = converter->has_been_cancelled();
  delete converter;

  if((m_globalProgress->maximum() == m_globalProgress->value()) || cancelled)
  {
    disconnect(m_cancelButton, SIGNAL(clicked()),
               this,           SLOT(stop()));

    connect(m_cancelButton,    SIGNAL(clicked()),
            this,              SLOT(exit_dialog()));

    m_cancelButton->setText("Exit");
    m_clipboard->setEnabled(true);
  }

  if(m_music_files.empty() && m_num_workers == 0)
  {
    m_finished_transcoding = true;
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
    create_transcoder();
  }

  if(m_finished_transcoding)
  {
    while(m_num_workers < m_max_workers && m_music_folders.size() > 0)
    {
      create_playlistGenerator();
    }
  }
}

//-----------------------------------------------------------------
void ProcessDialog::create_transcoder()
{
  auto fs_handle = m_music_files.first();
  m_music_files.removeFirst();

  ++m_num_workers;

  ConverterThread *converter;

  if(Utils::isModuleFile(fs_handle))
  {
    converter = new ModuleConverter(fs_handle, m_configuration);
  }
  else
  {
    if(Utils::isMP3File(fs_handle))
    {
      converter = new MP3Converter(fs_handle, m_configuration);
    }
    else
    {
      converter = new AudioConverter(fs_handle, m_configuration);
    }
  }

  auto message = QString("%1").arg(fs_handle.absoluteFilePath().split('/').last());
  assign_bar_to_converter(converter, message);

  converter->start();
}

//-----------------------------------------------------------------
void ProcessDialog::create_playlistGenerator()
{
  auto fs_handle = m_music_folders.first();
  m_music_folders.removeFirst();

  ++m_num_workers;

  auto generator = new PlaylistGenerator(fs_handle, m_configuration);

  auto message = QString("Generating playlist for %1").arg(fs_handle.absoluteFilePath().split('/').last());
  assign_bar_to_converter(generator, message);

  generator->start();
}

//-----------------------------------------------------------------
void ProcessDialog::assign_bar_to_converter(ConverterThread* converter, const QString& message)
{
  connect(converter, SIGNAL(error_message(const QString &)),
          this,      SLOT(log_error(const QString &)));

  connect(converter, SIGNAL(information_message(const QString &)),
          this,      SLOT(log_information(const QString &)));

  connect(converter, SIGNAL(finished()),
          this,      SLOT(increment_global_progress()));

  for(auto bar: m_progress_bars.keys())
  {
    if(m_progress_bars[bar] == nullptr)
    {
      m_progress_bars[bar] = converter;
      bar->setValue(0);
      bar->setEnabled(true);
      bar->setFormat(message);

      connect(converter, SIGNAL(progress(int)),
              bar,       SLOT(setValue(int)));

      break;
    }
  }
}

//-----------------------------------------------------------------
void ProcessDialog::onClipboardPressed() const
{
  m_log->selectAll();
  m_log->copy();
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
