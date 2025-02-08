/****************************************************************************
 ** Makou Reactor Final Fantasy VII Field Script Editor
 ** Copyright (C) 2009-2013 Arzel Jérôme <myst6re@gmail.com>
 **
 ** This program is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation, either version 3 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/
// ********* Félix de las Pozas: slightly modified from the original source.
#ifndef QTASKBARBUTTON_H
#define QTASKBARBUTTON_H

// Qt
#include <QObject>
#include <QPixmap>
class QWidget;

#ifdef Q_OS_WIN
#include <qwindowdefs.h>
#include <shobjidl.h>
#define QTASKBAR_WIN
#endif

/** \class QTaskBarButton
 * \brief Implements a progress bar and overlay icon in a Windows tray icon. 
 *
 */
class QTaskBarButton
: public QObject
{
		Q_OBJECT
	public:
		/** \brief State of the icon */
		enum State
		{
			Invisible,		 /** overlay icon and progress not visible. */
			Normal,				 /** overlay icon and progress visible. */
			Indeterminate, /** indeterminate status. */
			Paused,				 /** paused state.  */
			Error					 /** error state. */
		};

		/** \brief QTaskBarButton class constructor. 
		 * \param[in] widget Raw pointer of the widget parent of this one. 
		 *
		 */
		explicit QTaskBarButton(QWidget *widget = nullptr);

		/** \brief QTaskBarButton class virtual destructor.
		 *
		 */
		virtual ~QTaskBarButton();

		/** \brief Sets if the overlay icon if the given pixmap is not null.
		 * \param[in] pixmap Icon pixmap reference. 
		 * \param[in] text Icon text. 
		 *
		 */
		void setOverlayIcon(const QPixmap &pixmap, const QString &text = QString());

		/** \brief Sets the tray overlay icon and progress state. 
		 * \param[in] state State enum constant.
		 *
		 */
		void setState(State state);

		/** \brief Returns the progress minimum value.
		 *
		 */
		int maximum() const;

		/** \brief Returns the progress maximum value. 
		 *
		 */
		int minimum() const;

		/** \brief Returns the tray overlay icon and progress current state. 
		 *
		 */
		State state() const;

		/** \brief Returns the progress current value. 
		 *
		 */
		int value() const;

		/** \brief Restarts the tray icon progress and overlay. Needed if usen a tray application when main dialog hides.
		 *
		 */
		void restart();

	signals:
		void valueChanged(int value);
	public slots:
		/** \brief Resets the overlay and progress values and sets state to Normal. 
		 *
		 */
		void reset();

		/** \brief Sets the maximum progress value. 
		 * \param[in] maximum integer value.
		 *
		 */
		void setMaximum(int maximum);

		/** \brief Sets the minimum progress value. 
		 * \param[in] maximum integer value.
		 *
		 */
		void setMinimum(int minimum);

		/** \brief Sets the progress range. 
		 * \param[in] minimum integer value. 
		 * \param[in] maximum integer value. 
		 *
		 */
		void setRange(int minimum, int maximum);

		/** \brief Sets the progress value.
		 * \param[in] value integer value.
		 */
		void setValue(int value);

	private:
		/** \brief Initializes the icon.
		 *
		 */
		void initialize();

		/** \brief Deinitializes the icon.
		 *
		 */
		void shutdown();

		/** \brief Helper method that implements the setting and removing of the overlay icon. 
		 *
		 */
		void setOverlayIconImplementation();

		QWidget *m_parent; /** parent widget pointer. */
		QPixmap m_pixmap;	 /** overlay icon */
		QString m_text;		 /** overlay text */

#ifdef QTASKBAR_WIN
		WId _winId;						 /** Window id in Win32 API format. */
		ITaskbarList3 *pITask; /** Implements ITaskbarList3 interfacce. */
#endif // Q_OS_WIN
		int _minimum;					 /** Progress minimum value. */
		int _maximum;					 /** Progress maximum value. */
		int _value;						 /** Progress current value. */
		State _state;					 /** Current Icon state. */
};

#endif // QTASKBARBUTTON_H
