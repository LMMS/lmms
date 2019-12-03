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

#include <QMutex>
#include <QWaitCondition>

#include "lmms_basics.h"
#include "lmms_export.h"
#include "../src/3rdparty/ringbuffer/include/ringbuffer/ringbuffer.h"


//! A convenience layer for a realtime-safe and thread-safe multi-reader ring buffer library.
template <class T>
class LocklessRingBufferBase
{
	template<class _T>
	friend class LocklessRingBufferReader;
public:
	LocklessRingBufferBase(std::size_t sz) : m_buffer(sz)
	{
		m_buffer.touch();	// reserve storage space before realtime operation starts
	}
	~LocklessRingBufferBase() {};

	std::size_t capacity() const {return m_buffer.maximum_eventual_write_space();}
	std::size_t free() const {return m_buffer.write_space();}
	void wakeAll() {m_notifier.wakeAll();}

protected:
	ringbuffer_t<T> m_buffer;
	QWaitCondition m_notifier;
};


// The SampleFrameCopier is required because sampleFrame is just a two-element
// array and therefore does not have a copy constructor needed by std::copy.
class SampleFrameCopier
{
	const sampleFrame* m_src;
public:
	SampleFrameCopier(const sampleFrame* src) : m_src(src) {}
	void operator()(std::size_t src_offset, std::size_t count, sampleFrame* dest)
	{
		for (std::size_t i = src_offset; i < src_offset + count; i++, dest++)
		{
			(*dest)[0] = m_src[i][0];
			(*dest)[1] = m_src[i][1];
		}
	}
};


//! Standard ring buffer template for data types with copy constructor.
template <class T>
class LocklessRingBuffer : public LocklessRingBufferBase<T>
{
public:
	LocklessRingBuffer(std::size_t sz) : LocklessRingBufferBase<T>(sz) {};

	std::size_t write(const sampleFrame *src, std::size_t cnt, bool notify = false)
	{
		std::size_t written = LocklessRingBufferBase<T>::m_buffer.write(src, cnt);
		// Let all waiting readers know new data are available.
		if (notify) {LocklessRingBufferBase<T>::m_notifier.wakeAll();}
		return written;
	}
};


//! Specialized ring buffer template with write function modified to support sampleFrame.
template <>
class LocklessRingBuffer<sampleFrame> : public LocklessRingBufferBase<sampleFrame>
{
public:
	LocklessRingBuffer(std::size_t sz) : LocklessRingBufferBase<sampleFrame>(sz) {};

	std::size_t write(const sampleFrame *src, std::size_t cnt, bool notify = false)
	{
		SampleFrameCopier copier(src);
		std::size_t written = LocklessRingBufferBase<sampleFrame>::m_buffer.write_func<SampleFrameCopier>(copier, cnt);
		// Let all waiting readers know new data are available.
		if (notify) {LocklessRingBufferBase<sampleFrame>::m_notifier.wakeAll();}
		return written;
	}
};


//! Wrapper for lockless ringbuffer reader
template <class T>
class LocklessRingBufferReader : public ringbuffer_reader_t<T>
{
public:
	LocklessRingBufferReader(LocklessRingBuffer<T> &rb) :
		ringbuffer_reader_t<T>(rb.m_buffer),
		m_notifier(&rb.m_notifier) {};

	bool empty() const {return !this->read_space();}
	void waitForData()
	{
		QMutex useless_lock;
		useless_lock.lock();
		m_notifier->wait(&useless_lock);
		useless_lock.unlock();
	}
private:
	QWaitCondition *m_notifier;
};

#endif //LOCKLESSRINGBUFFER_H
