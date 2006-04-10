/*
 * play_handle.h - base-class playHandle which is needed by
 *                 LMMS-Player-Engine
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
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


class playHandle
{
public:
	enum types
	{
		NOTE_PLAY_HANDLE, INSTRUMENT_PLAY_HANDLE, SAMPLE_PLAY_HANDLE,
		PRESET_PREVIEW_PLAY_HANDLE
	} ;

	playHandle( const types _type ) :
		m_type( _type )
	{
	}

	virtual inline ~playHandle()
	{
	}

	inline types type( void ) const
	{
		return( m_type );
	}

	virtual void play( void ) = 0;
	virtual bool done( void ) const = 0;

	// play-handles can invalidate themselves if an object they depend on
	// is going to be deleted or things like that - every of those objects
	// has to call mixer::inst()->checkValidityOfPlayHandles() in it's dtor
	// and set flag before, so LMMS doesn't crash because these play-handles
	// would continue using pointers to deleted objects...
	virtual void checkValidity( void )
	{
	}


private:
	types m_type;

} ;


typedef vvector<playHandle *> playHandleVector;
typedef vvector<const playHandle *> constPlayHandleVector;


#endif
