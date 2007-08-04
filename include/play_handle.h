/*
 * play_handle.h - base-class playHandle which is needed by
 *                 LMMS-Player-Engine
 *
 * Copyright (c) 2004-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _PLAY_HANDLE_H
#define _PLAY_HANDLE_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "qt3support.h"

#ifdef QT4

#include <QtCore/QVector>

#else

#include <qvaluevector.h>

#endif

#include "types.h"

class track;


class playHandle
{
public:
	enum types
	{
		NotePlayHandle,
		InstrumentPlayHandle,
		SamplePlayHandle,
		PresetPreviewHandle
	} ;

	playHandle( const types _type, f_cnt_t _offset = 0 ) :
		m_type( _type ),
		m_offset( _offset )
	{
	}

	virtual inline ~playHandle()
	{
	}

	inline types type( void ) const
	{
		return( m_type );
	}

	virtual void play( bool _try_parallelizing = FALSE ) = 0;
	virtual bool done( void ) const = 0;

	// returns how many frames this play-handle is aligned ahead, i.e.
	// at which position it is inserted in the according buffer
	inline f_cnt_t offset( void ) const
	{
		return ( m_offset );
	}

	inline void setOffset( f_cnt_t _offset )
	{
		m_offset = _offset;
	}


	virtual bool isFromTrack( const track * _track ) const = 0;

	virtual bool supportsParallelizing( void ) const
	{
		return( FALSE );
	}

	virtual void waitForWorkerThread( void )
	{
	}


private:
	types m_type;
	f_cnt_t m_offset;

} ;


typedef vvector<playHandle *> playHandleVector;
typedef vvector<const playHandle *> constPlayHandleVector;


#endif
