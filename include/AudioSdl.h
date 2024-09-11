/*
 * AudioSdl.h - device-class that performs PCM-output via SDL
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_AUDIO_SDL_H
#define LMMS_AUDIO_SDL_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_SDL

#include <SDL2/SDL_audio.h>

#include "AudioDevice.h"
#include "AudioDeviceSetupWidget.h"

class QComboBox;

namespace lmms
{

class AudioSdl : public AudioDevice
{
public:
	AudioSdl( bool & _success_ful, AudioEngine* audioEngine );
	~AudioSdl() override;

	inline static QString name()
	{
		return QT_TRANSLATE_NOOP( "AudioDeviceSetupWidget",
					"SDL (Simple DirectMedia Layer)" );
	}


	class setupWidget : public gui::AudioDeviceSetupWidget
	{
	public:
		setupWidget( QWidget * _parent );
		~setupWidget() override = default;

		void saveSettings() override;
	
	private:
		void populatePlaybackDeviceComboBox();
		void populateInputDeviceComboBox();

	private:
		QComboBox* m_playbackDeviceComboBox = nullptr;
		QComboBox* m_inputDeviceComboBox = nullptr;

		static QString s_systemDefaultDevice;
	} ;


private:
	void startProcessing() override;
	void stopProcessing() override;

	static void sdlAudioCallback( void * _udata, Uint8 * _buf, int _len );
	void sdlAudioCallback( Uint8 * _buf, int _len );

	static void sdlInputAudioCallback( void * _udata, Uint8 * _buf, int _len );
	void sdlInputAudioCallback( Uint8 * _buf, int _len );

	SDL_AudioSpec m_audioHandle;

	SampleFrame* m_outBuf;

	size_t m_currentBufferFramePos;
	size_t m_currentBufferFramesCount;

	bool m_stopped;

	SDL_AudioDeviceID m_outputDevice;

	SDL_AudioSpec m_inputAudioHandle;
	SDL_AudioDeviceID m_inputDevice;
} ;


} // namespace lmms

#endif // LMMS_HAVE_SDL

#endif // LMMS_AUDIO_SDL_H
