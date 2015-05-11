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

    static const QStringList MODULE_FILE_EXTENSIONS;
    static const QStringList WAVE_FILE_EXTENSIONS;
    static const QStringList MOVIE_FILE_EXTENSIONS;

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

/** \brief Returns true if the file given as parameter has a audio extension.
 * \param[in] file file QFileInfo struct.
 */
bool isAudioFile(const QFileInfo &file);

/** \brief Returns true if the file given as parameter has a video extension.
 * \param[in] file file QFileInfo struct.
 */
bool isVideoFile(const QFileInfo &file);

/** \brief Returns true if the file given as parameter has a module extension.
 * \param[in] file file QFileInfo struct.
 */
bool isModuleFile(const QFileInfo &file);

#endif // MUSIC_CONVERTER_H_
