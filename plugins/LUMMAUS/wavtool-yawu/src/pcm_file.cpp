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

#include "pcm_file.hpp"
#include <cstdint>
#include <cstdio>
#include <ios>
#include <libwintf8/fileio.h>
#ifdef _WIN32
#include <libwintf8/localconv.h>
#endif
#include <libwintf8/u8str.h>
#ifdef _WIN32
#include <windows.h>
#define ENABLE_SNDFILE_WINDOWS_PROTOTYPES 1
#endif
#include <sndfile.hh>

namespace YAWU {

struct PCMFile::Private {
    SndfileHandle sndfile;
    bool is_open = false;
};

PCMFile::PCMFile() {
}

PCMFile::PCMFile(const WTF8::u8string &filename, std::ios_base::openmode mode, int format, int channels, int sample_rate) {
    open(filename, mode, format, channels, sample_rate);
}

PCMFile::~PCMFile() {
    close();
}

PCMFile &PCMFile::open(const WTF8::u8string &filename, std::ios_base::openmode mode, int format, int channels, int sample_rate) {
    close();
    int sfmode = SFM_READ;
    if((mode & (std::ios_base::in | std::ios_base::out)) == (std::ios_base::in | std::ios_base::out)) {
        sfmode = SFM_RDWR;
    } else if(mode & std::ios_base::out) {
        sfmode = SFM_WRITE;
    }
#ifdef _WIN32
    p->sndfile = SndFileHandle(filename.to_wide().c_str(), sfmode, format, channels, sample_rate);
#else
    p->sndfile = SndFileHandle(filename, sfmode, format, channels, sample_rate);
#endif
    if(p->sndfile.error() != 0)
        throw FileError(p->sndfile.strError());
    p->is_open = true;
    return *this;
}

PCMFile &PCMFile::close() {
    p->sndfile = SndFileHandle();
    p->is_open = false;
    return *this;
}

bool PCMFile::is_open() const {
    return p->is_open;
}

int64_t PCMFile::frames() const {
    return p->sndfile.frames();
}

int PCMFile::format() const {
    return p->sndfile.format();
}

int PCMFile::channels() const {
    return p->sndfile.channels();
}

int PCMFile::sample_rate() const {
    return p->sndfile.samplerate();
}

size_t PCMFile::read(short *output_buf, size_t frames) {
    auto read_count = p->sndfile.readf(output_buf, frames);
    if(p->sndfile.error() != 0)
        throw FileError(p->sndfile.strError());
    return read_count;
}

size_t PCMFile::read(int *output_buf, size_t frames) {
    auto read_count = p->sndfile.readf(output_buf, frames);
    if(p->sndfile.error() != 0)
        throw FileError(p->sndfile.strError());
    return read_count;
}

size_t PCMFile::read(float *output_buf, size_t frames) {
    auto read_count = p->sndfile.readf(output_buf, frames);
    if(p->sndfile.error() != 0)
        throw FileError(p->sndfile.strError());
    return read_count;
}

size_t PCMFile::read(double *output_buf, size_t frames) {
    auto read_count = p->sndfile.readf(output_buf, frames);
    if(p->sndfile.error() != 0)
        throw FileError(p->sndfile.strError());
    return read_count;
}

size_t PCMFile::write(const short *input_buf, size_t frames) {
    auto write_count = p->sndfile.writef(input_buf, frames);
    if(p->sndfile.error() != 0)
        throw FileError(p->sndfile.strError());
    return write_count;
}

size_t PCMFile::write(const int *input_buf, size_t frames) {
    auto write_count = p->sndfile.writef(input_buf, frames);
    if(p->sndfile.error() != 0)
        throw FileError(p->sndfile.strError());
    return write_count;
}

size_t PCMFile::write(const float *input_buf, size_t frames) {
    auto write_count = p->sndfile.writef(input_buf, frames);
    if(p->sndfile.error() != 0)
        throw FileError(p->sndfile.strError());
    return write_count;
}

size_t PCMFile::write(const double *input_buf, size_t frames) {
    auto write_count = p->sndfile.writef(input_buf, frames);
    if(p->sndfile.error() != 0)
        throw FileError(p->sndfile.strError());
    return write_count;
}

int64_t PCMFile::seek(int64_t frames, int whence) {
    auto new_offset = p->sndfile.seek(frames, whence);
    if(p->sndfile.error() != 0)
        throw FileError(p->sndfile.strError());
    return new_offset;
}

int PCMFile::command(int cmd, void *data, size_t data_size) {
    auto result = p->sndfile.command(cmd, data, data_size);
    if(p->sndfile.error() != 0)
        throw FileError(p->sndfile.strError());
    return result;
}

SndfileHandle &PCMFile::sndfile_cxx() const {
    return p->sndfile;
}

SNDFILE *PCMFile::sndfile_c() const {
    return p->sndfile.rawHandle();
}

PCMFile::FileError::FileError(const char *what) :
    std::runtime_error(what),
#ifdef _WIN32
    m_what(WTF8::local_to_utf8(std::string(what)))
#else
    m_what(what)
#endif
    {
}

}
