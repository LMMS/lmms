#ifndef SINGLE_SOURCE_COMPILE

/*
 * midi_port.cpp - abstraction of MIDI-ports which are part of LMMS's MIDI-
 *                 sequencing system
 *
 * Copyright (c) 2005-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "midi_port.h"
#include "midi_client.h"
#include "volume.h"



midiPort::midiPort( midiClient * _mc, midiEventProcessor * _mep,
					const QString & _name, Modes _mode ) :
	m_midiClient( _mc ),
	m_midiEventProcessor( _mep ),
	m_name( _name ),
	m_mode( _mode ),
	m_inputChannel( -1 ),
	m_outputChannel( -1 ),
	m_defaultVelocityForInEventsEnabled( FALSE ),
	m_defaultVelocityForOutEventsEnabled( FALSE )
{
}




midiPort::~midiPort()
{
}




void midiPort::setName( const QString & _name )
{
	m_name = _name;
	m_midiClient->applyPortName( this );
}




void midiPort::setMode( Modes _mode )
{
	m_mode = _mode;
	m_midiClient->applyPortMode( this );
}




void midiPort::processInEvent( const midiEvent & _me, const midiTime & _time )
{
	// mask event
	if( ( mode() == Input || mode() == Duplex ) &&
		( inputChannel() == _me.m_channel || inputChannel() == -1 ) )
	{
		midiEvent ev = _me;
		if( m_defaultVelocityForInEventsEnabled == TRUE &&
							_me.velocity() > 0 )
		{
			ev.velocity() = DefaultVolume;
		}
		m_midiEventProcessor->processInEvent( ev, _time );
	}
}




void midiPort::processOutEvent( const midiEvent & _me, const midiTime & _time )
{
	// mask event
	if( ( mode() == Output || mode() == Duplex ) &&
		( outputChannel() == _me.m_channel && outputChannel() != -1 ) )
	{
		midiEvent ev = _me;
		if( m_defaultVelocityForOutEventsEnabled == TRUE &&
							_me.velocity() > 0 )
		{
			ev.velocity() = DefaultVolume;
		}
		m_midiClient->processOutEvent( ev, _time, this );
	}
}



#endif
