/*
 * audio_port.cpp - base-class for objects providing sound at a port
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


#include "audio_port.h"
#include "audio_device.h"
#include "buffer_allocator.h"


audioPort::audioPort( const QString & _name, engine * _engine ) :
	engineObject( _engine ),
	m_bufferUsage( NONE ),
	m_firstBuffer( bufferAllocator::alloc<surroundSampleFrame>(
				eng()->getMixer()->framesPerAudioBuffer() ) ),
	m_secondBuffer( bufferAllocator::alloc<surroundSampleFrame>(
				eng()->getMixer()->framesPerAudioBuffer() ) ),
	m_extOutputEnabled( FALSE ),
	m_nextFxChannel( -1 ),
	m_name( "unnamed port" )
{
	eng()->getMixer()->clearAudioBuffer( m_firstBuffer,
				eng()->getMixer()->framesPerAudioBuffer() );
	eng()->getMixer()->clearAudioBuffer( m_secondBuffer,
				eng()->getMixer()->framesPerAudioBuffer() );
	eng()->getMixer()->addAudioPort( this );
	setExtOutputEnabled( TRUE );
}




audioPort::~audioPort()
{
	eng()->getMixer()->removeAudioPort( this );
	if( m_extOutputEnabled == TRUE )
	{
		eng()->getMixer()->audioDev()->unregisterPort( this );
	}
	bufferAllocator::free( m_firstBuffer );
	bufferAllocator::free( m_secondBuffer );
}




void audioPort::nextPeriod( void )
{
	eng()->getMixer()->clearAudioBuffer( m_firstBuffer,
				eng()->getMixer()->framesPerAudioBuffer() );
	qSwap( m_firstBuffer, m_secondBuffer );
	// this is how we decrease state of buffer-usage ;-)
	m_bufferUsage = ( m_bufferUsage != NONE ) ?
		( ( m_bufferUsage == FIRST ) ? NONE : FIRST ) : NONE;
}




void audioPort::setExtOutputEnabled( bool _enabled )
{
	if( _enabled != m_extOutputEnabled )
	{
		m_extOutputEnabled = _enabled;
		if( m_extOutputEnabled )
		{
			eng()->getMixer()->audioDev()->registerPort( this );
		}
		else
		{
			eng()->getMixer()->audioDev()->unregisterPort( this );
		}
	}
}




void audioPort::setName( const QString & _name )
{
	m_name = _name;
	eng()->getMixer()->audioDev()->renamePort( this );
}

