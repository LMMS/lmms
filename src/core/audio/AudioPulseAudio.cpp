/*
 * AudioPulseAudio.cpp - device-class which implements PulseAudio-output
 *
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QtGui/QLineEdit>
#include <QtGui/QLabel>

#include "AudioPulseAudio.h"

#ifdef LMMS_HAVE_PULSEAUDIO

#include "endian_handling.h"
#include "config_mgr.h"
#include "LcdSpinBox.h"
#include "gui_templates.h"
#include "templates.h"
#include "engine.h"


static void stream_write_callback(pa_stream *s, size_t length, void *userdata)
{
	static_cast<AudioPulseAudio *>( userdata )->streamWriteCallback( s, length );
}




AudioPulseAudio::AudioPulseAudio( bool & _success_ful, Mixer*  _mixer ) :
	AudioDevice( tLimit<ch_cnt_t>(
		configManager::inst()->value( "audiopa", "channels" ).toInt(),
					DEFAULT_CHANNELS, SURROUND_CHANNELS ),
								_mixer ),
	m_s( NULL ),
	m_quit( false ),
	m_convertEndian( false )
{
	_success_ful = false;

	m_sampleSpec.format = PA_SAMPLE_S16LE;
	m_sampleSpec.rate = sampleRate();
	m_sampleSpec.channels = channels();

	_success_ful = true;
}




AudioPulseAudio::~AudioPulseAudio()
{
	stopProcessing();
}




QString AudioPulseAudio::probeDevice()
{
	QString dev = configManager::inst()->value( "audiopa", "device" );
	if( dev.isEmpty() )
	{
		if( getenv( "AUDIODEV" ) != NULL )
		{
			return getenv( "AUDIODEV" );
		}
		return "default";
	}
	return dev;
}




void AudioPulseAudio::startProcessing()
{
	if( !isRunning() )
	{
		start( QThread::HighPriority );
	}
}




void AudioPulseAudio::stopProcessing()
{
	if( isRunning() )
	{
		wait( 1000 );
		terminate();
	}
}




void AudioPulseAudio::applyQualitySettings()
{
	if( hqAudio() )
	{
//		setSampleRate( engine::mixer()->processingSampleRate() );

	}

	AudioDevice::applyQualitySettings();
}




/* This routine is called whenever the stream state changes */
static void stream_state_callback( pa_stream *s, void * userdata )
{
	switch( pa_stream_get_state( s ) )
	{
		case PA_STREAM_CREATING:
		case PA_STREAM_TERMINATED:
			break;

		case PA_STREAM_READY:
			qDebug( "Stream successfully created\n" );
			break;

		case PA_STREAM_FAILED:
		default:
			qCritical( "Stream errror: %s\n",
					pa_strerror(pa_context_errno(
						pa_stream_get_context( s ) ) ) );
	}
}



/* This is called whenever the context status changes */
static void context_state_callback(pa_context *c, void *userdata)
{
	AudioPulseAudio * _this = static_cast<AudioPulseAudio *>( userdata );
	switch( pa_context_get_state( c ) )
	{
		case PA_CONTEXT_CONNECTING:
		case PA_CONTEXT_AUTHORIZING:
		case PA_CONTEXT_SETTING_NAME:
			break;

		case PA_CONTEXT_READY:
		{
			qDebug( "Connection established.\n" );
			_this->m_s = pa_stream_new( c, "lmms", &_this->m_sampleSpec,  NULL);
			pa_stream_set_state_callback( _this->m_s, stream_state_callback, _this );
			pa_stream_set_write_callback( _this->m_s, stream_write_callback, _this );

			pa_buffer_attr buffer_attr;

			buffer_attr.maxlength = (uint32_t)(-1);

			// play silence in case of buffer underun instead of using default rewind
			buffer_attr.prebuf = 0;

			buffer_attr.minreq = (uint32_t)(-1);
			buffer_attr.fragsize = (uint32_t)(-1);

			double latency = (double)( engine::mixer()->framesPerPeriod() ) /
													(double)_this->sampleRate();

			// ask PulseAudio for the desired latency (which might not be approved)
			buffer_attr.tlength = pa_usec_to_bytes( latency * PA_USEC_PER_MSEC,
														&_this->m_sampleSpec );

			pa_stream_connect_playback( _this->m_s, NULL, &buffer_attr,
										PA_STREAM_ADJUST_LATENCY,
										NULL,	// volume
										NULL );
			break;
		}

		case PA_CONTEXT_TERMINATED:
			break;

		case PA_CONTEXT_FAILED:
		default:
			qCritical( "Connection failure: %s\n", pa_strerror( pa_context_errno( c ) ) );
	}
}




void AudioPulseAudio::run()
{
	pa_mainloop * mainLoop = pa_mainloop_new();
	if( !mainLoop )
	{
		qCritical( "pa_mainloop_new() failed.\n" );
		return;
	}
	pa_mainloop_api * mainloop_api = pa_mainloop_get_api( mainLoop );

	pa_context *context = pa_context_new( mainloop_api, "lmms" );
	if ( context == NULL )
	{
		qCritical( "pa_context_new() failed." );
		return;
	}

	pa_context_set_state_callback( context, context_state_callback, this  );
	// connect the context
	pa_context_connect( context, NULL, (pa_context_flags) 0, NULL );

	// run the main loop
	int ret = 0;
	m_quit = false;
	while( m_quit == false && pa_mainloop_iterate( mainLoop, 1, &ret ) >= 0 )
	{
	}

	pa_stream_disconnect( m_s );
	pa_stream_unref( m_s );

	pa_context_disconnect( context );
	pa_context_unref( context );

	pa_mainloop_free( mainLoop );
}




void AudioPulseAudio::streamWriteCallback( pa_stream *s, size_t length )
{
	const fpp_t fpp = mixer()->framesPerPeriod();
	surroundSampleFrame * temp = new surroundSampleFrame[fpp];
	int_sample_t* pcmbuf = (int_sample_t *)pa_xmalloc( fpp * channels() * sizeof(int_sample_t) );

	size_t fd = 0;
	while( fd < length/4 && m_quit == false )
	{
		const fpp_t frames = getNextBuffer( temp );
		if( !frames )
		{
			m_quit = true;
			break;
		}
		int bytes = convertToS16( temp, frames,
						mixer()->masterGain(),
						pcmbuf,
						m_convertEndian );
		if( bytes > 0 )
		{
			pa_stream_write( m_s, pcmbuf, bytes, NULL, 0,
							PA_SEEK_RELATIVE );
		}
		fd += frames;
	}

	pa_xfree( pcmbuf );
	delete[] temp;
}




AudioPulseAudio::setupWidget::setupWidget( QWidget * _parent ) :
	AudioDevice::setupWidget( AudioPulseAudio::name(), _parent )
{
	m_device = new QLineEdit( AudioPulseAudio::probeDevice(), this );
	m_device->setGeometry( 10, 20, 160, 20 );

	QLabel * dev_lbl = new QLabel( tr( "DEVICE" ), this );
	dev_lbl->setFont( pointSize<7>( dev_lbl->font() ) );
	dev_lbl->setGeometry( 10, 40, 160, 10 );

	LcdSpinBoxModel * m = new LcdSpinBoxModel( /* this */ );
	m->setRange( DEFAULT_CHANNELS, SURROUND_CHANNELS );
	m->setStep( 2 );
	m->setValue( configManager::inst()->value( "audiopa",
							"channels" ).toInt() );

	m_channels = new LcdSpinBox( 1, this );
	m_channels->setModel( m );
	m_channels->setLabel( tr( "CHANNELS" ) );
	m_channels->move( 180, 20 );

}




AudioPulseAudio::setupWidget::~setupWidget()
{
	delete m_channels->model();
}




void AudioPulseAudio::setupWidget::saveSettings()
{
	configManager::inst()->setValue( "audiopa", "device",
							m_device->text() );
	configManager::inst()->setValue( "audiopa", "channels",
				QString::number( m_channels->value<int>() ) );
}


#endif

