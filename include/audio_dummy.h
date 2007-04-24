/*
 * audio_dummy.h - dummy-audio-device
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


#ifndef _AUDIO_DUMMY_H
#define _AUDIO_DUMMY_H

#include "audio_device.h"
#include "micro_timer.h"


class audioDummy : public audioDevice, public QThread
{
public:
	audioDummy( const sample_rate_t _sample_rate, bool & _success_ful,
							mixer * _mixer ) :
		audioDevice( _sample_rate, DEFAULT_CHANNELS, _mixer )
	{
		_success_ful = TRUE;
	}

	virtual ~audioDummy()
	{
		stopProcessing();
	}

	inline static QString name( void )
	{
		return( QT_TRANSLATE_NOOP( "setupWidget",
			"Dummy (no sound output)" ) );
	}


	class setupWidget : public audioDevice::setupWidget
	{
	public:
		setupWidget( QWidget * _parent ) :
			audioDevice::setupWidget( audioDummy::name(), _parent )
		{
		}

		virtual ~setupWidget()
		{
		}

		virtual void saveSettings( void )
		{
		}

	} ;


private:
	virtual void startProcessing( void )
	{
		start();
	}

	virtual void stopProcessing( void )
	{
		if( isRunning() )
		{
			wait( 1000 );
			terminate();
		}
	}

	virtual void run( void )
	{
		microTimer timer;
		while( TRUE )
		{
			timer.reset();
			const surroundSampleFrame * b =
						getMixer()->nextBuffer();
			if( !b )
			{
				break;
			}
			delete[] b;

			const Sint32 microseconds = static_cast<Sint32>(
					getMixer()->framesPerAudioBuffer() *
					1000000.0f / getMixer()->sampleRate() -
							timer.elapsed() );
			if( microseconds > 0 )
			{
				usleep( microseconds );
			}
		}
	}

} ;


#endif
