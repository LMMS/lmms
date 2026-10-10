/*
 * RingBuffer.h - an effective and flexible implementation of a ringbuffer for LMMS
 *
 * Copyright (c) 2014 Vesa Kivimäki
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_RING_BUFFER_H
#define LMMS_RING_BUFFER_H

#include <cmath>
#include <concepts>
#include <QObject>

#include "AudioBuffer.h"
#include "LmmsTypes.h"
#include "lmms_export.h"

namespace lmms
{

//! @brief A basic LMMS ring buffer for single-thread use.
//!
//! For thread and realtime safe alternative see LocklessRingBuffer.
class LMMS_EXPORT RingBuffer : public QObject
{
	Q_OBJECT
public:
	//! @brief Constructs a ringbuffer of specified size, will not care about samplerate changes
	//! @param frames The size of the buffer in frames. The actual size will be frames + period size
	//! @param channels The number of planar channels in the buffer
	explicit RingBuffer(f_cnt_t frames, ch_cnt_t channels = DEFAULT_CHANNELS);

	//! @brief Constructs a ringbuffer of specified samplerate-dependent size, which will be updated when samplerate changes
	//! @param milliseconds The size of the buffer in milliseconds. The actual size will be size + period size
	//! @param channels The number of planar channels in the buffer
	explicit RingBuffer(float milliseconds, ch_cnt_t channels = DEFAULT_CHANNELS);

	~RingBuffer() override = default;

////////////////////////////////////
//       Provided functions       //
////////////////////////////////////

// utility functions

	//! @brief Clears the ringbuffer of any data and resets the position to 0
	void reset();

	//! @brief Changes the size of the ringbuffer. Clears all data.
	//! @param frames New size in frames
	void changeSize(f_cnt_t frames);

	//! @brief Changes the size of the ringbuffer. Clears all data.
	//! @param milliseconds New size in milliseconds
	void changeSize(float milliseconds);

	//! @brief Sets whether the ringbuffer size is adjusted for samplerate when samplerate changes
	//! @param b True if samplerate should affect buffer size
	void setSamplerateAware(bool b);

// position adjustment functions

	//! @brief Advances the position by one period
	void advance();

	//! @brief Moves position forwards/backwards by an amount of frames
	//! @param frames Number of frames to move, may be negative
	void movePosition(int frames);

	//! @brief Moves position forwards/backwards by an amount of milliseconds
	//! @param milliseconds Number of milliseconds to move, may be negative
	void movePosition(float milliseconds);

// read functions

	//! @brief Destructively reads from the current position, writes it
	//! to a specified destination, and advances the position by one period
	//! @param dst Destination span and read size
	void pop(PlanarBufferSpan<float> dst);

	//! @brief Reads from the ringbuffer and writes it to a specified destination
	//! @param dst Destination span and read size
	//! @param frames Offset in frames against current position
	void read(PlanarBufferSpan<float> dst, f_cnt_t frames = 0) const;

	//! @brief Reads a period-sized buffer from the ringbuffer and writes it to a specified destination
	//! @param dst Destination pointer and read size
	//! @param milliseconds Offset in milliseconds against current position
	void read(PlanarBufferSpan<float> dst, float milliseconds = 0) const;


// write functions

	//! @brief Reads from @p src and writes to the ringbuffer using the function @p func
	//! @param src Source buffer; also specifies the write amount
	//! @param dstOffset The write offset in frames
	//! @param func A function accepting the parameters
	//!             (PlanarBufferSpan<float> d, PlanarBufferSpan<const float> s)
	//!             which writes the @a s buffer to @a d in whatever way desired.
	template<std::invocable<PlanarBufferSpan<float>, PlanarBufferSpan<const float>> Func>
	void write(PlanarBufferSpan<const float> src, f_cnt_t dstOffset, Func&& func)
	{
		const auto writePosition = (m_position + dstOffset) % m_buf.frames();
		const auto writeAmount = src.frames();

		if (writePosition + writeAmount <= m_buf.frames())
		{
			func(PlanarBufferSpan{m_buf.allBuffers(), writePosition}, src.first(writeAmount));
		}
		else
		{
			const auto first = m_buf.frames() - writePosition;
			const auto second = writeAmount - first;

			func(PlanarBufferSpan{m_buf.allBuffers(), writePosition}, src.first(first));
			func(PlanarBufferSpan{m_buf.allBuffers()}, src.subspan(first, second));
		}
	}

	//! @brief Reads from @p src and writes to the ringbuffer using the function @p func
	//! @param src Source buffer; also specifies the write amount
	//! @param dstOffsetMs The write offset in milliseconds
	//! @param func A function accepting the parameters
	//!             (PlanarBufferSpan<float> d, PlanarBufferSpan<const float> s)
	//!             which writes the @a s buffer to @a d in whatever way desired.
	template<std::invocable<PlanarBufferSpan<float>, PlanarBufferSpan<const float>> Func>
	void write(PlanarBufferSpan<const float> src, float dstOffsetMs, Func&& func)
	{
		write(src, static_cast<f_cnt_t>(msToFrames(dstOffsetMs)), std::forward<Func>(func));
	}

	//! @brief Reads from @p src and writes to the ringbuffer using the function @p func
	//! @param src Source buffer; also specifies the write amount
	//! @param func A function accepting the parameters
	//!             (PlanarBufferSpan<float> d, PlanarBufferSpan<const float> s)
	//!             which writes the @a s buffer to @a d in whatever way desired.
	template<std::invocable<PlanarBufferSpan<float>, PlanarBufferSpan<const float>> Func>
	void write(PlanarBufferSpan<const float> src, Func&& func)
	{
		write(src, static_cast<f_cnt_t>(0), std::forward<Func>(func));
	}

protected slots:
	void updateSamplerate();

private:
	int msToFrames(float ms) const
	{
		return static_cast<int>(std::ceil(ms * m_samplerate * 0.001f));
	}

	const f_cnt_t m_fpp;
	sample_rate_t m_samplerate;
	AudioBuffer m_buf;

	volatile unsigned int m_position;
};


} // namespace lmms

#endif // LMMS_RING_BUFFER_H
