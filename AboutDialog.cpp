/*
 File: AboutDialog.cpp
 Created on: 13/5/2015
 Author: Felix

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
#include <AboutDialog.h>

// Lame
#include <lame.h>

// libAV
#include <avversion.h>

// TagLib
#include <taglib.h>

// libopenmpt
#include <libopenmpt/libopenmpt.hpp>

// Qt
#include <QtGlobal>

const QString AboutDialog::VERSION = QString("version 1.2.6");

//-----------------------------------------------------------------
AboutDialog::AboutDialog(QWidget *parent, Qt::WindowFlags flags)
: QDialog{parent, flags}
{
  setupUi(this);

  setWindowFlags(windowFlags() & ~(Qt::WindowContextHelpButtonHint) & ~(Qt::WindowMaximizeButtonHint) & ~(Qt::WindowMinimizeButtonHint));

  auto compilation_date = QString(__DATE__);
  auto compilation_time = QString(" (") + QString(__TIME__) + QString(")");

  m_compilationDate->setText(tr("Compiled on ") + compilation_date + compilation_time);
  m_version->setText(VERSION);

  m_lameVersion->setText(tr("version %1").arg(get_lame_version()));
  m_libavVersion->setText(tr("version %1").arg(LIBAV_VERSION));
  // m_libcueVersion->setText(tr("")); // does not provide version definition, go with the one in the .ui file.
  m_taglibVersion->setText(tr("version %1.%2.%3").arg(TAGLIB_MAJOR_VERSION).arg(TAGLIB_MINOR_VERSION).arg(TAGLIB_PATCH_VERSION));
  const int omptVersion = openmpt::get_library_version();
  m_openmptVersion->setText(tr("version %1.%2.%3").arg(omptVersion >> 24 & 0xF).arg(omptVersion >> 16 & 0xF).arg(omptVersion & 0XFF));
  m_qtVersion->setText(tr("version %1.%2.%3").arg(QT_VERSION_MAJOR).arg(QT_VERSION_MINOR).arg(QT_VERSION_PATCH));
}

