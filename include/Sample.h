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

#include <cmath>
#include <memory>

#include "Note.h"
#include "SampleBuffer.h"
#include "lmms_export.h"

#ifdef __MINGW32__
#include <mingw.mutex.h>
#include <mingw.shared_mutex.h>
#else
#include <mutex>
#include <shared_mutex>
#endif

class QPainter;
class QRect;

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
		PlaybackState(bool varyingPitch = false, int mode = SRC_LINEAR);
		~PlaybackState() noexcept;

		auto frameIndex() const -> f_cnt_t;
		auto varyingPitch() const -> bool;
		auto isBackwards() const -> bool;
		auto interpolationMode() const -> int;

		auto setFrameIndex(f_cnt_t index) -> void;
		auto setVaryingPitch(bool varyingPitch) -> void;
		auto setBackwards(bool backwards) -> void;

	private:
		f_cnt_t m_frameIndex = 0;
		bool m_varyingPitch = false;
		bool m_backwards = false;
		SRC_STATE* m_resampleState = nullptr;
		int m_interpolationMode = SRC_LINEAR;
		friend class Sample;
	};

	Sample() = default;
	Sample(const QString& audioFile);
	Sample(const QByteArray& base64, int sampleRate = Engine::audioEngine()->processingSampleRate());
	Sample(const sampleFrame* data, int numFrames, int sampleRate = Engine::audioEngine()->processingSampleRate());
	Sample(std::shared_ptr<const SampleBuffer> buffer);
	Sample(const Sample& other);
	Sample(Sample&& other) noexcept;

	Sample& operator=(Sample other) noexcept;
	friend auto swap(Sample& first, Sample& second) -> void;

	auto play(sampleFrame* dst, PlaybackState* state, int numFrames, float desiredFrequency = DefaultBaseFreq,
		Loop loopMode = Loop::Off) const -> bool;
	auto playbackSize() const -> int;
	auto visualize(QPainter& p, const QRect& dr, int fromFrame = 0, int toFrame = 0) const -> void;

	auto sampleDuration() const -> int;
	auto sampleFile() const -> const QString&;
	auto sampleRate() const -> int;
	auto sampleSize() const -> int;

	auto toBase64() const -> QString;

	auto buffer() const -> std::shared_ptr<const SampleBuffer>;
	auto startFrame() const -> int;
	auto endFrame() const -> int;
	auto loopStartFrame() const -> int;
	auto loopEndFrame() const -> int;
	auto amplification() const -> float;
	auto frequency() const -> float;
	auto reversed() const -> bool;

	auto setStartFrame(int startFrame) -> void;
	auto setEndFrame(int endFrame) -> void;
	auto setLoopStartFrame(int loopStartFrame) -> void;
	auto setLoopEndFrame(int loopEndFrame) -> void;
	auto setAllPointFrames(int startFrame, int endFrame, int loopStartFrame, int loopEndFrame) -> void;
	auto setAmplification(float amplification) -> void;
	auto setFrequency(float frequency) -> void;
	auto setReversed(bool reversed) -> void;

private:
	auto playSampleRange(PlaybackState* state, sampleFrame* dst, size_t numFrames) const -> void;
	auto resampleSampleRange(SRC_STATE* state, sampleFrame* src, sampleFrame* dst, size_t numInputFrames,
		size_t numOutputFrames, double ratio) const -> SRC_DATA;
	auto amplifySampleRange(sampleFrame* src, int numFrames) const -> void;
	auto copyBufferForward(sampleFrame* dst, int initialPosition, int advanceAmount) const -> void;
	auto copyBufferBackward(sampleFrame* dst, int initialPosition, int advanceAmount) const -> void;

private:
	std::shared_ptr<const SampleBuffer> m_buffer = std::make_shared<SampleBuffer>();
	int m_startFrame = 0;
	int m_endFrame = 0;
	int m_loopStartFrame = 0;
	int m_loopEndFrame = 0;
	float m_amplification = 1.0f;
	float m_frequency = DefaultBaseFreq;
	bool m_reversed = false;
	mutable std::shared_mutex m_mutex;
};
} // namespace lmms
#endif
