#ifndef SINGLE_SOURCE_COMPILE

/*
 * audio_sdl.cpp - device-class that performs PCM-output via SDL
 *
 * Copyright (c) 2004-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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



#include "audio_sdl.h"

#ifdef SDL_AUDIO_SUPPORT

#ifdef QT4

#include <QtGui/QLabel>
#include <QtGui/QLineEdit>

#else

#include <qlineedit.h>
#include <qlabel.h>

#endif


#include "debug.h"
#include "config_mgr.h"
#include "gui_templates.h"
#include "templates.h"




audioSDL::audioSDL( const sample_rate_t _sample_rate, bool & _success_ful,
							mixer * _mixer ) :
	audioDevice( _sample_rate, DEFAULT_CHANNELS, _mixer ),
	m_outBuf( new surroundSampleFrame[getMixer()->framesPerPeriod()] ),
	m_convertedBuf_pos( 0 ),
	m_convertEndian( FALSE ),
	m_stop_semaphore( 1 )
{
	_success_ful = FALSE;

	m_convertedBuf_size = getMixer()->framesPerPeriod() * channels()
						* sizeof( int_sample_t );
	m_convertedBuf = new Uint8[m_convertedBuf_size];

/*	// if device is set, we set AUDIODEV-environment-variable, so that
	// SDL can evaluate and use it
	QString dev = configManager::inst()->value( "audiosdl", "device" );
	if( dev != "" )
	{
		putenv( const_cast<char *>( ( "AUDIODEV=" + dev ).
#ifdef QT4
			toAscii().constData() ) );
#else
			ascii() ) );
#endif
	}*/


	if( SDL_Init( SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE ) < 0 )
	{
		printf( "Couldn't initialize SDL: %s\n", SDL_GetError() );
		return;
	}

	m_audioHandle.freq = _sample_rate;
	m_audioHandle.format = AUDIO_S16SYS;	// we want it in byte-order
						// of system, so we don't have
						// to convert the buffers
	m_audioHandle.channels = channels();
	m_audioHandle.samples = getMixer()->framesPerPeriod();

	m_audioHandle.callback = sdlAudioCallback;
	m_audioHandle.userdata = this;

  	SDL_AudioSpec actual; 

	// open the audio device, forcing the desired format
	if( SDL_OpenAudio( &m_audioHandle, &actual ) < 0 )
	{
		printf( "Couldn't open SDL-audio: %s\n", SDL_GetError() );
		return;
	}
	m_convertEndian = ( m_audioHandle.format != actual.format );

#ifndef QT3
	m_stop_semaphore.acquire();
#else
	m_stop_semaphore++;
#endif

	_success_ful = TRUE;
}




audioSDL::~audioSDL()
{
	stopProcessing();
#ifndef QT3
	m_stop_semaphore.release();
#else
	m_stop_semaphore--;
#endif
	SDL_CloseAudio();
	SDL_Quit();
	delete[] m_convertedBuf;
	delete[] m_outBuf;
}




void audioSDL::startProcessing( void )
{
	m_stopped = FALSE;

	SDL_PauseAudio( 0 );
	SDL_UnlockAudio();
}




void audioSDL::stopProcessing( void )
{
	if( SDL_GetAudioStatus() == SDL_AUDIO_PLAYING )
	{
#ifndef QT3
		m_stop_semaphore.acquire();
#else
		m_stop_semaphore++;
#endif

		SDL_LockAudio();
		SDL_PauseAudio( 1 );
	}
}




void audioSDL::sdlAudioCallback( void * _udata, Uint8 * _buf, int _len )
{
	audioSDL * _this = static_cast<audioSDL *>( _udata );

#ifdef LMMS_DEBUG
	assert( _this != NULL );
#endif

	_this->sdlAudioCallback( _buf, _len );
}




void audioSDL::sdlAudioCallback( Uint8 * _buf, int _len )
{
	if( m_stopped )
	{
		memset( _buf, 0, _len );
		return;
	}

	while( _len )
	{
		if( m_convertedBuf_pos == 0 )
		{
			// frames depend on the sample rate
			const fpp_t frames = getNextBuffer( m_outBuf );
			if( !frames )
			{
				m_stopped = TRUE;
#ifndef QT3
				m_stop_semaphore.release();
#else
				m_stop_semaphore--;
#endif
				memset( _buf, 0, _len );
				return;
			}
			m_convertedBuf_size = frames * channels()
						* sizeof( int_sample_t );

			convertToS16( m_outBuf, frames,
						getMixer()->masterGain(),
						(int_sample_t *)m_convertedBuf,
						m_convertEndian );
		}
		int min_len = tMin( _len, m_convertedBuf_size
							- m_convertedBuf_pos );
		memcpy( _buf, m_convertedBuf + m_convertedBuf_pos, min_len );
		_buf += min_len;
		_len -= min_len;
		m_convertedBuf_pos += min_len;
		m_convertedBuf_pos %= m_convertedBuf_size;
	}
}




audioSDL::setupWidget::setupWidget( QWidget * _parent ) :
	audioDevice::setupWidget( audioSDL::name(), _parent )
{
	QString dev = configManager::inst()->value( "audiosdl", "device" );
	m_device = new QLineEdit( dev, this );
	m_device->setGeometry( 10, 20, 160, 20 );

	QLabel * dev_lbl = new QLabel( tr( "DEVICE" ), this );
	dev_lbl->setFont( pointSize<6>( dev_lbl->font() ) );
	dev_lbl->setGeometry( 10, 40, 160, 10 );

}




audioSDL::setupWidget::~setupWidget()
{
}




void audioSDL::setupWidget::saveSettings( void )
{
	configManager::inst()->setValue( "audiosdl", "device",
							m_device->text() );
}


#endif

#endif
