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

// Tag parser
#include <version.h>

// libopenmpt
#include <libopenmpt/libopenmpt.hpp>

// Qt
#include <QtGlobal>

const QString AboutDialog::VERSION = QString("version 1.3.4");

//-----------------------------------------------------------------
AboutDialog::AboutDialog(QWidget *parent, Qt::WindowFlags flags)
: QDialog(parent, flags)
{
  setupUi(this);

  setWindowFlags(windowFlags() & ~(Qt::WindowContextHelpButtonHint) & ~(Qt::WindowMaximizeButtonHint) & ~(Qt::WindowMinimizeButtonHint));

  const auto compilation_date = QString(__DATE__);
  const auto compilation_time = QString(" (") + QString(__TIME__) + QString(")");

  m_compilationDate->setText(tr("Compiled on ") + compilation_date + compilation_time);
  m_version->setText(VERSION);

  m_lameVersion->setText(tr("version %1").arg(get_lame_version()));
  m_libavVersion->setText(tr("version %1").arg(LIBAV_VERSION));
  // m_libcueVersion->setText(tr("")); // does not provide version definition, go with the one in the .ui file.
  m_tagparserVersion->setText(tr("version %1").arg(TAG_PARSER_VERSION_STR));
  m_openmptVersion->setText(tr("version %1").arg(QString::fromStdString(openmpt::string::get("library_version"))));
  m_qtVersion->setText(tr("version %1").arg(QT_VERSION_STR));
}

