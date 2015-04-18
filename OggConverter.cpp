/*
 File: OggConverter.cpp
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

// Project
#include "OggConverter.h"

//-----------------------------------------------------------------
OGGConverter::OGGConverter(const QFileInfo &origin_info, const QString &destination)
: ConverterThread{origin_info, destination}
{
}

//-----------------------------------------------------------------
OGGConverter::~OGGConverter()
{
}

//-----------------------------------------------------------------
int OGGConverter::read_data()
{
  return 0;
}

//-----------------------------------------------------------------
void OGGConverter::get_source_properties(Source_Info &information)
{

}

