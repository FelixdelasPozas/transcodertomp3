/*
	File: Utils.cpp
  Created on: 17/4/2015
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

// Project
#include "Utils.h"

// Qt
#include <QDirIterator>

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
QString Utils::formatString(const QString filename, const Utils::FormatConfiguration conf)
{
  // works for filenames and plain strings
  auto fileInfo = QFileInfo(QFile(filename));
  auto name = fileInfo.absoluteFilePath().split('/').last();
  auto extension = name.split('.').last();
  QString formattedName = name;
  if(name.contains('.'))
  {
    formattedName = name.remove(name.lastIndexOf('.'), extension.length() + 1);
  }

  // delete specified chars
  for(int i = 0; i < conf.chars_to_delete.length(); ++i)
  {
    formattedName.remove(conf.chars_to_delete[i], Qt::CaseInsensitive);
  }

  // replace specified strings
  for(int i = 0; i < conf.chars_to_replace.size(); ++i)
  {
    auto charPair = conf.chars_to_replace[i];
    formattedName.replace(charPair.first, charPair.second, Qt::CaseInsensitive);
  }

  // remove consecutive spaces
  QStringList parts = formattedName.split(' ');
  parts.removeAll("");

  formattedName.clear();
  int index = 0;

  // adjust the number prefix and insert the default separator.
  if(conf.check_number_prefix)
  {
    QRegExp re("\\d*");

    // only check number format if it exists
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

      formattedName = parts[index];
      ++index;
    }
  }

  // capitalize the first letter of every word
  if(conf.to_title_case)
  {
    int i = index;
    while(i < parts.size())
    {
      if(parts[i].isEmpty()) continue;
      bool starts_with_parenthesis = false;
      bool ends_with_parenthesis = false;

      while(parts[i].startsWith('(') && parts[i].size() > 1)
      {
        starts_with_parenthesis = true;
        parts[i].remove('(');
      }

      while(parts[i].endsWith(')') && parts[i].size() > 1)
      {
        ends_with_parenthesis = true;
        parts[i].remove(')');
      }

      if(isRomanNumerals(parts[i]))
      {
        parts[i] = parts[i].toUpper();
      }
      else
      {
        parts[i] = parts[i].toLower();
        parts[i].replace(0,1, parts[i][0].toUpper());
      }

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

  formattedName += parts[index++];

  // compose the name.
  while(index < parts.size())
  {
    formattedName += ' ' + parts[index++];
  }
  formattedName += ".mp3";

  return formattedName;
}


//-----------------------------------------------------------------
bool Utils::isRomanNumerals(const QString string_part)
{
  QString numerals("IVXLCDM");
  for(unsigned int i = 0; i < string_part.length(); ++i)
  {
    if(numerals.contains(string_part.at(i)))
      continue;

    return false;
  }

  return true;
}
