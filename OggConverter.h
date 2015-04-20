/*
 File: OggConverter.h
 Created on: 18/4/2015
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

#ifndef OGG_CONVERTER_H_
#define OGG_CONVERTER_H_

// Project
#include <ConverterThread.h>

// Ogg Vorbis lib
#include <vorbis/vorbisfile.h>

class OGGConverter
: public ConverterThread
{
  public:
    explicit OGGConverter(const QFileInfo origin_info);
    virtual ~OGGConverter();

  private:
    virtual bool open_source_file() override;

    virtual long int read_data() override;

    virtual void get_source_properties(Source_Info &information) override;

    OggVorbis_File m_vorbis_file;
    bool           m_init;
};

#endif // OGG_CONVERTER_H_
