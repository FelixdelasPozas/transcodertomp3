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
#include <QMutex>

// C++
#include <functional>

namespace Utils
{
  extern const QStringList MODULE_FILE_EXTENSIONS;
  extern const QStringList WAVE_FILE_EXTENSIONS;
  extern const QStringList MOVIE_FILE_EXTENSIONS;
  extern const QString TEMPORAL_FILE_EXTENSION;
  static QMutex s_mutex;

  /** \brief Returns true if the file given as parameter has a audio extension.
   * \param[in] file file QFileInfo struct.
   *
   */
  bool isAudioFile(const QFileInfo &file);

  /** \brief Returns true if the file given as parameter has a video extension.
   * \param[in] file file QFileInfo struct.
   *
   */
  bool isVideoFile(const QFileInfo &file);

  /** \brief Returns true if the file given as parameter has a module extension.
   * \param[in] file file QFileInfo struct.
   *
   */
  bool isModuleFile(const QFileInfo &file);

  /** \brief Returns true if the file given as parameter has "mp3" as extension.
   * \param[in] file file QFileInfo struct.
   *
   */
  bool isMP3File(const QFileInfo &file);

  /** \brief Returs true if the string has only spaces.
   *
   */
  bool isSpaces(const QString &string);

  /** \brief Returns the files in the specified directory tree that has the specified extensions.
   * \param[in] rootDir starting directory.
   * \param[in] filters extensions of the files to return.
   * \param[in] with_subdirectories boolean that indicates if all the files in the subdiretories that
   *            comply the conditions must be returned.
   * \param[in] condition additional condition that the files must comply with.
   *
   * NOTE: while this will return any file info (that complies with the filter and the conditions) mp3
   *       files will be returned first. That's because i want to process those before anything else.
   *       Yes, it's arbitrary but doesn't affect the results.
   *
   */
  QList<QFileInfo> findFiles(const QDir rootDirectory,
                             const QStringList extensions,
                             bool with_subdirectories = true,
                             const std::function<bool (const QFileInfo &)> &condition = [](const QFileInfo &info) { return true; });

  struct FormatConfiguration
  {
    bool                          apply;                     // true to apply the formatting process.
    QString                       chars_to_delete;           // characters to delete.
    QList<QPair<QString,QString>> chars_to_replace;          // strings to replace (pair <to replace, with>).
    int                           number_of_digits;          // number of digits the number should have.
    QString                       number_and_name_separator; // separator between the number and the rest of the name.
    bool                          to_title_case;             // capitalize the first letter of every word in the name.

    FormatConfiguration()
    {
      apply                     = true;
      chars_to_delete           = QString("¿?");
      chars_to_replace          << QPair<QString, QString>("[", "(")
                                << QPair<QString, QString>("]", ")")
                                << QPair<QString, QString>("}", ")")
                                << QPair<QString, QString>("{", "(")
                                << QPair<QString, QString>(".", " ")
                                << QPair<QString, QString>("_", " ")
                                << QPair<QString, QString>(":", ",")
                                << QPair<QString, QString>("~", "-")
                                << QPair<QString, QString>("/", "-")
                                << QPair<QString, QString>(";", "-")
                                << QPair<QString, QString>("\"", "''")
                                << QPair<QString, QString>("pt ", "part ")
                                << QPair<QString, QString>("#", "Nº")
                                << QPair<QString, QString>("arr ", "arranged ")
                                << QPair<QString, QString>("cond ", "conductor ")
                                << QPair<QString, QString>("comp ", "composer ")
                                << QPair<QString, QString>("feat ", "featuring ")
                                << QPair<QString, QString>("alt ", "alternate ");
      number_and_name_separator = '-';
      number_of_digits          = 2;
      to_title_case             = true;
    }
  };

  /** \brief Returns a transformed filename according to the specified parameters.
   * \param[in] filename file name with absolute path.
   * \param[in] conf configuration parameters.
   * \param[in] add_extension true to add the ".mp3" extension to the formatted string.
   *
   */
  QString formatString(const QString filename,
                       const FormatConfiguration &conf,
                       bool add_mp3_extension = true);

  /** \brief Returns true if the string represents a roman numeral.
   *
   */
  bool isRomanNumeral(const QString string_part);

  class TranscoderConfiguration
  {
    public:
      /** \brief TranscoderConfiguration class constructor.
       *
       */
      TranscoderConfiguration();

      /** \brief Loads the configuration data from disk.
       *
       */
      void load();

      /** \brief Saves the configuration to disk.
       *
       */
      void save() const;

      /** \brief Returns the root directory to start searching for files.
       *
       */
      const QString &rootDirectory() const;

      /** \brief Returns the number of simultaneous threads in the transcoding process.
       *
       */
      int numberOfThreads() const;

      /** \brief Returns the bitrate of the encoding process.
       *
       */
      int bitrate() const;

      /** \brief Returns the cover picture name
       *
       */
      const QString &coverPictureName() const;

      /** \brief Returns true if the output must be delete on process cancellation.
       *
       */
      bool deleteOutputOnCancellation() const;

      /** \brief Returns true if the cover picture is to be extracted if found in the input metadata.
       *
       */
      bool extractMetadataCoverPicture() const;

      /** \brief Returns true if the input file is to be renamed after a successfull transcoding.
       *
       */
      bool renameInputOnSuccess() const;

      /** \brief Returns the extension to use to rename input files after a successfull transcoding.
       *
       */
      const QString renamedInputFilesExtension() const;

      /** \brief Returns the reformatting configuration.
       *
       */
      const FormatConfiguration &formatConfiguration() const;

      /** \brief Returns the quality of the encoding process.
       *
       */
      int quality() const;

      /** \brief Returns true if after transcoding M3U playlists must be created per input directory.
       *
       */
      bool createM3Ufiles() const;

      /** \brief Returns true if the output file name must be reformatted.
       *
       */
      bool reformatOutputFilename() const;

      /** \brief Returns true if the tags in mp3 input files must be deleted.
       *
       */
      bool stripTagsFromMp3() const;

      /** \brief Returns true if the audio files must be processed.
       *
       */
      bool transcodeAudio() const;

      /** \brief Returns true if module files must be processed.
       *
       */
      bool transcodeModule() const;

      /** \brief Returns true if video files must be processed.
       *
       */
      bool transcodeVideo() const;

      /** \brief Returns true if the output must be splitted into several files using a CUE file.
       *
       */
      bool useCueToSplit() const;

      /** \brief Returns true if the output file name must be constructed from the metadata in the input file.
       *
       */
      bool useMetadataToRenameOutput() const;

      /** \brief Sets the root directory to start searching for files to transcode.
       * \param[in] path root directory path.
       *
       */
      void setRootDirectory(const QString &path);

      /** \brief Sets the number of simultaneous threads to use in the transcoding process.
       * \param[in] value number of threads.
       *
       */
      void setNumberOfThreads(int value);

      /** \brief Sets the bitrate.
       * \param[in] value encoding process bitrate.
       *
       */
      void setBitrate(int value);

      /** \brief Sets the file name for the cover picture file.
       * \param[in] filename cover file name without extension.
       *
       */
      void setCoverPictureName(const QString& filename);

      /** \brief Sets if the output must be deleted if the process couldn't finish.
       * \param[in] enabled boolean value.
       *
       */
      void setDeleteOutputOnCancellation(bool enabled);

      /** \brief Sets if the cover picture present in the input file must be extracted to disk.
       * \param[in] enabled boolean value.
       *
       */
      void setExtractMetadataCoverPicture(bool enabled);

      /** \brief Sets if the input file must be renamed after a successfull transcoding.
       * \param[in] enabled boolean value.
       *
       */
      void setRenameInputOnSuccess(bool enabled);

      /** \brief Sets the extension to use to rename input files after a successfil transcoding.
       * \param[in] extension extension string.
       *
       */
      void setRenamedInputFilesExtension(const QString &extension);

      /** \brief Sets the reformatting configuration.
       * \param[in] configuration FormatConfiguration struct.
       *
       */
      void setFormatConfiguration(const FormatConfiguration& configuration);

      /** \brief Sets the quality of the encoding process.
       * \param[in] value quality value according to lame encoder.
       *
       */
      void setQuality(int value);

      /** \brief Sets if after transcoding a M3U playlists must be created per input directory.
       *
       */
      void setCreateM3Ufiles(bool enabled);

      /** \brief Sets if the output file name must be reformatted.
       * \param[in] enabled boolean value.
       *
       */
      void setReformatOutputFilename(bool enabled);

      /** \brief Sets if the tags of input mp3 files must be deleted.
       * \param[in] enabled boolean value.
       *
       */
      void setStripTagsFromMp3(bool enabled);

      /** \brief Sets if the audio files must be processed.
       * \param[in] enabled boolean value.
       *
       */
      void setTranscodeAudio(bool enabled);

      /** \brief Sets if the module files must be processed.
       * \param[in] enabled boolean value.
       *
       */
      void setTranscodeModule(bool enabled);

      /** \brief Sets if the audio track of video files must be processed.
       * \param[in] enabled boolean value.
       *
       */
      void setTranscodeVideo(bool enabled);

      /** \brief Sets if the output must be splitted into several files using a CUE file.
       * \param[in] enabled boolean value.
       *
       */
      void setUseCueToSplit(bool enabled);

      /** \brief Sets if the output file name must be constructed from the title and track metadata in the input file.
       * \param[in] enabled boolean value.
       *
       */
      void setUseMetadataToRenameOutput(bool enabled);

    private:
      QString m_root_directory;
      int     m_number_of_threads;
      bool    m_transcode_audio;
      bool    m_transcode_video;
      bool    m_transcode_module;
      bool    m_strip_tags_from_MP3;
      bool    m_use_CUE_to_split;
      bool    m_rename_input_on_success;
      QString m_renamed_input_extension;
      bool    m_use_metadata_to_rename_output;
      bool    m_delete_output_on_cancellation;
      bool    m_extract_metadata_cover_picture;
      QString m_cover_picture_name;
      int     m_bitrate;
      int     m_quality;
      bool    m_create_M3U_files;

      FormatConfiguration m_format_configuration;

      static const QString ROOT_DIRECTORY;
      static const QString NUMBER_OF_THREADS;
      static const QString TRANSCODE_AUDIO;
      static const QString TRANSCODE_VIDEO;
      static const QString TRANSCODE_MODULE;
      static const QString STRIP_MP3;
      static const QString USE_CUE_SHEET;
      static const QString RENAME_INPUT_ON_SUCCESS;
      static const QString RENAMED_INPUT_EXTENSION;
      static const QString USE_METADATA_TO_RENAME;
      static const QString DELETE_ON_CANCELLATION;
      static const QString EXTRACT_COVER_PICTURE;
      static const QString COVER_PICTURE_NAME;
      static const QString BITRATE;
      static const QString QUALITY;
      static const QString CREATE_M3U_FILES;
      static const QString REFORMAT_APPLY;
      static const QString REFORMAT_CHARS_TO_DELETE;
      static const QString REFORMAT_CHARS_TO_REPLACE_FROM;
      static const QString REFORMAT_CHARS_TO_REPLACE_TO;
      static const QString REFORMAT_SEPARATOR;
      static const QString REFORMAT_NUMBER_OF_DIGITS;
      static const QString REFORMAT_USE_TITLE_CASE;

      static const QString SETTINGS_FILENAME;
  };

}

#endif // UTILS_H_
