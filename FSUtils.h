/*
		File: FSUtils.h
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


#ifndef FILESYSTEM_UTILS_H_
#define FILESYSTEM_UTILS_H_

#include <QDir>
#include <QString>
#include <QPair>

namespace FileSystemUtils
{
  /** \brief Returns the files in the specified directory tree that has the specfied extensions.
   * \param[in] rootDir starting directory.
   * \param[in] extensions extesions of the files to return.
   *
   */
  QList<QFileInfo> findFiles(const QDir rootDirectory, const QStringList extensions);

  struct CleanConfiguration
  {
    QString deleteCharacters;                     // characters to delete
    QList<QPair<QChar,QChar>> replaceCharacters;  // characters to replace (pair <to replace, with>
    bool checkNumberPrefix;                       // check if the name starts with a number
    int numberDigits;                             // number of digits the number should have.
    QChar numberAndNameSeparator;                 // separator character between the number and the rest of the name.
    bool toTitleCase;                             // capitalize the first letter of every word in the name.
  };

  /** \brief Returns a transformed filename according to the specified parameters.
   * \param[in] filename file name with absolute path.
   * \param[in] conf configuration.
   *
   */
  QString cleanName(const QString filename, const CleanConfiguration conf);
}



#endif // FSUTILS_H_
