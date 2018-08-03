/*
 * AutoDetectMidiController.cpp - says from which controller last event is
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail.com>
 *
 * This file is part of LMMS - https://lmms.io
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

#include "AutoDetectMidiController.h"
#include "MidiController.h"
#include "MidiEvent.h"
#include "MidiPort.h"
#include "MidiTime.h"
#include "Mixer.h"
#include "Model.h"


AutoDetectMidiController::AutoDetectMidiController( Model* parent ) :
	MidiController( parent ),
	m_detectedMidiChannel( 0 ),
	m_detectedMidiController( 0 )
{
	updateName();

	MidiPort::Map map = m_midiPort.readablePorts();

	for( MidiPort::Map::Iterator it = map.begin(); it != map.end(); ++it )
	{
		it.value() = true;
	}

	subscribeReadablePorts( map );
}


AutoDetectMidiController::~AutoDetectMidiController()
{
}


void AutoDetectMidiController::processInEvent( const MidiEvent& event, const MidiTime& time, f_cnt_t offset )
{
	if( event.type() == MidiControlChange &&
		( m_midiPort.inputChannel() == 0 || m_midiPort.inputChannel() == event.channel() + 1 ) )
	{
		m_detectedMidiChannel = event.channel() + 1;
		m_detectedMidiController = event.controllerNumber() + 1;
		m_detectedMidiPort = Engine::mixer()->midiClient()->sourcePortName( event );

		emit valueChanged();
	}
}


// Would be a nice copy ctor, but too hard to add copy ctor because
// model has none.
MidiController* AutoDetectMidiController::copyToMidiController( Model* parent )
{
	MidiController* c = new MidiController( parent );
	c->m_midiPort.setInputChannel( m_midiPort.inputChannel() );
	c->m_midiPort.setInputController( m_midiPort.inputController() );
	c->subscribeReadablePorts( m_midiPort.readablePorts() );
	c->updateName();

	return c;
}


void AutoDetectMidiController::useDetected()
{
	m_midiPort.setInputChannel( m_detectedMidiChannel );
	m_midiPort.setInputController( m_detectedMidiController );

	const MidiPort::Map& map = m_midiPort.readablePorts();
	for( MidiPort::Map::ConstIterator it = map.begin(); it != map.end(); ++it )
	{
		m_midiPort.subscribeReadablePort( it.key(),
						m_detectedMidiPort.isEmpty() || ( it.key() == m_detectedMidiPort ) );
	}
}


void AutoDetectMidiController::reset()
{
	m_midiPort.setInputChannel( 0 );
	m_midiPort.setInputController( 0 );
}
