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

// C++
#include <functional>

// Qt
#include <QDir>
#include <QString>
#include <QPair>
#include <QMutex>
#include <QLabel>

// C++
#include <memory>

class QSettings;

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

  /** \brief Checks the given directory for existance and readability. If the directory is
   *  not readable or doesn't exist it will try the parent recursively until returning a valid
   *  path or the user home directory.
   * \param[in] directory path.
   */
  const QString validDirectoryCheck(const QString &directory);

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
    bool                          apply;                     /** true to apply the formatting process.                  */
    QString                       chars_to_delete;           /** characters to delete.                                  */
    QList<QPair<QString,QString>> chars_to_replace;          /** strings to replace (pair <to replace, with>).          */
    int                           number_of_digits;          /** number of digits the number should have.               */
    QString                       number_and_name_separator; /** separator between the number and the rest of the name. */
    bool                          to_title_case;             /** capitalize the first letter of every word in the name. */
    bool                          prefix_disk_num;           /** true to prefix the track number with the disk number.  */
    bool                          character_simplification;  /** true to use unicode character decomposition.           */

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
      prefix_disk_num           = false;
      character_simplification  = false;
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

  /** \brief Returns the short file name in the Windows filesystem.
   * \param[in] utffilename Long windows file name.
   *
   */
  std::string shortFileName(const QString &utffilename);

  /** \class TranscoderConfiguration
   * \brief Implements the configuration storage/management.
   *
   */
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
      inline const QString &rootDirectory() const
      { return m_root_directory; }

      /** \brief Returns the number of simultaneous threads in the transcoding process.
       *
       */
      inline int numberOfThreads() const
      { return m_number_of_threads; }

      /** \brief Returns the bitrate of the encoding process.
       *
       */
      inline int bitrate() const
      { return m_bitrate; }

      /** \brief Returns the cover picture name
       *
       */
      inline const QString &coverPictureName() const
      { return m_cover_picture_name; }

      /** \brief Returns true if the output must be delete on process cancellation.
       *
       */
      inline bool deleteOutputOnCancellation() const
      { return m_delete_output_on_cancellation; }

      /** \brief Returns true if the cover picture is to be extracted if found in the input metadata.
       *
       */
      inline bool extractMetadataCoverPicture() const
      { return m_extract_metadata_cover_picture; }

      /** \brief Returns true if the input file is to be renamed after a successfull transcoding.
       *
       */
      inline bool renameInputOnSuccess() const
      { return m_rename_input_on_success; }

      /** \brief Returns the extension to use to rename input files after a successfull transcoding.
       *
       */
      inline const QString renamedInputFilesExtension() const
      { return m_renamed_input_extension; }

      /** \brief Returns the reformatting configuration.
       *
       */
      inline const FormatConfiguration &formatConfiguration() const
      { return m_format_configuration; }

      /** \brief Returns the quality of the encoding process.
       *
       */
      inline int quality() const
      { return m_quality; }

      /** \brief Returns true if after transcoding M3U playlists must be created per input directory.
       *
       */
      inline bool createM3Ufiles() const
      { return m_create_M3U_files; }

      /** \brief Returns true if the output file name must be reformatted.
       *
       */
      inline bool reformatOutputFilename() const
      { return m_format_configuration.apply; }

      /** \brief Returns true if the tags in mp3 input files must be deleted.
       *
       */
      inline bool stripTagsFromMp3() const
      { return m_strip_tags_from_MP3; }

      /** \brief Returns true if the audio files must be processed.
       *
       */
      inline bool transcodeAudio() const
      { return m_transcode_audio; }

      /** \brief Returns true if module files must be processed.
       *
       */
      inline bool transcodeModule() const
      { return m_transcode_module; }

      /** \brief Returns true if video files must be processed.
       *
       */
      inline bool transcodeVideo() const
      { return m_transcode_video; }

      /** \brief Returns true if the output must be splitted into several files using a CUE file.
       *
       */
      inline bool useCueToSplit() const
      { return m_use_CUE_to_split; }

      /** \brief Returns true if the output file name must be constructed from the metadata in the input file.
       *
       */
      inline bool useMetadataToRenameOutput() const
      { return m_use_metadata_to_rename_output; }

      /** \brief Sets the root directory to start searching for files to transcode.
       * \param[in] path root directory path.
       *
       */
      inline void setRootDirectory(const QString &path)
      { m_root_directory = path; }

      /** \brief Sets the number of simultaneous threads to use in the transcoding process.
       * \param[in] value number of threads.
       *
       */
      inline void setNumberOfThreads(int value)
      { m_number_of_threads = value; }

      /** \brief Sets the bitrate.
       * \param[in] value encoding process bitrate.
       *
       */
      inline void setBitrate(int value)
      { m_bitrate = value; }

      /** \brief Sets the file name for the cover picture file.
       * \param[in] filename cover file name without extension.
       *
       */
      inline void setCoverPictureName(const QString& filename)
      { m_cover_picture_name = filename; }

      /** \brief Sets if the output must be deleted if the process couldn't finish.
       * \param[in] value boolean value.
       *
       */
      inline void setDeleteOutputOnCancellation(bool value)
      { m_delete_output_on_cancellation = value; }

      /** \brief Sets if the cover picture present in the input file must be extracted to disk.
       * \param[in] value boolean value.
       *
       */
      inline void setExtractMetadataCoverPicture(bool value)
      { m_extract_metadata_cover_picture = value; }

      /** \brief Sets if the input file must be renamed after a successfull transcoding.
       * \param[in] enabled boolean value.
       *
       */
      inline void setRenameInputOnSuccess(bool value)
      { m_rename_input_on_success = value; }

      /** \brief Sets the extension to use to rename input files after a successfil transcoding.
       * \param[in] extension extension string.
       *
       */
      inline void setRenamedInputFilesExtension(const QString &extension)
      { m_renamed_input_extension = extension; }

      /** \brief Sets the reformatting configuration.
       * \param[in] configuration FormatConfiguration struct.
       *
       */
      inline void setFormatConfiguration(const FormatConfiguration& configuration)
      { m_format_configuration = configuration; }

      /** \brief Sets the quality of the encoding process.
       * \param[in] value quality value according to lame encoder.
       *
       */
      inline void setQuality(int value)
      { m_quality = value; }

      /** \brief Sets if after transcoding a M3U playlists must be created per input directory.
       * \param[in] value Boolean value.
       *
       */
      inline void setCreateM3Ufiles(bool value)
      { m_create_M3U_files = value; }

      /** \brief Sets if the output file name must be reformatted.
       * \param[in] value Boolean value.
       *
       */
      inline void setReformatOutputFilename(bool value)
      { m_format_configuration.apply = value; }

      /** \brief Sets if the tags of input mp3 files must be deleted.
       * \param[in] value Boolean value.
       *
       */
      inline void setStripTagsFromMp3(bool value)
      { m_strip_tags_from_MP3 = value; }

      /** \brief Sets if the audio files must be processed.
       * \param[in] value Boolean value.
       *
       */
      inline void setTranscodeAudio(bool value)
      { m_transcode_audio = value; }

      /** \brief Sets if the module files must be processed.
       * \param[in] value Boolean value.
       *
       */
      inline void setTranscodeModule(bool value)
      { m_transcode_module = value; }

      /** \brief Sets if the audio track of video files must be processed.
       * \param[in] value boolean value.
       *
       */
      inline void setTranscodeVideo(bool value)
      { m_transcode_video = value; }

      /** \brief Sets if the output must be splitted into several files using a CUE file.
       * \param[in] value boolean value.
       *
       */
      inline void setUseCueToSplit(bool value)
      { m_use_CUE_to_split = value; }

      /** \brief Sets if the output file name must be constructed from the title and track metadata in the input file.
       * \param[in] value boolean value.
       *
       */
      inline void setUseMetadataToRenameOutput(bool value)
      { m_use_metadata_to_rename_output = value; }

    private:
      QString m_root_directory;                  /** last used directory.                                                         */
      int     m_number_of_threads;               /** number of threads to use.                                                    */
      bool    m_transcode_audio;                 /** true to transcode audio files to mp3, false otherwise.                       */
      bool    m_transcode_video;                 /** true to extract and transcode audio stream of video files, false otherwise.  */
      bool    m_transcode_module;                /** true to transcode module files to mp3, false otherwise.                      */
      bool    m_strip_tags_from_MP3;             /** true to remove metadata from mp3 files, false otherwise.                     */
      bool    m_use_CUE_to_split;                /** true to use CUE files to split audio files, false otherwise.                 */
      bool    m_rename_input_on_success;         /** true to rename the output file on a successful transcoding, false otherwise. */
      QString m_renamed_input_extension;         /** extension to add to succesfully transcoded audio files.                      */
      bool    m_use_metadata_to_rename_output;   /** use metadata if found to rename the output mp3 file.                         */
      bool    m_delete_output_on_cancellation;   /** deletes output file if the process is cancelled.                             */
      bool    m_extract_metadata_cover_picture;  /** true to extract the metadata cover if found, false otherwise.                */
      QString m_cover_picture_name;              /** name of the cover picture file.                                              */
      int     m_bitrate;                         /** mp3 output file bitrate.                                                     */
      int     m_quality;                         /** mp3 output file quality level.                                               */
      bool    m_create_M3U_files;                /** true to create playlists after the transcoding process.                      */

      FormatConfiguration m_format_configuration; /** title formatting configuration. */

      /** settings key strings. */
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
      static const QString REFORMAT_PREFIX_DISK_NUMBER;
      static const QString REFORMAT_APPLY_CHAR_SIMPLIFICATION;
  };

  /** \class ClickableHoverLabel
  * \brief ClickableLabel subclass that changes the mouse cursor when hovered.
  *
  */
  class ClickableHoverLabel
  : public QLabel
  {
      Q_OBJECT
    public:
      /** \brief ClickableHoverLabel class constructor.
      * \param[in] parent Raw pointer of the widget parent of this one.
      * \f Widget flags.
      *
      */
      explicit ClickableHoverLabel(QWidget *parent=nullptr, Qt::WindowFlags f=Qt::WindowFlags())
      : QLabel(parent, f)
      {};

      /** \brief ClickableHoverLabel class constructor.
      * \param[in] text Label text.
      * \param[in] parent Raw pointer of the widget parent of this one.
      * \f Widget flags.
      *
      */
      explicit ClickableHoverLabel(const QString &text, QWidget *parent=0, Qt::WindowFlags f=Qt::WindowFlags())
      : QLabel(text, parent, f)
      {};
      
      /** \brief ClickableHoverLabel class virtual destructor.
      *
      */
      virtual ~ClickableHoverLabel()
      {};

    signals:
      void clicked();

    protected:
      void mousePressEvent(QMouseEvent* e)
      {
        emit clicked();
        QLabel::mousePressEvent(e);
      }  

      virtual void enterEvent(QEnterEvent *event) override
      {
        setCursor(Qt::PointingHandCursor);
        QLabel::enterEvent(event);
      }

      virtual void leaveEvent(QEvent *event) override
      {
        setCursor(Qt::ArrowCursor);
        QLabel::leaveEvent(event);
      }
  };

  /** \brief Returns the relevant QSettings object depeding on the presence of the INI filename.
   */
  std::unique_ptr<QSettings> applicationSettings();
}

#endif // UTILS_H_
