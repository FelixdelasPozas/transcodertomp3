/*
 File: MusicConverter.h
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

#ifndef MUSIC_CONVERTER_H_
#define MUSIC_CONVERTER_H_

// Application
#include "ui_MusicConverter.h"

// Qt
#include <QMainWindow>
#include <QFileInfo>

class MusicConverter
: public QMainWindow
, public Ui_MusicConverter
{
	Q_OBJECT

	public:
		explicit MusicConverter();
		~MusicConverter();

	private slots:
	  void onDirectoryChanged();
	  void onConversionStarted();

	private:
	  enum class MusicFileType: unsigned char { UNKNOWN = 1, WMA = 2, M4A = 3, FLAC = 4, APE = 5, WAV = 6, TRACKER = 7 };

	  static const QString ROOT_DIRECTORY;
	  static const QString NUMBER_OF_THREADS;
	  static const QString CLEAN_FILENAMES;

	  void loadSettings();

	  QList<QFileInfo> m_files;
};

#endif // MUSIC_CONVERTER_H_
