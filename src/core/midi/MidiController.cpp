/*
 * MidiController.cpp - implementation of class midi-controller which handles
 *                      MIDI control change messages
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


#include "AudioEngine.h"
#include "MidiController.h"

namespace lmms
{


MidiController::MidiController( Model * _parent ) :
	Controller( ControllerType::Midi, _parent, tr( "MIDI Controller" ) ),
	MidiEventProcessor(),
	m_midiPort( tr( "unnamed_midi_controller" ), Engine::audioEngine()->midiClient(), this, this, MidiPort::Mode::Input ),
	m_lastValue( 0.0f ),
	m_previousValue( 0.0f )
{
	setSampleExact( true );
	connect( &m_midiPort, SIGNAL(modeChanged()),
			this, SLOT(updateName()));
}




void MidiController::updateValueBuffer()
{
	if( m_previousValue != m_lastValue )
	{
		m_valueBuffer.interpolate( m_previousValue, m_lastValue );
		m_previousValue = m_lastValue;
	}
	else
	{
		m_valueBuffer.fill( m_lastValue );
	}
	m_bufferLastUpdated = s_periods;
}


void MidiController::updateName()
{
	setName( QString("MIDI ch%1 ctrl%2").
			arg( m_midiPort.inputChannel() ).
			arg( m_midiPort.inputController() ) );
}




void MidiController::processInEvent(const MidiEvent& event, const TimePos& time, f_cnt_t offset)
{
	switch(event.type())
	{
		case MidiControlChange:
		{
			unsigned char controllerNum = event.controllerNumber();

			if (m_midiPort.inputController() == controllerNum &&
				(m_midiPort.inputChannel() == event.channel() + 1 || m_midiPort.inputChannel() == 0))
			{
				unsigned char val = event.controllerValue();
				m_previousValue = m_lastValue;
				m_lastValue = static_cast<float>(val) / 127.0f;
				emit valueChanged();
			}
			break;
		}
		default:
			// Don't care - maybe add special cases for pitch and mod later
			break;
	}
}




void MidiController::subscribeReadablePorts( const MidiPort::Map & _map )
{
	for( MidiPort::Map::ConstIterator it = _map.constBegin();
						it != _map.constEnd(); ++it )
	{
		m_midiPort.subscribeReadablePort( it.key(), *it );
	}
}




void MidiController::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	Controller::saveSettings( _doc, _this );
	m_midiPort.saveSettings( _doc, _this );

}




void MidiController::loadSettings( const QDomElement & _this )
{
	Controller::loadSettings( _this );

	m_midiPort.loadSettings( _this );

	updateName();
}




QString MidiController::nodeName() const
{
	return( "Midicontroller" );
}




gui::ControllerDialog* MidiController::createDialog( QWidget * _parent )
{
	return nullptr;
}



} // namespace lmms
