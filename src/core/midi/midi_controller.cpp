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

#include <QtXml/QDomElement>
#include <QtCore/QObject>
#include <QtCore/QVector>


#include "song.h"
#include "engine.h"
#include "mixer.h"
#include "midi_client.h"
#include "midi_controller.h"



midiController::midiController( model * _parent ) :
	controller( MidiController, _parent, tr( "MIDI Controller" ) ),
	midiEventProcessor(),
	m_midiPort( tr( "unnamed_midi_controller" ),
			engine::getMixer()->getMidiClient(), this, this,
							midiPort::Input ),
	m_lastValue( 0.0f )
{
	connect( &m_midiPort, SIGNAL( modeChanged() ),
			this, SLOT( updateName() ) );
}




midiController::~midiController()
{
}




float midiController::value( int _offset )
{
	return m_lastValue;
}




void midiController::updateName( void )
{
	setName( QString("MIDI ch%1 ctrl%2").
			arg( m_midiPort.inputChannel() ).
			arg( m_midiPort.inputController() ) );
}




void midiController::processInEvent( const midiEvent & _me,
						const midiTime & _time )
{
	Uint8 controllerNum;
	const Uint8 * bytes;
	switch( _me.m_type )
	{
		case MidiControlChange:
			bytes = _me.m_data.m_bytes;
			controllerNum = _me.m_data.m_bytes[0] & 0x7F;

			if( m_midiPort.inputController() == controllerNum + 1 &&
					( m_midiPort.inputChannel() == _me.m_channel + 1 ||
					  m_midiPort.inputChannel() == 0 ) )
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




void midiController::subscribeReadablePorts( const midiPort::map & _map )
{
	for( midiPort::map::const_iterator it = _map.constBegin();
						it != _map.constEnd(); ++it )
	{
		m_midiPort.subscribeReadablePort( it.key(), *it );
	}
}




void midiController::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	controller::saveSettings( _doc, _this );
	m_midiPort.saveSettings( _doc, _this );

}




void midiController::loadSettings( const QDomElement & _this )
{
	controller::loadSettings( _this );

	m_midiPort.loadSettings( _this );

	updateName();
}




QString midiController::nodeName( void ) const
{
	return( "midicontroller" );
}




controllerDialog * midiController::createDialog( QWidget * _parent )
{
	return NULL;
}


#include "moc_midi_controller.cxx"


#endif

