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
	  /** \brief MusicConverter class constructor.
	   *
	   */
		explicit MusicConverter();

		/** \brief MusicConverter class destructor.
		 *
		 */
		~MusicConverter();

	private slots:
	  /** \brief Displays the directory selection dialog.
	   *
	   */
	  void onDirectoryChanged();

	  /** \brief Starts the conversion process if files can be found in the specified directory.
	   *
	   */
	  void onConversionStarted();

	private:
	  static const QString ROOT_DIRECTORY;
	  static const QString NUMBER_OF_THREADS;

	  /** \brief Loads the program settings.
	   *
	   */
	  void loadSettings();

	  /** \brief Saves the program settings.
	   *
	   */
	  void saveSettings() const;

	  QList<QFileInfo> m_files;
};

#endif // MUSIC_CONVERTER_H_
