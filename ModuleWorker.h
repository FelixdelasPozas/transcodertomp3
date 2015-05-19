/*
 File: ModuleWorker.h
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

#ifndef MODULE_WORKER_H_
#define MODULE_WORKER_H_

// Project
#include "Worker.h"

class ModuleWorker
: public Worker
{
  public:
    /** \brief ModuleWorker class constructor.
     * \param[in] source_info QFileInfo struct of the source file.
     * \param[in] configuration configuration struct reference.
     *
     */
    explicit ModuleWorker(const QFileInfo source_info, const Utils::TranscoderConfiguration &configuration);

    /** \brief ModuleWorker class virtual destructor.
     *
     */
    virtual ~ModuleWorker();

  protected:
    virtual void run_implementation() override final;

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

    short int m_left_buffer[BUFFER_SIZE];
    short int m_right_buffer[BUFFER_SIZE];
    QString m_module_file_name;
};

#endif // MODULE_WORKER_H_
