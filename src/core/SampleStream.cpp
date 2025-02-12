/*
 * SampleStream.cpp - an object meant to stream audio files
 *
 * Copyright (c) 2025 Sotonye Atemie <sakertooth@gmail.com>
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#include "SampleStream.h"

#include "ThreadPool.h"

namespace lmms {
SampleStream::SampleStream(const std::filesystem::path& path, std::size_t size)
	: m_audioFile(path, AudioFile::Mode::Read)
	, m_buffer(size)
	, m_diskStream(ThreadPool::instance().enqueue(&SampleStream::runDiskStream, this))
{
}

SampleStream::~SampleStream()
{
    m_quit = true;
	m_diskStream.wait();
}

auto SampleStream::read(SampleFrame* dst, std::size_t size) -> std::size_t
{
    const auto numFramesToRead = std::min({size, streamSize(), m_buffer.size()});
    assert(numFramesToRead > 0);

    std::fill_n(dst, size, SampleFrame{});
    if (m_readIndex >= m_writeIndex || numFramesToRead == 0) { return 0; }

    std::copy_n(m_buffer.begin() + m_readIndex, numFramesToRead, dst);
    m_readIndex += numFramesToRead % m_buffer.size();
    return numFramesToRead;
}

void SampleStream::runDiskStream()
{
    while (!m_quit)
    {
        if (m_writeIndex <= m_readIndex) { continue; }
        const auto numFramesToWrite = m_buffer.size() - m_writeIndex;
        m_audioFile.read(m_buffer.data() + m_writeIndex, numFramesToWrite);
    }
}

} // namespace lmms