/*
  File: Utils.h
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

#ifndef UTILS_H_
#define UTILS_H_

// Qt
#include <QDir>
#include <QString>
#include <QPair>

namespace Utils
{
  /** \brief Returns the files in the specified directory tree that has the specified extensions.
   * \param[in] rootDir starting directory.
   * \param[in] extensions extensions of the files to return.
   *
   */
  QList<QFileInfo> findFiles(const QDir rootDirectory, const QStringList extensions);

  struct FormatConfiguration
  {
    bool                          apply;                     // true to apply the formatting process.
    QString                       chars_to_delete;           // characters to delete.
    QList<QPair<QString,QString>> chars_to_replace;          // strings to replace (pair <to replace, with>).
    bool                          check_number_prefix;       // check if the name starts with a number and format it.
    int                           number_of_digits;          // number of digits the number should have.
    QChar                         number_and_name_separator; // separator character between the number and the rest of the name.
    bool                          to_title_case;             // capitalize the first letter of every word in the name.

    FormatConfiguration()
    {
      apply                     = true;
      check_number_prefix       = true;
      chars_to_replace          << QPair<QString, QString>("_", " ")
                                << QPair<QString, QString>(".", " ")
                                << QPair<QString, QString>("[", "(")
                                << QPair<QString, QString>("]", ")");
      number_and_name_separator = '-';
      number_of_digits          = 2;
      to_title_case             = true;
    }
  };

  /** \brief Returns a transformed filename according to the specified parameters.
   * \param[in] filename file name with absolute path.
   * \param[in] conf configuration parameters.
   *
   */
  QString formatString(const QString filename, const FormatConfiguration conf);

  /** \brief Returns true if the string represents a roman numeral.
   *
   */
  bool isRomanNumeral(const QString string_part);

  class Configuration
  {
    private:
      static const QString ROOT_DIRECTORY;
      static const QString NUMBER_OF_THREADS;

      FormatConfiguration m_format_conf;
  };
}



#endif // UTILS_H_
