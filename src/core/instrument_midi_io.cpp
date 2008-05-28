#ifndef SINGLE_SOURCE_COMPILE

/*
 * instrument_midi_io.cpp - class instrumentMidiIO
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


#include <Qt/QtXml>


#include "instrument_midi_io.h"
#include "engine.h"
#include "instrument_track.h"
#include "midi_client.h"
#include "midi_port.h"
#include "song.h"



instrumentMidiIO::instrumentMidiIO( instrumentTrack * _instrument_track,
							midiPort * _port ) :
	model( _instrument_track ),
	m_instrumentTrack( _instrument_track ),
	m_midiPort( _port ),
        m_inputChannelModel( m_midiPort->inputChannel() + 1,
				0, MIDI_CHANNEL_COUNT, this ),
        m_outputChannelModel( m_midiPort->outputChannel() + 1,
				1, MIDI_CHANNEL_COUNT, this ),
        m_receiveEnabledModel( FALSE, this ),
        m_sendEnabledModel( FALSE, this ),
        m_defaultVelocityInEnabledModel( FALSE, this ),
        m_defaultVelocityOutEnabledModel( FALSE, this )
{
	m_inputChannelModel.setTrack( m_instrumentTrack );
	m_outputChannelModel.setTrack( m_instrumentTrack );
	m_receiveEnabledModel.setTrack( m_instrumentTrack );
	m_defaultVelocityInEnabledModel.setTrack( m_instrumentTrack );
	m_sendEnabledModel.setTrack( m_instrumentTrack );
	m_defaultVelocityOutEnabledModel.setTrack( m_instrumentTrack );

	connect( &m_inputChannelModel, SIGNAL( dataChanged() ),
				this, SLOT( updateInputChannel() ) );
	connect( &m_outputChannelModel, SIGNAL( dataChanged() ),
				this, SLOT( updateOutputChannel() ) );
	connect( &m_receiveEnabledModel, SIGNAL( dataChanged() ),
				this, SLOT( updateMidiPortMode() ) );
	connect( &m_defaultVelocityInEnabledModel, SIGNAL( dataChanged() ),
				this, SLOT( updateDefaultVelIn() ) );
	connect( &m_sendEnabledModel, SIGNAL( dataChanged() ),
				this, SLOT( updateMidiPortMode() ) );
	connect( &m_defaultVelocityOutEnabledModel, SIGNAL( dataChanged() ),
				this, SLOT( updateDefaultVelOut() ) );

	updateInputChannel();
	updateOutputChannel();


	const midiPort::Modes m = m_midiPort->mode();
	m_receiveEnabledModel.setValue( m == midiPort::Input ||
							m == midiPort::Duplex );
	m_sendEnabledModel.setValue( m == midiPort::Output ||
							m == midiPort::Duplex );

	// when using with non-raw-clients we can provide buttons showing
	// our port-menus when being clicked
	midiClient * mc = engine::getMixer()->getMIDIClient();
	if( mc->isRaw() == FALSE )
	{
		updateReadablePorts();
		updateWriteablePorts();

		// we want to get informed about port-changes!
		mc->connectRPChanged( this, SLOT( updateReadablePorts() ) );
		mc->connectWPChanged( this, SLOT( updateWriteablePorts() ) );
	}
}




instrumentMidiIO::~instrumentMidiIO()
{
}




void instrumentMidiIO::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	m_inputChannelModel.saveSettings( _doc, _this, "inputchannel" );
	m_outputChannelModel.saveSettings( _doc, _this, "outputchannel" );
	m_receiveEnabledModel.saveSettings( _doc, _this, "receive" );
	m_sendEnabledModel.saveSettings( _doc, _this, "send" );
	m_defaultVelocityInEnabledModel.saveSettings( _doc, _this, "defvelin" );
	m_defaultVelocityOutEnabledModel.saveSettings( _doc, _this, "defvelout" );

	if( m_receiveEnabledModel.value() == TRUE )
	{
		QString rp;
		for( midiPortMap::iterator it = m_readablePorts.begin();
					it != m_readablePorts.end(); ++it )
		{
			if( it->second )
			{
				rp += it->first + ",";
			}
		}
		// cut off comma
		if( rp.length() > 0 )
		{
			rp.truncate( rp.length() - 1 );
		}
		_this.setAttribute( "inports", rp );
	}

	if( m_sendEnabledModel.value() == TRUE )
	{
		QString wp;
		for( midiPortMap::iterator it = m_writeablePorts.begin();
					it != m_writeablePorts.end(); ++it )
		{
			if( it->second )
			{
				wp += it->first + ",";
			}
		}
		// cut off comma
		if( wp.length() > 0 )
		{
			wp.truncate( wp.length() - 1 );
		}
		_this.setAttribute( "outports", wp );
	}
}




void instrumentMidiIO::loadSettings( const QDomElement & _this )
{
	m_inputChannelModel.loadSettings( _this, "inputchannel" );
	m_outputChannelModel.loadSettings( _this, "outputchannel" );
	m_receiveEnabledModel.loadSettings( _this, "receive" );
	m_sendEnabledModel.loadSettings( _this, "send" );
	m_defaultVelocityInEnabledModel.loadSettings( _this, "defvelin" );
	m_defaultVelocityOutEnabledModel.loadSettings( _this, "defvelout" );

	// restore connections

	if( m_receiveEnabledModel.value() == TRUE )
	{
		QStringList rp = _this.attribute( "inports" ).split( ',' );
		for( midiPortMap::iterator it = m_readablePorts.begin();
					it != m_readablePorts.end(); ++it )
		{
			if( it->second != ( rp.indexOf( it->first ) != -1 ) )
			{
				it->second = TRUE;
				activatedReadablePort( *it );
			}
		}
	}

	if( m_sendEnabledModel.value() == TRUE )
	{
		QStringList wp = _this.attribute( "outports" ).split( ',' );
		for( midiPortMap::iterator it = m_writeablePorts.begin();
					it != m_writeablePorts.end(); ++it )
		{
			if( it->second != ( wp.indexOf( it->first ) != -1 ) )
			{
				it->second = TRUE;
				activatedWriteablePort( *it );
			}
		}
	}
}




void instrumentMidiIO::updateInputChannel( void )
{
	m_midiPort->setInputChannel( m_inputChannelModel.value() - 1 );
	engine::getSong()->setModified();
}




void instrumentMidiIO::updateOutputChannel( void )
{
	m_midiPort->setOutputChannel( m_outputChannelModel.value() - 1 );
	engine::getSong()->setModified();
}




void instrumentMidiIO::updateDefaultVelIn( void )
{
	m_midiPort->enableDefaultVelocityForInEvents(
				m_defaultVelocityInEnabledModel.value() );
}




void instrumentMidiIO::updateDefaultVelOut( void )
{
	m_midiPort->enableDefaultVelocityForOutEvents(
				m_defaultVelocityOutEnabledModel.value() );
}




void instrumentMidiIO::updateMidiPortMode( void )
{
	// this small lookup-table makes everything easier
	static const midiPort::Modes modeTable[2][2] =
	{
		{ midiPort::Disabled, midiPort::Output },
		{ midiPort::Input, midiPort::Duplex }
	} ;
	m_midiPort->setMode( modeTable[m_receiveEnabledModel.value()]
					[m_sendEnabledModel.value()] );

	// check whether we have to dis-check items in connection-menu
	if( m_receiveEnabledModel.value() == FALSE )
	{
		for( midiPortMap::iterator it = m_readablePorts.begin();
					it != m_readablePorts.end(); ++it )
		{
			if( it->second == TRUE )
			{
				it->second = FALSE;
				activatedReadablePort( *it );
			}
		}
	}

	if( m_sendEnabledModel.value() == FALSE )
	{
		for( midiPortMap::iterator it = m_writeablePorts.begin();
					it != m_writeablePorts.end(); ++it )
		{
			if( it->second == TRUE )
			{
				it->second = FALSE;
				activatedWriteablePort( *it );
			}
		}
	}
	engine::getSong()->setModified();
}




void instrumentMidiIO::updateReadablePorts( void )
{
	// first save all selected ports
	QStringList selected_ports;
	for( midiPortMap::iterator it = m_readablePorts.begin();
					it != m_readablePorts.end(); ++it )
	{
		if( it->second == TRUE )
		{
			selected_ports.push_back( it->first );
		}
	}

	m_readablePorts.clear();
	const QStringList & wp = engine::getMixer()->getMIDIClient()->
								readablePorts();
	// now insert new ports and restore selections
	for( QStringList::const_iterator it = wp.begin(); it != wp.end(); ++it )
	{
		m_readablePorts.push_back( qMakePair( *it,
					selected_ports.indexOf( *it ) != -1 ) );
	}
	emit readablePortsChanged();
}




void instrumentMidiIO::updateWriteablePorts( void )
{
	// first save all selected ports
	QStringList selected_ports;
	for( midiPortMap::iterator it = m_writeablePorts.begin();
					it != m_writeablePorts.end(); ++it )
	{
		if( it->second == TRUE )
		{
			selected_ports.push_back( it->first );
		}
	}

	m_writeablePorts.clear();
	const QStringList & wp = engine::getMixer()->getMIDIClient()->
							writeablePorts();
	// now insert new ports and restore selections
	for( QStringList::const_iterator it = wp.begin(); it != wp.end(); ++it )
	{
		m_writeablePorts.push_back( qMakePair( *it,
					selected_ports.indexOf( *it ) != -1 ) );
	}
	emit writeablePortsChanged();
}




void instrumentMidiIO::activatedReadablePort(
					const descriptiveMidiPort & _port )
{
	// make sure, MIDI-port is configured for input
	if( _port.second == TRUE &&
		m_midiPort->mode() != midiPort::Input &&
		m_midiPort->mode() != midiPort::Duplex )
	{
		m_receiveEnabledModel.setValue( TRUE );
	}
	engine::getMixer()->getMIDIClient()->subscribeReadablePort( m_midiPort,
						_port.first, !_port.second );
}




void instrumentMidiIO::activatedWriteablePort(
					const descriptiveMidiPort & _port )
{
	// make sure, MIDI-port is configured for output
	if( _port.second == TRUE &&
		m_midiPort->mode() != midiPort::Output &&
		m_midiPort->mode() != midiPort::Duplex )
	{
		m_sendEnabledModel.setValue( TRUE );
	}
	engine::getMixer()->getMIDIClient()->subscribeWriteablePort( m_midiPort,
						_port.first, !_port.second );
}



#include "instrument_midi_io.moc"


#endif
