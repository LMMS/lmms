/*
 * Sample.cpp
 *
 * Copyright (c) 2025 saker <sakertooth@gmail.com>
 * Copyright (c) 2026 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#include "Sample.h"

#include "MixHelpers.h"

namespace lmms {

Sample::Sample(PlanarBufferSpan<const float> data, int sampleRate)
	: m_buffer(std::make_shared<SampleBuffer>(data, SampleImportModification::Unmodified, sampleRate))
	, m_startFrame(0)
	, m_endFrame(m_buffer->frames())
	, m_loopStartFrame(0)
	, m_loopEndFrame(m_buffer->frames())
{
}

Sample::Sample(const SampleFrame* data, f_cnt_t numFrames, int sampleRate)
	: m_buffer(std::make_shared<SampleBuffer>(
		std::span{data, numFrames}, SampleImportModification::Unmodified, sampleRate))
	, m_startFrame(0)
	, m_endFrame(m_buffer->frames())
	, m_loopStartFrame(0)
	, m_loopEndFrame(m_buffer->frames())
{
}

Sample::Sample(std::shared_ptr<const SampleBuffer> buffer)
	: m_buffer(buffer)
	, m_startFrame(0)
	, m_endFrame(m_buffer->frames())
	, m_loopStartFrame(0)
	, m_loopEndFrame(m_buffer->frames())
{
}

Sample::Sample(const Sample& other)
	: m_buffer(other.m_buffer)
	, m_startFrame(other.startFrame())
	, m_endFrame(other.endFrame())
	, m_loopStartFrame(other.loopStartFrame())
	, m_loopEndFrame(other.loopEndFrame())
	, m_amplification(other.amplification())
	, m_frequency(other.frequency())
	, m_reversed(other.reversed())
{
}

Sample::Sample(Sample&& other) noexcept
	: m_buffer(std::move(other.m_buffer))
	, m_startFrame(other.startFrame())
	, m_endFrame(other.endFrame())
	, m_loopStartFrame(other.loopStartFrame())
	, m_loopEndFrame(other.loopEndFrame())
	, m_amplification(other.amplification())
	, m_frequency(other.frequency())
	, m_reversed(other.reversed())
{
}

auto Sample::operator=(const Sample& other) -> Sample&
{
	m_buffer = other.m_buffer;
	m_startFrame = other.startFrame();
	m_endFrame = other.endFrame();
	m_loopStartFrame = other.loopStartFrame();
	m_loopEndFrame = other.loopEndFrame();
	m_amplification = other.amplification();
	m_frequency = other.frequency();
	m_reversed = other.reversed();

	return *this;
}

auto Sample::operator=(Sample&& other) noexcept -> Sample&
{
	m_buffer = std::move(other.m_buffer);
	m_startFrame = other.startFrame();
	m_endFrame = other.endFrame();
	m_loopStartFrame = other.loopStartFrame();
	m_loopEndFrame = other.loopEndFrame();
	m_amplification = other.amplification();
	m_frequency = other.frequency();
	m_reversed = other.reversed();

	return *this;
}

auto Sample::play(PlanarBufferSpan<float> dst, PlaybackState* state,
	Loop loop, double ratio) const -> bool
{
	if (!m_buffer || m_buffer->empty()) { return false; }

	state->m_frameIndex = std::clamp<f_cnt_t>(state->m_frameIndex, m_startFrame, m_endFrame);

	const auto sampleRateRatio = static_cast<double>(Engine::audioEngine()->outputSampleRate()) / m_buffer->sampleRate();
	const auto freqRatio = frequency() / DefaultBaseFreq;
	state->m_resampler.setRatio(sampleRateRatio * freqRatio * ratio);

	// TODO: These kind of playback pipelines/graphs are repeated within other parts of the codebase that work with
	// audio samples. We should find a way to unify this but the right abstraction is not so clear yet.
	f_cnt_t numFrames = dst.frames();
	while (numFrames > 0)
	{
		if (state->m_bufferSpan.empty())
		{
			const auto rendered = render(state, loop);
			state->m_bufferSpan = PlanarBufferSpan{state->m_buffer.allBuffers().first(rendered)};
		}

		const auto [inputFramesUsed, outputFramesGenerated] = state->m_resampler.process(
			state->m_bufferSpan,
			dst
		);

		if (inputFramesUsed == 0 && outputFramesGenerated == 0)
		{
			MixHelpers::zero(dst);
			break;
		}

		state->m_bufferSpan = state->m_bufferSpan.subspan(inputFramesUsed);
		dst = dst.subspan(outputFramesGenerated);
		numFrames -= outputFramesGenerated;
	}

	return numFrames < Engine::audioEngine()->framesPerPeriod(); // TODO: Is this right?
}

f_cnt_t Sample::render(PlaybackState* state, Loop loop) const
{
	const auto dst = state->m_buffer.allBuffers();
	const auto src = m_buffer->data();

	assert(!src.empty());
	assert(src.channels() == dst.channels());
	assert(m_endFrame <= src.frames()); // ???
	assert(m_loopEndFrame <= src.frames()); // ???

	using CopyFunction = auto(*)(
		float* const*       dst, f_cnt_t dstBegin, f_cnt_t dstEnd,
		const float* const* src, f_cnt_t srcBegin, f_cnt_t srcEnd, f_cnt_t& srcReadPos,
		ch_cnt_t channels, float amp) -> f_cnt_t;

	constexpr CopyFunction copyForwardRead = +[](
		float* const*       dst, f_cnt_t dstBegin, f_cnt_t dstEnd,
		const float* const* src, f_cnt_t srcBegin, f_cnt_t srcEnd, f_cnt_t& srcReadPos,
		ch_cnt_t channels, float amp) -> f_cnt_t
	{
		if (srcBegin == srcEnd) { return 0; }

		assert(srcBegin <= srcReadPos);
		assert(srcReadPos <= srcEnd);

		const auto maxWriteAmount = dstEnd - dstBegin;
		const auto maxReadAmount = srcEnd - srcReadPos;
		const auto framesWritten = std::min(maxWriteAmount, maxReadAmount);

		for (ch_cnt_t ch = 0; ch < channels; ++ch)
		{
			float* const       dstPtr = dst[ch] + dstBegin;
			const float* const srcPtr = src[ch] + srcReadPos;
			for (f_cnt_t frame = 0; frame < framesWritten; ++frame)
			{
				dstPtr[frame] = srcPtr[frame] * amp;
			}
		}

		srcReadPos += framesWritten;

		return framesWritten;
	};

	constexpr CopyFunction copyBackwardRead = +[](
		float* const*       dst, f_cnt_t dstBegin, f_cnt_t dstEnd,
		const float* const* src, f_cnt_t srcBegin, f_cnt_t srcEnd, f_cnt_t& srcReadPos,
		ch_cnt_t channels, float amp) -> f_cnt_t
	{
		if (srcBegin == srcEnd) { return 0; }

		assert(srcBegin <= srcReadPos);
		assert(srcReadPos <= srcEnd);

		const auto maxWriteAmount = dstEnd - dstBegin;
		const auto maxReadAmount = srcReadPos - srcBegin + 1;
		const auto framesWritten = std::min(maxWriteAmount, maxReadAmount);

		for (ch_cnt_t ch = 0; ch < channels; ++ch)
		{
			float* const       dstPtr = dst[ch] + dstBegin;
			const float* const srcPtr = src[ch] + srcReadPos;
			for (f_cnt_t frame = 0; frame < framesWritten; ++frame)
			{
				dstPtr[frame] = *(srcPtr - frame) * amp;
			}
		}

		// NOTE: Since we're using unsigned frame counts, this may underflow to
		//       static_cast<f_cnt_t>(-1) for the reverse-past-the-end sentinel.
		srcReadPos -= framesWritten;

		return framesWritten;
	};

	// Call like this: copy[state->m_backwards](...)
	std::array<CopyFunction, 2> copy = m_reversed.load()
		? std::array{copyBackwardRead, copyForwardRead/* <-- might need further restrictions */}
		: std::array{copyForwardRead, copyBackwardRead};

	// std::minmax but without dangling references
	constexpr auto minmax = [](f_cnt_t a, f_cnt_t b) -> std::pair<f_cnt_t, f_cnt_t> {
		return (b < a) ? std::pair{b, a} : std::pair{a, b};
	};

	const auto dstFrames = dst.frames();
	const auto [startFrame, endFrame] = minmax(m_startFrame.load(), m_endFrame.load());
	const auto [loopStartFrame, loopEndFrame] = minmax(m_loopStartFrame.load(), m_loopEndFrame.load());

	switch (loop)
	{
		case Loop::Off:
		{
			const auto backwards = state->m_backwards;

			f_cnt_t srcStartFrame;
			f_cnt_t srcEndFrame;

			// Check if the sample reached the point where it would normally loop
			if (backwards)
			{
				if (state->m_frameIndex >= endFrame)
				{
					// Jump backwards to end (starting position when playing backward)
					state->m_frameIndex = endFrame - 1;
				}

				if (state->m_frameIndex < loopStartFrame || state->m_frameIndex == static_cast<f_cnt_t>(-1))
				{
					// The sample reached the end - stop playback
					return 0;
				}

				srcStartFrame = loopStartFrame;
				srcEndFrame = endFrame;
			}
			else
			{
				if (state->m_frameIndex < startFrame)
				{
					// Jump forwards to start
					state->m_frameIndex = startFrame;
				}

				if (state->m_frameIndex >= loopEndFrame)
				{
					// The sample reached the end - stop playback
					return 0;
				}

				srcStartFrame = startFrame;
				srcEndFrame = loopEndFrame;
			}

			return copy[backwards](
				dst.data(), 0, dstFrames,
				src.data(), srcStartFrame, srcEndFrame, state->m_frameIndex,
				dst.channels(), m_amplification
			);
		}
		case Loop::On:
		{
			const auto backwards = state->m_backwards;

			f_cnt_t framesWritten = 0;
			f_cnt_t framesLeft = dstFrames;
			while (framesLeft > 0)
			{
				f_cnt_t srcStartFrame;
				f_cnt_t srcEndFrame;

				// Check if the sample reached the point where it would loop
				if (backwards)
				{
					if (state->m_frameIndex >= endFrame)
					{
						// Jump backwards to end (starting position when playing backward)
						state->m_frameIndex = endFrame - 1;
					}

					if (state->m_frameIndex < loopStartFrame || state->m_frameIndex == static_cast<f_cnt_t>(-1))
					{
						// The sample reached the end - loop back to start (end position when playing backward)
						state->m_frameIndex = loopEndFrame - 1; // may underflow
					}

					srcStartFrame = loopStartFrame;
					srcEndFrame = endFrame;
				}
				else
				{
					if (state->m_frameIndex < startFrame)
					{
						// Jump forwards to start
						state->m_frameIndex = startFrame;
					}

					if (state->m_frameIndex >= loopEndFrame)
					{
						// The sample reached the end - loop back to start
						state->m_frameIndex = loopStartFrame;
					}

					srcStartFrame = startFrame;
					srcEndFrame = loopEndFrame;
				}

				// Copy contiguous section
				const auto written = copy[backwards](
					dst.data(), framesWritten, dstFrames,
					src.data(), srcStartFrame, srcEndFrame, state->m_frameIndex,
					dst.channels(), m_amplification
				);

				framesWritten += written;
				framesLeft -= written;
			}
			break;
		}
		case Loop::PingPong:
		{
			f_cnt_t framesWritten = 0;
			f_cnt_t framesLeft = dstFrames;
			while (framesLeft > 0)
			{
				f_cnt_t srcStartFrame;
				f_cnt_t srcEndFrame;

				// Loop ping-pong
				if (state->m_backwards)
				{
					if (state->m_frameIndex >= endFrame)
					{
						// Jump backwards to end (starting position when playing backward)
						state->m_frameIndex = endFrame - 1;
					}

					if (state->m_frameIndex < loopStartFrame || state->m_frameIndex == static_cast<f_cnt_t>(-1))
					{
						// The sample reached the end - reverse loop direction
						state->m_frameIndex = loopStartFrame;
						state->m_backwards = false;
					}

					srcStartFrame = loopStartFrame;
					srcEndFrame = endFrame;
				}
				else
				{
					if (state->m_frameIndex < startFrame)
					{
						// Jump forwards to start
						state->m_frameIndex = startFrame;
					}

					if (state->m_frameIndex >= loopEndFrame)
					{
						// The sample reached the end - reverse loop direction
						state->m_frameIndex = loopEndFrame - 1; // may underflow
						state->m_backwards = true;
					}

					srcStartFrame = startFrame;
					srcEndFrame = loopEndFrame;
				}

				// Copy contiguous section
				const auto written = copy[state->m_backwards](
					dst.data(), framesWritten, dstFrames,
					src.data(), srcStartFrame, srcEndFrame, state->m_frameIndex,
					dst.channels(), m_amplification
				);

				framesWritten += written;
				framesLeft -= written;
			}
			break;
		}
		default:
			assert(false);
			return 0;
	}

	return dst.frames();
}

auto Sample::sampleDuration() const -> std::chrono::milliseconds
{
	const auto numFrames = endFrame() - startFrame();
	const auto duration = numFrames / static_cast<float>(m_buffer->sampleRate()) * 1000;
	return std::chrono::milliseconds{static_cast<int>(duration)};
}

void Sample::setAllPointFrames(f_cnt_t startFrame, f_cnt_t endFrame, f_cnt_t loopStartFrame, f_cnt_t loopEndFrame)
{
	assert(startFrame <= endFrame);
	assert(loopStartFrame <= loopEndFrame);
	assert(startFrame <= loopStartFrame);
	assert(loopEndFrame <= endFrame);

	setStartFrame(startFrame);
	setEndFrame(endFrame);
	setLoopStartFrame(loopStartFrame);
	setLoopEndFrame(loopEndFrame);
}

} // namespace lmms
