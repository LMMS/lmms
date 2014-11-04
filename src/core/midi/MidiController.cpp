/*
 * MidiController.cpp - implementation of class midi-controller which handles
 *                      MIDI control change messages
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail.com>
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

#include <QtXml/QDomElement>
#include <QtCore/QObject>
#include <QtCore/QVector>

#include "song.h"
#include "engine.h"
#include "Mixer.h"
#include "MidiClient.h"
#include "MidiController.h"


MidiController::MidiController( Model * _parent ) :
	Controller( Controller::MidiController, _parent, tr( "MIDI Controller" ) ),
	MidiEventProcessor(),
	m_midiPort( tr( "unnamed_midi_controller" ),
			engine::mixer()->midiClient(), this, this, MidiPort::Input ),
	m_lastValue( 0.0f )
{
	connect( &m_midiPort, SIGNAL( modeChanged() ),
			this, SLOT( updateName() ) );
}




MidiController::~MidiController()
{
}




float MidiController::value( int _offset )
{
	return m_lastValue;
}




void MidiController::updateName()
{
	setName( QString("MIDI ch%1 ctrl%2").
			arg( m_midiPort.inputChannel() ).
			arg( m_midiPort.inputController() ) );
}




void MidiController::processInEvent( const MidiEvent& event, const MidiTime& time, f_cnt_t offset )
{
	unsigned char controllerNum;
	switch( event.type() )
	{
		case MidiControlChange:
			controllerNum = event.controllerNumber();

			if( m_midiPort.inputController() == controllerNum + 1 &&
					( m_midiPort.inputChannel() == event.channel() + 1 ||
					  m_midiPort.inputChannel() == 0 ) )
			{
				unsigned char val = event.controllerValue();
				m_lastValue = (float)( val ) / 127.0f;
				emit valueChanged();
			}
			break;

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




ControllerDialog * MidiController::createDialog( QWidget * _parent )
{
	return NULL;
}


#include "moc_MidiController.cxx"

