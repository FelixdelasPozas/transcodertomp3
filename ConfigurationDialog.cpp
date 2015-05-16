/*
 File: ConfigurationDialog.cpp
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
#include <ConfigurationDialog.h>

// Qt
#include <QMessageBox>
#include <QDebug>

const QStringList ConfigurationDialog::QUALITY_NAMES = { tr("Very high"), tr("High"), tr("Normal"), tr("Low"), tr("Very low") };
const QList<int>  ConfigurationDialog::QUALITY_VALUES = { 0,2,5,7,9 };
const QStringList ConfigurationDialog::BITRATE_NAMES = { tr("320"), tr("256"), tr("224"), tr("192"), tr("160"), tr("128"), tr("112"), tr("96"), tr("80"), tr("64") };
const QList<int>  ConfigurationDialog::BITRATE_VALUES = { 320, 256, 224, 192, 160, 128, 112, 96, 80, 64 };


//-----------------------------------------------------------------
ConfigurationDialog::ConfigurationDialog(const Utils::TranscoderConfiguration &configuration)
{
  setupUi(this);

  setWindowFlags(windowFlags() & ~(Qt::WindowContextHelpButtonHint) & ~(Qt::WindowMaximizeButtonHint) & ~(Qt::WindowMinimizeButtonHint));

  m_quality->addItems(QUALITY_NAMES);
  m_quality->setCurrentIndex(0);

  m_bitrate->addItems(BITRATE_NAMES);
  m_bitrate->setCurrentIndex(0);

  QStringList labels = { tr("from"), tr("to") };
  m_replaceChars->setHorizontalHeaderLabels(labels);
  m_replaceChars->setSelectionBehavior(QTableWidget::SelectionBehavior::SelectRows);
  m_replaceChars->setColumnWidth(0, 110);
  m_replaceChars->setColumnWidth(1, 110);
  m_replaceChars->horizontalHeader()->setResizeMode(QHeaderView::Fixed);

  applyConfiguration(configuration);
  connectSignals();
}

//-----------------------------------------------------------------
ConfigurationDialog::~ConfigurationDialog()
{
}

//-----------------------------------------------------------------
void ConfigurationDialog::onTranscodeCheckStateChanged(int state)
{
  int atLeastOneChecked = false;
  for (auto checkBox: { m_transcodeAudio, m_transcodeVideo, m_transcodeModule})
  {
    atLeastOneChecked |= checkBox->isChecked();
  }

  if(!atLeastOneChecked)
  {
    QMessageBox msgBox;
    msgBox.setWindowIcon(QIcon(":/MusicConverter/settings.ico"));
    msgBox.setText("Invalid options.");
    msgBox.setInformativeText("At least one of the transcoding options must be checked for the program to do something.");
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.exec();

    auto checkBox = qobject_cast<QCheckBox *>(sender());
    checkBox->setCheckState(Qt::Checked);
  }
}

//-----------------------------------------------------------------
void ConfigurationDialog::onReformatCheckStateChanged(int state)
{
  bool enabled = (state == Qt::Checked);

  m_reformatGroup->setEnabled(enabled);
}

//-----------------------------------------------------------------
void ConfigurationDialog::onAddButtonPressed()
{
  m_replaceChars->insertRow(m_replaceChars->rowCount());
  m_replaceChars->selectRow(m_replaceChars->rowCount()-1);

  m_removeButton->setEnabled(true);
}

//-----------------------------------------------------------------
void ConfigurationDialog::onRemoveButtonPressed()
{
  auto currentRow = m_replaceChars->currentRow();
  m_replaceChars->removeRow(currentRow);

  if(m_replaceChars->rowCount() > 0)
  {
    m_replaceChars->selectRow(currentRow-1);
  }
  else
  {
    m_removeButton->setEnabled(false);
  }
}

//-----------------------------------------------------------------
void ConfigurationDialog::onCoverExtractCheckStateChanged(int state)
{
  auto enabled = (state == Qt::Checked);

  m_coverName->setEnabled(enabled);
  m_coverNameLabel->setEnabled(enabled);
}

//-----------------------------------------------------------------
void ConfigurationDialog::applyConfiguration(const Utils::TranscoderConfiguration& configuration)
{
  m_transcodeAudio->setChecked(configuration.transcodeAudio());
  m_transcodeVideo->setChecked(configuration.transcodeVideo());
  m_transcodeModule->setChecked(configuration.transcodeModule());
  m_stripMP3->setChecked(configuration.stripTagsFromMp3());
  m_cueSplit->setChecked(configuration.useCueToSplit());

  m_renameOutput->setChecked(configuration.useMetadataToRenameOutput());
  m_reformat->setChecked(configuration.reformatOutputFilename());
  m_deleteOnCancel->setChecked(configuration.deleteOutputOnCancellation());
  m_extractInputCover->setChecked(configuration.extractMetadataCoverPicture());
  m_coverName->setText(configuration.coverPictureName());
  m_bitrate->setCurrentIndex(BITRATE_VALUES.indexOf(configuration.bitrate()));
  m_quality->setCurrentIndex(QUALITY_VALUES.indexOf(configuration.quality()));

  m_deleteChars->setText(configuration.formatConfiguration().chars_to_delete);

  m_replaceChars->setRowCount(configuration.formatConfiguration().chars_to_replace.size());
  for(auto i = 0; i < configuration.formatConfiguration().chars_to_replace.size(); ++i)
  {
    auto pair = configuration.formatConfiguration().chars_to_replace[i];

    auto newItem = new QTableWidgetItem(pair.first);
    newItem->setTextAlignment(Qt::AlignHCenter);
    m_replaceChars->setItem(i, 0, newItem);

    newItem = new QTableWidgetItem(pair.second);
    newItem->setTextAlignment(Qt::AlignHCenter);
    m_replaceChars->setItem(i, 1, newItem);
  }
  m_replaceChars->selectRow(0);

  m_prefixNumDigits->setValue(configuration.formatConfiguration().number_of_digits);
  m_separator->setText(configuration.formatConfiguration().number_and_name_separator);
  m_titleCase->setChecked(configuration.formatConfiguration().to_title_case);
}

//-----------------------------------------------------------------
void ConfigurationDialog::connectSignals()
{
  connect(m_transcodeAudio,    SIGNAL(stateChanged(int)),
          this,                SLOT(onTranscodeCheckStateChanged(int)));

  connect(m_transcodeVideo,    SIGNAL(stateChanged(int)),
          this,                SLOT(onTranscodeCheckStateChanged(int)));

  connect(m_transcodeModule,   SIGNAL(stateChanged(int)),
          this,                SLOT(onTranscodeCheckStateChanged(int)));

  connect(m_reformat,          SIGNAL(stateChanged(int)),
          this,                SLOT(onReformatCheckStateChanged(int)));

  connect(m_addButton,         SIGNAL(pressed()),
          this,                SLOT(onAddButtonPressed()));

  connect(m_removeButton,      SIGNAL(pressed()),
          this,                SLOT(onRemoveButtonPressed()));

  connect(m_extractInputCover, SIGNAL(stateChanged(int)),
          this,                SLOT(onCoverExtractCheckStateChanged(int)));
}

//-----------------------------------------------------------------
const Utils::TranscoderConfiguration ConfigurationDialog::getConfiguration() const
{
  Utils::TranscoderConfiguration configuration;

  configuration.setBitrate(BITRATE_VALUES[m_bitrate->currentIndex()]);
  configuration.setCoverPictureName(m_coverName->text());
  configuration.setDeleteOutputOnCancellation(m_deleteOnCancel->isChecked());
  configuration.setExtractMetadataCoverPicture(m_extractInputCover->isChecked());
  configuration.setQuality(QUALITY_VALUES[m_quality->currentIndex()]);
  configuration.setReformatOutputFilename(m_reformat->isChecked());
  configuration.setStripTagsFromMp3(m_stripMP3->isChecked());
  configuration.setTranscodeAudio(m_transcodeAudio->isChecked());
  configuration.setTranscodeModule(m_transcodeModule->isChecked());
  configuration.setTranscodeVideo(m_transcodeVideo->isChecked());
  configuration.setUseCueToSplit(m_cueSplit->isChecked());
  configuration.setUseMetadataToRenameOutput(m_renameOutput->isChecked());

  return configuration;
}
