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


AudioPort::AudioPort( const QString & _name, bool _has_effect_chain ) :
	m_bufferUsage( NoUsage ),
	m_firstBuffer( new sampleFrame[engine::mixer()->framesPerPeriod()] ),
	m_secondBuffer( new sampleFrame[
				engine::mixer()->framesPerPeriod()] ),
	m_extOutputEnabled( false ),
	m_nextFxChannel( 0 ),
	m_name( "unnamed port" ),
	m_effects( _has_effect_chain ? new EffectChain( NULL ) : NULL )
{
	engine::mixer()->clearAudioBuffer( m_firstBuffer, engine::mixer()->framesPerPeriod() );
	engine::mixer()->clearAudioBuffer( m_secondBuffer, engine::mixer()->framesPerPeriod() );
	engine::mixer()->addAudioPort( this );
	setExtOutputEnabled( true );
}




AudioPort::~AudioPort()
{
	setExtOutputEnabled( false );
	engine::mixer()->removeAudioPort( this );
	delete[] m_firstBuffer;
	delete[] m_secondBuffer;
	delete m_effects;
}




void AudioPort::nextPeriod()
{
	m_firstBufferLock.lock();
	engine::mixer()->clearAudioBuffer( m_firstBuffer, engine::mixer()->framesPerPeriod() );
	qSwap( m_firstBuffer, m_secondBuffer );

	// this is how we decrease state of buffer-usage ;-)
	m_bufferUsage = ( m_bufferUsage != NoUsage ) ?
		( ( m_bufferUsage == FirstBuffer ) ?
					NoUsage : FirstBuffer ) : NoUsage;

	m_firstBufferLock.unlock();
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
		lockFirstBuffer();
		bool hasInputNoise = m_bufferUsage != NoUsage;
		bool more = m_effects->processAudioBuffer( m_firstBuffer, engine::mixer()->framesPerPeriod(), hasInputNoise );
		unlockFirstBuffer();
		return more;
	}
	return false;
}


void AudioPort::doProcessing( sampleFrame * )
{
	const bool me = processEffects();
	if( me || m_bufferUsage != NoUsage )
	{
		engine::fxMixer()->mixToChannel( firstBuffer(), nextFxChannel() );
		nextPeriod();
	}
}

