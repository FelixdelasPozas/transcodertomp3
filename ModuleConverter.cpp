/*
 File: ModuleConverter.cpp
 Created on: 10/5/2015
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
#include <ModuleConverter.h>

//-----------------------------------------------------------------
ModuleConverter::ModuleConverter(const QFileInfo source_info)
: ConverterThread{source_info}
, m_filename     {nullptr}
, m_module       {nullptr}
{
}

//-----------------------------------------------------------------
ModuleConverter::~ModuleConverter()
{
}

//-----------------------------------------------------------------
void ModuleConverter::run()
{
  open_next_destination_file();

  init_mikmodlib();

  process_module();

  deinit_mikmodlib();

  close_destination_file();
}

//-----------------------------------------------------------------
bool ModuleConverter::init_mikmodlib()
{
//  if (!MikMod_InitThreads())
//  {
//    emit error_message(QString("Could not initialize mikmod library for %1. Error: Multiple threads not supported.").arg(m_source_info.absoluteFilePath()));
//    return false;
//  }
//
//  MikMod_RegisterDriver(&drv_nos);
//  MikMod_RegisterAllLoaders();
//
//  md_mode |= DMODE_SOFT_MUSIC;
//  if (MikMod_Init(""))
//  {
//    emit error_message(QString("Could not initialize mikmod library for %1. Error: %2.").arg(m_source_info.absoluteFilePath()).arg(QString(MikMod_strerror(MikMod_errno))));
//    return false;
//  }
//
//  auto module_name = m_source_info.absoluteFilePath().split('/').last();
//  m_module = Player_Load(module_name.toStdString().c_str(), 256, 1);
//  if (!m_module)
//  {
//    emit error_message(QString("Could not load module %1. Error: %2").arg(m_source_info.absoluteFilePath()).arg(QString(MikMod_strerror(MikMod_errno))));
//    return false;
//  }

  return true;
}

//-----------------------------------------------------------------
bool ModuleConverter::process_module()
{
  /* start module */
//  Player_Start(m_module);
//
//  while (Player_Active())
//  {
//    /* we're playing */
//    auto output_bytes = VC_WriteBytes(reinterpret_cast<SBYTE *>(&m_buffer), BUFFER_SIZE);
//
//    lame_encode_internal_buffer(0, output_bytes, reinterpret_cast<unsigned char *>(&m_buffer), reinterpret_cast<unsigned char *>(&m_buffer));
//
//    MikMod_Update();
//  }
//
//  Player_Stop();

  return true;
}


//-----------------------------------------------------------------
void ModuleConverter::deinit_mikmodlib()
{
//  Player_Free(m_module);
//  MikMod_Exit();
}
