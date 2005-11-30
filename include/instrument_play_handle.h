/*
 * instrument_play_handle.h - play-handle for playing an instrument
 *
 * Copyright (c) 2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _INSTRUMENT_PLAY_HANDLE_H
#define _INSTRUMENT_PLAY_HANDLE_H

#include "play_handle.h"
#include "instrument.h"


class instrumentPlayHandle : public playHandle
{
public:
	inline instrumentPlayHandle( instrument * _instrument ) :
		playHandle(),
		m_instrument( _instrument )
	{
	}

	inline virtual ~instrumentPlayHandle()
	{
	}


	inline virtual void play( void )
	{
		m_instrument->play();
	}

	inline virtual bool done( void ) const
	{
		return( m_instrument == NULL );
	}

	inline virtual void checkValidity( void )
	{
		if( !m_instrument->valid() )
		{
			m_instrument = NULL;
		}
	}


private:
	instrument * m_instrument;

} ;


#endif
