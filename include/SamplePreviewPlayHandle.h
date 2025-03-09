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
	SamplePreviewPlayHandle(const std::filesystem::path& path, std::size_t size = DefaultStreamCapacity);

	SamplePreviewPlayHandle(const SamplePreviewPlayHandle&) = delete;
	SamplePreviewPlayHandle(SamplePreviewPlayHandle&&) = delete;

	SamplePreviewPlayHandle& operator=(const SamplePreviewPlayHandle&) = delete;
	SamplePreviewPlayHandle& operator=(SamplePreviewPlayHandle&&) = delete;

	~SamplePreviewPlayHandle() noexcept;

	void play(SampleFrame* dst) override;
	bool isFromTrack(const Track* _track) const override { return false; };
	bool isFinished() const override { return !m_diskStream.valid(); }

private:
	void runDiskStream();
	float* bufferAt(std::size_t frameIndex) { return &m_buffer[frameIndex * DEFAULT_CHANNELS]; }
	std::size_t numFrames() const { return m_buffer.size() / DEFAULT_CHANNELS; }

	SNDFILE* m_sndfile = nullptr;
	std::vector<float> m_buffer;
	std::atomic<bool> m_quit = false;
	std::atomic<std::size_t> m_readFrameIndex = 0;
	std::atomic<std::size_t> m_writeFrameIndex = 0;
	SF_INFO m_sfinfo = SF_INFO{};
	std::future<void> m_diskStream;
	static constexpr auto DefaultStreamCapacity = 8192;
};

} // namespace lmms

#endif // LMMS_SAMPLE_PREVIEW_PLAY_HANDLE_H