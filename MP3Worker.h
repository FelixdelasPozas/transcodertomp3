/*
 File: MP3Worker.h
 Created on: 9/5/2015
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

#ifndef MP3_WORKER_H_
#define MP3_WORKER_H_

// Project
#include "AudioWorker.h"

// Qt
#include <QMutex>

// id3lib
#include <id3/tag.h>

class MP3Worker
: public AudioWorker
{
  public:
    /** \brief MP3Worker class constructor.
     * \param[in] source_info QFileInfo struct of the source file.
     * \param[in] configuration configuration struct reference.
     *
     */
    explicit MP3Worker(const QFileInfo source_info, const Utils::TranscoderConfiguration &configuration);

    /** \brief MP3Worker class virtual destructor.
     *
     */
    virtual ~MP3Worker();

  protected:
    virtual void run_implementation() override final;

  private:
    static QString MP3_EXTENSION;
    static QString COVER_MIME_TYPE_1;
    static QString COVER_MIME_TYPE_2;

    /** \brief Helper method to extract the cover.
     *
     */
    bool extract_cover(const ID3_Tag &file_tag);

    static QMutex s_mutex;
};

#endif // MP3_WORKER_H_
