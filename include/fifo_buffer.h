/*
 * fifo_buffer.h - FIFO fixed-size buffer
 *
 * Copyright (c) 2007 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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

#ifndef _FIFO_BUFFER_H
#define _FIFO_BUFFER_H

#include <QtCore/QSemaphore>


template<typename T>
class fifoBuffer
{
public:
	fifoBuffer( int _size ) :
		m_readerSem( _size ),
		m_writerSem( _size ),
		m_readerIndex( 0 ),
		m_writerIndex( 0 ),
		m_size( _size )
	{
		m_buffer = new T[_size];
		m_readerSem.acquire( _size );
	}

	~fifoBuffer()
	{
		delete[] m_buffer;
		m_readerSem.release( m_size );
	}

	void write( T _element )
	{
		m_writerSem.acquire();
		m_buffer[m_writerIndex++] = _element;
		m_writerIndex %= m_size;
		m_readerSem.release();
	}

	T read()
	{
		m_readerSem.acquire();
		T element = m_buffer[m_readerIndex++];
		m_readerIndex %= m_size;
		m_writerSem.release();
		return element;
	}

	bool available()
	{
		return m_readerSem.available();
	}


private:
	QSemaphore m_readerSem;
	QSemaphore m_writerSem;
	int m_readerIndex;
	int m_writerIndex;
	int m_size;
	T * m_buffer;

} ;




#endif
