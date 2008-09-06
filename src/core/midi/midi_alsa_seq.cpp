#ifndef SINGLE_SOURCE_COMPILE

/*
 * midi_alsa_seq.cpp - ALSA-sequencer-client
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


#include <QtGui/QLabel>
#include <QtGui/QLineEdit>


#include "midi_alsa_seq.h"
#include "config_mgr.h"
#include "engine.h"
#include "gui_templates.h"
#include "song.h"
#include "midi_port.h"
#include "note.h"


#ifdef LMMS_HAVE_ALSA


midiALSASeq::midiALSASeq( void ) :
	midiClient(),
	m_seqHandle( NULL ),
	m_queueID( -1 ),
	m_quit( FALSE ),
	m_portListUpdateTimer( this )
{
	int err;
	if( ( err = snd_seq_open( &m_seqHandle,
					probeDevice().toAscii().constData(),
						SND_SEQ_OPEN_DUPLEX, 0 ) ) < 0 )
	{
		fprintf( stderr, "cannot open sequencer: %s\n",
							snd_strerror( err ) );
		return;
	}
	snd_seq_set_client_name( m_seqHandle, "LMMS" );


	m_queueID = snd_seq_alloc_queue( m_seqHandle );
	snd_seq_queue_tempo_t * tempo;
	snd_seq_queue_tempo_malloc( &tempo );
	snd_seq_queue_tempo_set_tempo( tempo, 6000000 /
					engine::getSong()->getTempo() );
	snd_seq_queue_tempo_set_ppq( tempo, 16 );
	snd_seq_set_queue_tempo( m_seqHandle, m_queueID, tempo );
	snd_seq_queue_tempo_free( tempo );

	snd_seq_start_queue( m_seqHandle, m_queueID, NULL );
	changeQueueTempo( engine::getSong()->getTempo() );
	connect( engine::getSong(), SIGNAL( tempoChanged( bpm_t ) ),
			this, SLOT( changeQueueTempo( bpm_t ) ) );

	// initial list-update
	updatePortList();

	connect( &m_portListUpdateTimer, SIGNAL( timeout() ),
					this, SLOT( updatePortList() ) );
	// we check for port-changes every second
	m_portListUpdateTimer.start( 1000 );

	// use a pipe to detect shutdown
	if( pipe( m_pipe ) == -1 )
	{
		perror( __FILE__ ": pipe" );
	}

	start( QThread::IdlePriority );
}




midiALSASeq::~midiALSASeq()
{
	if( isRunning() )
	{
		m_quit = TRUE;
		// wake up input queue
		write( m_pipe[1], "\n", 1 );
		wait( 1000 );

		snd_seq_stop_queue( m_seqHandle, m_queueID, NULL );
		snd_seq_free_queue( m_seqHandle, m_queueID );
		snd_seq_close( m_seqHandle );
	}
}




QString midiALSASeq::probeDevice( void )
{
	QString dev = configManager::inst()->value( "midialsaseq", "device" );
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




void midiALSASeq::processOutEvent( const midiEvent & _me,
						const midiTime & _time,
						const midiPort * _port )
{
	// HACK!!! - need a better solution which isn't that easy since we
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
			fprintf( stderr, "ALSA-sequencer: unhandled output "
					"event %d\n", (int) _me.m_type );
			return;
	}

	snd_seq_event_output( m_seqHandle, &ev );
	snd_seq_drain_output( m_seqHandle );

}




void midiALSASeq::applyPortMode( midiPort * _port )
{
	// determine port-capabilities
	unsigned int caps[2] = { 0, 0 };

	switch( _port->mode() )
	{
		case midiPort::Duplex:
			caps[1] |= SND_SEQ_PORT_CAP_READ |
						SND_SEQ_PORT_CAP_SUBS_READ;

		case midiPort::Input:
			caps[0] |= SND_SEQ_PORT_CAP_WRITE |
						SND_SEQ_PORT_CAP_SUBS_WRITE;
			break;

		case midiPort::Output:
			caps[0] |= SND_SEQ_PORT_CAP_READ |
						SND_SEQ_PORT_CAP_SUBS_READ;
			break;

		default:
			break;
	}

	for( int i = 0; i < 2; ++i )
	{
		if( caps[i] != 0 )
		{
			// no port there yet?
			if( m_portIDs[_port][i] == -1 )
			{
				// then create one;
				m_portIDs[_port][i] =
						snd_seq_create_simple_port(
							m_seqHandle,
					_port->name().toAscii().constData(),
							caps[i],
						SND_SEQ_PORT_TYPE_MIDI_GENERIC |
						SND_SEQ_PORT_TYPE_APPLICATION );
				continue;
			}
			// this C-API sucks!! normally we at least could create
			// a local snd_seq_port_info_t variable but the type-
			// info for this is hidden and we have to mess with
			// pointers...
			snd_seq_port_info_t * port_info;
			snd_seq_port_info_malloc( &port_info );
			snd_seq_get_port_info( m_seqHandle, m_portIDs[_port][i],
							port_info );
			snd_seq_port_info_set_capability( port_info, caps[i] );
			snd_seq_set_port_info( m_seqHandle, m_portIDs[_port][i],
							port_info );
			snd_seq_port_info_free( port_info );
		}
		// still a port there although no caps? ( = dummy port)
		else if( m_portIDs[_port][i] != -1 )
		{
			// then remove this port
			snd_seq_delete_simple_port( m_seqHandle,
							m_portIDs[_port][i] );
			m_portIDs[_port][i] = -1;
		}
	}

}




void midiALSASeq::applyPortName( midiPort * _port )
{
	for( int i = 0; i < 2; ++i )
	{
		if( m_portIDs[_port][i] == -1 )
		{
			continue;
		}
		// this C-API sucks!! normally we at least could create a local
		// snd_seq_port_info_t variable but the type-info for this is
		// hidden and we have to mess with pointers...
		snd_seq_port_info_t * port_info;
		snd_seq_port_info_malloc( &port_info );
		snd_seq_get_port_info( m_seqHandle, m_portIDs[_port][i],
							port_info );
		snd_seq_port_info_set_name( port_info,
					_port->name().toAscii().constData() );
		snd_seq_set_port_info( m_seqHandle, m_portIDs[_port][i],
							port_info );
		snd_seq_port_info_free( port_info );
	}
	// this small workaround would make qjackctl refresh it's MIDI-
	// connection-window since it doesn't update it automatically if only
	// the name of a client-port changes
/*	snd_seq_delete_simple_port( m_seqHandle,
			snd_seq_create_simple_port( m_seqHandle, "", 0,
					SND_SEQ_PORT_TYPE_APPLICATION ) );*/
}




void midiALSASeq::removePort( midiPort * _port )
{
	if( m_portIDs.contains( _port ) )
	{
		snd_seq_delete_simple_port( m_seqHandle, m_portIDs[_port][0] );
		snd_seq_delete_simple_port( m_seqHandle, m_portIDs[_port][1] );
		m_portIDs.remove( _port );
	}
	midiClient::removePort( _port );
}




void midiALSASeq::subscribeReadablePort( midiPort * _port,
						const QString & _dest,
						bool _subscribe )
{
	if( !m_portIDs.contains( _port ) || m_portIDs[_port][0] < 0 )
	{
		return;
	}
	snd_seq_addr_t sender;
	if( snd_seq_parse_address( m_seqHandle, &sender,
			_dest.section( ' ', 0, 0 ).toAscii().constData() ) )
	{
		fprintf( stderr, "error parsing sender-address!\n" );
		return;
	}
	snd_seq_port_info_t * port_info;
	snd_seq_port_info_malloc( &port_info );
	snd_seq_get_port_info( m_seqHandle, m_portIDs[_port][0], port_info );
	const snd_seq_addr_t * dest = snd_seq_port_info_get_addr( port_info );
	snd_seq_port_subscribe_t * subs;
	snd_seq_port_subscribe_malloc( &subs );
	snd_seq_port_subscribe_set_sender( subs, &sender );
	snd_seq_port_subscribe_set_dest( subs, dest );
	if( _subscribe )
	{
		snd_seq_subscribe_port( m_seqHandle, subs );
	}
	else
	{
		snd_seq_unsubscribe_port( m_seqHandle, subs );
	}
	snd_seq_port_subscribe_free( subs );
	snd_seq_port_info_free( port_info );
}




void midiALSASeq::subscribeWriteablePort( midiPort * _port,
						const QString & _dest,
						bool _subscribe )
{
	if( !m_portIDs.contains( _port ) || m_portIDs[_port][1] < 0 )
	{
		return;
	}
	snd_seq_addr_t dest;
	if( snd_seq_parse_address( m_seqHandle, &dest,
			_dest.section( ' ', 0, 0 ).toAscii().constData() ) )
	{
		fprintf( stderr, "error parsing dest-address!\n" );
		return;
	}
	snd_seq_port_info_t * port_info;
	snd_seq_port_info_malloc( &port_info );
	snd_seq_get_port_info( m_seqHandle, m_portIDs[_port][1] == -1,
								port_info );
	const snd_seq_addr_t * sender = snd_seq_port_info_get_addr( port_info );
	snd_seq_port_subscribe_t * subs;
	snd_seq_port_subscribe_malloc( &subs );
	snd_seq_port_subscribe_set_sender( subs, sender );
	snd_seq_port_subscribe_set_dest( subs, &dest );
	if( _subscribe )
	{
		snd_seq_subscribe_port( m_seqHandle, subs );
	}
	else
	{
		snd_seq_unsubscribe_port( m_seqHandle, subs );
	}
	snd_seq_port_subscribe_free( subs );
	snd_seq_port_info_free( port_info );
}




void midiALSASeq::run( void )
{
	// watch the pipe and sequencer input events
	int pollfd_count = snd_seq_poll_descriptors_count( m_seqHandle,
								POLLIN );
	struct pollfd * pollfd_set = new struct pollfd[pollfd_count + 1];
	snd_seq_poll_descriptors( m_seqHandle, pollfd_set + 1, pollfd_count,
								POLLIN );
	pollfd_set[0].fd = m_pipe[0];
	pollfd_set[0].events = POLLIN;
	++pollfd_count;

	while( m_quit == FALSE )
	{
		if( poll( pollfd_set, pollfd_count, -1 ) == -1 )
		{
			// gdb may interrupt the poll
			if( errno == EINTR )
			{
				continue;
			}
			perror( __FILE__ ": poll" );
		}
		// shutdown?
		if( pollfd_set[0].revents )
		{
			break;
		}

		do	// while event queue is not empty
		{

		snd_seq_event_t * ev;
		snd_seq_event_input( m_seqHandle, &ev );

		midiPort * dest = NULL;
		for( int i = 0; i < m_portIDs.size(); ++i )
		{
			if( m_portIDs.values()[i][0] == ev->dest.port )
			{
				dest = m_portIDs.keys()[i];
			}
		}

		if( dest == NULL )
		{
			continue;
		}

		switch( ev->type )
		{
			case SND_SEQ_EVENT_NOTEON:
				dest->processInEvent( midiEvent( MidiNoteOn,
							ev->data.note.channel,
							ev->data.note.note -
							KeysPerOctave,
							ev->data.note.velocity
							),
						midiTime( ev->time.tick ) );
				break;

			case SND_SEQ_EVENT_NOTEOFF:
				dest->processInEvent( midiEvent( MidiNoteOff,
							ev->data.note.channel,
							ev->data.note.note -
							KeysPerOctave,
							ev->data.note.velocity
							),
						midiTime( ev->time.tick) );
				break;

			case SND_SEQ_EVENT_KEYPRESS:
				dest->processInEvent( midiEvent(
								MidiKeyPressure,
							ev->data.note.channel,
							ev->data.note.note -
							KeysPerOctave,
							ev->data.note.velocity
							), midiTime() );
				break;

			case SND_SEQ_EVENT_CONTROLLER:
				dest->processInEvent( midiEvent(
							MidiControlChange,
						ev->data.control.channel,
						ev->data.control.param,
						ev->data.control.value ),
								midiTime() );
				break;

			case SND_SEQ_EVENT_PGMCHANGE:
				dest->processInEvent( midiEvent(
							MidiProgramChange,
						ev->data.control.channel,
						ev->data.control.param,
						ev->data.control.value ),
								midiTime() );
				break;

			case SND_SEQ_EVENT_CHANPRESS:
				dest->processInEvent( midiEvent(
							MidiChannelPressure,
						ev->data.control.channel,
						ev->data.control.param,
						ev->data.control.value ),
								midiTime() );
				break;

			case SND_SEQ_EVENT_PITCHBEND:
				dest->processInEvent( midiEvent( MidiPitchBend,
						ev->data.control.channel,
						ev->data.control.value + 8192,
							0 ), midiTime() );
				break;

			case SND_SEQ_EVENT_SENSING:
			case SND_SEQ_EVENT_CLOCK:
				break;

			default:
				fprintf( stderr,
					"ALSA-sequencer: unhandled input "
						"event %d\n", ev->type );
				break;
		}

		} while( snd_seq_event_input_pending( m_seqHandle, 0 ) > 0 );

	}

	delete[] pollfd_set;
}




void midiALSASeq::changeQueueTempo( bpm_t _bpm )
{
	snd_seq_change_queue_tempo( m_seqHandle, m_queueID,
					60000000 / (int) _bpm, NULL );
	snd_seq_drain_output( m_seqHandle );
}




void midiALSASeq::updatePortList( void )
{
	QStringList readable_ports;
	QStringList writeable_ports;

	// get input- and output-ports
	snd_seq_client_info_t * cinfo;
	snd_seq_port_info_t * pinfo;

	snd_seq_client_info_malloc( &cinfo );
	snd_seq_port_info_malloc( &pinfo );

	snd_seq_client_info_set_client( cinfo, -1 );
	while( snd_seq_query_next_client( m_seqHandle, cinfo ) >= 0 )
	{
		int client = snd_seq_client_info_get_client( cinfo );

		snd_seq_port_info_set_client( pinfo, client );
		snd_seq_port_info_set_port( pinfo, -1 );
		while( snd_seq_query_next_port( m_seqHandle, pinfo ) >= 0 )
		{
			// we need both READ and SUBS_READ
			if( ( snd_seq_port_info_get_capability( pinfo )
			     & ( SND_SEQ_PORT_CAP_READ |
					SND_SEQ_PORT_CAP_SUBS_READ ) ) ==
					( SND_SEQ_PORT_CAP_READ |
					  	SND_SEQ_PORT_CAP_SUBS_READ ) )
			{
				readable_ports.push_back( 
					QString( "%1:%2 %3:%4" ).
					arg( snd_seq_port_info_get_client(
								pinfo ) ).
					arg( snd_seq_port_info_get_port(
								pinfo ) ).
					arg( snd_seq_client_info_get_name(
								cinfo ) ).
					arg( snd_seq_port_info_get_name(
								pinfo ) ) );
			}
			if( ( snd_seq_port_info_get_capability( pinfo )
			     & ( SND_SEQ_PORT_CAP_WRITE |
					SND_SEQ_PORT_CAP_SUBS_WRITE ) ) ==
					( SND_SEQ_PORT_CAP_WRITE |
					  	SND_SEQ_PORT_CAP_SUBS_WRITE ) )
			{
				writeable_ports.push_back( 
					QString( "%1:%2 %3:%4" ).
					arg( snd_seq_port_info_get_client(
								pinfo ) ).
					arg( snd_seq_port_info_get_port(
								pinfo ) ).
					arg( snd_seq_client_info_get_name(
								cinfo ) ).
					arg( snd_seq_port_info_get_name(
								pinfo ) ) );
			}
		}
	}

	snd_seq_client_info_free( cinfo );
	snd_seq_port_info_free( pinfo );

	if( m_readablePorts != readable_ports )
	{
		m_readablePorts = readable_ports;
		emit( readablePortsChanged() );
	}

	if( m_writeablePorts != writeable_ports )
	{
		m_writeablePorts = writeable_ports;
		emit( writeablePortsChanged() );
	}
}







midiALSASeq::setupWidget::setupWidget( QWidget * _parent ) :
	midiClient::setupWidget( midiALSASeq::name(), _parent )
{
	m_device = new QLineEdit( midiALSASeq::probeDevice(), this );
	m_device->setGeometry( 10, 20, 160, 20 );

	QLabel * dev_lbl = new QLabel( tr( "DEVICE" ), this );
	dev_lbl->setFont( pointSize<6>( dev_lbl->font() ) );
	dev_lbl->setGeometry( 10, 40, 160, 10 );
}




midiALSASeq::setupWidget::~setupWidget()
{
}




void midiALSASeq::setupWidget::saveSettings( void )
{
	configManager::inst()->setValue( "midialsaseq", "device",
							m_device->text() );
}


#include "moc_midi_alsa_seq.cxx"


#endif

#endif
