/*
 File: ModuleConverter.h
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

#ifndef MODULECONVERTER_H_
#define MODULECONVERTER_H_

// Project
#include <ConverterThread.h>

class ModuleConverter
: public ConverterThread
{
  public:
    /** \brief ModuleConverter class constructor.
     * \param[in] source_info QFileInfo struct of the source file.
     *
     */
    explicit ModuleConverter(const QFileInfo source_info);

    /** \brief ModuleConverter class virtual destructor.
     *
     */
    virtual ~ModuleConverter();

  protected:
    virtual void run() override final;

  private:
    /** \brief Gets the information about the module.
     *
     */
    bool init();

    /** \brief Converts the module to mp3.
     *
     */
    void process_module();

    virtual Destinations compute_destinations() override final;

    static const int BUFFER_SIZE = 16000;
    static const int SAMPLE_RATE = 44100;

    short int m_left_buffer[BUFFER_SIZE];
    short int m_right_buffer[BUFFER_SIZE];
    QString m_module_file_name;
};

#endif // MODULECONVERTER_H_
