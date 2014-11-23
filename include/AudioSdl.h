/*
 * AudioSdl.h - device-class that performs PCM-output via SDL
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _AUDIO_SDL_H
#define _AUDIO_SDL_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_SDL

#include <SDL/SDL.h>
#include <SDL/SDL_audio.h>

#include "AudioDevice.h"

class QLineEdit;


class AudioSdl : public AudioDevice
{
public:
	AudioSdl( bool & _success_ful, Mixer* mixer );
	virtual ~AudioSdl();

	inline static QString name()
	{
		return QT_TRANSLATE_NOOP( "setupWidget",
					"SDL (Simple DirectMedia Layer)" );
	}


	class setupWidget : public AudioDevice::setupWidget
	{
	public:
		setupWidget( QWidget * _parent );
		virtual ~setupWidget();

		virtual void saveSettings();

	private:
		QLineEdit * m_device;

	} ;


private:
	virtual void startProcessing();
	virtual void stopProcessing();
	virtual void applyQualitySettings();

	static void sdlAudioCallback( void * _udata, Uint8 * _buf, int _len );
	void sdlAudioCallback( Uint8 * _buf, int _len );

	SDL_AudioSpec m_audioHandle;

	surroundSampleFrame * m_outBuf;
	Uint8 * m_convertedBuf;
	int m_convertedBufPos;
	int m_convertedBufSize;

	bool m_convertEndian;

	volatile bool m_stopped;
	QSemaphore m_stopSemaphore;

} ;

#endif

#endif
