// Project
#include "MusicConverter.h"

// Qt
#include <QSettings>
#include <QFileDialog>
#include <QDebug>
#include <QMessageBox>
#include <QStack>
#include <QDirIterator>

// C++
#include <thread>

const QString MusicConverter::ROOT_DIRECTORY = QString("Root Directory");
const QString MusicConverter::NUMBER_OF_THREADS = QString("Number Of Threads");
const QString MusicConverter::CLEAN_FILENAMES = QString("Clean File Names");

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

  m_directory.setPath(settings.value(ROOT_DIRECTORY, QDir::currentPath()).toString());
  m_threadsNum = settings.value(NUMBER_OF_THREADS, std::thread::hardware_concurrency()/2).toInt();
  m_cleanNames->setChecked(settings.value(CLEAN_FILENAMES, true).toBool());
}

//-----------------------------------------------------------------
void MusicConverter::saveConfiguration()
{
  QSettings settings("MusicConverter.ini", QSettings::IniFormat);

  settings.setValue(ROOT_DIRECTORY, m_directory.absolutePath());
  settings.setValue(NUMBER_OF_THREADS, m_threads->value());
  settings.setValue(CLEAN_FILENAMES, m_cleanNames->isChecked());

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
  m_directory.setFilter(QDir::Files|QDir::Dirs|QDir::NoDot|QDir::NoDotDot);
  QStringList fileTypes;
  fileTypes << "*.ogg" << "*.flac" << "*.wma" << "*.m4a" << "*.mod" << "*.it" << "*.s3m" << "*.xt" << "*.wav" << "*.ape";
  m_directory.setNameFilters(fileTypes);

  QDirIterator it(m_directory, QDirIterator::Subdirectories);
  while(it.hasNext())
  {
    it.next();

    m_files << it.fileInfo();
  }
}
