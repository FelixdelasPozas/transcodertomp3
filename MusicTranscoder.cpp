/*
 File: MusicConverter.cpp
 Created on: 15/4/2015
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

// Project
#include "MusicTranscoder.h"
#include "ProcessDialog.h"
#include "AboutDialog.h"
#include "ConfigurationDialog.h"
#include "Utils.h"

// Qt
#include <QDir>
#include <QDirIterator>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>

// C++
#include <thread>

const QString MusicTranscoder::ROOT_DIRECTORY = QString("Root Directory");
const QString MusicTranscoder::NUMBER_OF_THREADS = QString("Number Of Threads");

const QStringList MusicTranscoder::MODULE_FILE_EXTENSIONS = {"*.669", "*.amf", "*.apun", "*.dsm", "*.far", "*.gdm", "*.it", "*.imf", "*.mod", "*.med", "*.mtm", "*.okt", "*.s3m", "*.stm", "*.stx", "*.ult", "*.uni", "*.xt", "*.xm"};
const QStringList MusicTranscoder::WAVE_FILE_EXTENSIONS   = {"*.flac", "*.ogg", "*.ape", "*.wav", "*.wma", "*.m4a", "*.voc", "*.wv", "*.mp3"};
const QStringList MusicTranscoder::MOVIE_FILE_EXTENSIONS  = {"*.mp4", "*.avi", "*.ogv", "*.webm" };

//-----------------------------------------------------------------
MusicTranscoder::MusicTranscoder()
{
	setupUi(this);

	loadSettings();

	m_threads->setMinimum(1);
	m_threads->setMaximum(std::thread::hardware_concurrency());

	connect(m_directoryButton, SIGNAL(clicked()),
	        this,              SLOT(onDirectoryChanged()));

	connect(m_startButton, SIGNAL(clicked()),
	        this,          SLOT(onConversionStarted()));

  connect(m_aboutButton, SIGNAL(clicked()),
          this,          SLOT(onAboutButtonPressed()));

  connect(m_configButton, SIGNAL(clicked()),
          this,           SLOT(onConfigurationButtonPressed()));
}

//-----------------------------------------------------------------
MusicTranscoder::~MusicTranscoder()
{
  saveSettings();
}

//-----------------------------------------------------------------
void MusicTranscoder::onDirectoryChanged()
{
  QFileDialog fileBrowser;

  fileBrowser.setDirectory(m_directoryText->text());
  fileBrowser.setWindowTitle("Select root directory");
  fileBrowser.setFileMode(QFileDialog::Directory);
  fileBrowser.setOption(QFileDialog::DontUseNativeDialog, false);
  fileBrowser.setOption(QFileDialog::ShowDirsOnly);
  fileBrowser.setViewMode(QFileDialog::List);

  if(fileBrowser.exec() == QDialog::Accepted)
  {
    auto newDirectory = fileBrowser.selectedFiles().first();
    m_directoryText->setText(newDirectory.replace('/','\\'));
  }
}

//-----------------------------------------------------------------
void MusicTranscoder::onConversionStarted()
{
  m_files = Utils::findFiles(m_directoryText->text(), WAVE_FILE_EXTENSIONS + MOVIE_FILE_EXTENSIONS + MODULE_FILE_EXTENSIONS);

  if(m_files.empty())
  {
    QMessageBox msgBox;
    msgBox.setText("Can't find any file in the specified folder that can be processed.");
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setWindowIcon(QIcon(":/MusicConverter/application.ico"));
    msgBox.setWindowTitle(QObject::tr("Unable to start the conversion process"));
    msgBox.exec();

    return;
  }

  this->hide();

  ProcessDialog pd(m_files, m_threads->value());
  pd.exec();

  this->show();
}

//-----------------------------------------------------------------
void MusicTranscoder::onAboutButtonPressed()
{
  AboutDialog dialog;
  dialog.setModal(true);
  dialog.exec();
}

//-----------------------------------------------------------------
void MusicTranscoder::onConfigurationButtonPressed()
{
  ConfigurationDialog dialog;
  dialog.setModal(true);
  if(dialog.exec() == QDialog::Accepted)
  {
    // TODO
  }
}


//-----------------------------------------------------------------
void MusicTranscoder::loadSettings()
{
  QSettings settings("MusicConverter.ini", QSettings::IniFormat);

  m_directoryText->setText(settings.value(ROOT_DIRECTORY, QDir::currentPath()).toString());
  m_threads->setValue(settings.value(NUMBER_OF_THREADS, std::thread::hardware_concurrency()/2).toInt());
}

//-----------------------------------------------------------------
void MusicTranscoder::saveSettings() const
{
  QSettings settings("MusicConverter.ini", QSettings::IniFormat);

  settings.setValue(ROOT_DIRECTORY, m_directoryText->text());
  settings.setValue(NUMBER_OF_THREADS, m_threads->value());

  settings.sync();
}

//-----------------------------------------------------------------
bool isAudioFile(const QFileInfo& file)
{
  auto extension = file.absoluteFilePath().split('.').last().toLower();

  return MusicTranscoder::WAVE_FILE_EXTENSIONS.contains("*." + extension);
}

//-----------------------------------------------------------------
bool isVideoFile(const QFileInfo& file)
{
  auto extension = file.absoluteFilePath().split('.').last().toLower();

  return MusicTranscoder::MOVIE_FILE_EXTENSIONS.contains("*." + extension);
}

//-----------------------------------------------------------------
bool isModuleFile(const QFileInfo& file)
{
  auto extension = file.absoluteFilePath().split('.').last().toLower();

  return MusicTranscoder::MODULE_FILE_EXTENSIONS.contains("*." + extension);
}
