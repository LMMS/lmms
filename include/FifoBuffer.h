/*
 * FifoBuffer.h - FIFO fixed-size buffer
 *
 * Copyright (c) 2007 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
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

#ifndef LMMS_FIFO_BUFFER_H
#define LMMS_FIFO_BUFFER_H

#include <QSemaphore>


namespace lmms
{


template<typename T>
class FifoBuffer
{
public:
	FifoBuffer(int size) :
		m_readSem(size),
		m_writeSem(size),
		m_readIndex(0),
		m_writeIndex(0),
		m_size(size)
	{
		m_buffer = new T[size];
		m_readSem.acquire(size);
	}

	~FifoBuffer()
	{
		delete[] m_buffer;
		m_readSem.release(m_size);
	}

	void write(T element)
	{
		m_writeSem.acquire();
		m_buffer[m_writeIndex++] = element;
		m_writeIndex %= m_size;
		m_readSem.release();
	}

	T read()
	{
		m_readSem.acquire();
		T element = m_buffer[m_readIndex++];
		m_readIndex %= m_size;
		m_writeSem.release();
		return element;
	}

	void waitUntilRead()
	{
		m_writeSem.acquire(m_size);
		m_writeSem.release(m_size);
	}

	bool available()
	{
		return m_readSem.available();
	}


private:
	QSemaphore m_readSem;
	QSemaphore m_writeSem;
	int m_readIndex;
	int m_writeIndex;
	int m_size;
	T * m_buffer;
} ;


} // namespace lmms

#endif // LMMS_FIFO_BUFFER_H
