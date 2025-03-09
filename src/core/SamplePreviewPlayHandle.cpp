/*
 * SamplePreviewPlayHandle.cpp
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

#include "SamplePreviewPlayHandle.h"

#include "AudioBusHandle.h"
#include "AudioEngine.h"
#include "Engine.h"
#include "SampleFrame.h"
#include "ThreadPool.h"

namespace lmms {
SamplePreviewPlayHandle::SamplePreviewPlayHandle(const std::filesystem::path& path, std::size_t size)
	: PlayHandle(Type::SamplePreviewPlayHandle)
	, m_buffer(size * DEFAULT_CHANNELS)
{
#ifdef LMMS_BUILD_WIN32
	m_sndfile = sf_wchar_open(path.c_str(), SFM_READ, &m_sfinfo);
#else
	m_sndfile = sf_open(path.c_str(), SFM_READ, &m_sfinfo);
#endif

	if (m_sndfile == nullptr) { throw std::runtime_error{"Failed to create sample preview stream"}; }
	m_diskStream = ThreadPool::instance().enqueue(&SamplePreviewPlayHandle::runDiskStream, this);
	setAudioBusHandle(new AudioBusHandle("SamplePreviewPlayHandle", false));
}

SamplePreviewPlayHandle::~SamplePreviewPlayHandle() noexcept
{
	m_quit = true;
	m_diskStream.wait();
	sf_close(m_sndfile);

	// TODO: Should be handled by base class
	delete audioBusHandle();
}

void SamplePreviewPlayHandle::play(SampleFrame* dst)
{
	const auto size = Engine::audioEngine()->framesPerPeriod();
	const auto framesAvailable = (m_writeFrameIndex + numFrames() - m_readFrameIndex) % numFrames();
	const auto framesToRead = std::min(framesAvailable, size);

	if (framesToRead == 0)
	{
		std::fill_n(dst, size, SampleFrame{});
		return;
	}

	const auto readIndex = m_readFrameIndex.load();
	for (auto i = std::size_t{0}; i < framesToRead; ++i)
	{
		const auto buffer = bufferAt((readIndex + i) % numFrames());
		dst[i][0] = buffer[0];
		dst[i][1] = m_sfinfo.channels == 1 ? buffer[0] : buffer[1];
	}

	std::fill_n(dst + framesToRead, size - framesToRead, SampleFrame{});
	m_readFrameIndex = (m_readFrameIndex + framesToRead) % numFrames();

}

void SamplePreviewPlayHandle::runDiskStream()
{
	auto framesWritten = std::size_t{0};
	const auto framesToWrite = static_cast<std::size_t>(m_sfinfo.frames);

	while (framesWritten < framesToWrite && !m_quit)
	{
		const auto framesAvailable = (m_readFrameIndex + numFrames() - m_writeFrameIndex - 1) % numFrames();
		const auto writeSize = std::min(framesAvailable, framesToWrite - framesWritten);
		if (writeSize == 0) { continue; }

		const auto writeToEndSize = std::min(writeSize, numFrames() - m_writeFrameIndex);
		sf_readf_float(m_sndfile, bufferAt(m_writeFrameIndex), static_cast<sf_count_t>(writeToEndSize));

		const auto wrapAroundSize = writeSize - writeToEndSize;
		sf_readf_float(m_sndfile, bufferAt(0), static_cast<sf_count_t>(wrapAroundSize));

		framesWritten += writeSize;
		m_writeFrameIndex = (m_writeFrameIndex + writeSize) % numFrames();
	}
}
} // namespace lmms
