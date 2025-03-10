/*
 * SampleStream.h
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

#ifndef LMMS_SAMPLE_PREVIEW_PLAY_HANDLE_H
#define LMMS_SAMPLE_PREVIEW_PLAY_HANDLE_H

#include <filesystem>
#include <future>
#include <sndfile.h>
#include <vector>

#include "PlayHandle.h"

namespace lmms {

class SamplePreviewPlayHandle : public PlayHandle
{
public:
	SamplePreviewPlayHandle(const std::filesystem::path& path);

	SamplePreviewPlayHandle(const SamplePreviewPlayHandle&) = delete;
	SamplePreviewPlayHandle(SamplePreviewPlayHandle&&) = delete;

	SamplePreviewPlayHandle& operator=(const SamplePreviewPlayHandle&) = delete;
	SamplePreviewPlayHandle& operator=(SamplePreviewPlayHandle&&) = delete;

	~SamplePreviewPlayHandle() noexcept;

	void play(SampleFrame* dst) override;
	bool isFromTrack(const Track* _track) const override { return false; };
	bool isFinished() const override { return false; }

private:
	void runDiskStream();
	std::size_t samplesAvailableToRead() const { return (m_writeIndex - m_readIndex + m_buffer.size()) % m_buffer.size(); }
	std::size_t samplesAvailableToWrite() const { return m_buffer.size() - samplesAvailableToRead() - 1; }

	std::vector<float> m_buffer;

	std::atomic<std::size_t> m_readIndex = 0;
	std::atomic<std::size_t> m_writeIndex = 0;
	std::atomic<bool> m_quit = false;

	std::size_t m_samplesWritten = 0;

	SNDFILE* m_sndfile = nullptr;
	SF_INFO m_sfinfo = SF_INFO{};

	std::future<void> m_diskStream;

	std::size_t m_chunkSize = DefaultChunkSize;
	static constexpr auto DefaultChunkSize = std::size_t{256};
};

} // namespace lmms

#endif // LMMS_SAMPLE_PREVIEW_PLAY_HANDLE_H