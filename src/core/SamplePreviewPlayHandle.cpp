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

void SamplePreviewPlayHandle::play(SampleFrame* buffer)
{
	const auto size = Engine::audioEngine()->framesPerPeriod();
	std::fill_n(buffer, size, SampleFrame{});

	const auto framesAvailable = (m_writeIndex + m_buffer.size() - m_readIndex) % m_buffer.size();
	const auto readSize = std::min(framesAvailable, size);
	if (readSize == 0) { return; }

	if (m_readIndex + readSize > m_buffer.size())
	{
		const auto readToEndSize = std::min(readSize, m_buffer.size() - m_readIndex);
		const auto wrapAroundSize = readSize - readToEndSize;
		mixCopy(buffer, m_buffer.data() + static_cast<int>(m_readIndex), readToEndSize, m_sfinfo.channels);
		mixCopy(buffer, m_buffer.data(), wrapAroundSize, m_sfinfo.channels);
	}
	else { mixCopy(buffer, m_buffer.data() + static_cast<int>(m_readIndex), readSize, m_sfinfo.channels); }

	m_readIndex = (m_readIndex + readSize) % m_buffer.size();
}

void SamplePreviewPlayHandle::runDiskStream()
{
	auto framesWritten = std::size_t{0};
	while (framesWritten < static_cast<std::size_t>(m_sfinfo.frames) && !m_quit)
	{
		const auto framesAvailable = (m_readIndex + m_buffer.size() - m_writeIndex - 1) % m_buffer.size();
		const auto writeSize = std::min(framesAvailable, m_buffer.size() - framesWritten);
		if (writeSize == 0) { continue; }

		if (m_writeIndex + writeSize > m_buffer.size())
		{
			const auto writeToEndSize = std::min(writeSize, m_buffer.size() - m_writeIndex);
			const auto wrapAroundSize = writeSize - writeToEndSize;
			sf_readf_float(m_sndfile, m_buffer.data() + m_writeIndex, static_cast<sf_count_t>(writeToEndSize));
			sf_readf_float(m_sndfile, m_buffer.data(), static_cast<sf_count_t>(wrapAroundSize));
		}
		else { sf_readf_float(m_sndfile, m_buffer.data() + m_writeIndex, static_cast<sf_count_t>(writeSize)); }

		m_writeIndex = (m_writeIndex + writeSize) % m_buffer.size();
		framesWritten += writeSize;
	}
}

void SamplePreviewPlayHandle::mixCopy(SampleFrame* dst, const float* src, std::size_t frames, int channels)
{
	for (auto i = std::size_t{0}; i < frames; ++i)
	{
		dst[i][0] = src[i];
		dst[i][1] = m_sfinfo.channels == 1 ? src[i] : src[i + 1];
	}
}

} // namespace lmms