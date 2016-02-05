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
#include <QTemporaryFile>

// C++
#include <thread>

const QStringList Utils::MODULE_FILE_EXTENSIONS  = {"*.669", "*.amf", "*.apun", "*.dsm", "*.far", "*.gdm", "*.it", "*.imf", "*.mod", "*.med", "*.mtm", "*.okt", "*.s3m", "*.stm", "*.stx", "*.ult", "*.uni", "*.xt", "*.xm"};
const QStringList Utils::WAVE_FILE_EXTENSIONS    = {"*.flac", "*.ogg", "*.ape", "*.wav", "*.wma", "*.m4a", "*.voc", "*.wv", "*.mp3", "*.aiff"};
const QStringList Utils::MOVIE_FILE_EXTENSIONS   = {"*.mp4", "*.avi", "*.ogv", "*.webm" };
const QString     Utils::TEMPORAL_FILE_EXTENSION = QString(".MusicTranscoderTemporal");

const QString Utils::TranscoderConfiguration::ROOT_DIRECTORY                 = QObject::tr("Root directory");
const QString Utils::TranscoderConfiguration::NUMBER_OF_THREADS              = QObject::tr("Number of threads");
const QString Utils::TranscoderConfiguration::TRANSCODE_AUDIO                = QObject::tr("Transcode audio");
const QString Utils::TranscoderConfiguration::TRANSCODE_VIDEO                = QObject::tr("Transcode video files");
const QString Utils::TranscoderConfiguration::TRANSCODE_MODULE               = QObject::tr("Transcode Module files");
const QString Utils::TranscoderConfiguration::STRIP_MP3                      = QObject::tr("Strip MP3 metadata");
const QString Utils::TranscoderConfiguration::USE_CUE_SHEET                  = QObject::tr("Use CUE sheet");
const QString Utils::TranscoderConfiguration::RENAME_INPUT_ON_SUCCESS        = QObject::tr("Rename input files on successfull transcoding");
const QString Utils::TranscoderConfiguration::RENAMED_INPUT_EXTENSION        = QObject::tr("Renamed input files extension");
const QString Utils::TranscoderConfiguration::USE_METADATA_TO_RENAME         = QObject::tr("Use metadata to rename output");
const QString Utils::TranscoderConfiguration::DELETE_ON_CANCELLATION         = QObject::tr("Delete output on cancel");
const QString Utils::TranscoderConfiguration::EXTRACT_COVER_PICTURE          = QObject::tr("Extract cover picture");
const QString Utils::TranscoderConfiguration::COVER_PICTURE_NAME             = QObject::tr("Cover picture output filename");
const QString Utils::TranscoderConfiguration::BITRATE                        = QObject::tr("Output bitrate");
const QString Utils::TranscoderConfiguration::QUALITY                        = QObject::tr("Output quality");
const QString Utils::TranscoderConfiguration::CREATE_M3U_FILES               = QObject::tr("Create M3U playlists in input directories");
const QString Utils::TranscoderConfiguration::REFORMAT_APPLY                 = QObject::tr("Reformat output filename");
const QString Utils::TranscoderConfiguration::REFORMAT_CHARS_TO_DELETE       = QObject::tr("Characters to delete");
const QString Utils::TranscoderConfiguration::REFORMAT_CHARS_TO_REPLACE_FROM = QObject::tr("List of characters to replace from");
const QString Utils::TranscoderConfiguration::REFORMAT_CHARS_TO_REPLACE_TO   = QObject::tr("List of characters to replace to");
const QString Utils::TranscoderConfiguration::REFORMAT_SEPARATOR             = QObject::tr("Track and title separator");
const QString Utils::TranscoderConfiguration::REFORMAT_NUMBER_OF_DIGITS      = QObject::tr("Number of digits");
const QString Utils::TranscoderConfiguration::REFORMAT_USE_TITLE_CASE        = QObject::tr("Use title case");

const QString Utils::TranscoderConfiguration::SETTINGS_FILENAME = QString("MusicTranscoder.ini");

//-----------------------------------------------------------------
bool Utils::isAudioFile(const QFileInfo &file)
{
  auto extension = file.absoluteFilePath().split('.').last().toLower();

  return WAVE_FILE_EXTENSIONS.contains("*." + extension);
}

//-----------------------------------------------------------------
bool Utils::isVideoFile(const QFileInfo &file)
{
  auto extension = file.absoluteFilePath().split('.').last().toLower();

  return MOVIE_FILE_EXTENSIONS.contains("*." + extension);
}

//-----------------------------------------------------------------
bool Utils::isModuleFile(const QFileInfo &file)
{
  auto extension = file.absoluteFilePath().split('.').last().toLower();

  return MODULE_FILE_EXTENSIONS.contains("*." + extension);
}

//-----------------------------------------------------------------
bool Utils::isMP3File(const QFileInfo &file)
{
  auto extension = file.absoluteFilePath().split('.').last().toLower();

  return extension.compare(QString("mp3")) == 0;
}

//-----------------------------------------------------------------
QList<QFileInfo> Utils::findFiles(const QDir initialDir,
                                  const QStringList extensions,
                                  bool with_subdirectories,
                                  const std::function<bool (const QFileInfo &)> &condition)
{
  QList<QFileInfo> otherFilesFound, mp3FilesFound;

  auto startingDir = initialDir;
  startingDir.setFilter(QDir::Files | QDir::Dirs | QDir::NoDot | QDir::NoDotDot);
  startingDir.setNameFilters(extensions);

  auto flag = (with_subdirectories ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags);
  QDirIterator it(startingDir, flag);
  while (it.hasNext())
  {
    it.next();


    auto info = it.fileInfo();

    if(!condition(info)) continue;

    auto extension = info.absoluteFilePath().split('.').last().toLower();

    if (isMP3File(info))
    {
      mp3FilesFound << info;
    }
    else
    {
      otherFilesFound << info;
    }
  }

  return mp3FilesFound + otherFilesFound;
}

//-----------------------------------------------------------------
QString Utils::formatString(const QString filename,
                            const Utils::FormatConfiguration &conf,
                            bool add_mp3_extension)
{
  // works for filenames and plain strings
  QString formattedName = filename;

  auto fileInfo = QFileInfo(filename);
  if(fileInfo.exists())
  {
    formattedName = fileInfo.absoluteFilePath().split('/').last();

    auto extension = formattedName.split('.').last();
    auto extension_id = QString("*.") + extension;
    bool identified = WAVE_FILE_EXTENSIONS.contains(extension_id)   ||
                      MODULE_FILE_EXTENSIONS.contains(extension_id) ||
                      MOVIE_FILE_EXTENSIONS.contains(extension_id);

    if (identified)
    {
      formattedName.remove(formattedName.lastIndexOf('.'), extension.length() + 1);
    }
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
    // Format 1: 01 ...
    QRegExp re1("\\d*");
    auto re1_match = re1.exactMatch(parts[index]);

    // Format 2: 1-01 ...
    QRegExp re2("\\d-\\d*");
    auto re2_match = re2.exactMatch(parts[index]);

    // only check number format if it exists
    if (re1_match || re2_match)
    {
      QString number_string, number_disc_id;
      if(re1_match)
      {
        number_string = parts[index];
      }
      else
      {
        auto splits = parts[index].split('-');
        number_disc_id = splits.first();
        number_string = splits.last();
      }

      while (conf.number_of_digits > number_string.length())
      {
        number_string = "0" + number_string;
      }

      if (index != parts.size() - 1)
      {
        if(parts[index + 1] != QString(conf.number_and_name_separator))
        {
          number_string += QString(' ' + conf.number_and_name_separator + ' ');
        }
        else
        {
          parts[index + 1] = QString(' ' + conf.number_and_name_separator);
        }
      }

      if(!number_disc_id.isEmpty())
      {
        number_string = number_disc_id + QString("-") + number_string;
      }
      formattedName = number_string;
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
        bool starts_with_quote = false;
        bool ends_with_quote = false;
        int begin_quote_num = 0;
        int end_quote_num = 0;

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
        
        if(parts[i].startsWith('\'') && parts[i].size() > 1)
        {
          starts_with_quote = true;
          while(parts[i].at(begin_quote_num) == QChar('\'') && begin_quote_num < parts[i].size())
          {
            ++begin_quote_num;
          }
        }
        
        if(parts[i].endsWith('\'') && parts[i].size() > 1)
        {
          ends_with_quote = true;
          auto part_size = parts[i].size() - 1;
          while(parts[i].at(part_size - end_quote_num) == QChar('\'') && end_quote_num < parts[i].size())
          {
            ++end_quote_num;
          }
        }

        if(starts_with_quote || ends_with_quote)
        {
          parts[i].remove(QChar('\''));
        }

        if(!isRomanNumeral(parts[i]))
        {
          parts[i] = parts[i].toLower();
          parts[i].replace(0, 1, parts[i].at(0).toUpper());
        }

        if(starts_with_quote)
        {
          while(begin_quote_num > 0)
          {
            parts[i].insert(0, QChar('\''));
            --begin_quote_num;
          }
        }

        if(ends_with_quote)
        {
          while(end_quote_num > 0)
          {
            parts[i].append(QChar('\''));
            --end_quote_num;
          }
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

    if(index < parts.size())
    {
      formattedName += parts[index++];
    }

    // compose the name.
    while (index < parts.size())
    {
      formattedName += ' ' + parts[index++];
    }
  }

  if(add_mp3_extension)
  {
    formattedName += ".mp3";
  }

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
, m_rename_input_on_success       {true}
, m_use_metadata_to_rename_output {true}
, m_delete_output_on_cancellation {true}
, m_extract_metadata_cover_picture{true}
, m_bitrate                       {320}
, m_quality                       {0}
, m_create_M3U_files              {true}
{
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::load()
{
  QSettings settings(SETTINGS_FILENAME, QSettings::IniFormat);

  m_root_directory                                 = settings.value(ROOT_DIRECTORY, QDir::currentPath()).toString().replace('/',QDir::separator());
  m_number_of_threads                              = settings.value(NUMBER_OF_THREADS, std::thread::hardware_concurrency() /2).toInt();
  m_transcode_audio                                = settings.value(TRANSCODE_AUDIO, true).toBool();
  m_transcode_video                                = settings.value(TRANSCODE_VIDEO, true).toBool();
  m_transcode_module                               = settings.value(TRANSCODE_MODULE, true).toBool();
  m_strip_tags_from_MP3                            = settings.value(STRIP_MP3, true).toBool();
  m_use_CUE_to_split                               = settings.value(USE_CUE_SHEET, true).toBool();
  m_rename_input_on_success                        = settings.value(RENAME_INPUT_ON_SUCCESS, true).toBool();
  m_renamed_input_extension                        = settings.value(RENAMED_INPUT_EXTENSION, QObject::tr("done")).toString();
  m_use_metadata_to_rename_output                  = settings.value(USE_METADATA_TO_RENAME, true).toBool();
  m_delete_output_on_cancellation                  = settings.value(DELETE_ON_CANCELLATION, true).toBool();
  m_extract_metadata_cover_picture                 = settings.value(EXTRACT_COVER_PICTURE, true).toBool();
  m_cover_picture_name                             = settings.value(COVER_PICTURE_NAME, QObject::tr("Frontal")).toString();
  m_bitrate                                        = settings.value(BITRATE, 320).toInt();
  m_quality                                        = settings.value(QUALITY, 0).toInt();
  m_create_M3U_files                               = settings.value(CREATE_M3U_FILES, true).toBool();
  m_format_configuration.apply                     = settings.value(REFORMAT_APPLY, true).toBool();
  m_format_configuration.chars_to_delete           = settings.value(REFORMAT_CHARS_TO_DELETE, QString()).toString();
  m_format_configuration.number_of_digits          = settings.value(REFORMAT_NUMBER_OF_DIGITS, 2).toInt();
  m_format_configuration.number_and_name_separator = settings.value(REFORMAT_SEPARATOR, QString("-")).toString();
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
  settings.setValue(RENAME_INPUT_ON_SUCCESS, m_rename_input_on_success);
  settings.setValue(RENAMED_INPUT_EXTENSION, m_renamed_input_extension);
  settings.setValue(USE_METADATA_TO_RENAME, m_use_metadata_to_rename_output);
  settings.setValue(DELETE_ON_CANCELLATION, m_delete_output_on_cancellation);
  settings.setValue(EXTRACT_COVER_PICTURE, m_extract_metadata_cover_picture);
  settings.setValue(COVER_PICTURE_NAME, m_cover_picture_name);
  settings.setValue(BITRATE, m_bitrate);
  settings.setValue(QUALITY, m_quality);
  settings.setValue(CREATE_M3U_FILES, m_create_M3U_files);
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
bool Utils::TranscoderConfiguration::renameInputOnSuccess() const
{
  return m_rename_input_on_success;
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::setRenameInputOnSuccess(bool enabled)
{
  m_rename_input_on_success = enabled;
}

//-----------------------------------------------------------------
const QString Utils::TranscoderConfiguration::renamedInputFilesExtension() const
{
  return m_renamed_input_extension;
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::setRenamedInputFilesExtension(const QString& extension)
{
  m_renamed_input_extension = extension;
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::setUseMetadataToRenameOutput(bool enabled)
{
  m_use_metadata_to_rename_output = enabled;
}

//-----------------------------------------------------------------
bool Utils::TranscoderConfiguration::createM3Ufiles() const
{
  return m_create_M3U_files;
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::setCreateM3Ufiles(bool enabled)
{
  m_create_M3U_files = enabled;
}

//-----------------------------------------------------------------
bool Utils::isSpaces(const QString& string)
{
  for(int i = 0; i < string.size(); ++i)
  {
    if(!string.at(i).isSpace())
    {
      return false;
    }
  }

  return true;
}
