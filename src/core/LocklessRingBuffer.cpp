/*
 * LocklessRingBuffer.cpp - LMMS wrapper for a lockless ringbuffer library
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

#include "LocklessRingBuffer.h"


template <class T>
LocklessRingBuffer<T>::LocklessRingBuffer(std::size_t sz) :
	m_buffer(sz)
{
}


template <class T>
LocklessRingBuffer<T>::~LocklessRingBuffer()
{
}


//! Specialized write function modified to support sampleFrame.
template <>
std::size_t LocklessRingBuffer<sampleFrame>::write(const sampleFrame *src, size_t cnt)
{
    sampleFrame_copier copier(src);
    std::size_t written = m_buffer.write_func<sampleFrame_copier>(copier, cnt);
	// Let all waiting readers know new data are available.
	m_notifier.wakeAll();
	return written;
}


template <class T>
std::size_t LocklessRingBuffer<T>::write(const T *src, size_t cnt)
{
	std::size_t written = m_buffer.write(src, cnt);
	m_notifier.wakeAll();	// Let all waiting readers know new data are available.
	return written;
}


template <class T>
std::size_t LocklessRingBuffer<T>::capacity()
{
	return m_buffer.maximum_eventual_write_space();
}


template <class T>
std::size_t LocklessRingBuffer<T>::free()
{
	return m_buffer.write_space();
}


template <class T>
void LocklessRingBuffer<T>::wakeAll()
{
	m_notifier.wakeAll();
}


template <class T>
LocklessRingBufferReader<T>::LocklessRingBufferReader(LocklessRingBuffer<T> &rb) :
	ringbuffer_reader_t<T>(rb.m_buffer),
	m_notifier(&rb.m_notifier)
{
}


template <class T>
bool LocklessRingBufferReader<T>::empty()
{
	return !this->read_space();
}


template <class T>
void LocklessRingBufferReader<T>::waitForData()
{
	QMutex useless_lock;
	m_notifier->wait(&useless_lock);
	useless_lock.unlock();
}
