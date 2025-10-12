/*
 * RingBuffer.h - an effective and flexible implementation of a ringbuffer for LMMS
 *
 * Copyright (c) 2014 Vesa Kivimäki
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

#ifndef LMMS_RING_BUFFER_H
#define LMMS_RING_BUFFER_H

#include <cmath>
#include <QObject>
#include "LmmsTypes.h"
#include "lmms_export.h"


namespace lmms
{

class SampleFrame;

/** \brief A basic LMMS ring buffer for single-thread use. For thread and realtime safe alternative see LocklessRingBuffer.
*/
class LMMS_EXPORT RingBuffer : public QObject
{
	Q_OBJECT
public:
/** \brief Constructs a ringbuffer of specified size, will not care about samplerate changes
 * 	\param size The size of the buffer in frames. The actual size will be size + period size
 */
	RingBuffer( f_cnt_t size );

/** \brief Constructs a ringbuffer of specified samplerate-dependent size, which will be updated when samplerate changes
 * 	\param size The size of the buffer in milliseconds. The actual size will be size + period size
 */
	RingBuffer( float size );
	~RingBuffer() override;




////////////////////////////////////
//       Provided functions       //
////////////////////////////////////

// utility functions

/** \brief Clears the ringbuffer of any data and resets the position to 0
 */
	void reset();

/** \brief Changes the size of the ringbuffer. Clears all data.
 * 	\param size New size in frames
 */
	void changeSize( f_cnt_t size );

/** \brief Changes the size of the ringbuffer. Clears all data.
 * 	\param size New size in milliseconds
 */
	void changeSize( float size );

/** \brief Sets whether the ringbuffer size is adjusted for samplerate when samplerate changes
 *	\param b True if samplerate should affect buffer size
 */
	void setSamplerateAware( bool b );


// position adjustment functions

/** \brief Advances the position by one period
 */
	void advance();

/** \brief Moves position forwards/backwards by an amount of frames
 * 	\param amount Number of frames to move, may be negative
 */
	void movePosition( f_cnt_t amount );

/** \brief Moves position forwards/backwards by an amount of milliseconds
 * 	\param amount Number of milliseconds to move, may be negative
 */
	void movePosition( float amount );


// read functions

/** \brief Destructively reads a period-sized buffer from the current position, writes it
 * 	to a specified destination, and advances the position by one period
 * 	\param dst Destination pointer
 */
	void pop( SampleFrame* dst );

// note: ringbuffer position is unaffected by all other read functions beside pop()

/** \brief Reads a period-sized buffer from the ringbuffer and writes it to a specified destination
 * 	\param dst Destination pointer
 * 	\param offset Offset in frames against current position, may be negative
 */
	void read( SampleFrame* dst, f_cnt_t offset = 0 );

/** \brief Reads a period-sized buffer from the ringbuffer and writes it to a specified destination
 * 	\param dst Destination pointer
 * 	\param offset Offset in milliseconds against current position, may be negative
 */
	void read( SampleFrame* dst, float offset );

/** \brief Reads a buffer of specified size from the ringbuffer and writes it to a specified destination
 * 	\param dst Destination pointer
 * 	\param offset Offset in frames against current position, may be negative
 * 	\param length Length in frames of the buffer to read - must not be higher than the size of the ringbuffer!
 */
	void read( SampleFrame* dst, f_cnt_t offset, f_cnt_t length );

/** \brief Reads a buffer of specified size from the ringbuffer and writes it to a specified destination
 * 	\param dst Destination pointer
 * 	\param offset Offset in milliseconds against current position, may be negative
 * 	\param length Length in frames of the buffer to read - must not be higher than the size of the ringbuffer!
 */
	void read( SampleFrame* dst, float offset, f_cnt_t length );


// write functions

/** \brief Writes a buffer of sampleframes to the ringbuffer at specified position
 * 	\param src Pointer to the source buffer
 * 	\param offset Offset in frames against current position, may *NOT* be negative
 * 	\param length Length of the source buffer, if zero, period size is used - must not be higher than the size of the ringbuffer!
 */
	void write( SampleFrame* src, f_cnt_t offset=0, f_cnt_t length=0 );

/** \brief Writes a buffer of sampleframes to the ringbuffer at specified position
 * 	\param src Pointer to the source buffer
 * 	\param offset Offset in milliseconds against current position, may *NOT* be negative
 * 	\param length Length of the source buffer, if zero, period size is used - must not be higher than the size of the ringbuffer!
 */
	void write( SampleFrame* src, float offset, f_cnt_t length=0 );

/** \brief Mixes a buffer of sampleframes additively to the ringbuffer at specified position
 * 	\param src Pointer to the source buffer
 * 	\param offset Offset in frames against current position, may *NOT* be negative
 * 	\param length Length of the source buffer, if zero, period size is used - must not be higher than the size of the ringbuffer!
 */
	void writeAdding( SampleFrame* src, f_cnt_t offset=0, f_cnt_t length=0 );

/** \brief Mixes a buffer of sampleframes additively to the ringbuffer at specified position
 * 	\param src Pointer to the source buffer
 * 	\param offset Offset in milliseconds against current position, may *NOT* be negative
 * 	\param length Length of the source buffer, if zero, period size is used - must not be higher than the size of the ringbuffer!
 */
	void writeAdding( SampleFrame* src, float offset, f_cnt_t length=0 );

/** \brief Mixes a buffer of sampleframes additively to the ringbuffer at specified position, with
 * 	a specified multiplier applied to the frames
 * 	\param	src Pointer to the source buffer
 * 	\param offset Offset in frames against current position, may *NOT* be negative
 * 	\param length Length of the source buffer, if zero, period size is used - must not be higher than the size of the ringbuffer!
 * 	\param level Multiplier applied to the frames before they're written to the ringbuffer
 */
	void writeAddingMultiplied( SampleFrame* src, f_cnt_t offset, f_cnt_t length, float level );

/** \brief Mixes a buffer of sampleframes additively to the ringbuffer at specified position, with
 * 	a specified multiplier applied to the frames
 * 	\param	src Pointer to the source buffer
 * 	\param offset Offset in milliseconds against current position, may *NOT* be negative
 * 	\param length Length of the source buffer, if zero, period size is used
 * 	\param level Multiplier applied to the frames before they're written to the ringbuffer
 */
	void writeAddingMultiplied( SampleFrame* src, float offset, f_cnt_t length, float level );

/** \brief Mixes a buffer of sampleframes additively to the ringbuffer at specified position, with
 * 	a specified multiplier applied to the frames, with swapped channels
 * 	\param	src Pointer to the source buffer
 * 	\param offset Offset in frames against current position, may *NOT* be negative
 * 	\param length Length of the source buffer, if zero, period size is used - must not be higher than the size of the ringbuffer!
 * 	\param level Multiplier applied to the frames before they're written to the ringbuffer
 */
	void writeSwappedAddingMultiplied( SampleFrame* src, f_cnt_t offset, f_cnt_t length, float level );

/** \brief Mixes a buffer of sampleframes additively to the ringbuffer at specified position, with
 * 	a specified multiplier applied to the frames, with swapped channels
 * 	\param	src Pointer to the source buffer
 * 	\param offset Offset in milliseconds against current position, may *NOT* be negative
 * 	\param length Length of the source buffer, if zero, period size is used
 * 	\param level Multiplier applied to the frames before they're written to the ringbuffer
 */
	void writeSwappedAddingMultiplied( SampleFrame* src, float offset, f_cnt_t length, float level );


protected slots:
	void updateSamplerate();

private:
	inline f_cnt_t msToFrames( float ms )
	{
		return static_cast<f_cnt_t>( ceilf( ms * (float)m_samplerate * 0.001f ) );
	}

	const fpp_t m_fpp;
	sample_rate_t m_samplerate;
	size_t m_size;
	SampleFrame* m_buffer;
	volatile unsigned int m_position;

};


} // namespace lmms

#endif // LMMS_RING_BUFFER_H
