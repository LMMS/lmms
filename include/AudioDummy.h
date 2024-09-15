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

#ifndef LMMS_AUDIO_DUMMY_H
#define LMMS_AUDIO_DUMMY_H

#include "AudioDevice.h"
#include "AudioDeviceSetupWidget.h"
#include "AudioEngine.h"
#include "MicroTimer.h"

namespace lmms
{

class AudioDummy : public QThread, public AudioDevice
{
	Q_OBJECT
public:
	AudioDummy( bool & _success_ful, AudioEngine* audioEngine ) :
		AudioDevice( DEFAULT_CHANNELS, audioEngine )
	{
		_success_ful = true;
	}

	~AudioDummy() override
	{
		stopProcessing();
	}

	inline static QString name()
	{
		return QT_TRANSLATE_NOOP( "AudioDeviceSetupWidget", "Dummy (no sound output)" );
	}


	class setupWidget : public gui::AudioDeviceSetupWidget
	{
	public:
		setupWidget( QWidget * _parent ) :
			gui::AudioDeviceSetupWidget( AudioDummy::name(), _parent )
		{
		}

		~setupWidget() override = default;

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
			const SampleFrame* b = audioEngine()->nextBuffer();
			if( !b )
			{
				break;
			}
			if( audioEngine()->hasFifoWriter() )
			{
				delete[] b;
			}

			const int microseconds = static_cast<int>( audioEngine()->framesPerPeriod() * 1000000.0f / audioEngine()->outputSampleRate() - timer.elapsed() );
			if( microseconds > 0 )
			{
				usleep( microseconds );
			}
		}
	}

} ;

} // namespace lmms

#endif // LMMS_AUDIO_DUMMY_H
