/*
 * AudioJack.cpp - support for JACK transport
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
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

#include "AudioJack.h"

#ifdef LMMS_HAVE_JACK

#include <QLineEdit>
#include <QLabel>
#include <QMessageBox>

#include "Engine.h"
#include "GuiApplication.h"
#include "gui_templates.h"
#include "ConfigManager.h"
#include "LcdSpinBox.h"
#include "MainWindow.h"
#include "AudioEngine.h"
#include "MidiJack.h"


namespace lmms
{

AudioJack::AudioJack( bool & _success_ful, AudioEngine*  _audioEngine ) :
	AudioDevice( qBound<int>(
		DEFAULT_CHANNELS,
		ConfigManager::inst()->value( "audiojack", "channels" ).toInt(),
		SURROUND_CHANNELS ), _audioEngine ),
	m_client( nullptr ),
	m_active( false ),
	m_midiClient( nullptr ),
	m_tempOutBufs( new jack_default_audio_sample_t *[channels()] ),
	m_outBuf( new surroundSampleFrame[audioEngine()->framesPerPeriod()] ),
	m_framesDoneInCurBuf( 0 ),
	m_framesToDoInCurBuf( 0 )
{
	m_stopped = true;

	_success_ful = initJackClient();
	if( _success_ful )
	{
		connect( this, SIGNAL(zombified()),
				this, SLOT(restartAfterZombified()),
				Qt::QueuedConnection );
	}

}




AudioJack::~AudioJack()
{
	stopProcessing();
#ifdef AUDIO_PORT_SUPPORT
	while( m_portMap.size() )
	{
		unregisterPort( m_portMap.begin().key() );
	}
#endif

	if( m_client != nullptr )
	{
		if( m_active )
		{
			jack_deactivate( m_client );
		}
		jack_client_close( m_client );
	}

	delete[] m_tempOutBufs;

	delete[] m_outBuf;
}




void AudioJack::restartAfterZombified()
{
	if( initJackClient() )
	{
		m_active = false;
		startProcessing();
		QMessageBox::information(gui::getGUI()->mainWindow(),
			tr( "JACK client restarted" ),
			tr( "LMMS was kicked by JACK for some reason. "
				"Therefore the JACK backend of LMMS has been "
				"restarted. You will have to make manual "
				"connections again." ) );
	}
	else
	{
		QMessageBox::information(gui::getGUI()->mainWindow(),
			tr( "JACK server down" ),
			tr( "The JACK server seems to have been shutdown "
				"and starting a new instance failed. "
				"Therefore LMMS is unable to proceed. "
				"You should save your project and restart "
						"JACK and LMMS." ) );
	}
}



AudioJack* AudioJack::addMidiClient(MidiJack *midiClient)
{
	if( m_client == nullptr )
		return nullptr;

	m_midiClient = midiClient;

	return this;
}

bool AudioJack::initJackClient()
{
	QString clientName = ConfigManager::inst()->value( "audiojack",
								"clientname" );
	if( clientName.isEmpty() )
	{
		clientName = "lmms";
	}

	const char * serverName = nullptr;
	jack_status_t status;
	m_client = jack_client_open( clientName.toLatin1().constData(),
						JackNullOption, &status,
								serverName );
	if( m_client == nullptr )
	{
		printf( "jack_client_open() failed, status 0x%2.0x\n", status );
		if( status & JackServerFailed )
		{
			printf( "Could not connect to JACK server.\n" );
		}
		return false;
	}
	if( status & JackNameNotUnique )
	{
		printf( "there's already a client with name '%s', so unique "
			"name '%s' was assigned\n", clientName.
							toLatin1().constData(),
					jack_get_client_name( m_client ) );
	}

	// set process-callback
	jack_set_process_callback( m_client, staticProcessCallback, this );

	// set shutdown-callback
	jack_on_shutdown( m_client, shutdownCallback, this );



	if( jack_get_sample_rate( m_client ) != sampleRate() )
	{
		setSampleRate( jack_get_sample_rate( m_client ) );
	}

	for( ch_cnt_t ch = 0; ch < channels(); ++ch )
	{
		QString name = QString( "master out " ) +
				( ( ch % 2 ) ? "R" : "L" ) +
				QString::number( ch / 2 + 1 );
		m_outputPorts.push_back( jack_port_register( m_client,
						name.toLatin1().constData(),
						JACK_DEFAULT_AUDIO_TYPE,
						JackPortIsOutput, 0 ) );
		if( m_outputPorts.back() == nullptr )
		{
			printf( "no more JACK-ports available!\n" );
			return false;
		}
	}

	return true;
}




void AudioJack::startProcessing()
{
	if( m_active || m_client == nullptr )
	{
		m_stopped = false;
		return;
	}

	if( jack_activate( m_client ) )
	{
		printf( "cannot activate client\n" );
		return;
	}

	m_active = true;


	// try to sync JACK's and LMMS's buffer-size
//	jack_set_buffer_size( m_client, audioEngine()->framesPerPeriod() );



	const char * * ports = jack_get_ports( m_client, nullptr, nullptr,
						JackPortIsPhysical |
						JackPortIsInput );
	if( ports == nullptr )
	{
		printf( "no physical playback ports. you'll have to do "
			"connections at your own!\n" );
	}
	else
	{
		for( ch_cnt_t ch = 0; ch < channels(); ++ch )
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

	m_stopped = false;
	free( ports );
}




void AudioJack::stopProcessing()
{
	m_stopped = true;
}




void AudioJack::applyQualitySettings()
{
	if( hqAudio() )
	{
		setSampleRate( Engine::audioEngine()->processingSampleRate() );

		if( jack_get_sample_rate( m_client ) != sampleRate() )
		{
			setSampleRate( jack_get_sample_rate( m_client ) );
		}
	}

	AudioDevice::applyQualitySettings();
}




void AudioJack::registerPort( AudioPort * _port )
{
#ifdef AUDIO_PORT_SUPPORT
	// make sure, port is not already registered
	unregisterPort( _port );
	const QString name[2] = { _port->name() + " L",
					_port->name() + " R" } ;

	for( ch_cnt_t ch = 0; ch < DEFAULT_CHANNELS; ++ch )
	{
		m_portMap[_port].ports[ch] = jack_port_register( m_client,
						name[ch].toLatin1().constData(),
						JACK_DEFAULT_AUDIO_TYPE,
							JackPortIsOutput, 0 );
	}
#endif
}




void AudioJack::unregisterPort( AudioPort * _port )
{
#ifdef AUDIO_PORT_SUPPORT
	if( m_portMap.contains( _port ) )
	{
		for( ch_cnt_t ch = 0; ch < DEFAULT_CHANNELS; ++ch )
		{
			if( m_portMap[_port].ports[ch] != nullptr )
			{
				jack_port_unregister( m_client,
						m_portMap[_port].ports[ch] );
			}
		}
		m_portMap.erase( m_portMap.find( _port ) );
	}
#endif
}




void AudioJack::renamePort( AudioPort * _port )
{
#ifdef AUDIO_PORT_SUPPORT
	if( m_portMap.contains( _port ) )
	{
		const QString name[2] = { _port->name() + " L",
					_port->name() + " R" };
		for( ch_cnt_t ch = 0; ch < DEFAULT_CHANNELS; ++ch )
		{
#ifdef LMMS_HAVE_JACK_PRENAME
			jack_port_rename( m_client, m_portMap[_port].ports[ch],
					name[ch].toLatin1().constData() );
#else
			jack_port_set_name( m_portMap[_port].ports[ch],
					name[ch].toLatin1().constData() );
#endif
		}
	}
#endif // AUDIO_PORT_SUPPORT
}




int AudioJack::processCallback( jack_nframes_t _nframes, void * _udata )
{

	// do midi processing first so that midi input can
	// add to the following sound processing
	if( m_midiClient && _nframes > 0 )
	{
		m_midiClient.load()->JackMidiRead(_nframes);
		m_midiClient.load()->JackMidiWrite(_nframes);
	}

	for( int c = 0; c < channels(); ++c )
	{
		m_tempOutBufs[c] =
			(jack_default_audio_sample_t *) jack_port_get_buffer(
												m_outputPorts[c], _nframes );
	}

#ifdef AUDIO_PORT_SUPPORT
	const int frames = qMin<int>( _nframes, audioEngine()->framesPerPeriod() );
	for( JackPortMap::iterator it = m_portMap.begin();
						it != m_portMap.end(); ++it )
	{
		for( ch_cnt_t ch = 0; ch < channels(); ++ch )
		{
			if( it.value().ports[ch] == nullptr )
			{
				continue;
			}
			jack_default_audio_sample_t * buf =
			(jack_default_audio_sample_t *) jack_port_get_buffer(
							it.value().ports[ch],
								_nframes );
			for( int frame = 0; frame < frames; ++frame )
			{
				buf[frame] = it.key()->buffer()[frame][ch];
			}
		}
	}
#endif

	jack_nframes_t done = 0;
	while( done < _nframes && m_stopped == false )
	{
		jack_nframes_t todo = qMin<jack_nframes_t>(
						_nframes,
						m_framesToDoInCurBuf -
							m_framesDoneInCurBuf );
		const float gain = audioEngine()->masterGain();
		for( int c = 0; c < channels(); ++c )
		{
			jack_default_audio_sample_t * o = m_tempOutBufs[c];
			for( jack_nframes_t frame = 0; frame < todo; ++frame )
			{
				o[done+frame] = m_outBuf[m_framesDoneInCurBuf+frame][c] * gain;
			}
		}
		done += todo;
		m_framesDoneInCurBuf += todo;
		if( m_framesDoneInCurBuf == m_framesToDoInCurBuf )
		{
			m_framesToDoInCurBuf = getNextBuffer( m_outBuf );
			m_framesDoneInCurBuf = 0;
			if( !m_framesToDoInCurBuf )
			{
				m_stopped = true;
				break;
			}
		}
	}

	if( _nframes != done )
	{
		for( int c = 0; c < channels(); ++c )
		{
			jack_default_audio_sample_t * b = m_tempOutBufs[c] + done;
			memset( b, 0, sizeof( *b ) * ( _nframes - done ) );
		}
	}

	return 0;
}




int AudioJack::staticProcessCallback( jack_nframes_t _nframes, void * _udata )
{
	return static_cast<AudioJack *>( _udata )->
					processCallback( _nframes, _udata );
}




void AudioJack::shutdownCallback( void * _udata )
{
	AudioJack * _this = static_cast<AudioJack *>( _udata );
	_this->m_client = nullptr;
	_this->zombified();
}





AudioJack::setupWidget::setupWidget( QWidget * _parent ) :
	AudioDeviceSetupWidget( AudioJack::name(), _parent )
{
	QString cn = ConfigManager::inst()->value( "audiojack", "clientname" );
	if( cn.isEmpty() )
	{
		cn = "lmms";
	}
	m_clientName = new QLineEdit( cn, this );
	m_clientName->setGeometry( 10, 20, 160, 20 );

	QLabel * cn_lbl = new QLabel( tr( "Client name" ), this );
	cn_lbl->setFont( pointSize<7>( cn_lbl->font() ) );
	cn_lbl->setGeometry( 10, 40, 160, 10 );

	gui::LcdSpinBoxModel * m = new gui::LcdSpinBoxModel( /* this */ );
	m->setRange( DEFAULT_CHANNELS, SURROUND_CHANNELS );
	m->setStep( 2 );
	m->setValue( ConfigManager::inst()->value( "audiojack",
							"channels" ).toInt() );

	m_channels = new gui::LcdSpinBox( 1, this );
	m_channels->setModel( m );
	m_channels->setLabel( tr( "Channels" ) );
	m_channels->move( 180, 20 );

}




AudioJack::setupWidget::~setupWidget()
{
	delete m_channels->model();
}




void AudioJack::setupWidget::saveSettings()
{
	ConfigManager::inst()->setValue( "audiojack", "clientname",
							m_clientName->text() );
	ConfigManager::inst()->setValue( "audiojack", "channels",
				QString::number( m_channels->value<int>() ) );
}



} // namespace lmms

#endif // LMMS_HAVE_JACK
