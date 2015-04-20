/*
		File: FSUtils.cpp
    Created on: 17/4/2015
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
#include "Utils.h"

// Qt
#include <QDirIterator>
#include <QDebug>

//-----------------------------------------------------------------
QList<QFileInfo> Utils::findFiles(const QDir initialDir, const QStringList extensions)
{
  QList<QFileInfo> filesFound;

  auto startingDir = initialDir;
  startingDir.setFilter(QDir::Files|QDir::Dirs|QDir::NoDot|QDir::NoDotDot);
  startingDir.setNameFilters(extensions);

  QDirIterator it(startingDir, QDirIterator::Subdirectories);
  while(it.hasNext())
  {
    it.next();

    filesFound << it.fileInfo();
  }

  return filesFound;
}

//-----------------------------------------------------------------
QString Utils::cleanName(const QString filename, const Utils::CleanConfiguration conf)
{
  // Just the name
  auto fileInfo = QFileInfo(QFile(filename));
  auto name = fileInfo.absoluteFilePath().split('/').last();
  auto extension = name.split('.').last();
  QString cleanedName = name.remove(name.lastIndexOf('.'), extension.length() + 1);

  // Delete characters
  for(int i = 0; i < conf.chars_to_delete.length(); ++i)
  {
    cleanedName.remove(conf.chars_to_delete[i], Qt::CaseInsensitive);
  }

  // Replace characters
  for(int i = 0; i < conf.chars_to_replace.size(); ++i)
  {
    auto charPair = conf.chars_to_replace[i];
    cleanedName.replace(charPair.first, charPair.second, Qt::CaseInsensitive);
  }

  // Get the parts and remove consecutive spaces.
  QStringList parts = cleanedName.split(' ');
  parts.removeAll("");

  cleanedName.clear();
  int index = 0;

  // Adjust the number prefix and insert the default separator.
  if(conf.check_number_prefix)
  {
    QRegExp re("\\d*");
    if (re.exactMatch(parts[index]))
    {
      while(conf.number_of_digits > parts[index].length())
      {
        parts[index] = "0" + parts[index];
      }

      if((parts.size() > index) && (parts[index+1] != QString(conf.number_and_name_separator)))
      {
        parts[index] += QString(' ' + conf.number_and_name_separator + ' ');
      }
      else
      {
        parts[index+1] = QString(' ' + conf.number_and_name_separator);
      }

      cleanedName = parts[index];
      ++index;
    }
  }

  // Capitalize the first letter of every word.
  if(conf.to_title_case)
  {
    int i = index;
    while(i < parts.size())
    {
      if(parts[i].isEmpty()) continue;
      bool starts_with_parenthesis = false;
      bool ends_with_parenthesis = false;

      if(parts[i].startsWith('(') && parts[i].size() > 1)
      {
        starts_with_parenthesis = true;
        parts[i].remove('(');
      }

      if(parts[i].endsWith(')') && parts[i].size() > 1)
      {
        ends_with_parenthesis = true;
        parts[i].remove(')');
      }

      parts[i] = parts[i].toLower();

      parts[i].replace(0,1, parts[i][0].toUpper());

      if(starts_with_parenthesis)
      {
        parts[i] = QString('(') + parts[i];
      }

      if(ends_with_parenthesis)
      {
        parts[i] = parts[i] + QString(')');
      }

      ++i;
    }
  }

  cleanedName += parts[index++];

  // Compose the name.
  while(index < parts.size())
  {
    cleanedName += ' ' + parts[index++];
  }
  cleanedName += ".mp3";

  return cleanedName;
}
