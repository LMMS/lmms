/*
 * midi_port.cpp - abstraction of MIDI-ports which are part of LMMS's MIDI-
 *                 sequencing system
 *
 * Linux MultiMedia Studio
 * Copyright (_c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * This file partly contains code from Fluidsynth, Peter Hanappe
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


#include "midi_port.h"
#include "midi_client.h"



midiPort::midiPort( midiClient * _mc, midiEventProcessor * _mep,
					const QString & _name, modes _mode ) :
	m_midiClient( _mc ),
	m_midiEventProcessor( _mep ),
	m_name( _name ),
	m_mode( _mode ),
	m_inputChannel( -1 ),
	m_outputChannel( -1 )
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




void midiPort::setMode( modes _mode )
{
	m_mode = _mode;
	m_midiClient->applyPortMode( this );
}




void midiPort::processInEvent( const midiEvent & _me, const midiTime & _time )
{
	// mask event
	if( ( mode() == INPUT || mode() == DUPLEX ) &&
		( inputChannel() == _me.m_channel || inputChannel() == -1 ) )
	{
		m_midiEventProcessor->processInEvent( _me, _time );
	}
}




void midiPort::processOutEvent( const midiEvent & _me, const midiTime & _time )
{
	// mask event
	if( ( mode() == OUTPUT || mode() == DUPLEX ) &&
		( outputChannel() == _me.m_channel || outputChannel() == -1 ) )
	{
		m_midiClient->processOutEvent( _me, _time, this );
	}
}


