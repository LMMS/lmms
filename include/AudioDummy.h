/*
 * AudioDummy.h - dummy audio-device
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _AUDIO_DUMMY_H
#define _AUDIO_DUMMY_H

#include "AudioDevice.h"
#include "MicroTimer.h"


class AudioDummy : public AudioDevice, public QThread
{
public:
	AudioDummy( bool & _success_ful, Mixer* mixer ) :
		AudioDevice( DEFAULT_CHANNELS, mixer )
	{
		_success_ful = true;
	}

	virtual ~AudioDummy()
	{
		stopProcessing();
	}

	inline static QString name()
	{
		return QT_TRANSLATE_NOOP( "setupWidget", "Dummy (no sound output)" );
	}


	class setupWidget : public AudioDevice::setupWidget
	{
	public:
		setupWidget( QWidget * _parent ) :
			AudioDevice::setupWidget( AudioDummy::name(), _parent )
		{
		}

		virtual ~setupWidget()
		{
		}

		virtual void saveSettings()
		{
		}

		virtual void show()
		{
			parentWidget()->hide();
			QWidget::show();
		}

	} ;


private:
	virtual void startProcessing()
	{
		start();
	}

	virtual void stopProcessing()
	{
		if( isRunning() )
		{
			wait( 1000 );
			terminate();
		}
	}

	virtual void run()
	{
		MicroTimer timer;
		while( true )
		{
			timer.reset();
			const surroundSampleFrame* b = mixer()->nextBuffer();
			if( !b )
			{
				break;
			}
			delete[] b;

			const int microseconds = static_cast<int>( mixer()->framesPerPeriod() * 1000000.0f / mixer()->processingSampleRate() - timer.elapsed() );
			if( microseconds > 0 )
			{
				usleep( microseconds );
			}
		}
	}

} ;


#endif
