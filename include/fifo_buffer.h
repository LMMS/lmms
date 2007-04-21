/*
 * fifo_buffer.h - FIFO fixed-size buffer
 *
 * Copyright (c) 2007 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
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


#ifdef QT4

#include <QtCore/QSemaphore>

#else

#include <qsemaphore.h>

#endif


template<typename T>
class fifoBuffer
{
public:
	fifoBuffer( int _size ) :
		m_reader_sem( _size ),
		m_writer_sem( _size ),
		m_reader_index( 0 ),
		m_writer_index( 0 ),
		m_size( _size )
	{
		m_buffer = new T[_size];
#ifndef QT3
		m_reader_sem.acquire( _size );
#else
		m_reader_sem += _size;
#endif
	}

	~fifoBuffer()
	{
		delete[] m_buffer;
#ifndef QT3
		m_reader_sem.release( m_size );
#else
		m_reader_sem -= m_size;
#endif
	}

	void write( T _element )
	{
#ifndef QT3
		m_writer_sem.acquire();
#else
		m_writer_sem++;
#endif
		m_buffer[m_writer_index++] = _element;
		m_writer_index %= m_size;
#ifndef QT3
		m_reader_sem.release();
#else
		m_reader_sem--;
#endif
	}

	T read( void )
	{
#ifndef QT3
		m_reader_sem.acquire();
#else
		m_reader_sem++;
#endif
		T element = m_buffer[m_reader_index++];
		m_reader_index %= m_size;
#ifndef QT3
		m_writer_sem.release();
#else
		m_writer_sem--;
#endif
		return( element );
	}

	bool available( void )
	{
		return( m_reader_sem.available() );
	}


private:
	QSemaphore m_reader_sem;
	QSemaphore m_writer_sem;
	int m_reader_index;
	int m_writer_index;
	int m_size;
	T * m_buffer;

} ;




#endif
