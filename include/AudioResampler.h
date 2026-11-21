/*
 * AudioResampler.h
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

#ifndef LMMS_AUDIO_RESAMPLER_H
#define LMMS_AUDIO_RESAMPLER_H

#ifdef LMMS_DEBUG
#include <iostream>
#endif

#include <samplerate.h>

#include "AudioBufferView.h"
#include "AudioEngine.h"
#include "lmms_export.h"

namespace lmms {

template <ch_cnt_t Channels>
class LMMS_EXPORT AudioResampler
{
public:
	/**
	 * @brief Construct a resampler with the given @a converter.
	 * @param converter The kind of converter (SRC_LINER, SRC_SINC_MEDIUM, etc)
	 */
	explicit AudioResampler(int converter)
		: m_state{src_new(converter, Channels, &m_error)}
	{
		if (!m_state) { throw std::runtime_error{src_strerror(m_error)}; }
	}

	/**
	 * @brief Deletes the internal state for the resampler and destroys the object.
	 */
	~AudioResampler() { src_delete(m_state); }

	/**
	 * @brief Process audio through the resampler.
	 * If an error occurs, the function returns 0.
	 *
	 * @return f_cnt_t The number of output frames generated.
	 */
	auto process() -> f_cnt_t
	{
		if (const auto error = src_process(m_state, &m_data))
		{
#ifdef LMMS_DEBUG
			std::cerr << "[AudioResampler::process]: " << src_strerror(error) << '\n';
#endif
			return 0;
		}

		m_data.data_in += m_data.input_frames_used * Channels;
		m_data.input_frames -= m_data.input_frames_used;

		m_data.data_out += m_data.output_frames_gen * Channels;
		m_data.output_frames -= m_data.output_frames_gen;

		return m_data.output_frames_gen;
	}

	/**
	 * @brief Input callback variant of process()
	 *
	 * @tparam InputCallback The input callback used to refill the resampler with input data.
	 * @see process()
	 *
	 * @return f_cnt_t The number of output frames generated.
	 */
	template <typename InputCallback>
		requires(std::is_invocable_r_v<f_cnt_t, InputCallback, InterleavedBufferView<float, Channels>>)
	auto process(InputCallback&& callback) -> f_cnt_t
	{
		if (!m_data.end_of_input && m_data.input_frames == 0)
		{
			const auto view = InterleavedBufferView<float, Channels>{m_buffer.data(), m_buffer.size() / Channels};
			const auto frames = callback(view);
			m_data.data_in = view.data();
			m_data.input_frames = frames;
		}

		return process();
	}

	/**
	 * @brief Input callback variant of processUntilDone()
	 *
	 * @tparam InputCallback The input callback used to refill the resampler with input data.
	 * @see processUntilDone()
	 */
	template <typename InputCallback>
		requires(std::is_invocable_r_v<f_cnt_t, InputCallback, InterleavedBufferView<float, Channels>>)
	auto processUntilDone(InputCallback&& callback) -> f_cnt_t
	{
		auto outputFrames = 0;

		do
		{
			outputFrames += process(std::forward<InputCallback>(callback));
		} while (!done());

		return outputFrames;
	}

	/**
	 * @brief Processes audio through the resampler until it is done.
	 *
	 * @returns The total number of output frames generated.
	 * @see done()
	 */
	auto processUntilDone() -> f_cnt_t
	{
		auto outputFrames = 0;

		do
		{
			outputFrames += process();
		} while (!done());

		return outputFrames;
	}

	/**
	 * @brief Checks if the resampler is done.
	 * This is based on if the last call to process() has used up and outputted any data.
	 *
	 * @returns true if the resampler has not used up and outputted any data.
	 * @returns false otherwise.
	 */
	bool done() const { return m_data.input_frames_used == 0 && m_data.output_frames_gen == 0; }

	/**
	 * @brief Resets the internal state of the resampler. Should be used when working on separate, unrelated pieces of
	 * audio.
	 */
	void reset()
	{
		if (const auto error = src_reset(m_state))
		{
#ifdef LMMS_DEBUG
			std::cerr << "[AudioResampler::reset]: " << src_strerror(error) << '\n';
#endif
		}
	}

	/**
	 * @brief Sets the input buffer for the resampler.
	 * If @a final is set to @c true, future calls to this function will be ignored.
	 *
	 * @param input The input buffer.
	 * @param final @c true if @a input is the last input buffer to be processed by the resampler, @c false otherwise.
	 */
	void setInputBuffer(InterleavedBufferView<const float, Channels> input, bool final = true)
	{
		if (m_data.end_of_input) { return; }

		assert(m_data.input_frames == 0 && "input still available, risk of dropped input data");
		m_data.data_in = input.data();
		m_data.input_frames = input.frames();
		m_data.end_of_input = final;
	}

	/**
	 * @brief Sets the output buffer for the resampler.
	 * @param output The output buffer.
	 */
	void setOutputBuffer(InterleavedBufferView<float, Channels> output)
	{
		m_data.data_out = output.data();
		m_data.output_frames = output.frames();
	}

	/**
	 * @brief Sets the resampling ratio to @a ratio.
	 * @param ratio The resampling ratio.
	 */
	void setRatio(double ratio) { m_data.src_ratio = ratio; }

	/**
	 * @brief Set the resampling ratio to @a output / @a input.
	 * @param input The input sample rate.
	 * @param output The output sample rate.
	 */
	void setRatio(sample_rate_t input, sample_rate_t output) { m_data.src_ratio = static_cast<double>(output) / input; }

private:
	SRC_STATE* m_state{};
	SRC_DATA m_data{};
	std::array<float, DEFAULT_BUFFER_SIZE * Channels> m_buffer;
	int m_error{};
};

} // namespace lmms

#endif // LMMS_AUDIO_RESAMPLER_H
