/*
 * AudioFilePlayHandle.h
 *
 * Copyright (c) 2025 saker <sakertooth@gmail.com>
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

#ifndef LMMS_AUDIO_FILE_PLAY_HANDLE_H
#define LMMS_AUDIO_FILE_PLAY_HANDLE_H

#include <filesystem>

#include "AudioFile.h"
#include "AudioResampler.h"
#include "PlayHandle.h"
#include "SampleFrame.h"

namespace lmms {

class AudioFilePlayHandle : public PlayHandle
{
public:
	AudioFilePlayHandle(const std::filesystem::path& path, int interpolationMode = SRC_LINEAR);

	AudioFilePlayHandle(const AudioFilePlayHandle&) = delete;
	AudioFilePlayHandle(AudioFilePlayHandle&&) = delete;

	AudioFilePlayHandle& operator=(const AudioFilePlayHandle&) = delete;
	AudioFilePlayHandle& operator=(AudioFilePlayHandle&&) = delete;

	~AudioFilePlayHandle();

	void play(SampleFrame* dst) override;
	f_cnt_t seek(f_cnt_t frames, AudioFile::Whence whence);

	bool isFinished() const override;
	bool isFromTrack(const Track* track) const override { return false; }
	bool affinityMatters() const override { return true; }

private:
	static constexpr auto FramesPerBuffer = 32;
	AudioFile m_audioFile;
	AudioResampler m_audioResampler;
	std::vector<float> m_sourceBuffer;
	std::vector<float> m_channelConvertBuffer;
	InterleavedBufferView<float, DEFAULT_CHANNELS> m_sourceBufferView;
	InterleavedBufferView<float, DEFAULT_CHANNELS> m_channelConvertBufferView;
	f_cnt_t m_framesRead = 0;
};

} // namespace lmms

#endif // LMMS_AUDIO_FILE_PLAY_HANDLE_H