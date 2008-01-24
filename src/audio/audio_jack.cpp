#ifndef SINGLE_SOURCE_COMPILE

/*
 * audio_jack.cpp - support for JACK-transport
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


#include "audio_jack.h"


#ifdef JACK_SUPPORT

#include <QtGui/QLineEdit>
#include <QtGui/QLabel>
#include <QtGui/QMessageBox>


#include <stdlib.h>


#include "debug.h"
#include "templates.h"
#include "automatable_model_templates.h"
#include "gui_templates.h"
#include "config_mgr.h"
#include "lcd_spinbox.h"
#include "audio_port.h"




audioJACK::audioJACK( const sample_rate_t _sample_rate, bool & _success_ful,
							mixer * _mixer ) :
	audioDevice( _sample_rate, tLimit<int>( configManager::inst()->value(
					"audiojack", "channels" ).toInt(),
					DEFAULT_CHANNELS, SURROUND_CHANNELS ),
								_mixer ),
	m_client( NULL ),
	m_active( FALSE ),
	m_stop_semaphore( 1 ),
	m_outBuf( new surroundSampleFrame[getMixer()->framesPerPeriod()] ),
	m_framesDoneInCurBuf( 0 ),
	m_framesToDoInCurBuf( 0 )
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
	m_client = jack_client_open( client_name.toAscii().constData(),
						JackNullOption, &status,
								server_name );
	if( m_client == NULL )
	{
		printf( "jack_client_open() failed, status 0x%2.0x\n", status );
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
							toAscii().constData(),
					jack_get_client_name( m_client ) );
	}

#else /* OLD_JACK */

	m_client = jack_client_new( client_name.toAscii().constData() );
	if( m_client == NULL )
	{
		printf( "jack_client_new() failed\n" );
		return;
	}

#endif

	// set process-callback
	jack_set_process_callback( m_client, processCallback, this );

	// set shutdown-callback
	jack_on_shutdown( m_client, shutdownCallback, this );



	if( jack_get_sample_rate( m_client ) != sampleRate() )
	{
		SAMPLE_RATES[0] = jack_get_sample_rate( m_client );
		SAMPLE_RATES[1] = 2 * SAMPLE_RATES[0];
		setSampleRate( SAMPLE_RATES[0] );
	}

	for( Uint8 ch = 0; ch < channels(); ++ch )
	{
		QString name = QString( "master out " ) +
				( ( ch % 2 ) ? "R" : "L" ) +
				QString::number( ch / 2 + 1 );
		m_outputPorts.push_back( jack_port_register( m_client,
						name.toAscii().constData(),
						JACK_DEFAULT_AUDIO_TYPE,
						JackPortIsOutput, 0 ) );
		if( m_outputPorts.back() == NULL )
		{
			printf( "no more JACK-ports available!\n" );
			return;
		}
	}

	m_stop_semaphore.acquire();

	_success_ful = TRUE;
}




audioJACK::~audioJACK()
{
	m_stop_semaphore.release();

	while( m_portMap.size() )
	{
		unregisterPort( m_portMap.begin().key() );
	}

	if( m_client != NULL )
	{
		if( m_active )
		{
			jack_deactivate( m_client );
		}
		jack_client_close( m_client );
	}

	delete[] m_outBuf;

}




void audioJACK::startProcessing( void )
{
	m_stopped = FALSE;

	if( m_active )
	{
		return;
	}

	if( jack_activate( m_client ) )
	{
		printf( "cannot activate client\n" );
		return;
	}

	m_active = TRUE;


	// make sure, JACK transport is rolling
	if( jack_transport_query( m_client, NULL ) != JackTransportRolling )
	{
		jack_transport_start( m_client );
	}


	// try to sync JACK's and LMMS's buffer-size
	jack_set_buffer_size( m_client, getMixer()->framesPerPeriod() );



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
}




void audioJACK::stopProcessing( void )
{
	m_stop_semaphore.acquire();
}




void audioJACK::registerPort( audioPort * _port )
{
#ifdef AUDIO_PORT_SUPPORT
	// make sure, port is not already registered
	unregisterPort( _port );
	const QString name[2] = { _port->name() + " L",
					_port->name() + " R" } ;

	for( Uint8 ch = 0; ch < DEFAULT_CHANNELS; ++ch )
	{
		m_portMap[_port].ports[ch] = jack_port_register( m_client,
						name[ch].toAscii().constData(),
						JACK_DEFAULT_AUDIO_TYPE,
							JackPortIsOutput, 0 );
	}
#endif
}




void audioJACK::unregisterPort( audioPort * _port )
{
#ifdef AUDIO_PORT_SUPPORT
	if( m_portMap.contains( _port ) )
	{
		for( Uint8 ch = 0; ch < DEFAULT_CHANNELS; ++ch )
		{
			if( m_portMap[_port].ports[ch] != NULL )
			{
				jack_port_unregister( m_client,
						m_portMap[_port].ports[ch] );
			}
		}
		m_portMap.erase( m_portMap.find( _port ) );
	}
#endif
}




void audioJACK::renamePort( audioPort * _port )
{
#ifdef AUDIO_PORT_SUPPORT
	if( m_portMap.contains( _port ) )
	{
		const QString name[2] = { _port->name() + " L",
					_port->name() + " R" };
		for( Uint8 ch = 0; ch < DEFAULT_CHANNELS; ++ch )
		{
			jack_port_set_name( m_portMap[_port].ports[ch],
					name[ch].toAscii().constData() );
		}
	}
#endif
}




int audioJACK::processCallback( jack_nframes_t _nframes, void * _udata )
{
	audioJACK * _this = static_cast<audioJACK *>( _udata );

/*	printf( "%f\n", jack_cpu_load( _this->m_client ) );*/

#ifdef LMMS_DEBUG
	assert( _this != NULL );
#endif
	jack_transport_state_t ts = jack_transport_query( _this->m_client,
									NULL );

	QVector<jack_default_audio_sample_t *> outbufs( _this->channels(),
									NULL );
	Uint8 chnl = 0;
	for( QVector<jack_default_audio_sample_t *>::iterator it =
			outbufs.begin(); it != outbufs.end(); ++it, ++chnl )
	{
		*it = (jack_default_audio_sample_t *) jack_port_get_buffer(
					_this->m_outputPorts[chnl], _nframes );
	}

#ifdef AUDIO_PORT_SUPPORT
	const Uint32 frames = tMin<Uint32>( _nframes,
					_this->getMixer()->framesPerPeriod() );
	for( jackPortMap::iterator it = _this->m_portMap.begin();
					it != _this->m_portMap.end(); ++it )
	{
		for( Uint8 ch = 0; ch < _this->channels(); ++ch )
		{
			if( it.data().ports[ch] == NULL )
			{
				continue;
			}
			jack_default_audio_sample_t * buf =
			(jack_default_audio_sample_t *) jack_port_get_buffer(
							it.data().ports[ch],
								_nframes );
			for( Uint32 frame = 0; frame < frames; ++frame )
			{
				buf[frame] = it.key()->firstBuffer()[frame][ch];
			}
		}
	}
#endif

	jack_nframes_t done = 0;
	while( done < _nframes && _this->m_stopped == FALSE )
	{
		jack_nframes_t todo = tMin<jack_nframes_t>(
						_nframes,
						_this->m_framesToDoInCurBuf -
						_this->m_framesDoneInCurBuf );
		if( ts == JackTransportRolling )
		{
			for( Uint8 chnl = 0; chnl < _this->channels(); ++chnl )
			{
				for( jack_nframes_t frame = 0; frame < todo;
								++frame )
				{
					outbufs[chnl][done+frame] = 
		_this->m_outBuf[_this->m_framesDoneInCurBuf+frame][chnl] *
						_this->getMixer()->masterGain();
				}
			}
		}
		done += todo;
		_this->m_framesDoneInCurBuf += todo;
		if( _this->m_framesDoneInCurBuf == _this->m_framesToDoInCurBuf )
		{
			_this->m_framesToDoInCurBuf = _this->getNextBuffer(
							_this->m_outBuf );
			if( !_this->m_framesToDoInCurBuf )
			{
				_this->m_stopped = TRUE;
				_this->m_stop_semaphore.release();
			}
			_this->m_framesDoneInCurBuf = 0;
		}
	}

	if( ts != JackTransportRolling || _this->m_stopped == TRUE )
	{
		for( Uint8 ch = 0; ch < _this->channels(); ++ch )
		{
			jack_default_audio_sample_t * b = outbufs[ch] + done;
			memset( b, 0, sizeof( *b ) * ( _nframes - done ) );
		}
	}

	return( 0 );
}




void audioJACK::shutdownCallback( void * _udata )
{
	audioJACK * _this = static_cast<audioJACK *>( _udata );
	_this->m_client = NULL;
/*	QMessageBox::information( 0, setupWidget::tr( "JACK-server down" ),
					setupWidget::tr( "You seem to have "
						"shutdown JACK-server, so "
						"LMMS is unable to proceed. "
						"You should save your project "
						"and restart LMMS!" ),
					QMessageBox::Ok );*/
}





audioJACK::setupWidget::setupWidget( QWidget * _parent ) :
	audioDevice::setupWidget( audioJACK::name(), _parent )
{
	QString cn = configManager::inst()->value( "audiojack", "clientname" );
	if( cn.isEmpty() )
	{
		cn = "lmms";
	}
	m_clientName = new QLineEdit( cn, this );
	m_clientName->setGeometry( 10, 20, 160, 20 );

	QLabel * cn_lbl = new QLabel( tr( "CLIENT-NAME" ), this );
	cn_lbl->setFont( pointSize<6>( cn_lbl->font() ) );
	cn_lbl->setGeometry( 10, 40, 160, 10 );

	lcdSpinBoxModel * m = new lcdSpinBoxModel( /* this */ );	
	m->setRange( DEFAULT_CHANNELS, SURROUND_CHANNELS );
	m->setStep( 2 );
	m->setValue( configManager::inst()->value( "audiojack",
							"channels" ).toInt() );

	m_channels = new lcdSpinBox( 1, this );
	m_channels->setModel( m );
	m_channels->setLabel( tr( "CHANNELS" ) );
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

#endif
