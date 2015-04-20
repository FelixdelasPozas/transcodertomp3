/*
 File: ConverterThread.h
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

#ifndef CONVERTER_THREAD_H_
#define CONVERTER_THREAD_H_

// Project
#include "Utils.h"

// Qt
#include <QThread>
#include <QFileInfo>

// Lame
#include <lame.h>

// C++
#include <fstream>

class ConverterThread
: public QThread
{
    Q_OBJECT
  public:
    explicit ConverterThread(const QFileInfo origin_info);
    virtual ~ConverterThread();

    void stop();
    bool has_been_cancelled();

  signals:
    void error_message(const QString message);
    void information_message(const QString message);
    void progress(int value);

  protected:
      virtual void run() override final;

      // the derived classes will write the raw PCM data into these buffers.
      // what buffer will be used to encode will be given by the "format"
      // field in the Source_Info of the source file.
      short int m_pcm_interleaved[4096];
      short int m_pcm_right[4096];
      short int m_pcm_left[4096];

      enum class PCM_FORMAT: unsigned char { STEREO, INTERLEAVED, UNSUPPORTED };

      // information of the PCM data the decoding of the source file will produce.
      struct Source_Info
      {
        int         num_channels;
        long        samplerate;
        MPEG_mode_e mode;
        PCM_FORMAT  format;
        long        num_samples;
      };

      const QFileInfo m_origin_info;
  private:
      // derived classes will use this funcion to open and make the necessary decoder
      // initializations.
      virtual bool open_source_file() = 0;

      // derived classes will use this function to decode the source file and write the
      // data to the PCM buffers. returns the number of bytes copies to the PCM buffer.
      virtual long int read_data() = 0;

      // derived classes will return the properties of the PCM data that will be generated.
      virtual void get_source_properties(Source_Info &information) = 0;

      int init_LAME_codec(const Source_Info &information);

      lame_global_flags *m_gfp;
      unsigned char      m_mp3_buffer[8480];
      bool               m_stop;
      const Utils::CleanConfiguration m_clean_configuration;
};

#endif // CONVERTER_THREAD_H_
