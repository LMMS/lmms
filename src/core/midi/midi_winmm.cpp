/*
 * midi_winmm.cpp - WinMM MIDI client
 *
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QtGui/QLabel>
#include <QtGui/QLineEdit>


#include "midi_winmm.h"
#include "config_mgr.h"
#include "engine.h"
#include "gui_templates.h"
#include "midi_port.h"
#include "note.h"


#ifdef LMMS_BUILD_WIN32


midiWinMM::midiWinMM( void ) :
	midiClient(),
	m_deviceListUpdateTimer( this )
{
	// initial list-update
	updateDeviceList();

	connect( &m_deviceListUpdateTimer, SIGNAL( timeout() ),
					this, SLOT( updateDeviceList() ) );
	// we check for port-changes every second
//	m_deviceListUpdateTimer.start( 1000 );
}




midiWinMM::~midiWinMM()
{
	closeDevices();
}




QString midiWinMM::probeDevice( void )
{
	QString dev = configManager::inst()->value( "midiwinmm", "device" );
	if( dev == "" )
	{
		if( getenv( "MIDIDEV" ) != NULL )
		{
			return( getenv( "MIDIDEV" ) );
		}
		return( "default" );
	}
	return( dev );
}




void midiWinMM::processOutEvent( const midiEvent & _me,
						const midiTime & _time,
						const midiPort * _port )
{
/*	// HACK!!! - need a better solution which isn't that easy since we
	// cannot store const-ptrs in our map because we need to call non-const
	// methods of MIDI-port - it's a mess...
	midiPort * p = const_cast<midiPort *>( _port );

	snd_seq_event_t ev;
	snd_seq_ev_clear( &ev );
	snd_seq_ev_set_source( &ev, ( m_portIDs[p][1] != -1 ) ?
					m_portIDs[p][1] : m_portIDs[p][0] );
	snd_seq_ev_set_subs( &ev );
	snd_seq_ev_schedule_tick( &ev, m_queueID, 1,
						static_cast<Sint32>( _time ) );
	ev.queue =  m_queueID;
	switch( _me.m_type )
	{
		case MidiNoteOn:
			snd_seq_ev_set_noteon( &ev,
						_port->outputChannel(),
						_me.key() + KeysPerOctave,
						_me.velocity() );
			break;

		case MidiNoteOff:
			snd_seq_ev_set_noteoff( &ev,
						_port->outputChannel(),
						_me.key() + KeysPerOctave,
						_me.velocity() );
			break;

		case MidiKeyPressure:
			snd_seq_ev_set_keypress( &ev,
						_port->outputChannel(),
						_me.key() + KeysPerOctave,
						_me.velocity() );
			break;

		case MidiControlChange:
			snd_seq_ev_set_controller( &ev,
						_port->outputChannel(),
						_me.m_data.m_param[0],
						_me.m_data.m_param[1] );
			break;

		case MidiProgramChange:
			snd_seq_ev_set_pgmchange( &ev,
						_port->outputChannel(),
						_me.m_data.m_param[0] );
			break;

		case MidiChannelPressure:
			snd_seq_ev_set_chanpress( &ev,
						_port->outputChannel(),
						_me.m_data.m_param[0] );
			break;

		case MidiPitchBend:
			snd_seq_ev_set_pitchbend( &ev,
						_port->outputChannel(),
						_me.m_data.m_param[0] - 8192 );
			break;

		default:
			printf( "ALSA-sequencer: unhandled output event %d\n",
							(int) _me.m_type );
			return;
	}*/
}




void midiWinMM::applyPortMode( midiPort * _port )
{
	// make sure no subscriptions exist which are not possible with
	// current port-mode
	if( !_port->inputEnabled() )
	{
		for( subMap::iterator it = m_inputSubs.begin();
						it != m_inputSubs.end(); ++it )
		{
			it.value().removeAll( _port );
		}
	}

	if( _port->outputEnabled() )
	{
		for( subMap::iterator it = m_outputSubs.begin();
						it != m_outputSubs.end(); ++it )
		{
			it.value().removeAll( _port );
		}
	}
}




void midiWinMM::removePort( midiPort * _port )
{
	for( subMap::iterator it = m_inputSubs.begin();
						it != m_inputSubs.end(); ++it )
	{
		it.value().removeAll( _port );
	}
	for( subMap::iterator it = m_outputSubs.begin();
						it != m_outputSubs.end(); ++it )
	{
		it.value().removeAll( _port );
	}
	midiClient::removePort( _port );
}




void midiWinMM::subscribeReadablePort( midiPort * _port,
						const QString & _dest,
						bool _subscribe )
{
	if( _port->inputEnabled() == FALSE )
	{
		printf( "port %s can't be (un)subscribed!\n",
					_port->name().toAscii().constData() );
		return;
	}

	if( m_inputSubs.contains( _dest ) )
	{
		m_inputSubs[_dest].removeAll( _port );
		if( _subscribe )
		{
			m_inputSubs[_dest].push_back( _port );
		}
	}
}




void midiWinMM::subscribeWriteablePort( midiPort * _port,
						const QString & _dest,
						bool _subscribe )
{
	if( _port->outputEnabled() == FALSE && _subscribe == FALSE )
	{
		printf( "port %s can't be (un)subscribed!\n",
					_port->name().toAscii().constData() );
		return;
	}

	if( m_outputSubs.contains( _dest ) )
	{
		m_outputSubs[_dest].removeAll( _port );
		if( _subscribe )
		{
			m_outputSubs[_dest].push_back( _port );
		}
	}
}




void CALLBACK midiWinMM::inputCallback( HMIDIIN _hm, UINT _msg, DWORD_PTR _inst,
					DWORD_PTR _param1, DWORD_PTR _param2 )
{
printf("input callback %d\n", (int) _hm );
	if( _msg == MIM_DATA )
	{
		( (midiWinMM *) _inst )->handleInputEvent( _hm, _param1 );
	}
}




void midiWinMM::handleInputEvent( HMIDIIN _hm, DWORD _ev )
{
	const int cmd = _ev & 0xff;
	if( cmd == MidiActiveSensing )
	{
		return;
	}
	const int par1 = ( _ev >> 8 ) & 0xff;
	const int par2 = _ev >> 16;
	const MidiEventTypes cmdtype =
				static_cast<MidiEventTypes>( cmd & 0xf0 );
	const int chan = cmd & 0x0f;

		printf("%d\n", cmd );
	const QString d = m_inputDevices.value( _hm );
	if( d.isEmpty() )
	{
printf("return\n");
		return;
	}

	const midiPortList & l = m_inputSubs[d];
	for( midiPortList::const_iterator it = l.begin(); it != l.end(); ++it )
	{
		switch( cmdtype )
		{
			case MidiNoteOn:
			case MidiNoteOff:
			case MidiKeyPressure:
				( *it )->processInEvent(
					midiEvent( cmdtype, chan,
							par1 - KeysPerOctave,
							par2 & 0xff ),
								midiTime() );
				break;

			case MidiControlChange:
			case MidiProgramChange:
			case MidiChannelPressure:
				( *it )->processInEvent(
					midiEvent( cmdtype, chan, par1,
							par2 & 0xff ),
								midiTime() );
		
				break;

			case MidiPitchBend:
				( *it )->processInEvent(
					midiEvent( cmdtype, chan,
							par1 + par2*128, 0 ),
								midiTime() );
			default:
				printf( "WinMM-MIDI: unhandled input "
							"event %d\n", cmdtype );
				break;
		}
	}
}




void midiWinMM::updateDeviceList( void )
{
	closeDevices();
	openDevices();
//	if( m_readablePorts != readable_ports )
	{
//		m_readablePorts = readable_ports;
		emit( readablePortsChanged() );
	}

//	if( m_writeablePorts != writeable_ports )
	{
//		m_writeablePorts = writeable_ports;
		emit( writeablePortsChanged() );
	}
}



void midiWinMM::closeDevices( void )
{
	for( QList<HMIDIIN>::const_iterator it = m_inputDevices.keys().begin();
				it != m_inputDevices.keys().end(); ++it )
	{
		midiInReset( *it );
		midiInClose( *it );
	}
	for( QList<HMIDIOUT>::const_iterator it =
					m_outputDevices.keys().begin();
				it != m_outputDevices.keys().end(); ++it )
	{
		//midiOutStop( *it );
		midiOutClose( *it );
	}
}




void midiWinMM::openDevices( void )
{
	m_inputDevices.clear();
	for( int i = 0; i < midiInGetNumDevs(); ++i )
	{
printf("opening %d\n", i );
		HMIDIIN hm = 0;
		MMRESULT res = midiInOpen( &hm, i, (DWORD) &inputCallback,
						(DWORD_PTR) this,
							CALLBACK_FUNCTION );
printf("opened %d\n", (int) hm);
		if( res == MMSYSERR_NOERROR )
		{
			midiInStart( hm );
printf("started\n" );
			MIDIINCAPS c;
			midiInGetDevCaps( (UINT) hm, &c, sizeof( c ) );
			m_inputDevices[hm] = c.szPname;
printf("caps done %s\n", c.szPname );
		}
	}
}




midiWinMM::setupWidget::setupWidget( QWidget * _parent ) :
	midiClient::setupWidget( midiWinMM::name(), _parent )
{
	m_device = new QLineEdit( midiWinMM::probeDevice(), this );
	m_device->setGeometry( 10, 20, 160, 20 );

	QLabel * dev_lbl = new QLabel( tr( "DEVICE" ), this );
	dev_lbl->setFont( pointSize<6>( dev_lbl->font() ) );
	dev_lbl->setGeometry( 10, 40, 160, 10 );
}




midiWinMM::setupWidget::~setupWidget()
{
}




void midiWinMM::setupWidget::saveSettings( void )
{
	configManager::inst()->setValue( "midiwinmm", "device",
							m_device->text() );
}


#include "midi_winmm.moc"


#endif

