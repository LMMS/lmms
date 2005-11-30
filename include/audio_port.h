/*
 * audio_port.h - base-class for objects providing sound at a port
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


#ifndef _AUDIO_PORT_H
#define _AUDIO_PORT_H

#include "qt3support.h"

#ifdef QT4

#include <QString>

#else

#include <qstring.h>

#endif


#include "mixer.h"
#include "effect_board.h"


class audioPort
{
public:
	audioPort( const QString & _name );
	~audioPort();

	inline surroundSampleFrame * firstBuffer( void )
	{
		return( m_firstBuffer );
	}
	inline surroundSampleFrame * secondBuffer( void )
	{
		return( m_secondBuffer );
	}

	void nextPeriod( void );


	// indicate whether JACK & Co should provide output-buffer at ext. port
	inline bool extOutputEnabled( void ) const
	{
		return( m_extOutputEnabled );
	}
	void FASTCALL setExtOutputEnabled( bool _enabled );


	// next effect-channel after this audio-port
	// (-1 = none  0 = master)
	inline fxChnl nextFxChannel( void ) const
	{
		return( m_nextFxChannel );
	}
	void setNextFxChannel( fxChnl _chnl )
	{
		m_nextFxChannel = _chnl;
	}

	void setName( const QString & _new_name );

	enum bufferUsages
	{
		NONE, FIRST, BOTH
	} m_bufferUsage;


private:
	surroundSampleFrame * m_firstBuffer;
	surroundSampleFrame * m_secondBuffer;
	bool m_extOutputEnabled;
	fxChnl m_nextFxChannel;

} ;


#endif
