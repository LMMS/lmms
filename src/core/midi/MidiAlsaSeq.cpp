/*
 * MidiAlsaSeq.cpp - ALSA sequencer client
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QtGui/QLabel>
#include <QtGui/QLineEdit>

#include "MidiAlsaSeq.h"
#include "config_mgr.h"
#include "engine.h"
#include "gui_templates.h"
#include "song.h"
#include "MidiPort.h"
#include "MidiTime.h"
#include "note.h"


#ifdef LMMS_HAVE_ALSA

const int EventPollTimeOut = 250;


// static helper functions
static QString __portName( snd_seq_client_info_t * _cinfo,
								snd_seq_port_info_t * _pinfo )
{
	return QString( "%1:%2 %3:%4" ).
					arg( snd_seq_port_info_get_client( _pinfo ) ).
					arg( snd_seq_port_info_get_port( _pinfo ) ).
					arg( snd_seq_client_info_get_name( _cinfo ) ).
					arg( snd_seq_port_info_get_name( _pinfo ) );
}

static QString __portName( snd_seq_t * _seq, const snd_seq_addr_t * _addr )
{
	snd_seq_client_info_t * cinfo;
	snd_seq_port_info_t * pinfo;

	snd_seq_client_info_malloc( &cinfo );
	snd_seq_port_info_malloc( &pinfo );

	snd_seq_get_any_port_info( _seq, _addr->client, _addr->port, pinfo );
	snd_seq_get_any_client_info( _seq, _addr->client, cinfo );

	const QString name = __portName( cinfo, pinfo );

	snd_seq_client_info_free( cinfo );
	snd_seq_port_info_free( pinfo );

	return name;
}



MidiAlsaSeq::MidiAlsaSeq() :
	MidiClient(),
	m_seqMutex(),
	m_seqHandle( NULL ),
	m_queueID( -1 ),
	m_quit( false ),
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




MidiAlsaSeq::~MidiAlsaSeq()
{
	if( isRunning() )
	{
		m_quit = true;
		wait( EventPollTimeOut*2 );

		m_seqMutex.lock();
		snd_seq_stop_queue( m_seqHandle, m_queueID, NULL );
		snd_seq_free_queue( m_seqHandle, m_queueID );
		snd_seq_close( m_seqHandle );
		m_seqMutex.unlock();
	}
}




QString MidiAlsaSeq::probeDevice()
{
	QString dev = configManager::inst()->value( "Midialsaseq", "device" );
	if( dev.isEmpty() )
	{
		if( getenv( "MIDIDEV" ) != NULL )
		{
			return getenv( "MIDIDEV" );
		}
		return "default";
	}
	return dev;
}




void MidiAlsaSeq::processOutEvent( const MidiEvent& event, const MidiTime& time, const MidiPort* port )
{
	// HACK!!! - need a better solution which isn't that easy since we
	// cannot store const-ptrs in our map because we need to call non-const
	// methods of MIDI-port - it's a mess...
	MidiPort* p = const_cast<MidiPort *>( port );

	snd_seq_event_t ev;
	snd_seq_ev_clear( &ev );
	snd_seq_ev_set_source( &ev, ( m_portIDs[p][1] != -1 ) ?
					m_portIDs[p][1] : m_portIDs[p][0] );
	snd_seq_ev_set_subs( &ev );
	snd_seq_ev_schedule_tick( &ev, m_queueID, 1, static_cast<int>( time ) );
	ev.queue =  m_queueID;
	switch( event.type() )
	{
		case MidiNoteOn:
			snd_seq_ev_set_noteon( &ev,
						event.channel(),
						event.key() + KeysPerOctave,
						event.velocity() );
			break;

		case MidiNoteOff:
			snd_seq_ev_set_noteoff( &ev,
						event.channel(),
						event.key() + KeysPerOctave,
						event.velocity() );
			break;

		case MidiKeyPressure:
			snd_seq_ev_set_keypress( &ev,
						event.channel(),
						event.key() + KeysPerOctave,
						event.velocity() );
			break;

		case MidiControlChange:
			snd_seq_ev_set_controller( &ev,
						event.channel(),
						event.controllerNumber(),
						event.controllerValue() );
			break;

		case MidiProgramChange:
			snd_seq_ev_set_pgmchange( &ev,
						event.channel(),
						event.program() );
			break;

		case MidiChannelPressure:
			snd_seq_ev_set_chanpress( &ev,
						event.channel(),
						event.channelPressure() );
			break;

		case MidiPitchBend:
			snd_seq_ev_set_pitchbend( &ev,
						event.channel(),
						event.param( 0 ) - 8192 );
			break;

		default:
			qWarning( "MidiAlsaSeq: unhandled output event %d\n", (int) event.type() );
			return;
	}

	m_seqMutex.lock();
	snd_seq_event_output( m_seqHandle, &ev );
	snd_seq_drain_output( m_seqHandle );
	m_seqMutex.unlock();

}




void MidiAlsaSeq::applyPortMode( MidiPort * _port )
{
	m_seqMutex.lock();

	// determine port-capabilities
	unsigned int caps[2] = { 0, 0 };

	switch( _port->mode() )
	{
		case MidiPort::Duplex:
			caps[1] |= SND_SEQ_PORT_CAP_READ |
						SND_SEQ_PORT_CAP_SUBS_READ;

		case MidiPort::Input:
			caps[0] |= SND_SEQ_PORT_CAP_WRITE |
						SND_SEQ_PORT_CAP_SUBS_WRITE;
			break;

		case MidiPort::Output:
			caps[1] |= SND_SEQ_PORT_CAP_READ |
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
				_port->displayName().toUtf8().constData(),
							caps[i],
						SND_SEQ_PORT_TYPE_MIDI_GENERIC |
						SND_SEQ_PORT_TYPE_APPLICATION );
				continue;
			}
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

	m_seqMutex.unlock();
}




void MidiAlsaSeq::applyPortName( MidiPort * _port )
{
	m_seqMutex.lock();

	for( int i = 0; i < 2; ++i )
	{
		if( m_portIDs[_port][i] == -1 )
		{
			continue;
		}
		snd_seq_port_info_t * port_info;
		snd_seq_port_info_malloc( &port_info );
		snd_seq_get_port_info( m_seqHandle, m_portIDs[_port][i],
							port_info );
		snd_seq_port_info_set_name( port_info,
				_port->displayName().toUtf8().constData() );
		snd_seq_set_port_info( m_seqHandle, m_portIDs[_port][i],
							port_info );
		snd_seq_port_info_free( port_info );
	}

	m_seqMutex.unlock();
}




void MidiAlsaSeq::removePort( MidiPort * _port )
{
	if( m_portIDs.contains( _port ) )
	{
		m_seqMutex.lock();
		snd_seq_delete_simple_port( m_seqHandle, m_portIDs[_port][0] );
		snd_seq_delete_simple_port( m_seqHandle, m_portIDs[_port][1] );
		m_seqMutex.unlock();

		m_portIDs.remove( _port );
	}
	MidiClient::removePort( _port );
}




QString MidiAlsaSeq::sourcePortName( const MidiEvent & _event ) const
{
	if( _event.sourcePort() )
	{
		const snd_seq_addr_t * addr =
			static_cast<const snd_seq_addr_t *>( _event.sourcePort() );
		return __portName( m_seqHandle, addr );
	}
	return MidiClient::sourcePortName( _event );
}




void MidiAlsaSeq::subscribeReadablePort( MidiPort * _port,
						const QString & _dest,
						bool _subscribe )
{
	if( !m_portIDs.contains( _port ) || m_portIDs[_port][0] < 0 )
	{
		return;
	}

	m_seqMutex.lock();

	snd_seq_addr_t sender;
	if( snd_seq_parse_address( m_seqHandle, &sender,
			_dest.section( ' ', 0, 0 ).toAscii().constData() ) )
	{
		fprintf( stderr, "error parsing sender-address!\n" );

		m_seqMutex.unlock();
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

	m_seqMutex.unlock();
}




void MidiAlsaSeq::subscribeWritablePort( MidiPort * _port,
						const QString & _dest,
						bool _subscribe )
{
	if( !m_portIDs.contains( _port ) )
	{
		return;
	}
	const int pid = m_portIDs[_port][1] < 0 ? m_portIDs[_port][0] :
							m_portIDs[_port][1];
	if( pid < 0 )
	{
		return;
	}

	m_seqMutex.lock();

	snd_seq_addr_t dest;
	if( snd_seq_parse_address( m_seqHandle, &dest,
			_dest.section( ' ', 0, 0 ).toAscii().constData() ) )
	{
		fprintf( stderr, "error parsing dest-address!\n" );
		m_seqMutex.unlock();
		return;
	}
	snd_seq_port_info_t * port_info;
	snd_seq_port_info_malloc( &port_info );
	snd_seq_get_port_info( m_seqHandle, pid, port_info );
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
	m_seqMutex.unlock();
}




void MidiAlsaSeq::run()
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

	while( m_quit == false )
	{
		int pollRet = poll( pollfd_set, pollfd_count, EventPollTimeOut );
		if( pollRet == 0 )
		{
			continue;
		}
		else if( pollRet == -1 )
		{
			// gdb may interrupt the poll
			if( errno == EINTR )
			{
				continue;
			}
			qCritical( "error while polling ALSA sequencer handle" );
			break;
		}
		// shutdown?
		if( m_quit )
		{
			break;
		}

		m_seqMutex.lock();

		// while event queue is not empty
		while( snd_seq_event_input_pending( m_seqHandle, true ) > 0 )
		{
		snd_seq_event_t * ev;
		if( snd_seq_event_input( m_seqHandle, &ev ) < 0 )
		{
			m_seqMutex.unlock();

			qCritical( "error while fetching MIDI event from sequencer" );
			break;
		}
		m_seqMutex.unlock();

		snd_seq_addr_t * source = NULL;
		MidiPort * dest = NULL;
		for( int i = 0; i < m_portIDs.size(); ++i )
		{
			if( m_portIDs.values()[i][0] == ev->dest.port )
			{
				dest = m_portIDs.keys()[i];
			}
			if( ( m_portIDs.values()[i][1] != -1 &&
					m_portIDs.values()[i][1] == ev->source.port ) ||
						m_portIDs.values()[i][0] == ev->source.port )
			{
				source = &ev->source;
			}
		}

		if( dest == NULL )
		{
			continue;
		}

		switch( ev->type )
		{
			case SND_SEQ_EVENT_NOTEON:
				dest->processInEvent( MidiEvent( MidiNoteOn,
							ev->data.note.channel,
							ev->data.note.note -
							KeysPerOctave,
							ev->data.note.velocity,
							source
							),
						MidiTime( ev->time.tick ) );
				break;

			case SND_SEQ_EVENT_NOTEOFF:
				dest->processInEvent( MidiEvent( MidiNoteOff,
							ev->data.note.channel,
							ev->data.note.note -
							KeysPerOctave,
							ev->data.note.velocity,
							source
							),
						MidiTime( ev->time.tick) );
				break;

			case SND_SEQ_EVENT_KEYPRESS:
				dest->processInEvent( MidiEvent(
								MidiKeyPressure,
							ev->data.note.channel,
							ev->data.note.note -
							KeysPerOctave,
							ev->data.note.velocity,
							source
							), MidiTime() );
				break;

			case SND_SEQ_EVENT_CONTROLLER:
				dest->processInEvent( MidiEvent(
							MidiControlChange,
						ev->data.control.channel,
						ev->data.control.param,
						ev->data.control.value, source ),
								MidiTime() );
				break;

			case SND_SEQ_EVENT_PGMCHANGE:
				dest->processInEvent( MidiEvent(
							MidiProgramChange,
						ev->data.control.channel,
						ev->data.control.param,
						ev->data.control.value, source ),
								MidiTime() );
				break;

			case SND_SEQ_EVENT_CHANPRESS:
				dest->processInEvent( MidiEvent(
							MidiChannelPressure,
						ev->data.control.channel,
						ev->data.control.param,
						ev->data.control.value, source ),
								MidiTime() );
				break;

			case SND_SEQ_EVENT_PITCHBEND:
				dest->processInEvent( MidiEvent( MidiPitchBend,
						ev->data.control.channel,
						ev->data.control.value + 8192, 0, source ),
								MidiTime() );
				break;

			case SND_SEQ_EVENT_SENSING:
			case SND_SEQ_EVENT_CLOCK:
				break;

			default:
				fprintf( stderr,
					"ALSA-sequencer: unhandled input "
						"event %d\n", ev->type );
				break;
		}	// end switch

		m_seqMutex.lock();

		}	// end while

		m_seqMutex.unlock();

	}

	delete[] pollfd_set;
}




void MidiAlsaSeq::changeQueueTempo( bpm_t _bpm )
{
	m_seqMutex.lock();

	snd_seq_change_queue_tempo( m_seqHandle, m_queueID,
					60000000 / (int) _bpm, NULL );
	snd_seq_drain_output( m_seqHandle );

	m_seqMutex.unlock();
}




void MidiAlsaSeq::updatePortList()
{
	QStringList readablePorts;
	QStringList writablePorts;

	// get input- and output-ports
	snd_seq_client_info_t * cinfo;
	snd_seq_port_info_t * pinfo;

	snd_seq_client_info_malloc( &cinfo );
	snd_seq_port_info_malloc( &pinfo );

	snd_seq_client_info_set_client( cinfo, -1 );

	m_seqMutex.lock();

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
				readablePorts.push_back( __portName( cinfo, pinfo ) );
			}
			if( ( snd_seq_port_info_get_capability( pinfo )
			     & ( SND_SEQ_PORT_CAP_WRITE |
					SND_SEQ_PORT_CAP_SUBS_WRITE ) ) ==
					( SND_SEQ_PORT_CAP_WRITE |
					  	SND_SEQ_PORT_CAP_SUBS_WRITE ) )
			{
				writablePorts.push_back( __portName( cinfo, pinfo ) );
			}
		}
	}

	m_seqMutex.unlock();


	snd_seq_client_info_free( cinfo );
	snd_seq_port_info_free( pinfo );

	if( m_readablePorts != readablePorts )
	{
		m_readablePorts = readablePorts;
		emit readablePortsChanged();
	}

	if( m_writablePorts != writablePorts )
	{
		m_writablePorts = writablePorts;
		emit writablePortsChanged();
	}
}







MidiAlsaSeq::setupWidget::setupWidget( QWidget * _parent ) :
	MidiClient::setupWidget( MidiAlsaSeq::name(), _parent )
{
	m_device = new QLineEdit( MidiAlsaSeq::probeDevice(), this );
	m_device->setGeometry( 10, 20, 160, 20 );

	QLabel * dev_lbl = new QLabel( tr( "DEVICE" ), this );
	dev_lbl->setFont( pointSize<7>( dev_lbl->font() ) );
	dev_lbl->setGeometry( 10, 40, 160, 10 );
}




MidiAlsaSeq::setupWidget::~setupWidget()
{
}




void MidiAlsaSeq::setupWidget::saveSettings()
{
	configManager::inst()->setValue( "Midialsaseq", "device",
							m_device->text() );
}


#include "moc_MidiAlsaSeq.cxx"


#endif

