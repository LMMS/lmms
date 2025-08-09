/*
 * Sample.h - State for container-class SampleBuffer
 *
 * Copyright (c) 2023 saker <sakertooth@gmail.com>
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

#ifndef LMMS_SAMPLE_H
#define LMMS_SAMPLE_H

#include <memory>

#include "AudioResampler.h"
#include "Note.h"
#include "SampleBuffer.h"
#include "lmms_export.h"

namespace lmms {
class LMMS_EXPORT Sample
{
public:
	// values for buffer margins, used for various libsamplerate interpolation modes
	// the array positions correspond to the converter_type parameter values in libsamplerate
	// if there appears problems with playback on some interpolation mode, then the value for that mode
	// may need to be higher - conversely, to optimize, some may work with lower values
	static constexpr auto s_interpolationMargins = std::array<int, 5>{64, 64, 64, 4, 4};

	enum class Loop
	{
		Off,
		On,
		PingPong
	};

	class LMMS_EXPORT PlaybackState
	{
	public:
		PlaybackState(bool varyingPitch = false, int interpolationMode = SRC_LINEAR)
			: m_resampler(interpolationMode, DEFAULT_CHANNELS)
			, m_varyingPitch(varyingPitch)
		{
		}

		auto resampler() -> AudioResampler& { return m_resampler; }
		auto frameIndex() const -> int { return m_frameIndex; }
		auto varyingPitch() const -> bool { return m_varyingPitch; }
		auto backwards() const -> bool { return m_backwards; }

		void setFrameIndex(int frameIndex) { m_frameIndex = frameIndex; }
		void setVaryingPitch(bool varyingPitch) { m_varyingPitch = varyingPitch; }
		void setBackwards(bool backwards) { m_backwards = backwards; }

	private:
		AudioResampler m_resampler;
		int m_frameIndex = 0;
		bool m_varyingPitch = false;
		bool m_backwards = false;
		friend class Sample;
	};

	Sample() = default;
	Sample(const QByteArray& base64, int sampleRate = Engine::audioEngine()->outputSampleRate());
	Sample(const SampleFrame* data, size_t numFrames, int sampleRate = Engine::audioEngine()->outputSampleRate());
	Sample(const Sample& other);
	Sample(Sample&& other);
	explicit Sample(const QString& audioFile);
	explicit Sample(std::shared_ptr<const SampleBuffer> buffer);

	auto operator=(const Sample&) -> Sample&;
	auto operator=(Sample&&) -> Sample&;

	auto play(SampleFrame* dst, PlaybackState* state, size_t numFrames, float desiredFrequency = DefaultBaseFreq,
		Loop loopMode = Loop::Off) const -> bool;

	auto sampleDuration() const -> std::chrono::milliseconds;
	auto sampleFile() const -> const QString& { return m_buffer->audioFile(); }
	auto sampleRate() const -> int { return m_buffer->sampleRate(); }
	auto sampleSize() const -> size_t { return m_buffer->size(); }

	auto toBase64() const -> QString { return m_buffer->toBase64(); }

	auto data() const -> const SampleFrame* { return m_buffer->data(); }
	auto buffer() const -> std::shared_ptr<const SampleBuffer> { return m_buffer; }
	auto startFrame() const -> int { return m_startFrame.load(std::memory_order_relaxed); }
	auto endFrame() const -> int { return m_endFrame.load(std::memory_order_relaxed); }
	auto loopStartFrame() const -> int { return m_loopStartFrame.load(std::memory_order_relaxed); }
	auto loopEndFrame() const -> int { return m_loopEndFrame.load(std::memory_order_relaxed); }
	auto amplification() const -> float { return m_amplification.load(std::memory_order_relaxed); }
	auto frequency() const -> float { return m_frequency.load(std::memory_order_relaxed); }
	auto reversed() const -> bool { return m_reversed.load(std::memory_order_relaxed); }

	void setStartFrame(int startFrame) { m_startFrame.store(startFrame, std::memory_order_relaxed); }
	void setEndFrame(int endFrame) { m_endFrame.store(endFrame, std::memory_order_relaxed); }
	void setLoopStartFrame(int loopStartFrame) { m_loopStartFrame.store(loopStartFrame, std::memory_order_relaxed); }
	void setLoopEndFrame(int loopEndFrame) { m_loopEndFrame.store(loopEndFrame, std::memory_order_relaxed); }
	void setAllPointFrames(int startFrame, int endFrame, int loopStartFrame, int loopEndFrame);
	void setAmplification(float amplification) { m_amplification.store(amplification, std::memory_order_relaxed); }
	void setFrequency(float frequency) { m_frequency.store(frequency, std::memory_order_relaxed); }
	void setReversed(bool reversed) { m_reversed.store(reversed, std::memory_order_relaxed); }

private:
	void playRaw(SampleFrame* dst, size_t numFrames, const PlaybackState* state, Loop loopMode) const;
	void advance(PlaybackState* state, size_t advanceAmount, Loop loopMode) const;

private:
	std::shared_ptr<const SampleBuffer> m_buffer = SampleBuffer::emptyBuffer();
	std::atomic<int> m_startFrame = 0;
	std::atomic<int> m_endFrame = 0;
	std::atomic<int> m_loopStartFrame = 0;
	std::atomic<int> m_loopEndFrame = 0;
	std::atomic<float> m_amplification = 1.0f;
	std::atomic<float> m_frequency = DefaultBaseFreq;
	std::atomic<bool> m_reversed = false;
};
} // namespace lmms
#endif
