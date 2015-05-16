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
#include <QSettings>
#include <QDirIterator>

// C++
#include <thread>

const QStringList Utils::MODULE_FILE_EXTENSIONS = {"*.669", "*.amf", "*.apun", "*.dsm", "*.far", "*.gdm", "*.it", "*.imf", "*.mod", "*.med", "*.mtm", "*.okt", "*.s3m", "*.stm", "*.stx", "*.ult", "*.uni", "*.xt", "*.xm"};
const QStringList Utils::WAVE_FILE_EXTENSIONS   = {"*.flac", "*.ogg", "*.ape", "*.wav", "*.wma", "*.m4a", "*.voc", "*.wv", "*.mp3"};
const QStringList Utils::MOVIE_FILE_EXTENSIONS  = {"*.mp4", "*.avi", "*.ogv", "*.webm" };

const QString Utils::TranscoderConfiguration::ROOT_DIRECTORY                 = QObject::tr("Root directory");
const QString Utils::TranscoderConfiguration::NUMBER_OF_THREADS              = QObject::tr("Number of threads");
const QString Utils::TranscoderConfiguration::TRANSCODE_AUDIO                = QObject::tr("Transcode audio");
const QString Utils::TranscoderConfiguration::TRANSCODE_VIDEO                = QObject::tr("Transcode video files");
const QString Utils::TranscoderConfiguration::TRANSCODE_MODULE               = QObject::tr("Transcode Module files");
const QString Utils::TranscoderConfiguration::STRIP_MP3                      = QObject::tr("Strip MP3 metadata");
const QString Utils::TranscoderConfiguration::USE_CUE_SHEET                  = QObject::tr("Use CUE sheet");
const QString Utils::TranscoderConfiguration::USE_METADATA_TO_RENAME         = QObject::tr("Use metadata to rename output");
const QString Utils::TranscoderConfiguration::DELETE_ON_CANCELLATION         = QObject::tr("Delete output on cancel");
const QString Utils::TranscoderConfiguration::EXTRACT_COVER_PICTURE          = QObject::tr("Extract cover picture");
const QString Utils::TranscoderConfiguration::COVER_PICTURE_NAME             = QObject::tr("Cover picture output filename");
const QString Utils::TranscoderConfiguration::BITRATE                        = QObject::tr("Output bitrate");
const QString Utils::TranscoderConfiguration::QUALITY                        = QObject::tr("Output quality");
const QString Utils::TranscoderConfiguration::REFORMAT_APPLY                 = QObject::tr("Reformat output filename");
const QString Utils::TranscoderConfiguration::REFORMAT_CHARS_TO_DELETE       = QObject::tr("Characters to delete");
const QString Utils::TranscoderConfiguration::REFORMAT_CHARS_TO_REPLACE_FROM = QObject::tr("List of characters to replace from");
const QString Utils::TranscoderConfiguration::REFORMAT_CHARS_TO_REPLACE_TO   = QObject::tr("List of characters to replace to");
const QString Utils::TranscoderConfiguration::REFORMAT_SEPARATOR             = QObject::tr("Track and title separator");
const QString Utils::TranscoderConfiguration::REFORMAT_NUMBER_OF_DIGITS      = QObject::tr("Number of digits");
const QString Utils::TranscoderConfiguration::REFORMAT_USE_TITLE_CASE        = QObject::tr("Use title case");

const QString Utils::TranscoderConfiguration::SETTINGS_FILENAME = QString("MusicTranscoder.ini");

//-----------------------------------------------------------------
bool Utils::isAudioFile(const QFileInfo& file)
{
  auto extension = file.absoluteFilePath().split('.').last().toLower();

  return WAVE_FILE_EXTENSIONS.contains("*." + extension);
}

//-----------------------------------------------------------------
bool Utils::isVideoFile(const QFileInfo& file)
{
  auto extension = file.absoluteFilePath().split('.').last().toLower();

  return MOVIE_FILE_EXTENSIONS.contains("*." + extension);
}

//-----------------------------------------------------------------
bool Utils::isModuleFile(const QFileInfo& file)
{
  auto extension = file.absoluteFilePath().split('.').last().toLower();

  return MODULE_FILE_EXTENSIONS.contains("*." + extension);
}

//-----------------------------------------------------------------
QList<QFileInfo> Utils::findFiles(const QDir initialDir, const QStringList extensions)
{
  QList<QFileInfo> otherFilesFound, mp3FilesFound;

  auto startingDir = initialDir;
  startingDir.setFilter(QDir::Files | QDir::Dirs | QDir::NoDot | QDir::NoDotDot);
  startingDir.setNameFilters(extensions);

  QDirIterator it(startingDir, QDirIterator::Subdirectories);
  while (it.hasNext())
  {
    it.next();

    auto info = it.fileInfo();
    auto extension = info.absoluteFilePath().split('.').last().toLower();

    if (extension == "mp3")
    {
      mp3FilesFound << info;
    }
    else
    {
      otherFilesFound << info;
    }
  }

  // I want mp3 files to be processed first
  return mp3FilesFound + otherFilesFound;
}

//-----------------------------------------------------------------
QString Utils::formatString(const QString filename, const Utils::FormatConfiguration conf)
{
  // works for filenames and plain strings
  auto fileInfo = QFileInfo(QFile(filename));
  auto name = fileInfo.absoluteFilePath().split('/').last();
  auto extension = name.split('.').last();
  QString formattedName = name;
  if (name.contains('.'))
  {
    formattedName = name.remove(name.lastIndexOf('.'), extension.length() + 1);
  }

  if (conf.apply)
  {
    // delete specified chars
    for (int i = 0; i < conf.chars_to_delete.length(); ++i)
    {
      formattedName.remove(conf.chars_to_delete[i], Qt::CaseInsensitive);
    }

    // replace specified strings
    for (int i = 0; i < conf.chars_to_replace.size(); ++i)
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
    QRegExp re("\\d*");

    // only check number format if it exists
    if (re.exactMatch(parts[index]))
    {
      while (conf.number_of_digits > parts[index].length())
      {
        parts[index] = "0" + parts[index];
      }

      if ((parts.size() > index) && (parts[index + 1] != QString(conf.number_and_name_separator)))
      {
        parts[index] += QString(' ' + conf.number_and_name_separator + ' ');
      }
      else
      {
        parts[index + 1] = QString(' ' + conf.number_and_name_separator);
      }

      formattedName = parts[index];
      ++index;
    }

    // capitalize the first letter of every word
    if (conf.to_title_case)
    {
      int i = index;
      while (i < parts.size())
      {
        if (parts[i].isEmpty()) continue;
        bool starts_with_parenthesis = false;
        bool ends_with_parenthesis = false;

        while (parts[i].startsWith('(') && parts[i].size() > 1)
        {
          starts_with_parenthesis = true;
          parts[i].remove('(');
        }

        while (parts[i].endsWith(')') && parts[i].size() > 1)
        {
          ends_with_parenthesis = true;
          parts[i].remove(')');
        }

        if (!isRomanNumeral(parts[i]))
        {
          parts[i] = parts[i].toLower();
          parts[i].replace(0, 1, parts[i][0].toUpper());
        }

        if (starts_with_parenthesis)
        {
          parts[i] = QString('(') + parts[i];
        }

        if (ends_with_parenthesis)
        {
          parts[i] = parts[i] + QString(')');
        }

        ++i;
      }
    }

    formattedName += parts[index++];

    // compose the name.
    while (index < parts.size())
    {
      formattedName += ' ' + parts[index++];
    }
  }

  formattedName += ".mp3";

  return formattedName;
}

//-----------------------------------------------------------------
bool Utils::isRomanNumeral(const QString string_part)
{
  QString numerals("IVXLCDM");
  for (int i = 0; i < string_part.length(); ++i)
  {
    if (numerals.contains(string_part.at(i).toUpper())) continue;

    return false;
  }

  return true;
}

//-----------------------------------------------------------------
Utils::TranscoderConfiguration::TranscoderConfiguration()
: m_number_of_threads             {0}
, m_transcode_audio               {true}
, m_transcode_video               {true}
, m_transcode_module              {true}
, m_strip_tags_from_MP3           {true}
, m_use_CUE_to_split              {true}
, m_use_metadata_to_rename_output {true}
, m_delete_output_on_cancellation {true}
, m_extract_metadata_cover_picture{true}
, m_bitrate                       {320}
, m_quality                       {0}
{
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::load()
{
  QSettings settings(SETTINGS_FILENAME, QSettings::IniFormat);

  m_root_directory                                 = settings.value(ROOT_DIRECTORY, QDir::currentPath()).toString().replace('/','\\');
  m_number_of_threads                              = settings.value(NUMBER_OF_THREADS, std::thread::hardware_concurrency() /2).toInt();
  m_transcode_audio                                = settings.value(TRANSCODE_AUDIO, true).toBool();
  m_transcode_video                                = settings.value(TRANSCODE_VIDEO, true).toBool();
  m_transcode_module                               = settings.value(TRANSCODE_MODULE, true).toBool();
  m_strip_tags_from_MP3                            = settings.value(STRIP_MP3, true).toBool();
  m_use_CUE_to_split                               = settings.value(USE_CUE_SHEET, true).toBool();
  m_use_metadata_to_rename_output                  = settings.value(USE_METADATA_TO_RENAME, true).toBool();
  m_delete_output_on_cancellation                  = settings.value(DELETE_ON_CANCELLATION, true).toBool();
  m_extract_metadata_cover_picture                 = settings.value(EXTRACT_COVER_PICTURE, true).toBool();
  m_cover_picture_name                             = settings.value(COVER_PICTURE_NAME, QObject::tr("Frontal")).toString();
  m_bitrate                                        = settings.value(BITRATE, 320).toInt();
  m_quality                                        = settings.value(QUALITY, 0).toInt();
  m_format_configuration.apply                     = settings.value(REFORMAT_APPLY, true).toBool();
  m_format_configuration.chars_to_delete           = settings.value(REFORMAT_CHARS_TO_DELETE, QString()).toString();
  m_format_configuration.number_of_digits          = settings.value(REFORMAT_NUMBER_OF_DIGITS, 2).toInt();
  m_format_configuration.number_and_name_separator = settings.value(REFORMAT_SEPARATOR, QChar('-')).toChar();
  m_format_configuration.to_title_case             = settings.value(REFORMAT_USE_TITLE_CASE, true).toBool();

  auto from = settings.value(REFORMAT_CHARS_TO_REPLACE_FROM, QStringList()).toStringList();
  auto to   = settings.value(REFORMAT_CHARS_TO_REPLACE_TO, QStringList()).toStringList();

  if(!from.isEmpty())
  {
    m_format_configuration.chars_to_replace.clear();
    for(int i = 0; i < from.size(); ++i)
    {
      auto pair = QPair<QString, QString>(from[i], to[i]);
      m_format_configuration.chars_to_replace << pair;
    }
  }
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::save() const
{
  QSettings settings(SETTINGS_FILENAME, QSettings::IniFormat);

  settings.setValue(ROOT_DIRECTORY, m_root_directory);
  settings.setValue(NUMBER_OF_THREADS, m_number_of_threads);
  settings.setValue(TRANSCODE_AUDIO, m_transcode_audio);
  settings.setValue(TRANSCODE_VIDEO, m_transcode_video);
  settings.setValue(TRANSCODE_MODULE, m_transcode_module);
  settings.setValue(STRIP_MP3, m_strip_tags_from_MP3);
  settings.setValue(USE_CUE_SHEET, m_use_CUE_to_split);
  settings.setValue(USE_METADATA_TO_RENAME, m_use_metadata_to_rename_output);
  settings.setValue(DELETE_ON_CANCELLATION, m_delete_output_on_cancellation);
  settings.setValue(EXTRACT_COVER_PICTURE, m_extract_metadata_cover_picture);
  settings.setValue(COVER_PICTURE_NAME, m_cover_picture_name);
  settings.setValue(BITRATE, m_bitrate);
  settings.setValue(QUALITY, m_quality);
  settings.setValue(REFORMAT_APPLY, m_format_configuration.apply);
  settings.setValue(REFORMAT_CHARS_TO_DELETE, m_format_configuration.chars_to_delete);
  settings.setValue(REFORMAT_NUMBER_OF_DIGITS, m_format_configuration.number_of_digits);
  settings.setValue(REFORMAT_SEPARATOR, m_format_configuration.number_and_name_separator);
  settings.setValue(REFORMAT_USE_TITLE_CASE, m_format_configuration.to_title_case);

  QStringList from, to;
  for(auto pair: m_format_configuration.chars_to_replace)
  {
    from << pair.first;
    to   << pair.second;
  }

  settings.setValue(REFORMAT_CHARS_TO_REPLACE_FROM, from);
  settings.setValue(REFORMAT_CHARS_TO_REPLACE_TO, to);

  settings.sync();
}

//-----------------------------------------------------------------
const QString& Utils::TranscoderConfiguration::rootDirectory() const
{
  return m_root_directory;
}

//-----------------------------------------------------------------
int Utils::TranscoderConfiguration::numberOfThreads() const
{
  return m_number_of_threads;
}

//-----------------------------------------------------------------
int Utils::TranscoderConfiguration::bitrate() const
{
  return m_bitrate;
}

//-----------------------------------------------------------------
const QString &Utils::TranscoderConfiguration::coverPictureName() const
{
  return m_cover_picture_name;
}

//-----------------------------------------------------------------
bool Utils::TranscoderConfiguration::deleteOutputOnCancellation() const
{
  return m_delete_output_on_cancellation;
}

//-----------------------------------------------------------------
bool Utils::TranscoderConfiguration::extractMetadataCoverPicture() const
{
  return m_extract_metadata_cover_picture;
}

//-----------------------------------------------------------------
const Utils::FormatConfiguration &Utils::TranscoderConfiguration::formatConfiguration() const
{
  return m_format_configuration;
}

//-----------------------------------------------------------------
int Utils::TranscoderConfiguration::quality() const
{
  return m_quality;
}

//-----------------------------------------------------------------
bool Utils::TranscoderConfiguration::reformatOutputFilename() const
{
  return m_format_configuration.apply;
}

//-----------------------------------------------------------------
bool Utils::TranscoderConfiguration::stripTagsFromMp3() const
{
  return m_strip_tags_from_MP3;
}

//-----------------------------------------------------------------
bool Utils::TranscoderConfiguration::transcodeAudio() const
{
  return m_transcode_audio;
}

//-----------------------------------------------------------------
bool Utils::TranscoderConfiguration::transcodeModule() const
{
  return m_transcode_module;
}

//-----------------------------------------------------------------
bool Utils::TranscoderConfiguration::transcodeVideo() const
{
  return m_transcode_video;
}

//-----------------------------------------------------------------
bool Utils::TranscoderConfiguration::useCueToSplit() const
{
  return m_use_CUE_to_split;
}

//-----------------------------------------------------------------
bool Utils::TranscoderConfiguration::useMetadataToRenameOutput() const
{
  return m_use_metadata_to_rename_output;
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::setRootDirectory(const QString& path)
{
  m_root_directory = path;
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::setNumberOfThreads(int value)
{
  m_number_of_threads = value;
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::setBitrate(int bitrate)
{
  m_bitrate = bitrate;
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::setCoverPictureName(const QString& filename)
{
  m_cover_picture_name = filename;
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::setDeleteOutputOnCancellation(bool enabled)
{
  m_delete_output_on_cancellation = enabled;
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::setExtractMetadataCoverPicture(bool enabled)
{
  m_extract_metadata_cover_picture = enabled;
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::setFormatConfiguration(const FormatConfiguration& configuration)
{
  m_format_configuration = configuration;
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::setQuality(int value)
{
  m_quality = value;
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::setReformatOutputFilename(bool enabled)
{
  m_format_configuration.apply = enabled;
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::setStripTagsFromMp3(bool enabled)
{
  m_strip_tags_from_MP3 = enabled;
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::setTranscodeAudio(bool enabled)
{
  m_transcode_audio = enabled;
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::setTranscodeModule(bool enabled)
{
  m_transcode_module = enabled;
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::setTranscodeVideo(bool enabled)
{
  m_transcode_video = enabled;
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::setUseCueToSplit(bool enabled)
{
  m_use_CUE_to_split = enabled;
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::setUseMetadataToRenameOutput(bool enabled)
{
  m_use_metadata_to_rename_output = enabled;
}
