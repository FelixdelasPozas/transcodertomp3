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
#include <fileapi.h>
#include <locale>
#include <codecvt>

const QStringList Utils::MODULE_FILE_EXTENSIONS  = {"*.669", "*.amf", "*.apun", "*.dsm", "*.far", "*.gdm", "*.it", "*.imf", "*.mod", "*.med", "*.mtm", "*.okt", "*.s3m", "*.stm", "*.stx", "*.ult", "*.uni", "*.xt", "*.xm"};
const QStringList Utils::WAVE_FILE_EXTENSIONS    = {"*.flac", "*.ogg", "*.ape", "*.wav", "*.wma", "*.m4a", "*.voc", "*.wv", "*.mp3", "*.aiff"};
const QStringList Utils::MOVIE_FILE_EXTENSIONS   = {"*.mp4", "*.avi", "*.ogv", "*.webm", "*.mkv" };
const QString     Utils::TEMPORAL_FILE_EXTENSION = QString(".MusicTranscoderTemporal");

const QStringList fromStrings{" No.", "[", "]", "}", "{", ".", "_", ":", "~", "|", "/", ";", "\\", "pt ", "#", "arr ", "cond ", "comp ", "feat ", "alt ", "tk ", "seq", "*"};
const QStringList toStrings{" Nº", "(", ")", ")", "(", " ", " ", " -", "-", "-", " - ", "-", "''", "part ", "Nº", "arranged ", "conductor ", "composer ", "featuring ", "alternate ", "take ", "sequence ", "_"  };


const QString Utils::TranscoderConfiguration::ROOT_DIRECTORY                     = QObject::tr("Root directory");
const QString Utils::TranscoderConfiguration::NUMBER_OF_THREADS                  = QObject::tr("Number of threads");
const QString Utils::TranscoderConfiguration::TRANSCODE_AUDIO                    = QObject::tr("Transcode audio");
const QString Utils::TranscoderConfiguration::TRANSCODE_VIDEO                    = QObject::tr("Transcode video files");
const QString Utils::TranscoderConfiguration::TRANSCODE_MODULE                   = QObject::tr("Transcode Module files");
const QString Utils::TranscoderConfiguration::STRIP_MP3                          = QObject::tr("Strip MP3 metadata");
const QString Utils::TranscoderConfiguration::USE_CUE_SHEET                      = QObject::tr("Use CUE sheet");
const QString Utils::TranscoderConfiguration::RENAME_INPUT_ON_SUCCESS            = QObject::tr("Rename input files on successfull transcoding");
const QString Utils::TranscoderConfiguration::RENAMED_INPUT_EXTENSION            = QObject::tr("Renamed input files extension");
const QString Utils::TranscoderConfiguration::USE_METADATA_TO_RENAME             = QObject::tr("Use metadata to rename output");
const QString Utils::TranscoderConfiguration::DELETE_ON_CANCELLATION             = QObject::tr("Delete output on cancel");
const QString Utils::TranscoderConfiguration::EXTRACT_COVER_PICTURE              = QObject::tr("Extract cover picture");
const QString Utils::TranscoderConfiguration::COVER_PICTURE_NAME                 = QObject::tr("Cover picture output filename");
const QString Utils::TranscoderConfiguration::BITRATE                            = QObject::tr("Output bitrate");
const QString Utils::TranscoderConfiguration::QUALITY                            = QObject::tr("Output quality");
const QString Utils::TranscoderConfiguration::CREATE_M3U_FILES                   = QObject::tr("Create M3U playlists in input directories");
const QString Utils::TranscoderConfiguration::REFORMAT_APPLY                     = QObject::tr("Reformat output filename");
const QString Utils::TranscoderConfiguration::REFORMAT_CHARS_TO_DELETE           = QObject::tr("Characters to delete");
const QString Utils::TranscoderConfiguration::REFORMAT_CHARS_TO_REPLACE_FROM     = QObject::tr("List of characters to replace from");
const QString Utils::TranscoderConfiguration::REFORMAT_CHARS_TO_REPLACE_TO       = QObject::tr("List of characters to replace to");
const QString Utils::TranscoderConfiguration::REFORMAT_SEPARATOR                 = QObject::tr("Track and title separator");
const QString Utils::TranscoderConfiguration::REFORMAT_NUMBER_OF_DIGITS          = QObject::tr("Number of digits");
const QString Utils::TranscoderConfiguration::REFORMAT_USE_TITLE_CASE            = QObject::tr("Use title case");
const QString Utils::TranscoderConfiguration::REFORMAT_PREFIX_DISK_NUMBER        = QObject::tr("Prefix track number with disk number");
const QString Utils::TranscoderConfiguration::REFORMAT_APPLY_CHAR_SIMPLIFICATION = QObject::tr("Character simplification");

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

  if(conf.character_simplification)
  {
    QString simplified;

    auto simplifyCharacter = [&simplified](QChar &c)
    {
      if(c.isLetter())
      {
        const auto decomposition = c.decomposition();
        if(decomposition.length() > 1)
        {
          simplified.append(decomposition.at(0));
          return;
        }
      }
      simplified.append(c);
    };
    std::for_each(formattedName.begin(), formattedName.end(), simplifyCharacter);

    if(!simplified.isEmpty()) formattedName = simplified;
  }

  // check for unwanted unicode chars
  while(formattedName.toLatin1().contains('?'))
  {
    auto index = formattedName.toLatin1().indexOf('?');

    switch(formattedName.at(index).category())
    {
      case QChar::Punctuation_Open:
      case QChar::Punctuation_Close:
      case QChar::Punctuation_InitialQuote:
      case QChar::Punctuation_FinalQuote:
        formattedName = formattedName.replace(formattedName.at(index), QString("''"));
        break;
      case QChar::Punctuation_Dash:
        formattedName = formattedName.replace(formattedName.at(index), '-');
        break;
      default:
        formattedName = formattedName.replace(formattedName.at(index), QChar::Space);
        break;
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

      if(!number_disc_id.isEmpty() && conf.prefix_disk_num)
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
        bool ends_with_comma = false;
        int begin_quote_num = 0;
        int end_quote_num = 0;


        if(parts[i].endsWith(',') && parts[i].size() > 1)
        {
          ends_with_comma = true;
          parts[i] = parts[i].mid(0, parts[i].length()-1);
        }

        if(parts[i].startsWith('(') && parts[i].size() > 1)
        {
          starts_with_parenthesis = true;
          parts[i] = parts[i].mid(1, parts[i].length()-1);
        }

        if(parts[i].endsWith(')') && parts[i].size() > 1)
        {
          ends_with_parenthesis = true;
          parts[i] = parts[i].mid(0, parts[i].length()-1);
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

        if(ends_with_comma)
        {
          parts[i] = parts[i] + QString(',');
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

  // remove any unwanted spaces
  formattedName = formattedName.simplified();

  // remove wrong apostrophes
  const auto count = formattedName.count("''");
  if(count == 1)
  {
    formattedName = formattedName.replace("''", "'");
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
  QSettings settings("Felix de las Pozas Alvarez", "MusicTranscoder");

  m_root_directory                                 = settings.value(ROOT_DIRECTORY, QDir::currentPath()).toString();
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
  m_format_configuration.prefix_disk_num           = settings.value(REFORMAT_PREFIX_DISK_NUMBER, false).toBool();
  m_format_configuration.character_simplification  = settings.value(REFORMAT_APPLY_CHAR_SIMPLIFICATION, false).toBool();

  auto from = settings.value(REFORMAT_CHARS_TO_REPLACE_FROM, fromStrings).toStringList();
  auto to   = settings.value(REFORMAT_CHARS_TO_REPLACE_TO, toStrings).toStringList();

  // go to parent or home if the saved directory no longer exists.
  m_root_directory = validDirectoryCheck(m_root_directory);

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
  QSettings settings("Felix de las Pozas Alvarez", "MusicTranscoder");

  settings.setValue(ROOT_DIRECTORY, QDir{validDirectoryCheck(m_root_directory)}.absolutePath());
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
  settings.setValue(REFORMAT_PREFIX_DISK_NUMBER, m_format_configuration.prefix_disk_num);
  settings.setValue(REFORMAT_APPLY_CHAR_SIMPLIFICATION, m_format_configuration.character_simplification);

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

//-----------------------------------------------------------------
const QString Utils::validDirectoryCheck(const QString& directory)
{
  QStringList drivesPath;
  for(auto path: QDir::drives())
  {
    drivesPath << path.absolutePath();
  }

  // go to parent or home if the saved directory no longer exists.
  QDir dir{directory};
  while(!dir.exists() && !dir.isRoot() && !drivesPath.contains(dir.absolutePath()))
  {
    // NOTE: dir.cdUp() doesn't work if the path is nested in more than two directories that
    // don't exist, returning false.
    auto path = dir.absolutePath();

    bool isValidDrive = false;
    for(auto drive: drivesPath)
    {
      if(path.startsWith(drive))
      {
        isValidDrive = true;
        break;
      }
    }

    if(!isValidDrive) break;

    path = path.left(path.lastIndexOf('/'));
    dir = QDir{path};
  }

  if(!dir.exists())
  {
    return QDir::homePath();
  }

  return QDir::toNativeSeparators(dir.absolutePath());
}

//-----------------------------------------------------------------
std::wstring string2widestring(const std::string& str)
{
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.from_bytes(str);
}

//-----------------------------------------------------------------
std::string widestring2string(const std::wstring& wstr)
{
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.to_bytes(wstr);
}

//-----------------------------------------------------------------
std::string Utils::shortFileName(const QString &utffilename)
{
  // First obtain the size needed by passing NULL and 0.
  auto length = GetShortPathNameW(utffilename.toStdWString().c_str(), nullptr, 0);
  if (length == 0) return utffilename.toStdString();

  // Dynamically allocate the correct size
  // (terminating null char was included in length)
  wchar_t *buffer = new wchar_t[length];

  // Now simply call again using same long path.
  length = GetShortPathNameW(utffilename.toStdWString().c_str(), buffer, length);
  if (length == 0) return utffilename.toStdString();

  std::wstring result(buffer, length);
  delete[] buffer;

  return widestring2string(result);
}
