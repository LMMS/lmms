/*
    wavtool-yawu
    Copyright (C) 2015 StarBrilliant <m13253@hotmail.com>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 3.0 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this program.  If not,
    see <http://www.gnu.org/licenses/>.
*/

#ifndef YAWU_PCM_FILE_HPP
#define YAWU_PCM_FILE_HPP

#include <cstdint>
#include <ios>
#include <stdexcept>
#include <string>
#include <libwintf8/u8str.h>
#include "proxy_ptr.hpp"

typedef struct SNDFILE_tag SNDFILE;
typedef class SndfileHandle SndFileHandle;

namespace YAWU {

class PCMFile {
public:
    PCMFile();
    PCMFile(const WTF8::u8string &filename, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out, int format = 0, int channels = 0, int sample_rate = 0);
    PCMFile &open(const WTF8::u8string &filename, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out, int format = 0, int channels = 0, int sample_rate = 0);
    ~PCMFile();
    PCMFile &close();
    bool is_open() const;

    int64_t frames() const;
    int format() const;
    int channels() const;
    int sample_rate() const;

    // frames = samples * channels
    size_t read(short *output_buf, size_t frames);
    size_t read(int *output_buf, size_t frames);
    size_t read(float *output_buf, size_t frames);
    size_t read(double *output_buf, size_t frames);

    size_t write(const short *input_buf, size_t frames);
    size_t write(const int *input_buf, size_t frames);
    size_t write(const float *input_buf, size_t frames);
    size_t write(const double *input_buf, size_t frames);

    int64_t seek(int64_t frames, int whence);
    int command(int cmd, void *data, size_t data_size);

    SndfileHandle &sndfile_cxx() const;
    SNDFILE *sndfile_c() const;

    class FileError;
private:
    struct Private;
    proxy_ptr<Private> p;
};

class PCMFile::FileError : public std::runtime_error {
public:
    FileError() :
        std::runtime_error("File operation failed"),
        m_what("File operation failed") {
    }
    FileError(const char *what);
    const char *what() const noexcept { return m_what.c_str(); }
private:
    std::string m_what;
};

}

#endif
