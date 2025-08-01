/*
 File: Main.cpp
 Created on: 14/4/2015
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

// Qt
#include <QApplication>
#include <QMessageBox>
#include <QSharedMemory>

// Project
#include <MusicTranscoder.h>

// C++
#include <iostream>

//-----------------------------------------------------------------
void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
  const char symbols[] = { 'I', 'E', '!', 'X' };
//  auto output = QString("[%1] %2 (%3:%4 -> %5)").arg( symbols[type] ).arg( msg ).arg(context.file).arg(context.line).arg(context.function);
  auto output = QString("[%1] %2").arg(symbols[type]).arg(msg);
  std::cerr << output.toStdString() << std::endl;
  if (type == QtFatalMsg) abort();
}

//-----------------------------------------------------------------
int main(int argc, char *argv[])
{
  qInstallMessageHandler(myMessageOutput);

	QApplication app(argc, argv);

  // allow only one instance
  QSharedMemory guard;
  guard.setKey("MusicTranscoder");

  if (!guard.create(1))
  {
    QMessageBox msgBox;
    msgBox.setWindowIcon(QIcon(":/MusicTranscoder/application.svg"));
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setText("Music Transcoder To MP3 is already running!");
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
    std::exit(0);
  }

	MusicTranscoder transcoder;
	transcoder.show();
	return app.exec();
}


