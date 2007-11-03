/*
 * file.h - file descriptor wrapper
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


#ifndef _FILE_H
#define _FILE_H


#include <unistd.h>
#include <sys/types.h>


class File
{
public:
	File( int _fd ) :
		m_fd( _fd )
	{
	}
	virtual ~File()
	{
		close( m_fd );
	}

	template<typename T>
	ssize_t read( T * _i, int _n = 1 )
	{
		return( ::read( m_fd, _i, _n * sizeof( T ) ) );
	}

	template<typename T>
	ssize_t write( const T * _i, int _n = 1 )
	{
		return( ::write( m_fd, _i, _n * sizeof( T ) ) );
	}

	off_t rewind( void )
	{
		return( lseek( m_fd, 0, SEEK_SET ) );
	}


private:
	const int m_fd;

} ;




#endif
