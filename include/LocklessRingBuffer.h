/*
 * LocklessRingBuffer.h - LMMS wrapper for a lockless ringbuffer library
 *
 * Copyright (c) 2019 Martin Pavelek <he29/dot/HS/at/gmail/dot/com>
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

#ifndef LOCKLESSRINGBUFFER_H
#define LOCKLESSRINGBUFFER_H

#include "lmms_export.h"
#include "../src/3rdparty/ringbuffer/include/ringbuffer/ringbuffer.h"
#include "lmms_basics.h"
#include <QMutex>
#include <QWaitCondition>


//! A convenience layer for a realtime-safe and thread-safe multi-reader ring buffer library.
template <class T>
class LocklessRingBuffer
{
	template<class _T>
	friend class LocklessRingBufferReader;
public:
	LocklessRingBuffer(std::size_t sz);
	~LocklessRingBuffer();

	std::size_t write(const T *src, size_t cnt);
	std::size_t capacity();
	std::size_t free();
	void wakeAll();

private:
	ringbuffer_t<T> m_buffer;
	QWaitCondition m_notifier;
};


// The sampleFrame_copier is required because sampleFrame is just a two-element
// array and therefore does not have a copy constructor needed by std::copy.
class LMMS_EXPORT sampleFrame_copier
{
	const sampleFrame* src;
public:
	sampleFrame_copier(const sampleFrame* src) : src(src) {}
	void operator()(std::size_t src_offset, std::size_t count, sampleFrame* dest)
	{
		for (std::size_t i = src_offset; i < src_offset + count; i++, dest++)
		{
			(*dest)[0] = src[i][0];
			(*dest)[1] = src[i][1];
		}
	}
};


//! Wrapper for lockless ringbuffer reader
template <class T>
class LocklessRingBufferReader : public ringbuffer_reader_t<T>
{
public:
	LocklessRingBufferReader(LocklessRingBuffer<T> &rb);

	bool empty();
	void waitForData();
private:
	QWaitCondition *m_notifier;
};


// This is required to force MSVC compilers to export symbols for template class methods.
// Any template instances that are not specified here will not work in plugins.
template class LMMS_EXPORT LocklessRingBuffer<sampleFrame>;
template class LMMS_EXPORT LocklessRingBufferReader<sampleFrame>;

#endif //LOCKLESSRINGBUFFER_H
