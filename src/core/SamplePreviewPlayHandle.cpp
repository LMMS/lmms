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
SamplePreviewPlayHandle::SamplePreviewPlayHandle(const std::filesystem::path& path)
	: PlayHandle(Type::SamplePreviewPlayHandle)
	, m_buffer(Engine::audioEngine()->outputSampleRate())
	, m_chunkSize(Engine::audioEngine()->framesPerPeriod())
{
#ifdef LMMS_BUILD_WIN32
	m_sndfile = sf_wchar_open(path.c_str(), SFM_READ, &m_sfinfo);
#else
	m_sndfile = sf_open(path.c_str(), SFM_READ, &m_sfinfo);
#endif

	if (m_sndfile == nullptr) { throw std::runtime_error{"Failed to create sample preview stream"}; }

	const auto preloadWriteCount
		= std::min(static_cast<sf_count_t>(samplesAvailableToWrite()), m_sfinfo.frames * m_sfinfo.channels);

	m_samplesWritten = sf_read_float(m_sndfile, &m_buffer[m_writeIndex], preloadWriteCount);
	m_writeIndex = (m_writeIndex + m_samplesWritten) % m_buffer.size();
	m_diskStream = ThreadPool::instance().enqueue(&SamplePreviewPlayHandle::runDiskStream, this);

	setAudioBusHandle(new AudioBusHandle("SamplePreviewPlayHandle", false));
}

SamplePreviewPlayHandle::~SamplePreviewPlayHandle() noexcept
{
	m_quit = true;
	m_diskStream.get();
	sf_close(m_sndfile);

	// TODO: Should be handled by base class
	delete audioBusHandle();
}

void SamplePreviewPlayHandle::play(SampleFrame* dst)
{
	const auto size = Engine::audioEngine()->framesPerPeriod();
	const auto samplesToRead = std::min(samplesAvailableToRead(), size * m_sfinfo.channels);

	if (samplesToRead == 0)
	{
		std::fill_n(dst, size, SampleFrame{});
		return;
	}

	const auto readIndex = m_readIndex.load(std::memory_order::acquire);
	for (auto i = std::size_t{0}; i < samplesToRead; i += m_sfinfo.channels)
	{
		const auto frame = i / m_sfinfo.channels;
		dst[frame][0] = m_buffer[(readIndex + i) % m_buffer.size()];
		dst[frame][1] = m_sfinfo.channels == 1 ? dst[frame][0] : m_buffer[(readIndex + i + 1) % m_buffer.size()];
	}

	m_readIndex.store((readIndex + samplesToRead) % m_buffer.size(), std::memory_order_release);
}

void SamplePreviewPlayHandle::runDiskStream()
{
	auto totalSamplesToWrite = static_cast<std::size_t>(m_sfinfo.frames * m_sfinfo.channels);
	while (m_samplesWritten < totalSamplesToWrite && !m_quit)
	{
		const auto samplesToWrite
			= std::min({samplesAvailableToWrite(), totalSamplesToWrite - m_samplesWritten, m_chunkSize});

		if (samplesToWrite == 0) { continue; }

		const auto writeIndex = m_writeIndex.load(std::memory_order_acquire);

		auto samplesWriteToEnd = std::min(m_buffer.size() - writeIndex, samplesToWrite);
		samplesWriteToEnd
			= sf_read_float(m_sndfile, &m_buffer[writeIndex], static_cast<sf_count_t>(samplesWriteToEnd));

		auto samplesWrapped = samplesToWrite - samplesWriteToEnd;
		samplesWrapped = sf_read_float(m_sndfile, &m_buffer[(writeIndex + samplesWriteToEnd) % m_buffer.size()],
			static_cast<sf_count_t>(samplesWrapped));

		const auto actualSamplesWrittten = samplesWriteToEnd + samplesWrapped;
		m_samplesWritten += actualSamplesWrittten;
		m_writeIndex.store((writeIndex + actualSamplesWrittten) % m_buffer.size(), std::memory_order_release);
	}
}
} // namespace lmms
