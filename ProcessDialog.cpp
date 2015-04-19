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
#include "Utils.h"
#include "OggConverter.h"

// Qt
#include <QLayout>
#include <QFileInfo>
#include <QProgressBar>
#include <QMutexLocker>
#include <QDebug>

//-----------------------------------------------------------------
ProcessDialog::ProcessDialog(const QList<QFileInfo> &files, const int threads_num)
: m_music_files{files}
, m_num_threads{threads_num}
{
  setupUi(this);

  connect(m_cancelButton, SIGNAL(clicked()),
          this,           SLOT(stop()));

  setWindowFlags(windowFlags() & ~(Qt::WindowContextHelpButtonHint) & Qt::WindowMaximizeButtonHint);

  m_clean_configuration.checkNumberPrefix = true;
  m_clean_configuration.replaceCharacters << QPair<QChar, QChar>('_', ' ') << QPair<QChar, QChar>('.', ' ')
                                       << QPair<QChar, QChar>('[', '(') << QPair<QChar, QChar>(']', ')');
  m_clean_configuration.numberAndNameSeparator = '-';
  m_clean_configuration.numberDigits = 2;
  m_clean_configuration.toTitleCase = false;

  m_globalProgress->setMinimum(0);
  m_globalProgress->setMaximum(m_music_files.size());

  for(auto file: m_music_files)
  {
    log_information(Utils::cleanName(file.absoluteFilePath(), m_clean_configuration));
  }

  auto boxLayout = new QVBoxLayout();
  m_converters->setLayout(boxLayout);

  while (m_music_files.size() > 0)
  {
    qDebug() << "enter" << m_music_files.size();
    while (m_progress_GUI.size() < m_num_threads && m_music_files.size() > 0)
    {
      auto music_file = m_music_files.first();
      m_music_files.removeFirst();

      auto extension = music_file.absoluteFilePath().split('.').last().toLower();
      auto output_name = clean_output_name(music_file);
      auto file_output_name = output_name.split('/').last();

      ConverterThread *converter = nullptr;

      qDebug() << extension << output_name;
      if (extension.compare("ogg"))
      {
        converter = new OGGConverter(music_file, output_name);
      }
      else
      {
        qDebug() << "ein?";
        break;
      }

      // TODO separar GUI y procesamiento, hace falta un thread de gestión de threads y dejar el GUI aparte.

      connect(converter, SIGNAL(progress(int)), this, SLOT(increment_thread_progress(int)));
      connect(converter, SIGNAL(error_message(const QString &)), this, SLOT(log_error(const QString &)));
      connect(converter, SIGNAL(information_message(const QString &)), this, SLOT(log_information(const QString &)));
      connect(converter, SIGNAL(finished()), this, SLOT(increment_global_progress()));

      auto bar = new QProgressBar();
      bar->setAlignment(Qt::AlignCenter);
      bar->setMaximum(0);
      bar->setMaximum(100);
      bar->setValue(0);
      bar->setFormat(QString("%1 - %2 %").arg(output_name.split('/').last()).arg(bar->value()));

      m_progress_GUI[converter] = bar;

      boxLayout->addWidget(bar);

      converter->start();
    }

    m_mutex_main.lock();
    qDebug() << "stopped at condition";
    m_wait_condition.wait(&m_mutex_main);
  }
  qDebug() << "salida";
}

//-----------------------------------------------------------------
ProcessDialog::~ProcessDialog()
{
  m_progress_GUI.clear();
}

//-----------------------------------------------------------------
void ProcessDialog::log_error(const QString &message)
{
  QMutexLocker lock(&m_mutex);
  m_log->setTextColor(Qt::red);
  m_log->append(message);
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
  for(auto converter: m_converter_threads)
  {
    disconnect(converter.get(), SIGNAL(progress(int)),
               this,            SLOT(increment_thread_progress(int)));

    disconnect(converter.get(), SIGNAL(error_message(const QString &)),
               this,            SLOT(log_error(const QString &)));

    disconnect(converter.get(), SIGNAL(information_message(const QString &)),
               this,            SLOT(log_information(const QString &)));

    converter->stop();
  }
}

//-----------------------------------------------------------------
void ProcessDialog::increment_global_progress()
{
  QMutexLocker lock(&m_mutex);
  auto value = m_globalProgress->value();
  m_globalProgress->setValue(++value);

  m_wait_condition.wakeAll();
}

//-----------------------------------------------------------------
void ProcessDialog::increment_thread_progress(int value)
{
  auto converter = qobject_cast<ConverterThread *>(sender());
  m_progress_GUI[converter]->setValue(value);
}

//-----------------------------------------------------------------
const QString ProcessDialog::clean_output_name(const QFileInfo file_info) const
{
  auto name = file_info.absoluteFilePath().split('/').last();
  auto path = file_info.absoluteFilePath().remove(name);

  return path + Utils::cleanName(file_info.absoluteFilePath(), m_clean_configuration);
}
