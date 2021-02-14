/*
 File: MusicTranscoder.cpp
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
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>

// C++
#include <thread>

//-----------------------------------------------------------------
MusicTranscoder::MusicTranscoder()
: m_messageVisible{false}
{
	setupUi(this);

	m_configuration.load();

  m_directoryText->setText(QDir::toNativeSeparators(m_configuration.rootDirectory()));
  m_threads->setValue(m_configuration.numberOfThreads());

	m_threads->setMinimum(1);
	m_threads->setMaximum(std::thread::hardware_concurrency());

	connect(m_directoryButton, SIGNAL(clicked()),
	        this,              SLOT(onDirectoryChanged()));

	connect(m_directoryText, SIGNAL(editingFinished()),
	        this,            SLOT(onTextChanged()));

	connect(m_startButton, SIGNAL(clicked()),
	        this,          SLOT(onConversionStarted()));

  connect(m_aboutButton, SIGNAL(clicked()),
          this,          SLOT(onAboutButtonPressed()));

  connect(m_configButton, SIGNAL(clicked()),
          this,           SLOT(onConfigurationButtonPressed()));

  connect(m_threads, SIGNAL(valueChanged(int)),
          this,      SLOT(onThreadsNumberChanged(int)));
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
  fileBrowser.setDirectory(Utils::validDirectoryCheck(m_directoryText->text()));
  fileBrowser.setWindowTitle("Select root directory");
  fileBrowser.setFileMode(QFileDialog::Directory);
  fileBrowser.setOption(QFileDialog::DontUseNativeDialog, false);
  fileBrowser.setOption(QFileDialog::ShowDirsOnly);
  fileBrowser.setViewMode(QFileDialog::List);
  fileBrowser.setWindowIcon(QIcon(":/MusicTranscoder/folder.ico"));

  if(fileBrowser.exec() == QDialog::Accepted)
  {
    auto newDirectory = QDir::toNativeSeparators(fileBrowser.selectedFiles().first());
    m_configuration.setRootDirectory(newDirectory);
    m_directoryText->setText(newDirectory);
  }
}

//-----------------------------------------------------------------
void MusicTranscoder::onConversionStarted()
{
  QList<QFileInfo> files;
  QList<QFileInfo> folders;

  QStringList filters;
  if(m_configuration.transcodeAudio())
  {
    filters << Utils::WAVE_FILE_EXTENSIONS;
  }

  if(m_configuration.transcodeVideo())
  {
    filters << Utils::MOVIE_FILE_EXTENSIONS;
  }

  if(m_configuration.transcodeModule())
  {
    filters << Utils::MODULE_FILE_EXTENSIONS;
  }

  if(!filters.empty())
  {
    files = Utils::findFiles(m_directoryText->text(), filters);
  }

  if(m_configuration.createM3Ufiles())
  {
    auto foldersCondition = [](const QFileInfo &info) { return info.isDir(); };
    folders = Utils::findFiles(m_directoryText->text(), QStringList(), true, foldersCondition);
  }

  folders << QFileInfo(m_directoryText->text());

  if(files.empty() && folders.empty())
  {
    QMessageBox msgBox;
    msgBox.setText("Can't find any file in the specified folder that can be processed.");
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setWindowIcon(QIcon(":/MusicTranscoder/application.ico"));
    msgBox.setWindowTitle(QObject::tr("Unable to start the conversion process"));
    msgBox.exec();

    return;
  }

  this->hide();

  ProcessDialog processDialog(files, folders, m_configuration);
  processDialog.exec();

  this->showNormal();
}

//-----------------------------------------------------------------
void MusicTranscoder::onAboutButtonPressed()
{
  AboutDialog dialog(this);
  dialog.setModal(true);
  dialog.exec();
}

//-----------------------------------------------------------------
void MusicTranscoder::onConfigurationButtonPressed()
{
  ConfigurationDialog dialog(m_configuration);
  dialog.setModal(true);
  if(dialog.exec() == QDialog::Accepted)
  {
    m_configuration = dialog.getConfiguration();

    // this configuration doesn't contain these values.
    m_configuration.setRootDirectory(m_directoryText->text());
    m_configuration.setNumberOfThreads(m_threads->value());
  }
}

//-----------------------------------------------------------------
void MusicTranscoder::onThreadsNumberChanged(int value)
{
  m_configuration.setNumberOfThreads(value);
}

//-----------------------------------------------------------------
bool MusicTranscoder::event(QEvent* e)
{
  if(e->type() == QEvent::KeyPress)
  {
    auto ke = dynamic_cast<QKeyEvent *>(e);
    if(ke && ke->key() == Qt::Key_Escape)
    {
      e->accept();
      close();
      return true;
    }
  }

  return QMainWindow::event(e);
}

//-----------------------------------------------------------------
void MusicTranscoder::onTextChanged()
{
  // Qt bug: QLineEdit launches the signal twice, once if the user press enter and another
  // for losing the focus. Still not addressed in 5.8 even when it was reported in 4.x series.
  // This is a hack but fixes it to avoid showing two message boxes.
  if(m_messageVisible) return;

  auto directory = m_directoryText->text();
  directory = QDir::toNativeSeparators(Utils::validDirectoryCheck(directory));

  if((directory != m_directoryText->text()) && (directory + QDir::separator() != m_directoryText->text()))
  {
    m_messageVisible = true;
    auto icon    = QIcon(":MusicTranscoder/application.ico");
    auto title   = tr("Invalid directory");
    auto message = tr("The directory entered is not valid.");
    QMessageBox msgBox(QMessageBox::Icon::Critical, title, message, QMessageBox::StandardButtons{QMessageBox::Ok}, this->centralWidget());
    msgBox.setWindowIcon(icon);
    msgBox.exec();

    m_directoryText->setText(directory);
    m_configuration.setRootDirectory(directory);
  }

  m_messageVisible = false;
}

//-----------------------------------------------------------------
void MusicTranscoder::closeEvent(QCloseEvent* event)
{
  disconnect(m_directoryText, SIGNAL(editingFinished()),
             this,            SLOT(onTextChanged()));

  QMainWindow::closeEvent(event);
}
