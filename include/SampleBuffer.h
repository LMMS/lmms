/*
 * SampleBuffer.h - container-class SampleBuffer
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_SAMPLE_BUFFER_H
#define LMMS_SAMPLE_BUFFER_H

#include <QString>
#include <memory>
#include <vector>

#include "AudioBuffer.h"
#include "AudioEngine.h"
#include "Engine.h"
#include "LmmsTypes.h"
#include "lmms_export.h"

namespace lmms {
class LMMS_EXPORT SampleBuffer
{
public:
	SampleBuffer() = default;
	// TODO: Give `mod` parameters default values then simplify call sites
	SampleBuffer(AudioBuffer data,
		sample_rate_t sampleRate = Engine::audioEngine()->outputSampleRate(), const QString& audioFile = "");
	SampleBuffer(PlanarBufferSpan<const float> data,
		sample_rate_t sampleRate = Engine::audioEngine()->outputSampleRate(), const QString& audioFile = "");
	SampleBuffer(std::span<const SampleFrame> data,
		sample_rate_t sampleRate = Engine::audioEngine()->outputSampleRate(), const QString& audioFile = "");

	friend void swap(SampleBuffer& first, SampleBuffer& second) noexcept;
	auto toBase64() const -> QString;

	//! @returns a view providing channel-wise access to the SampleBuffer
	auto data() const -> PlanarBufferView<const float> { return m_data.allBuffers(); }

	auto audioFile() const -> const QString& { return m_audioFile; }
	auto sampleRate() const -> sample_rate_t { return m_sampleRate; }

	auto channels() const -> ch_cnt_t { return m_data.totalChannels(); }
	auto frames() const -> f_cnt_t { return m_data.frames(); }
	auto empty() const -> bool { return m_data.empty(); }

	static auto emptyBuffer() -> std::shared_ptr<const SampleBuffer>;

	//! Loads sample from file path
	static std::shared_ptr<const SampleBuffer> fromFile(const QString& path);

	//! Loads sample from planar base64 data
	static std::shared_ptr<const SampleBuffer> fromBase64(const QString& str,
		sample_rate_t sampleRate = Engine::audioEngine()->outputSampleRate());

	//! Loads sample from old `SampleFrame` base64 data
	static std::shared_ptr<const SampleBuffer> fromLegacyBase64(const QString& str,
		sample_rate_t sampleRate = Engine::audioEngine()->outputSampleRate());

private:
	static std::shared_ptr<const SampleBuffer> fromBase64(bool legacyInterleaved,
		const QString& str, sample_rate_t sampleRate);

	using B64FrameCount = std::uint64_t;
	using B64ChannelCount = std::uint16_t;

	AudioBuffer m_data;
	QString m_audioFile;
	sample_rate_t m_sampleRate = Engine::audioEngine()->outputSampleRate();
};

} // namespace lmms

#endif // LMMS_SAMPLE_BUFFER_H
