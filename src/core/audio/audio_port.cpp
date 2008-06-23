#ifndef SINGLE_SOURCE_COMPILE

/*
 * audio_port.cpp - base-class for objects providing sound at a port
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "mixer.h"
#include "audio_device.h"
#include "config_mgr.h"

#include "audio_port.h"
#include "audio_device.h"
#include "engine.h"


audioPort::audioPort( const QString & _name ) :
	m_bufferUsage( NoUsage ),
	m_firstBuffer( new sampleFrame[engine::getMixer()->framesPerPeriod()] ),
	m_secondBuffer( new sampleFrame[
				engine::getMixer()->framesPerPeriod()] ),
	m_extOutputEnabled( FALSE ),
	m_nextFxChannel( 0 ),
	m_name( "unnamed port" ),
	m_effects( NULL )
{
	engine::getMixer()->clearAudioBuffer( m_firstBuffer,
				engine::getMixer()->framesPerPeriod() );
	engine::getMixer()->clearAudioBuffer( m_secondBuffer,
				engine::getMixer()->framesPerPeriod() );
	engine::getMixer()->addAudioPort( this );
	setExtOutputEnabled( TRUE );
}




audioPort::~audioPort()
{
	setExtOutputEnabled( FALSE );
	engine::getMixer()->removeAudioPort( this );
	delete[] m_firstBuffer;
	delete[] m_secondBuffer;
}




void audioPort::nextPeriod( void )
{
	m_firstBufferLock.lock();
	engine::getMixer()->clearAudioBuffer( m_firstBuffer,
				engine::getMixer()->framesPerPeriod() );
	qSwap( m_firstBuffer, m_secondBuffer );

	// this is how we decrease state of buffer-usage ;-)
	m_bufferUsage = ( m_bufferUsage != NoUsage ) ?
		( ( m_bufferUsage == FirstBuffer ) ?
					NoUsage : FirstBuffer ) : NoUsage;

	m_firstBufferLock.unlock();
}




void audioPort::setExtOutputEnabled( bool _enabled )
{
	if( _enabled != m_extOutputEnabled )
	{
		m_extOutputEnabled = _enabled;
		if( m_extOutputEnabled )
		{
			engine::getMixer()->audioDev()->registerPort( this );
		}
		else
		{
			engine::getMixer()->audioDev()->unregisterPort( this );
		}
	}
}




void audioPort::setName( const QString & _name )
{
	m_name = _name;
	engine::getMixer()->audioDev()->renamePort( this );
}




bool audioPort::processEffects( void )
{
	lockFirstBuffer();
	bool more = m_effects.processAudioBuffer( m_firstBuffer,
					engine::getMixer()->framesPerPeriod() );
	unlockFirstBuffer();
	return( more );
}



#endif
