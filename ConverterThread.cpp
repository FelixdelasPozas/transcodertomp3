/*
 File: ConverterThread.cpp
 Created on: 7/5/2015
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
#include "ConverterThread.h"

//-----------------------------------------------------------------
ConverterThread::ConverterThread(const QFileInfo source_info)
: m_source_info{source_info}
, m_source_path{m_source_info.absoluteFilePath().remove(m_source_info.absoluteFilePath().split('/').last())}
, m_stop       {false}
{
}

//-----------------------------------------------------------------
ConverterThread::~ConverterThread()
{
}

//-----------------------------------------------------------------
void ConverterThread::stop()
{
  emit information_message(QString("Converter for '%1' has been cancelled.").arg(m_source_info.absoluteFilePath()));
  m_stop = true;
}

//-----------------------------------------------------------------
bool ConverterThread::has_been_cancelled()
{
  return m_stop;
}


//-----------------------------------------------------------------
void ConverterThread::set_format_configuration(const Utils::FormatConfiguration format)
{
  m_format_configuration = format;
}
