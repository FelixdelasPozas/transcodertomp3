/*
 File: PlaylistGenerator.h
 Created on: 17/5/2015
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

#ifndef PLAYLISTGENERATOR_H_
#define PLAYLISTGENERATOR_H_

// Project
#include "AudioConverter.h"

class PlaylistGenerator
: public AudioConverter
{
  public:
    /** \brief Playlist Generator class constructor.
     * \param[in] source_info QFileInfo struct of the source file.
     * \param[in] configuration configuration struct reference.
     *
     */
    explicit PlaylistGenerator(const QFileInfo source_info, const Utils::TranscoderConfiguration &configuration);

    /** \brief PlaylistGenerator class virtual destructor.
     *
     */
    virtual ~PlaylistGenerator();

  protected:
    virtual void run_implementation() override final;

  private:
    /** \brief Returns the list of file names of the MP3 files in the directory,
     *         sorted in ascending order (case sensitively).
     */
    QStringList get_file_names() const;

    /** \brief Creates and saves the playlist file.
     *
     */
    void generate_playlist();

    /** \brief Returns true if the duration of the song passed as a parameter could be obtained.
     * \param[in] file_name name of a mp3 in the same folder, with extension.
     * \param[out] duration duration of the song in seconds.
     *
     */
    bool get_song_duration(const QString &file_name, long long &duration);
};

#endif // PLAYLISTGENERATOR_H_
