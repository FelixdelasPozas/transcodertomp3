#ifndef MUSIC_CONVERTER_H_
#define MUSIC_CONVERTER_H_

// Application
#include "ui_MusicConverter.h"

// Qt
#include <QDir>
#include <QMainWindow>

class MusicConverter
: public QMainWindow
, public Ui_MusicConverter
{
	Q_OBJECT

	public:
		explicit MusicConverter();
		~MusicConverter();

	private slots:
	  void changeDirectory();
	  void startConversion();

	private:
	  enum class MusicFileType: unsigned char { UNKNOWN = 1, WMA = 2, M4A = 3, FLAC = 4, APE = 5, WAV = 6, TRACKER = 7 };
	  struct MusicFile
	  {
	    QString fileName;
	    MusicFileType type;
	    unsigned long long size;
	    unsigned long long convertedSize;

	    MusicFile(): fileName{}, type{MusicFileType::UNKNOWN}, size{0}, convertedSize{0} {};
	    ~MusicFile() {};
	  };

	  static const QString ROOT_DIRECTORY;
	  static const QString NUMBER_OF_THREADS;

	  void loadConfiguration();
	  void saveConfiguration();
	  void findMusicFiles();

	  QDir         m_directory;
	  unsigned int m_threadsNum;
	  QList<MusicFile> m_files;
};

#endif // MUSIC_CONVERTER_H_
