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

//-----------------------------------------------------------------
MusicTranscoder::MusicTranscoder()
{
	setupUi(this);

	m_configuration.load();

  m_directoryText->setText(m_configuration.rootDirectory());
  m_threads->setValue(m_configuration.numberOfThreads());

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
  m_configuration.save();
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
  m_files = Utils::findFiles(m_directoryText->text(), Utils::WAVE_FILE_EXTENSIONS + Utils::MOVIE_FILE_EXTENSIONS + Utils::MODULE_FILE_EXTENSIONS);

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

  m_configuration.setRootDirectory(m_directoryText->text());
  m_configuration.setNumberOfThreads(m_threads->value());

  ProcessDialog pd(m_files, m_configuration);
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
    m_configuration = dialog.getConfiguration();
  }
}
