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


#include <QtXml/QDomElement>

#include "midi_port.h"
#include "midi_client.h"
#include "song.h"



midiPort::midiPort( const QString & _name, midiClient * _mc,
			midiEventProcessor * _mep, model * _parent,
							Modes _mode ) :
	model( _parent ),
	m_readablePortsMenu( NULL ),
	m_writablePortsMenu( NULL ),
	m_midiClient( _mc ),
	m_midiEventProcessor( _mep ),
	m_name( _name ),
	m_mode( _mode ),
	m_inputChannelModel( 0, 0, MidiChannelCount, this,
						tr( "Input channel" ) ),
	m_outputChannelModel( 1, 1, MidiChannelCount, this,
						tr( "Output channel" ) ),
	m_inputControllerModel( 0, 0, MidiControllerCount, this,
						tr( "Input controller" )  ),
	m_outputControllerModel( 0, 0, MidiControllerCount, this,
						tr( "Output controller" )  ),
	m_readableModel( FALSE, this, tr( "Receive MIDI-events" ) ),
	m_writableModel( FALSE, this, tr( "Send MIDI-events" ) ),
	m_defaultVelocityInEnabledModel( FALSE, this,
					tr( "Default input velocity" ) ),
	m_defaultVelocityOutEnabledModel( FALSE, this,
					tr( "Default output velocity" ) )
{
	m_midiClient->addPort( this );

	m_readableModel.setValue( m_mode == Input || m_mode == Duplex );
	m_writableModel.setValue( m_mode == Output || m_mode == Duplex );

	connect( &m_readableModel, SIGNAL( dataChanged() ),
				this, SLOT( updateMidiPortMode() ) );
	connect( &m_writableModel, SIGNAL( dataChanged() ),
				this, SLOT( updateMidiPortMode() ) );


	// when using with non-raw-clients we can provide buttons showing
	// our port-menus when being clicked
	if( m_midiClient->isRaw() == FALSE )
	{
		updateReadablePorts();
		updateWriteablePorts();

		// we want to get informed about port-changes!
		m_midiClient->connectRPChanged( this,
					SLOT( updateReadablePorts() ) );
		m_midiClient->connectWPChanged( this,
					SLOT( updateWriteablePorts() ) );
	}

	updateMidiPortMode();
}




midiPort::~midiPort()
{
	// unsubscribe ports
	m_readableModel.setValue( FALSE );
	m_writableModel.setValue( FALSE );

	// and finally unregister ourself
	m_midiClient->removePort( this );
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
		if( m_defaultVelocityInEnabledModel.value() == TRUE &&
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
	if( outputEnabled() &&
		( outputChannel()-1 == _me.m_channel && outputChannel() != 0 ) )
	{
		midiEvent ev = _me;
		if( m_defaultVelocityOutEnabledModel.value() == TRUE &&
							_me.velocity() > 0 )
		{
			ev.velocity() = DefaultVolume;
		}
		m_midiClient->processOutEvent( ev, _time, this );
	}
}




void midiPort::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	m_inputChannelModel.saveSettings( _doc, _this, "inputchannel" );
	m_outputChannelModel.saveSettings( _doc, _this, "outputchannel" );
	m_inputControllerModel.saveSettings( _doc, _this, "inputcontroller" );
	m_outputControllerModel.saveSettings( _doc, _this, "outputcontroller" );
	m_readableModel.saveSettings( _doc, _this, "readable" );
	m_writableModel.saveSettings( _doc, _this, "writable" );
	m_defaultVelocityInEnabledModel.saveSettings( _doc, _this, "defvelin" );
	m_defaultVelocityOutEnabledModel.saveSettings( _doc, _this,
								"defvelout" );

	if( inputEnabled() )
	{
		QString rp;
		for( midiPort::map::iterator it = m_readablePorts.begin();
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
		for( map::const_iterator it = m_writablePorts.begin();
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




void midiPort::loadSettings( const QDomElement & _this )
{
	m_inputChannelModel.loadSettings( _this, "inputchannel" );
	m_outputChannelModel.loadSettings( _this, "outputchannel" );
	m_inputControllerModel.loadSettings( _this, "inputcontroller" );
	m_outputControllerModel.loadSettings( _this, "outputcontroller" );
	m_readableModel.loadSettings( _this, "readable" );
	m_writableModel.loadSettings( _this, "writable" );
	m_defaultVelocityInEnabledModel.loadSettings( _this, "defvelin" );
	m_defaultVelocityOutEnabledModel.loadSettings( _this, "defvelout" );

	// restore connections

	if( inputEnabled() )
	{
		QStringList rp = _this.attribute( "inports" ).split( ',' );
		for( map::const_iterator it = m_readablePorts.begin();
					it != m_readablePorts.end(); ++it )
		{
			if( it.value() != ( rp.indexOf( it.key() ) != -1 ) )
			{
				subscribeReadablePort( it.key() );
			}
		}
	}

	if( outputEnabled() )
	{
		QStringList wp = _this.attribute( "outports" ).split( ',' );
		for( map::const_iterator it = m_writablePorts.begin();
					it != m_writablePorts.end(); ++it )
		{
			if( it.value() != ( wp.indexOf( it.key() ) != -1 ) )
			{
				subscribeReadablePort( it.key() );
			}
		}
	}
}




void midiPort::updateMidiPortMode( void )
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
		for( map::const_iterator it = m_readablePorts.begin();
					it != m_readablePorts.end(); ++it )
		{
			// subscribed?
			if( it.value() )
			{
				subscribeReadablePort( it.key(), FALSE );
			}
		}
	}

	if( !outputEnabled() )
	{
		for( map::const_iterator it = m_writablePorts.begin();
					it != m_writablePorts.end(); ++it )
		{
			// subscribed?
			if( it.value() )
			{
				subscribeWriteablePort( it.key(), FALSE );
			}
		}
	}

	emit readablePortsChanged();
	emit writeablePortsChanged();
	emit modeChanged();

	engine::getSong()->setModified();
}




void midiPort::updateReadablePorts( void )
{
	// first save all selected ports
	QStringList selected_ports;
	for( midiPort::map::iterator it = m_readablePorts.begin();
					it != m_readablePorts.end(); ++it )
	{
		if( it.value() == TRUE )
		{
			selected_ports.push_back( it.key() );
		}
	}

	m_readablePorts.clear();
	const QStringList & wp = m_midiClient->readablePorts();
	// now insert new ports and restore selections
	for( QStringList::const_iterator it = wp.begin(); it != wp.end(); ++it )
	{
		m_readablePorts[*it] = ( selected_ports.indexOf( *it ) != -1 );
	}
	emit readablePortsChanged();
}




void midiPort::updateWriteablePorts( void )
{
	// first save all selected ports
	QStringList selected_ports;
	for( midiPort::map::iterator it = m_writablePorts.begin();
					it != m_writablePorts.end(); ++it )
	{
		if( it.value() == TRUE )
		{
			selected_ports.push_back( it.key() );
		}
	}

	m_writablePorts.clear();
	const QStringList & wp = m_midiClient->writeablePorts();
	// now insert new ports and restore selections
	for( QStringList::const_iterator it = wp.begin(); it != wp.end(); ++it )
	{
		m_writablePorts[*it] = ( selected_ports.indexOf( *it ) != -1 );
	}
	emit writeablePortsChanged();
}




void midiPort::subscribeReadablePort( const QString & _port, bool _subscribe )
{
	m_readablePorts[_port] = _subscribe;
	// make sure, MIDI-port is configured for input
	if( _subscribe == TRUE && !inputEnabled() )
	{
		m_readableModel.setValue( TRUE );
	}
	m_midiClient->subscribeReadablePort( this, _port, _subscribe );
}




void midiPort::subscribeWriteablePort( const QString & _port, bool _subscribe )
{
	m_writablePorts[_port] = _subscribe;
	// make sure, MIDI-port is configured for output
	if( _subscribe == TRUE && !outputEnabled() )
	{
		m_writableModel.setValue( TRUE );
	}
	m_midiClient->subscribeWriteablePort( this, _port, _subscribe );
}



#include "moc_midi_port.cxx"


#endif
