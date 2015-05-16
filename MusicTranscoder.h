/*
 File: MusicTranscoder.h
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

#ifndef MUSIC_TRANSCODER_H_
#define MUSIC_TRANSCODER_H_

// Application
#include "Utils.h"
#include "ui_MusicTranscoder.h"

// Qt
#include <QMainWindow>

class QFileInfo;

class MusicTranscoder
: public QMainWindow
, public Ui_MusicTranscoder
{
	Q_OBJECT

	public:
	  /** \brief MusicTranscoder class constructor.
	   *
	   */
		explicit MusicTranscoder();

		/** \brief MusicTranscoder class destructor.
		 *
		 */
		~MusicTranscoder();

	private slots:
	  /** \brief Displays the directory selection dialog.
	   *
	   */
	  void onDirectoryChanged();

	  /** \brief Starts the conversion process if files can be found in the specified directory.
	   *
	   */
	  void onConversionStarted();

	  /** \brief Shows the About dialog
	   *
	   */
	  void onAboutButtonPressed();

	  /** \brief Shows the configuration dialog.
	   *
	   */
	  void onConfigurationButtonPressed();

	  /** \brief Updates the configuration threads number value.
	   *
	   */
	  void onThreadsNumberChanged(int value);

	private:
	  QList<QFileInfo>               m_files;
	  Utils::TranscoderConfiguration m_configuration;
};

#endif // MUSIC_TRANSCODER_H_
