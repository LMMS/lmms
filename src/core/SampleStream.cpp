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
#include <qdebug.h>

#include "ThreadPool.h"

namespace lmms {
SampleStream::SampleStream(const std::filesystem::path& path, std::size_t size)
	: m_sampleFile(path, SampleFile::Mode::Read)
	, m_buffer(size)
{
}

SampleStream::SampleStream(SampleStream&& stream) noexcept
    : m_sampleFile(std::move(stream.m_sampleFile))
    , m_buffer(std::move(stream.m_buffer))
    , m_diskStream(std::move(stream.m_diskStream))
    , m_quit(stream.m_quit.load())
    , m_readIndex(stream.m_readIndex.load())
    , m_writeIndex(stream.m_writeIndex.load())
{
    stop();
    start();
}

SampleStream& SampleStream::operator=(SampleStream&& stream) noexcept
{
    m_sampleFile = std::move(stream.m_sampleFile);
    m_buffer = std::move(stream.m_buffer);
    m_diskStream = std::move(stream.m_diskStream);
    m_quit = stream.m_quit.load();
    m_readIndex = stream.m_readIndex.load();
    m_writeIndex = stream.m_writeIndex.load();

    stop();
    start();

    return *this;
}

SampleStream::~SampleStream()
{
    stop();
}

void SampleStream::start()
{
    if (m_diskStream.valid()) { return; }

    m_quit = false;
    m_diskStream = ThreadPool::instance().enqueue(&SampleStream::runDiskStream, this);
}

void SampleStream::stop()
{
    if (!m_diskStream.valid()) { return; }

    m_quit = true;
	m_diskStream.wait();
}

auto SampleStream::read(SampleFrame* dst, std::size_t size) -> std::size_t
{
    std::fill_n(dst, size, SampleFrame{});

    const auto framesAvailable = (m_writeIndex + m_buffer.size() - m_readIndex) % m_buffer.size();
	if (framesAvailable == 0) { return 0; }

	const auto readSize = std::min(framesAvailable, size);

    if (m_readIndex + readSize > m_buffer.size())
    {
        const auto readToEndSize = std::min(readSize, m_buffer.size() - m_readIndex);
        const auto wrapAroundSize = readSize - readToEndSize;
        std::copy_n(m_buffer.begin() + m_readIndex, readToEndSize, dst);
        std::copy_n(m_buffer.begin(), wrapAroundSize, dst + readToEndSize);
    }
    else
    {
        std::copy_n(m_buffer.begin() + m_readIndex, readSize, dst);
    }

    m_readIndex = (m_readIndex + readSize) % m_buffer.size();
    return readSize;
}

void SampleStream::runDiskStream()
{
    auto framesWritten = std::size_t{0};

	while (framesWritten < streamSize() && !m_quit)
	{
		const auto framesAvailable = (m_readIndex + m_buffer.size() - m_writeIndex - 1) % m_buffer.size();
		if (framesAvailable == 0) { continue; }

        const auto writeSize = std::min(framesAvailable, streamSize() - framesWritten);

        if (m_writeIndex + writeSize > m_buffer.size())
        {
            const auto writeToEndSize = std::min(writeSize, m_buffer.size() - m_writeIndex);
            const auto wrapAroundSize = writeSize - writeToEndSize;
            m_sampleFile.read(m_buffer.data() + m_writeIndex, writeToEndSize);
            m_sampleFile.read(m_buffer.data(), wrapAroundSize);
        }
        else
        {
            m_sampleFile.read(m_buffer.data() + m_writeIndex, writeSize);
        }

        m_writeIndex = (m_writeIndex + writeSize) % m_buffer.size();
        framesWritten += writeSize;
	}
}

} // namespace lmms