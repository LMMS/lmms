/*
 * AudioPort.cpp - base-class for objects providing sound at a port
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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

#include "AudioPort.h"
#include "AudioDevice.h"
#include "EffectChain.h"
#include "FxMixer.h"
#include "engine.h"
#include "MixHelpers.h"
#include "BufferManager.h"


AudioPort::AudioPort( const QString & _name, bool _has_effect_chain ) :
	m_bufferUsage( false ),
	m_portBuffer( NULL ),
	m_extOutputEnabled( false ),
	m_nextFxChannel( 0 ),
	m_name( "unnamed port" ),
	m_effects( _has_effect_chain ? new EffectChain( NULL ) : NULL )
{
	engine::mixer()->addAudioPort( this );
	setExtOutputEnabled( true );
}




AudioPort::~AudioPort()
{
	setExtOutputEnabled( false );
	engine::mixer()->removeAudioPort( this );
	delete m_effects;
}




void AudioPort::setExtOutputEnabled( bool _enabled )
{
	if( _enabled != m_extOutputEnabled )
	{
		m_extOutputEnabled = _enabled;
		if( m_extOutputEnabled )
		{
			engine::mixer()->audioDev()->registerPort( this );
		}
		else
		{
			engine::mixer()->audioDev()->unregisterPort( this );
		}
	}
}




void AudioPort::setName( const QString & _name )
{
	m_name = _name;
	engine::mixer()->audioDev()->renamePort( this );
}




bool AudioPort::processEffects()
{
	if( m_effects )
	{
		bool more = m_effects->processAudioBuffer( m_portBuffer, engine::mixer()->framesPerPeriod(), m_bufferUsage );
		return more;
	}
	return false;
}


void AudioPort::doProcessing()
{
	const fpp_t fpp = engine::mixer()->framesPerPeriod();

	if( m_playHandles.isEmpty() ) return; // skip processing if no playhandles are connected

	m_portBuffer = BufferManager::acquire(); // get buffer for processing

	engine::mixer()->clearAudioBuffer( m_portBuffer, fpp ); // clear the audioport buffer so we can use it

	//qDebug( "Playhandles: %d", m_playHandles.size() );
	foreach( PlayHandle * ph, m_playHandles ) // now we mix all playhandle buffers into the audioport buffer
	{
		if( ph->buffer() )
		{
			if( ph->usesBuffer() )
			{
				m_bufferUsage = true;
				MixHelpers::add( m_portBuffer, ph->buffer(), fpp );
			}
			ph->releaseBuffer(); 	// gets rid of playhandle's buffer and sets 
									// pointer to null, so if it doesn't get re-acquired we know to skip it next time
		}
	}
	
	const bool me = processEffects();
	if( me || m_bufferUsage )
	{
		engine::fxMixer()->mixToChannel( m_portBuffer, m_nextFxChannel );
		m_bufferUsage = false;
	}
	
	BufferManager::release( m_portBuffer ); // release buffer, we don't need it anymore
}


void AudioPort::addPlayHandle( PlayHandle * handle )
{
	m_playHandleLock.lock();
		m_playHandles.append( handle );
	m_playHandleLock.unlock();
}


void AudioPort::removePlayHandle( PlayHandle * handle )
{
	m_playHandleLock.lock();
		PlayHandleList::Iterator it =	qFind( m_playHandles.begin(), m_playHandles.end(), handle );
		if( it != m_playHandles.end() )
		{
			m_playHandles.erase( it );
		}
	m_playHandleLock.unlock();
}
