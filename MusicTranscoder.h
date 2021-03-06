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

/** \class MusicTranscoder
 * \brief Application main window.
 *
 */
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

		virtual bool event(QEvent *e) override;

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

	  /** \brief Checks the directory when the user enters it manually in the directory field.
	   *
	   */
	  void onTextChanged();

	protected:
	  virtual void closeEvent(QCloseEvent *event);

	private:
	  Utils::TranscoderConfiguration m_configuration;  /** application configuration.                                    */
	  std::atomic<bool>              m_messageVisible; /** fix Qt double signal in onTextChanged(). See notes in method. */
};

#endif // MUSIC_TRANSCODER_H_
