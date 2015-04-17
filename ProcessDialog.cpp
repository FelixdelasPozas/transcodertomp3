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

//-----------------------------------------------------------------
ProcessDialog::ProcessDialog(const QList<QFileInfo> &files, const int threadsNum)
{
  setupUi(this);

  Utils::CleanConfiguration conf;
  conf.checkNumberPrefix = true;
  conf.replaceCharacters << QPair<QChar, QChar>('_', ' ') << QPair<QChar, QChar>('.', ' ');
  conf.numberAndNameSeparator = '-';
  conf.numberDigits = 2;
  conf.toTitleCase = true;

  for(auto file: files)
  {
    m_log->append(Utils::cleanName(file.absoluteFilePath(), conf));
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
void ProcessDialog::stop()
{
}

