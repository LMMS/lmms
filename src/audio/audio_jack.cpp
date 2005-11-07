/*
 * audio_jack.cpp - support for JACK-transport
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#include "audio_jack.h"


#ifdef JACK_SUPPORT

#ifdef HAVE_UNISTD_H
// for usleep
#include <unistd.h>
#endif


#ifdef QT4

#include <QLineEdit>
#include <QLabel>

#else

#include <qlineedit.h>
#include <qlabel.h>

#endif


#include "debug.h"
#include "templates.h"
#include "gui_templates.h"
#include "buffer_allocator.h"
#include "config_mgr.h"
#include "lcd_spinbox.h"



audioJACK::audioJACK( Uint32 _sample_rate, bool & _success_ful ) :
	audioDevice( _sample_rate, tLimit<int>( configManager::inst()->value(
					"audiojack", "channels" ).toInt(),
					DEFAULT_CHANNELS, SURROUND_CHANNELS ) ),
	m_framesDoneInCurBuf( 0 ),
	m_frameSync( 0 ),
	m_jackBufSize( 0 ),
	m_bufMutex()
{
	_success_ful = FALSE;

	QString client_name = configManager::inst()->value( "audiojack",
								"clientname" );
	if( client_name == "" )
	{
		client_name = "lmms";
	}

#ifndef OLD_JACK
	const char * server_name = NULL;
	jack_status_t status;
	m_client = jack_client_open( client_name.
#ifdef QT4
							toAscii().constData(),
#else
							ascii(),
#endif
						JackNullOption, &status,
								server_name );
	if( m_client == NULL )
	{
		printf( "jack_client_open() failed with status %d\n", status );
		if( status & JackServerFailed )
		{
			printf( "Could not connect to JACK server.\n" );
		}
		return;
	}
	if( status & JackNameNotUnique )
	{
		printf( "there's already a client with name '%s', so unique "
			"name '%s' was assigned\n", client_name.
#ifdef QT4
							toAscii().constData(),
#else
							ascii(),
#endif
					jack_get_client_name( m_client ) );
	}
#else
	m_client = jack_client_new( client_name.
#ifdef QT4
						toAscii().constData()
#else
						ascii()
#endif
					);
#endif

	// set process-callback
	jack_set_process_callback( m_client, processCallback, this );

	// we need to know about buffer-size changes to know how long to block
	// in writeToDev()-method
	jack_set_buffer_size_callback( m_client, bufSizeCallback, this );

	// set shutdown-callback
	//jack_on_shutdown( m_client, shutdown, this );

	m_jackBufSize = jack_get_buffer_size( m_client );


	if( jack_get_sample_rate( m_client ) != sampleRate() )
	{
		SAMPLE_RATES[0] = jack_get_sample_rate( m_client );
		SAMPLE_RATES[1] = 2 * SAMPLE_RATES[0];
		setSampleRate( SAMPLE_RATES[0] );
	}

	for( Uint8 ch = 0; ch < channels(); ++ch )
	{
		QString name = QString( "master_out_" ) +
				( ( ch % 2 ) ? "R" : "L" ) +
				QString::number( ch / 2 + 1 );
		m_outputPorts.push_back( jack_port_register( m_client,
#ifdef QT4
						name.toAscii().constData(),
#else
						name.ascii(),
#endif
						JACK_DEFAULT_AUDIO_TYPE,
						JackPortIsOutput, 0 ) );
		if( m_outputPorts.back() == NULL )
		{
			printf( "no more JACK-ports available!\n" );
			return;
		}
	}


	if( jack_activate( m_client ) )
	{
		printf( "cannot activate client\n" );
		return;
	}

	const char * * ports = jack_get_ports( m_client, NULL, NULL,
						JackPortIsPhysical |
						JackPortIsInput );
	if( ports == NULL )
	{
		printf( "no physical playback ports. you'll have to do "
			"connections at your own!\n" );
	}
	else
	{
		for( Uint8 ch = 0; ch < channels(); ++ch )
		{
			if( jack_connect( m_client, jack_port_name(
							m_outputPorts[ch] ),
								ports[ch] ) )
			{
				printf( "cannot connect output ports. you'll "
					"have to do connections at your own!\n"
									);
			}
		}
	}

	free( ports );

	_success_ful = TRUE;
}




audioJACK::~audioJACK()
{
	jack_deactivate( m_client );
	jack_client_close( m_client );

	while( m_bufferSets.size() )
	{
		while( m_bufferSets.front().size() )
		{
			bufferAllocator::free(
					m_bufferSets.front().front().buf );
			m_bufferSets.front().erase(
						m_bufferSets.front().begin() );
		}
		m_bufferSets.erase( m_bufferSets.begin() );
	}
}




void audioJACK::writeBufferToDev( surroundSampleFrame * _ab, Uint32 _frames,
							float _master_gain )
{
	m_bufMutex.lock();

	jack_transport_state_t ts = jack_transport_query( m_client, NULL );
	if( ts == JackTransportRolling )
	{
		vvector<bufset> bufs;
		for( Uint8 chnl = 0; chnl < channels(); ++chnl )
		{
			sampleType * buf = bufferAllocator::alloc<sampleType>(
								_frames );
			for( Uint32 frame = 0; frame < _frames; ++frame )
			{
				buf[frame] = _ab[frame][chnl] * _master_gain;
			}
			bufset b = { buf, _frames } ;
			bufs.push_back( b );
		}
		m_bufferSets.push_back( bufs );
	}

	m_frameSync += _frames;

	m_bufMutex.unlock();

	// now wait until data has been collected/skipped by processCallback()
	while( m_frameSync > m_jackBufSize )
	{
#ifdef HAVE_UNISTD_H
#ifdef HAVE_USLEEP
		// just wait and give cpu-time to other processes
 		// 	tobydox 20051019: causes LMMS to hang up when locking
		//			  several other mutexes, so skip it
		//usleep( 200 );

#endif
#endif
	}
}




int audioJACK::processCallback( jack_nframes_t _nframes, void * _udata )
{
	audioJACK * _this = static_cast<audioJACK *>( _udata );

#ifdef LMMS_DEBUG
	assert( _this != NULL );
#endif
	jack_transport_state_t ts = jack_transport_query( _this->m_client,
									NULL );
	_this->m_bufMutex.lock();

	if( ts != JackTransportRolling )
	{
		// always decrease frame-sync-var as we would do it if running
		// in normal mode, so that the mixer-thread does up
		if( _nframes < _this->m_frameSync )
		{
			_this->m_frameSync -= _nframes;
		}
		else
		{
			_this->m_frameSync = 0;
		}
		_this->m_bufMutex.unlock();
		return( 0 );
	}

	vvector<jack_default_audio_sample_t *> outbufs( _this->channels(),
									NULL );
	Uint8 ch = 0;
	for( vvector<jack_default_audio_sample_t *>::iterator it =
			outbufs.begin(); it != outbufs.end(); ++it, ++ch )
	{
		*it = (jack_default_audio_sample_t *) jack_port_get_buffer(
					_this->m_outputPorts[ch], _nframes );
	}


	jack_nframes_t done = 0;
	while( done < _nframes )
	{
		if( _this->m_bufferSets.size() == 0 )
		{
			break;
		}
		jack_nframes_t todo = tMin( _nframes - done,
					_this->m_bufferSets.front()[0].frames -
						_this->m_framesDoneInCurBuf );
		for( Uint8 ch = 0; ch < _this->channels(); ++ch )
		{
			memcpy( outbufs[ch] + done,
				_this->m_bufferSets.front()[ch].buf +
					_this->m_framesDoneInCurBuf,
					sizeof( jack_default_audio_sample_t ) *
									todo );
		}
		_this->m_framesDoneInCurBuf += todo;
		if( _this->m_framesDoneInCurBuf >=
					_this->m_bufferSets.front()[0].frames )
		{
			for( Uint8 ch = 0; ch < _this->channels(); ++ch )
			{
				bufferAllocator::free(
					_this->m_bufferSets.front()[ch].buf );
			}
			_this->m_bufferSets.erase(
						_this->m_bufferSets.begin() );
			_this->m_framesDoneInCurBuf = 0;
		}
		done += todo;
		_this->m_frameSync -= todo;
	}

	// we have to clear the part of the buffers, we could not fill because
	// no usable data is left, otherwise there's baaaaaad noise... ;-)
	if( done < _nframes )
	{
		for( Uint8 ch = 0; ch < _this->channels(); ++ch )
		{
			jack_default_audio_sample_t * b = outbufs[ch];
			for( Uint32 frame = done; frame < _nframes; ++frame )
			{
				b[frame] = 0.0f;
			}
		}
	}

	_this->m_bufMutex.unlock();

	return( 0 );
}




int audioJACK::bufSizeCallback( jack_nframes_t _nframes, void * _udata )
{
	audioJACK * _this = static_cast<audioJACK *>( _udata );

#ifdef LMMS_DEBUG
	assert( _this != NULL );
#endif
	_this->m_jackBufSize = _nframes;

	return( 0 );
}




audioJACK::setupWidget::setupWidget( QWidget * _parent ) :
	audioDevice::setupWidget( audioJACK::name(), _parent )
{
	QString cn = configManager::inst()->value( "audiojack", "clientname" );
	if( cn == "" )
	{
		cn = "lmms";
	}
	m_clientName = new QLineEdit( cn, this );
	m_clientName->setGeometry( 10, 20, 160, 20 );

	QLabel * cn_lbl = new QLabel( tr( "CLIENT-NAME" ), this );
	cn_lbl->setFont( pointSize<6>( cn_lbl->font() ) );
	cn_lbl->setGeometry( 10, 40, 160, 10 );

	m_channels = new lcdSpinBox( DEFAULT_CHANNELS, SURROUND_CHANNELS, 1,
									this );
	m_channels->setStep( 2 );
	m_channels->setLabel( tr( "CHANNELS" ) );
	m_channels->setValue( configManager::inst()->value( "audiojack",
							"channels" ).toInt() );
	m_channels->move( 180, 20 );

}




audioJACK::setupWidget::~setupWidget()
{
}




void audioJACK::setupWidget::saveSettings( void )
{
	configManager::inst()->setValue( "audiojack", "clientname",
							m_clientName->text() );
	configManager::inst()->setValue( "audiojack", "channels",
				QString::number( m_channels->value() ) );
}


#endif
