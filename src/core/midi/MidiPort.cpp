/*
 * MidiPort.cpp - abstraction of MIDI-ports which are part of LMMS's MIDI-
 *                sequencing system
 *
 * Copyright (c) 2005-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "MidiPort.h"
#include "MidiClient.h"
#include "song.h"



MidiPort::MidiPort( const QString & _name, MidiClient * _mc,
			MidiEventProcessor * _mep, model * _parent,
							Modes _mode ) :
	model( _parent ),
	m_readablePortsMenu( NULL ),
	m_writablePortsMenu( NULL ),
	m_midiClient( _mc ),
	m_midiEventProcessor( _mep ),
	m_mode( _mode ),
	m_inputChannelModel( 0, 0, MidiChannelCount, this,
						tr( "Input channel" ) ),
	m_outputChannelModel( 1, 1, MidiChannelCount, this,
						tr( "Output channel" ) ),
	m_inputControllerModel( 0, 0, MidiControllerCount, this,
						tr( "Input controller" )  ),
	m_outputControllerModel( 0, 0, MidiControllerCount, this,
						tr( "Output controller" )  ),
	m_fixedInputVelocityModel( -1, -1, MidiMaxVelocity, this,
						tr( "Fixed input velocity" )  ),
	m_fixedOutputVelocityModel( -1, -1, MidiMaxVelocity, this,
						tr( "Fixed output velocity" ) ),
	m_outputProgramModel( 1, 1, MidiProgramCount, this,
						tr( "Output MIDI program" )  ),
	m_readableModel( false, this, tr( "Receive MIDI-events" ) ),
	m_writableModel( false, this, tr( "Send MIDI-events" ) )
{
	m_midiClient->addPort( this );

	m_readableModel.setValue( m_mode == Input || m_mode == Duplex );
	m_writableModel.setValue( m_mode == Output || m_mode == Duplex );

	connect( &m_readableModel, SIGNAL( dataChanged() ),
				this, SLOT( updateMidiPortMode() ) );
	connect( &m_writableModel, SIGNAL( dataChanged() ),
				this, SLOT( updateMidiPortMode() ) );
	connect( &m_outputProgramModel, SIGNAL( dataChanged() ),
				this, SLOT( updateOutputProgram() ) );


	// when using with non-raw-clients we can provide buttons showing
	// our port-menus when being clicked
	if( m_midiClient->isRaw() == false )
	{
		updateReadablePorts();
		updateWritablePorts();

		// we want to get informed about port-changes!
		m_midiClient->connectRPChanged( this,
					SLOT( updateReadablePorts() ) );
		m_midiClient->connectWPChanged( this,
					SLOT( updateWritablePorts() ) );
	}

	updateMidiPortMode();
}




MidiPort::~MidiPort()
{
	// unsubscribe ports
	m_readableModel.setValue( false );
	m_writableModel.setValue( false );

	// and finally unregister ourself
	m_midiClient->removePort( this );
}




void MidiPort::setName( const QString & _name )
{
	setDisplayName( _name );
	m_midiClient->applyPortName( this );
}




void MidiPort::setMode( Modes _mode )
{
	m_mode = _mode;
	m_midiClient->applyPortMode( this );
}




void MidiPort::processInEvent( const midiEvent & _me, const midiTime & _time )
{
	// mask event
	if( inputEnabled() &&
		( inputChannel()-1 == _me.m_channel || inputChannel() == 0 ) )
	{
		if( _me.m_type == MidiNoteOn ||
			_me.m_type == MidiNoteOff ||
			_me.m_type == MidiKeyPressure )
		{
			if( _me.key() < 0 || _me.key() >= NumKeys )
			{
				return;
			}
		}
		midiEvent ev = _me;
		if( fixedInputVelocity() >= 0 && _me.velocity() > 0 )
		{
			ev.velocity() = fixedInputVelocity();
		}
		m_midiEventProcessor->processInEvent( ev, _time );
	}
}




void MidiPort::processOutEvent( const midiEvent & _me, const midiTime & _time )
{
	// mask event
	if( outputEnabled() && realOutputChannel() == _me.m_channel )
	{
		midiEvent ev = _me;
		// we use/display MIDI channels 1...16 but we need 0...15 for
		// the outside world
		if( ev.m_channel > 0 )
		{
			--ev.m_channel;
		}
		if( fixedOutputVelocity() >= 0 && _me.velocity() > 0 &&
			( _me.m_type == MidiNoteOn ||
					_me.m_type == MidiKeyPressure ) )
		{
			ev.velocity() = fixedOutputVelocity();
		}
		m_midiClient->processOutEvent( ev, _time, this );
	}
}




void MidiPort::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	m_inputChannelModel.saveSettings( _doc, _this, "inputchannel" );
	m_outputChannelModel.saveSettings( _doc, _this, "outputchannel" );
	m_inputControllerModel.saveSettings( _doc, _this, "inputcontroller" );
	m_outputControllerModel.saveSettings( _doc, _this, "outputcontroller" );
	m_fixedInputVelocityModel.saveSettings( _doc, _this,
							"fixedinputvelocity" );
	m_fixedOutputVelocityModel.saveSettings( _doc, _this,
							"fixedoutputvelocity" );
	m_outputProgramModel.saveSettings( _doc, _this, "outputprogram" );
	m_readableModel.saveSettings( _doc, _this, "readable" );
	m_writableModel.saveSettings( _doc, _this, "writable" );

	if( inputEnabled() )
	{
		QString rp;
		for( Map::ConstIterator it = m_readablePorts.begin();
					it != m_readablePorts.end(); ++it )
		{
			if( it.value() )
			{
				rp += it.key() + ",";
			}
		}
		// cut off comma
		if( rp.length() > 0 )
		{
			rp.truncate( rp.length() - 1 );
		}
		_this.setAttribute( "inports", rp );
	}

	if( outputEnabled() )
	{
		QString wp;
		for( Map::ConstIterator it = m_writablePorts.begin();
					it != m_writablePorts.end(); ++it )
		{
			if( it.value() )
			{
				wp += it.key() + ",";
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




void MidiPort::loadSettings( const QDomElement & _this )
{
	m_inputChannelModel.loadSettings( _this, "inputchannel" );
	m_outputChannelModel.loadSettings( _this, "outputchannel" );
	m_inputControllerModel.loadSettings( _this, "inputcontroller" );
	m_outputControllerModel.loadSettings( _this, "outputcontroller" );
	m_fixedInputVelocityModel.loadSettings( _this, "fixedinputvelocity" );
	m_fixedOutputVelocityModel.loadSettings( _this, "fixedoutputvelocity" );
	m_outputProgramModel.loadSettings( _this, "outputprogram" );
	m_readableModel.loadSettings( _this, "readable" );
	m_writableModel.loadSettings( _this, "writable" );

	// restore connections

	if( inputEnabled() )
	{
		QStringList rp = _this.attribute( "inports" ).split( ',' );
		for( Map::ConstIterator it = m_readablePorts.begin();
					it != m_readablePorts.end(); ++it )
		{
			if( it.value() != ( rp.indexOf( it.key() ) != -1 ) )
			{
				subscribeReadablePort( it.key() );
			}
		}
		emit readablePortsChanged();
	}

	if( outputEnabled() )
	{
		QStringList wp = _this.attribute( "outports" ).split( ',' );
		for( Map::ConstIterator it = m_writablePorts.begin();
					it != m_writablePorts.end(); ++it )
		{
			if( it.value() != ( wp.indexOf( it.key() ) != -1 ) )
			{
				subscribeWritablePort( it.key() );
			}
		}
		emit writablePortsChanged();
	}
}




void MidiPort::subscribeReadablePort( const QString & _port, bool _subscribe )
{
	m_readablePorts[_port] = _subscribe;
	// make sure, MIDI-port is configured for input
	if( _subscribe == true && !inputEnabled() )
	{
		m_readableModel.setValue( true );
	}
	m_midiClient->subscribeReadablePort( this, _port, _subscribe );
}




void MidiPort::subscribeWritablePort( const QString & _port, bool _subscribe )
{
	m_writablePorts[_port] = _subscribe;
	// make sure, MIDI-port is configured for output
	if( _subscribe == true && !outputEnabled() )
	{
		m_writableModel.setValue( true );
	}
	m_midiClient->subscribeWritablePort( this, _port, _subscribe );
}




void MidiPort::unsubscribeAllReadablePorts()
{
	for( Map::ConstIterator it = m_readablePorts.begin();
	    it != m_readablePorts.end(); ++it )
	{
		// subscribed?
		if( it.value() )
		{
			subscribeReadablePort( it.key(), false );
		}
	}
}




void MidiPort::unsubscribeAllWriteablePorts()
{
	for( Map::ConstIterator it = m_writablePorts.begin();
	    it != m_writablePorts.end(); ++it )
	{
		// subscribed?
		if( it.value() )
		{
			subscribeWritablePort( it.key(), false );
		}
	}
}




void MidiPort::updateMidiPortMode()
{
	// this small lookup-table makes everything easier
	static const Modes modeTable[2][2] =
	{
		{ Disabled, Output },
		{ Input, Duplex }
	} ;
	setMode( modeTable[m_readableModel.value()][m_writableModel.value()] );

	// check whether we have to dis-check items in connection-menu
	if( !inputEnabled() )
	{
		unsubscribeAllReadablePorts();
	}

	if( !outputEnabled() )
	{
		unsubscribeAllWriteablePorts();
	}

	emit readablePortsChanged();
	emit writablePortsChanged();
	emit modeChanged();

	engine::getSong()->setModified();
}




void MidiPort::updateReadablePorts()
{
	// first save all selected ports
	QStringList selected_ports;
	for( Map::ConstIterator it = m_readablePorts.begin();
					it != m_readablePorts.end(); ++it )
	{
		if( it.value() == true )
		{
			selected_ports.push_back( it.key() );
		}
	}

	m_readablePorts.clear();
	const QStringList & wp = m_midiClient->readablePorts();
	// now insert new ports and restore selections
	for( QStringList::ConstIterator it = wp.begin(); it != wp.end(); ++it )
	{
		m_readablePorts[*it] = ( selected_ports.indexOf( *it ) != -1 );
	}
	emit readablePortsChanged();
}




void MidiPort::updateWritablePorts()
{
	// first save all selected ports
	QStringList selected_ports;
	for( Map::ConstIterator it = m_writablePorts.begin();
					it != m_writablePorts.end(); ++it )
	{
		if( it.value() == true )
		{
			selected_ports.push_back( it.key() );
		}
	}

	m_writablePorts.clear();
	const QStringList & wp = m_midiClient->writablePorts();
	// now insert new ports and restore selections
	for( QStringList::ConstIterator it = wp.begin(); it != wp.end(); ++it )
	{
		m_writablePorts[*it] = ( selected_ports.indexOf( *it ) != -1 );
	}
	emit writablePortsChanged();
}




void MidiPort::updateOutputProgram()
{
	processOutEvent( midiEvent( MidiProgramChange,
					realOutputChannel(),
					outputProgram()-1 ), midiTime( 0 ) );
}



#include "moc_MidiPort.cxx"

