#ifndef SINGLE_SOURCE_COMPILE

/*
 * midi_controller.cpp - implementation of class midi-controller which handles
 *                      MIDI control change messages
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail.com>
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

#include <Qt/QtXml>
#include <QtCore/QObject>
#include <QtCore/QVector>


#include "song.h"
#include "engine.h"
#include "mixer.h"
#include "midi_client.h"
#include "midi_port.h"
#include "midi_controller.h"



midiController::midiController( model * _parent ) :
	controller( MidiController, _parent ),
	midiEventProcessor(),
	m_midiChannel( 0, 0, MIDI_CHANNEL_COUNT, this ),
	m_midiController( 0, 0, MIDI_CONTROLLER_COUNT, this ),
	m_midiPort( engine::getMixer()->getMIDIClient()->addPort(
			this, tr( "unnamed_channel" ) ) ),
	m_lastValue( 0.0f )
{
	m_midiPort->setMode( midiPort::Input );
	connect( &m_midiChannel, SIGNAL( dataChanged() ),
			this, SLOT( updateMidiPort() ) );
	connect( &m_midiController, SIGNAL( dataChanged() ),
			this, SLOT( updateMidiPort() ) );
}



midiController::~midiController()
{
	m_midiChannel.disconnect( this );
	m_midiController.disconnect( this );

	engine::getMixer()->getMIDIClient()->removePort( m_midiPort );
}




float midiController::value( int _offset )
{
	return m_lastValue;
}




void midiController::updateMidiPort( void )
{
	m_midiPort->setInputChannel( m_midiChannel.value() - 1 );
	setName( QString("MIDI ch%1 ctrl%2").
			arg( m_midiChannel.value() ).
			arg( m_midiController.value() ) );
}




void midiController::processInEvent( const midiEvent & _me,
					const midiTime & _time, bool _lock )
{
	Uint8 controllerNum;
	const Uint8 * bytes;
	switch( _me.m_type )
	{
		case CONTROL_CHANGE:
			bytes = _me.m_data.m_bytes;
			controllerNum = _me.m_data.m_bytes[0] & 0x7F;

			if( m_midiController.value() == controllerNum + 1 &&
					( m_midiChannel.value() == _me.m_channel + 1 ||
					  m_midiChannel.value() == 0 ) )
			{
				Uint8 val = _me.m_data.m_bytes[2] & 0x7F;
				m_lastValue = (float)( val ) / 127.0f;
				emit valueChanged();
			}
			break;

		default:
			// Don't care - maybe add special cases for pitch and mod later
			break;
	}
}




void midiController::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	controller::saveSettings( _doc, _this );

	m_midiChannel.saveSettings( _doc, _this, "channel" );
	m_midiController.saveSettings( _doc, _this, "controller" );
}




void midiController::loadSettings( const QDomElement & _this )
{
	controller::loadSettings( _this );

	m_midiChannel.loadSettings( _this, "channel" );
	m_midiController.loadSettings( _this, "controller" );

	updateMidiPort();
}




QString midiController::nodeName( void ) const
{
	return( "midicontroller" );
}




controllerDialog * midiController::createDialog( QWidget * _parent )
{
	return NULL;
}


#include "midi_controller.moc"


#endif

