/*
 * MidiCustomController.cpp - implementation of class midi-controller which handles
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

#include <QDomElement>
#include <QObject>

#include "Song.h"
#include "Mixer.h"
#include "MidiClient.h"
#include "MidiCustomController.h"


MidiCustomController::MidiCustomController( Model * _parent ) :
	MidiController( _parent ),
	m_midiPort( tr( "unnamed_midi_controller" ),
			Engine::mixer()->midiClient(), this, this, MidiPort::Input ),
	m_lastValue( 0.0f ),
	m_previousValue( 0.0f )
{
	setSampleExact( true );
	connect( &m_midiPort, SIGNAL( modeChanged() ),
			this, SLOT( updateName() ) );

	//RIKIS
//	m_MinVal = -std::numeric_limits<float>::infinity();
//	m_MaxVal =  std::numeric_limits<float>::infinity();
//	m_ControllerType = original;
	//RIKIS
}




MidiCustomController::~MidiCustomController()
{
}




void MidiCustomController::updateValueBuffer()
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


void MidiCustomController::updateName()
{
	setName( QString("MIDI ch%1 ctrl%2").
			arg( m_midiPort.inputChannel() ).
			arg( m_midiPort.inputController() ) );
}




void MidiCustomController::processInEvent( const MidiEvent& event, const MidiTime& time, f_cnt_t offset )
{
	unsigned char controllerNum;

	controllerNum = event.controllerNumber();

	if( m_midiPort.inputController() == controllerNum + 1 &&
			( m_midiPort.inputChannel() == event.channel() + 1 ||
				m_midiPort.inputChannel() == 0 ) )
		{

			m_MidiControllerValue=event.controllerValue();
			m_MidiKey=event.key();
			m_MidiPanning=event.panning();
			m_MidiPitchbend=event.pitchBend();
			m_MidiVelocity=event.velocity();

			switch ( m_MidiType=event.type() )
				{
				case MidiProgramChange:
				case MidiControlChange:
				case MidiNoteOn:
				case MidiNoteOff:
				case MidiKeyPressure:
					{
						unsigned char controllerVal = event.controllerValue();
						m_previousValue = m_lastValue;
						m_lastValue = (float)( controllerVal ) / MidiMaxControllerValue;
						emit valueChanged();
					}
					break;
				case MidiPitchBend:
					{
						unsigned int pitchVal = event.pitchBend();
						m_previousValue = m_lastValue;
						m_lastValue = (float)( pitchVal ) / MidiMaxPitchBend;
						emit valueChanged();
					}
					break;
				default:
					break;
				}
	}
}




void MidiCustomController::subscribeReadablePorts( const MidiPort::Map & _map )
{
	for( MidiPort::Map::ConstIterator it = _map.constBegin();
						it != _map.constEnd(); ++it )
	{
		m_midiPort.subscribeReadablePort( it.key(), *it );
	}
}




void MidiCustomController::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	Controller::saveSettings( _doc, _this );
	m_midiPort.saveSettings( _doc, _this );

}




void MidiCustomController::loadSettings( const QDomElement & _this )
{
	Controller::loadSettings( _this );

	m_midiPort.loadSettings( _this );

	updateName();
}




QString MidiCustomController::nodeName() const
{
	return( "MidiCustomController" );
}




ControllerDialog * MidiCustomController::createDialog( QWidget * _parent )
{
	return NULL;
}




