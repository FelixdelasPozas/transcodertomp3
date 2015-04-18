/*
 File: ProcessDialog.cpp
 Created on: 16/4/2015
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

// Application
#include <ProcessDialog.h>
#include "Utils.h"

// Qt
#include <QLayout>
#include <QFileInfo>
#include <QProgressBar>

//-----------------------------------------------------------------
ProcessDialog::ProcessDialog(const QList<QFileInfo> &files, const int threadsNum)
: m_musicFiles{files}
, m_numThreads{threadsNum}
{
  setupUi(this);

  connect(m_cancelButton, SIGNAL(clicked()),
          this,           SLOT(stop()));

  setWindowFlags(windowFlags() & ~(Qt::WindowContextHelpButtonHint) & Qt::WindowMaximizeButtonHint);

  cleanConfiguration.checkNumberPrefix = true;
  cleanConfiguration.replaceCharacters << QPair<QChar, QChar>('_', ' ') << QPair<QChar, QChar>('.', ' ')
                                       << QPair<QChar, QChar>('[', '(') << QPair<QChar, QChar>(']', ')');
  cleanConfiguration.numberAndNameSeparator = '-';
  cleanConfiguration.numberDigits = 2;
  cleanConfiguration.toTitleCase = true;

  for(auto file: files)
  {
    m_log->append(Utils::cleanName(file.absoluteFilePath(), cleanConfiguration));
  }

  auto boxLayout = new QVBoxLayout();

  for(auto i = 0; i < threadsNum; ++i)
  {
    auto bar = std::make_shared<QProgressBar>(this);
    bar->setAlignment(Qt::AlignCenter);
    bar->setValue(i*10);
    bar->setFormat(QString("%1 %").arg(bar->value()));
    m_progressGUI << bar;
    boxLayout->addWidget(bar.get(),1);
  }
  m_converters->setLayout(boxLayout);
}

//-----------------------------------------------------------------
ProcessDialog::~ProcessDialog()
{
  m_progressGUI.clear();
}

//-----------------------------------------------------------------
void ProcessDialog::log(const QString &message)
{
}

//-----------------------------------------------------------------
void ProcessDialog::progress(int value, int converter)
{
  m_progressGUI[converter]->setValue(value);
}

//-----------------------------------------------------------------
void ProcessDialog::stop()
{
//  for(auto converter: m_converterThreads)
//  {
//    converter->stop();
//  }
}

