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

#ifndef LMMS_LOCKLESS_RING_BUFFER_H
#define LMMS_LOCKLESS_RING_BUFFER_H

#include <QMutex>
#include <QWaitCondition>

#include <ringbuffer/ringbuffer.h>

#include "LmmsTypes.h"

namespace lmms
{

//! A convenience layer for a realtime-safe and thread-safe multi-reader ringbuffer
template <class T>
class LocklessRingBuffer
{
	template<class _T>
	friend class LocklessRingBufferReader;
public:
	LocklessRingBuffer(std::size_t sz) : m_buffer(sz)
	{
		m_buffer.touch();	// reserve storage space before realtime operation starts
	}
	~LocklessRingBuffer() = default;

	std::size_t capacity() const {return m_buffer.maximum_eventual_write_space();}
	std::size_t free() const {return m_buffer.write_space();}
	void wakeAll() {m_notifier.wakeAll();}
	std::size_t write(const T *src, std::size_t cnt, bool notify = false)
	{
		std::size_t written = LocklessRingBuffer<T>::m_buffer.write(src, cnt);
		// Let all waiting readers know new data are available.
		if (notify) {LocklessRingBuffer<T>::m_notifier.wakeAll();}
		return written;
	}
	void mlock() { m_buffer.mlock(); }

protected:
	ringbuffer_t<T> m_buffer;
	QWaitCondition m_notifier;
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


} // namespace lmms

#endif // LMMS_LOCKLESS_RING_BUFFER_H
