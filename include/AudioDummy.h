/*
 * AudioDummy.h - dummy audio-device
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef AUDIO_DUMMY_H
#define AUDIO_DUMMY_H

#include "AudioDevice.h"
#include "AudioDeviceSetupWidget.h"
#include "MicroTimer.h"
#include "Mixer.h"


class AudioDummy : public QThread, public AudioDevice
{
	Q_OBJECT
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


	class setupWidget : public AudioDeviceSetupWidget
	{
	public:
		setupWidget( QWidget * _parent ) :
			AudioDeviceSetupWidget( AudioDummy::name(), _parent )
		{
		}

		virtual ~setupWidget()
		{
		}

		void saveSettings() override
		{
		}

		void show() override
		{
			parentWidget()->hide();
			QWidget::show();
		}

	} ;


private:
	void startProcessing() override
	{
		start();
	}

	void stopProcessing() override
	{
		stopProcessingThread( this );
	}

	void run() override
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
			if( mixer()->hasFifoWriter() )
			{
				delete[] b;
			}

			const int microseconds = static_cast<int>( mixer()->framesPerPeriod() * 1000000.0f / mixer()->processingSampleRate() - timer.elapsed() );
			if( microseconds > 0 )
			{
				usleep( microseconds );
			}
		}
	}

} ;


#endif
