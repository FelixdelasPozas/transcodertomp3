/*
 File: AboutDialog.h
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

#ifndef ABOUTDIALOG_H_
#define ABOUTDIALOG_H_

#include <QDialog>
#include "ui_AboutDialog.h"

/** \class AboutDialog
 * \brief Implements the narcissitic dialog.
 *
 */
class AboutDialog
: public QDialog
, public Ui_AboutDialog
{
    Q_OBJECT
  public:
    /** \brief AboutDialog class constructor.
     * \param[in] parent QWidget parent of this one.
     * \param[in] flags dialog flags.
     *
     */
    AboutDialog(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());

    /** \briof AboutDialog class virtual destructor.
     *
     */
    virtual ~AboutDialog()
    {}

  private:
    static const QString VERSION; /** application version string. */
};

#endif // ABOUTDIALOG_H_
