/*
 * audio_sdl.cpp - device-class that performs PCM-output via SDL
 *
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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



#include "audio_sdl.h"

#ifdef SDL_AUDIO_SUPPORT

#ifdef QT4

#include <QLineEdit>
#include <QLabel>

#else

#include <qlineedit.h>
#include <qlabel.h>

#endif


#include "buffer_allocator.h"
#include "debug.h"
#include "config_mgr.h"
#include "gui_templates.h"



audioSDL::audioSDL( Uint32 _sample_rate, bool & _success_ful ) :
	audioDevice( _sample_rate, DEFAULT_CHANNELS ),
	m_buffer( bufferAllocator::alloc<outputSampleType>(
					mixer::inst()->framesPerAudioBuffer() *
					channels() ) ),
	m_bufMutex(),
	m_callbackMutex(),
	m_convertEndian( FALSE )
{
	_success_ful = FALSE;

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
	m_audioHandle.samples = mixer::inst()->framesPerAudioBuffer();

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

	clearS16Buffer( m_buffer, m_audioHandle.samples );

	// start playing
	SDL_PauseAudio( 0 );

	_success_ful = TRUE;
}




audioSDL::~audioSDL()
{
	SDL_PauseAudio( 1 );
	SDL_CloseAudio();
	SDL_Quit();

	m_bufMutex.lock();
	bufferAllocator::free( m_buffer );
	m_bufMutex.unlock();
}





void audioSDL::writeBufferToDev( surroundSampleFrame * _ab, Uint32 _frames,
						float _master_gain )
{
	m_bufMutex.lock();
	convertToS16( _ab, _frames, _master_gain, m_buffer,
							m_convertEndian );
	m_bufMutex.unlock();
	// before returning make sure, callback was called, so we're synced
	// with it (otherwise it could be that (if there's not much to render)
	// this function is called several times although we had to wait until
	// we can proceed with next audio-output)
	m_callbackMutex.lock();
}




void audioSDL::sdlAudioCallback( void * _udata, Uint8 * _buf, int _len )
{
	audioSDL * _this = static_cast<audioSDL *>( _udata );

#ifdef LMMS_DEBUG
	assert( _this != NULL );
#endif

	_this->m_bufMutex.lock();

	// writeBufferToDev() prepared everything for us, so we just have
	// to do a memcpy() :-)
	memcpy( _buf, _this->m_buffer, _len );

	// clear our output buffer, so that we don't output the same noise
	// when being called again without that writeBufferToDev() was called
	// (e.g. if there's too much to render)
	_this->clearS16Buffer( _this->m_buffer, _this->m_audioHandle.samples );

	_this->m_bufMutex.unlock();

	// we got our last buffer, so we let writeBufferToDev() return
	_this->m_callbackMutex.unlock();
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
