#ifndef SINGLE_SOURCE_COMPILE

/*
 * audio_alsa.cpp - device-class which implements ALSA-PCM-output
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */



#include "qt3support.h"

#ifdef QT4

#include <QLineEdit>
#include <QLabel>

#else

#include <qpair.h>
#include <qlineedit.h>
#include <qlabel.h>

#endif


#include "audio_alsa.h"


#ifdef ALSA_SUPPORT

#include "endian_handling.h"
#include "buffer_allocator.h"
#include "config_mgr.h"
#include "lcd_spinbox.h"
#include "gui_templates.h"
#include "templates.h"



audioALSA::audioALSA( const sample_rate_t _sample_rate, bool & _success_ful,
							mixer * _mixer ) :
	audioDevice( _sample_rate, tLimit<ch_cnt_t>(
		configManager::inst()->value( "audioalsa", "channels" ).toInt(),
					DEFAULT_CHANNELS, SURROUND_CHANNELS ),
								_mixer ),
	m_handle( NULL ),
	m_hwParams( NULL ),
	m_swParams( NULL ),
	m_littleEndian( isLittleEndian() ),
	m_quit( FALSE )
{
	_success_ful = FALSE;

	int err;

	if( ( err = snd_pcm_open( &m_handle,
#ifdef QT4
					probeDevice().toAscii().constData(),
#else
					probeDevice().ascii(),
#endif
						SND_PCM_STREAM_PLAYBACK,
						SND_PCM_NONBLOCK ) ) < 0 )
	{
		printf( "Playback open error: %s\n", snd_strerror( err ) );
		return;
	}

	snd_pcm_hw_params_alloca( &m_hwParams );
	snd_pcm_sw_params_alloca( &m_swParams );

	if( ( err = setHWParams( _sample_rate, channels(),
					SND_PCM_ACCESS_RW_INTERLEAVED ) ) < 0 )
	{
		printf( "Setting of hwparams failed: %s\n",
							snd_strerror( err ) );
		return;
	}
	if( ( err = setSWParams() ) < 0 )
	{
		printf( "Setting of swparams failed: %s\n",
							snd_strerror( err ) );
		return;
	}

	_success_ful = TRUE;
}




audioALSA::~audioALSA()
{
	stopProcessing();
	if( m_handle != NULL )
	{
		snd_pcm_close( m_handle );
	}
	// the following code doesn't work and leads to a crash...
/*	if( m_hwParams != NULL )
	{
		snd_pcm_hw_params_free( m_hwParams );
	}

	if( m_swParams != NULL )
	{
		snd_pcm_sw_params_free( m_swParams );
	}*/
}




QString audioALSA::probeDevice( void )
{
	QString dev = configManager::inst()->value( "audioalsa", "device" );
	if( dev == "" )
	{
		if( getenv( "AUDIODEV" ) != NULL )
		{
			return( getenv( "AUDIODEV" ) );
		}
		return( "default" );
	}
	return( dev );
}




int audioALSA::handleError( int _err )
{
	if( _err == -EPIPE )
	{
		// under-run
		_err = snd_pcm_prepare( m_handle );
		if( _err < 0 )
			printf( "Can't recovery from underrun, prepare "
					"failed: %s\n", snd_strerror( _err ) );
		return ( 0 );
	}
	else if( _err == -ESTRPIPE )
	{
		while( ( _err = snd_pcm_resume( m_handle ) ) == -EAGAIN )
		{
			sleep( 1 );	// wait until the suspend flag
					// is released
		}

		if( _err < 0 )
		{
			_err = snd_pcm_prepare( m_handle );
			if( _err < 0 )
				printf( "Can't recovery from suspend, prepare "
					"failed: %s\n", snd_strerror( _err ) );
		}
		return ( 0 );
	}
	return( _err );
}




void audioALSA::startProcessing( void )
{
	if( !isRunning() )
	{
		start(
#ifdef QT4
			QThread::HighPriority
#else
#if QT_VERSION >= 0x030505
			QThread::HighestPriority
#endif
#endif
							);
	}
}




void audioALSA::stopProcessing( void )
{
	if( isRunning() )
	{
		m_quit = TRUE;
		wait( 1000 );
		terminate();
	}
}




void audioALSA::run( void )
{
	surroundSampleFrame * temp =
			bufferAllocator::alloc<surroundSampleFrame>(
					getMixer()->framesPerAudioBuffer() );
	int_sample_t * outbuf = bufferAllocator::alloc<int_sample_t>(
					getMixer()->framesPerAudioBuffer() *
								channels() );
	m_quit = FALSE;

	while( m_quit == FALSE )
	{
		const f_cnt_t frames = getNextBuffer( temp );

		convertToS16( temp, frames, getMixer()->masterGain(), outbuf,
					m_littleEndian != isLittleEndian() );

		f_cnt_t frame = 0;
		int_sample_t * ptr = outbuf;

		while( frame < frames )
		{
			int err = snd_pcm_writei( m_handle, ptr, frames );

			if( err == -EAGAIN )
			{
				usleep( 10 );
				continue;
			}

			if( err < 0 )
			{
				if( handleError( err ) < 0 )
				{
					printf( "Write error: %s\n",
							snd_strerror( err ) );
				}
				break;	// skip this buffer
			}
			ptr += err * channels();
			frame += err;
		}
	}

	bufferAllocator::free( temp );
	bufferAllocator::free( outbuf );
}




int audioALSA::setHWParams( const sample_rate_t _sample_rate,
						const ch_cnt_t _channels,
						snd_pcm_access_t _access )
{
	int err, dir;

	// choose all parameters
	if( ( err = snd_pcm_hw_params_any( m_handle, m_hwParams ) ) < 0 )
	{
		printf( "Broken configuration for playback: no configurations "
				"available: %s\n", snd_strerror( err ) );
		return( err );
	}

	// set the interleaved read/write format
	if( ( err = snd_pcm_hw_params_set_access( m_handle, m_hwParams,
							_access ) ) < 0 )
	{
		printf( "Access type not available for playback: %s\n",
							snd_strerror( err ) );
		return( err );
	}

	// set the sample format
	if( ( snd_pcm_hw_params_set_format( m_handle, m_hwParams,
						SND_PCM_FORMAT_S16_LE ) ) < 0 )
	{
		if( ( snd_pcm_hw_params_set_format( m_handle, m_hwParams,
						SND_PCM_FORMAT_S16_BE ) ) < 0 )
		{
			printf( "Neither little- nor big-endian available for "
					"playback: %s\n", snd_strerror( err ) );
			return( err );
		}
		m_littleEndian = FALSE;
	}

	// set the count of channels
	if( ( err = snd_pcm_hw_params_set_channels( m_handle, m_hwParams,
							_channels ) ) < 0 )
	{
		printf( "Channel count (%i) not available for playbacks: %s\n"
				"(Does your soundcard not support surround?)\n",
					_channels, snd_strerror( err ) );
		return( err );
	}

	// set the sample rate
	if( ( err = snd_pcm_hw_params_set_rate( m_handle, m_hwParams,
						_sample_rate, 0 ) ) < 0 )
	{
		int q = 0;
		if( sampleRate() == SAMPLE_RATES[1] )
		{
			q = 1;
		}
		if( sampleRate() == 44100 || sampleRate() == 88200 )
		{
			SAMPLE_RATES[0] = 48000;
			SAMPLE_RATES[1] = 96000;
		}
		else
		{
			SAMPLE_RATES[0] = 44100;
			SAMPLE_RATES[1] = 82000;
		}
		setSampleRate( SAMPLE_RATES[q] );
		if( ( err = snd_pcm_hw_params_set_rate( m_handle, m_hwParams,
						SAMPLE_RATES[q], 0 ) ) < 0 )
		{
			printf( "Could not set sample rate: %s\n",
							snd_strerror( err ) );
			return( err );
		}
	}

	m_periodSize = getMixer()->framesPerAudioBuffer();
	m_bufferSize = m_periodSize * 8;
	dir = 0;
	err = snd_pcm_hw_params_set_period_size_near( m_handle, m_hwParams,
							&m_periodSize, &dir );
	if( err < 0 )
	{
		printf( "Unable to set period size %lu for playback: %s\n",
					m_periodSize, snd_strerror( err ) );
		return( err );
	}
	dir = 0;
	err = snd_pcm_hw_params_get_period_size( m_hwParams, &m_periodSize,
									&dir );
	if( err < 0 )
	{
		printf( "Unable to get period size for playback: %s\n",
							snd_strerror( err ) );
	}

	dir = 0;
	err = snd_pcm_hw_params_set_buffer_size_near( m_handle, m_hwParams,
								&m_bufferSize );
	if( err < 0 )
	{
		printf( "Unable to set buffer size %lu for playback: %s\n",
					m_bufferSize, snd_strerror( err ) );
		return ( err );
	}
	err = snd_pcm_hw_params_get_buffer_size( m_hwParams, &m_bufferSize );

	if( 2 * m_periodSize > m_bufferSize )
	{
		printf( "buffer to small, could not use\n" );
		return ( err );
	}


	// write the parameters to device
	err = snd_pcm_hw_params( m_handle, m_hwParams );
	if( err < 0 )
	{
		printf( "Unable to set hw params for playback: %s\n",
							snd_strerror( err ) );
		return ( err );
	}

	return ( 0 );	// all ok
}




int audioALSA::setSWParams( void )
{
	int err;

	// get the current swparams
	if( ( err = snd_pcm_sw_params_current( m_handle, m_swParams ) ) < 0 )
	{
		printf( "Unable to determine current swparams for playback: %s"
						"\n", snd_strerror( err ) );
		return( err );
	}

	// start the transfer when a period is full
	if( ( err = snd_pcm_sw_params_set_start_threshold( m_handle,
					m_swParams, m_periodSize ) ) < 0 )
	{
		printf( "Unable to set start threshold mode for playback: %s\n",
							snd_strerror( err ) );
		return( err );
	}

	// allow the transfer when at least m_periodSize samples can be
	// processed
	if( ( err = snd_pcm_sw_params_set_avail_min( m_handle, m_swParams,
							m_periodSize ) ) < 0 )
	{
		printf( "Unable to set avail min for playback: %s\n",
							snd_strerror( err ) );
		return( err );
	}

	// align all transfers to 1 sample
	if( ( err = snd_pcm_sw_params_set_xfer_align( m_handle,
							m_swParams, 1 ) ) < 0 )
	{
		printf( "Unable to set transfer align for playback: %s\n",
							snd_strerror( err ) );
		return( err );
	}

	// write the parameters to the playback device
	if( ( err = snd_pcm_sw_params( m_handle, m_swParams ) ) < 0 )
	{
		printf( "Unable to set sw params for playback: %s\n",
							snd_strerror( err ) );
		return( err );
	}

	return( 0 );	// all ok
}





audioALSA::setupWidget::setupWidget( QWidget * _parent ) :
	audioDevice::setupWidget( audioALSA::name(), _parent )
{
	m_device = new QLineEdit( audioALSA::probeDevice(), this );
	m_device->setGeometry( 10, 20, 160, 20 );

	QLabel * dev_lbl = new QLabel( tr( "DEVICE" ), this );
	dev_lbl->setFont( pointSize<6>( dev_lbl->font() ) );
	dev_lbl->setGeometry( 10, 40, 160, 10 );

	m_channels = new lcdSpinBox( DEFAULT_CHANNELS, SURROUND_CHANNELS, 1,
									this );
	m_channels->setStep( 2 );
	m_channels->setLabel( tr( "CHANNELS" ) );
	m_channels->setValue( configManager::inst()->value( "audioalsa",
							"channels" ).toInt() );
	m_channels->move( 180, 20 );

}




audioALSA::setupWidget::~setupWidget()
{

}




void audioALSA::setupWidget::saveSettings( void )
{
	configManager::inst()->setValue( "audioalsa", "device",
							m_device->text() );
	configManager::inst()->setValue( "audioalsa", "channels",
				QString::number( m_channels->value() ) );
}


#endif


#endif
