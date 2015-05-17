/*
 File: PlaylistGenerator.cpp
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

// Project
#include "PlaylistGenerator.h"

//-----------------------------------------------------------------
PlaylistGenerator::PlaylistGenerator(const QFileInfo source_info, const Utils::TranscoderConfiguration& configuration)
: ConverterThread(source_info, configuration)
{
}

//-----------------------------------------------------------------
PlaylistGenerator::~PlaylistGenerator()
{
}

//-----------------------------------------------------------------
void PlaylistGenerator::run_implementation()
{
}

// TODO: implement
