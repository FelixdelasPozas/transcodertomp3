/*
 File: ConfigurationDialog.h
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

#ifndef CONFIGURATIONDIALOG_H_
#define CONFIGURATIONDIALOG_H_

// Qt
#include <QDialog>
#include "ui_ConfigurationDialog.h"

class ConfigurationDialog
: public QDialog
, public Ui_ConfigurationDialog
{
    Q_OBJECT
  public:
    /** \brief ConfigurationDialog class constructor.
     *
     */
    ConfigurationDialog();

    /** \brief ConfigurationDialog class virtual destructor.
     *
     */
    virtual ~ConfigurationDialog();

  private slots:
    void onTranscodeCheckStateChanged(int state);
    void onReformatCheckStateChanged(int state);
    void onAddButtonPressed();
    void onRemoveButtonPressed();
    void onCoverExtractCheckStateChanged(int state);

  private:
    /** \brief Helper method to connects all the signals.
     *
     */
    void connectSignals();

    // can't do this with QMap in Qt 4.8.6
    static const QStringList QUALITY_NAMES;
    static const QList<int>  QUALITY_VALUES;
    static const QStringList BITRATE_NAMES;
    static const QList<int>  BITRATE_VALUES;


};

#endif // CONFIGURATIONDIALOG_H_
