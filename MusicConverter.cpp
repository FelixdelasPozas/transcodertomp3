// Project
#include "MusicConverter.h"

// Qt
#include <QSettings>
#include <QFileDialog>
#include <QDebug>
#include <QMessageBox>

// C++
#include <thread>

const QString MusicConverter::ROOT_DIRECTORY = QString("Root Directory");
const QString MusicConverter::NUMBER_OF_THREADS = QString("Number Of Threads");

//-----------------------------------------------------------------
MusicConverter::MusicConverter()
: m_directory{}
, m_threadsNum{0}
{
	setupUi(this);

	setWindowTitle(QObject::tr("Music Converter To MP3"));
	setWindowIcon(QIcon(":/MusicConverter/application.ico"));

	loadConfiguration();

	m_threads->setMinimum(1);
	m_threads->setMaximum(std::thread::hardware_concurrency());
	m_threads->setValue(m_threadsNum);

	m_directoryText->setText(m_directory.absolutePath().replace('/','\\'));

	connect(m_directoryButton, SIGNAL(clicked()),
	        this,              SLOT(changeDirectory()));

	connect(m_startButton, SIGNAL(clicked()),
	        this,          SLOT(startConversion()));
}

//-----------------------------------------------------------------
MusicConverter::~MusicConverter()
{
  saveConfiguration();
}

//-----------------------------------------------------------------
void MusicConverter::loadConfiguration()
{
  QSettings settings("MusicConverter.ini", QSettings::IniFormat);

  if (settings.contains(ROOT_DIRECTORY))
    m_directory.setPath(settings.value(ROOT_DIRECTORY).toString());
  else
    m_directory.setPath(QDir::currentPath());

  if (settings.contains(NUMBER_OF_THREADS))
    m_threadsNum = settings.value(NUMBER_OF_THREADS).toInt();
  else
    m_threadsNum = std::thread::hardware_concurrency()/2;
}

//-----------------------------------------------------------------
void MusicConverter::saveConfiguration()
{
  QSettings settings("MusicConverter.ini", QSettings::IniFormat);

  settings.setValue(ROOT_DIRECTORY, m_directory.absolutePath());
  settings.setValue(NUMBER_OF_THREADS, m_threads->value());

  settings.sync();
}

//-----------------------------------------------------------------
void MusicConverter::changeDirectory()
{
  QFileDialog fileBrowser;

  fileBrowser.setDirectory(m_directory.absolutePath());
  fileBrowser.setWindowTitle("Select root directory");
  fileBrowser.setFileMode(QFileDialog::Directory);
  fileBrowser.setOption(QFileDialog::DontUseNativeDialog, false);
  fileBrowser.setOption(QFileDialog::ShowDirsOnly);
  fileBrowser.setViewMode(QFileDialog::List);

  if(fileBrowser.exec() == QDialog::Accepted)
  {
    auto newDirectory = fileBrowser.selectedFiles().first();
    m_directory.setPath(newDirectory);
    m_directoryText->setText(newDirectory.replace('/','\\'));
  }
}

//-----------------------------------------------------------------
void MusicConverter::startConversion()
{
  findMusicFiles();

  if(m_files.empty())
  {
    QMessageBox msgBox;
    msgBox.setText("The specified folder doesn't contain files of the types that can be converted.");
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setWindowIcon(QIcon(":/MusicConverter/application.ico"));
    msgBox.setWindowTitle(QObject::tr("Unable to start conversion process"));
    msgBox.exec();

    return;
  }
}

//-----------------------------------------------------------------
void MusicConverter::findMusicFiles()
{
  // TODO: recorrer el arbol de directorios a partir del root y rellenar m_files con los ficheros a convertir.
}
