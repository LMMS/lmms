/*
 * AudioSdl.cpp - device-class that performs PCM-output via SDL
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

#include "AudioSdl.h"
#include "lmms_basics.h"

#ifdef LMMS_HAVE_SDL

#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <SDL.h>

#include "AudioEngine.h"
#include "ConfigManager.h"

namespace lmms
{

constexpr char const* const SectionSDL = "audiosdl";
constexpr char const* const PlaybackDeviceSDL = "device";
constexpr char const* const InputDeviceSDL = "inputdevice";

AudioSdl::AudioSdl( bool & _success_ful, AudioEngine*  _audioEngine ) :
	AudioDevice( DEFAULT_CHANNELS, _audioEngine ),
	m_outBuf(new SampleFrame[audioEngine()->framesPerPeriod()])
{
	_success_ful = false;

#ifdef LMMS_HAVE_SDL2
	m_currentBufferFramesCount = 0;
	m_currentBufferFramePos = 0;
#else
	m_convertedBufSize = audioEngine()->framesPerPeriod() * channels()
						* sizeof( int_sample_t );
	m_convertedBufPos = 0;
	m_convertedBuf = new Uint8[m_convertedBufSize];
#endif

	if( SDL_Init( SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE ) < 0 )
	{
		qCritical( "Couldn't initialize SDL: %s\n", SDL_GetError() );
		return;
	}

	m_audioHandle.freq = sampleRate();
#ifdef LMMS_HAVE_SDL2
	m_audioHandle.format = AUDIO_F32SYS;	// we want it in byte-order
						// of system, so we don't have
						// to convert the buffers
#else
	m_audioHandle.format = AUDIO_S16SYS;	// we want it in byte-order
						// of system, so we don't have
						// to convert the buffers
#endif
	m_audioHandle.channels = channels();
	m_audioHandle.samples = std::max(f_cnt_t{1024}, audioEngine()->framesPerPeriod() * 2);

	m_audioHandle.callback = sdlAudioCallback;
	m_audioHandle.userdata = this;

  	SDL_AudioSpec actual; 

#ifdef LMMS_HAVE_SDL2
	const auto playbackDevice = ConfigManager::inst()->value(SectionSDL, PlaybackDeviceSDL).toStdString();
	const bool isDefaultPlayback = playbackDevice.empty();

	// Try with the configured device
	const auto playbackDeviceCStr = isDefaultPlayback ? nullptr : playbackDevice.c_str();
	m_outputDevice = SDL_OpenAudioDevice(playbackDeviceCStr, 0, &m_audioHandle, &actual, 0);

	// If we did not get a device ID try again with the default device if we did not try that before
	if (m_outputDevice == 0 && !isDefaultPlayback)
	{
		m_outputDevice = SDL_OpenAudioDevice(nullptr, 0, &m_audioHandle, &actual, 0);
	}

	if (m_outputDevice == 0) {
		qCritical( "Couldn't open SDL-audio: %s\n", SDL_GetError() );
		return;
	}
#else
	// open the audio device, forcing the desired format
	if( SDL_OpenAudio( &m_audioHandle, &actual ) < 0 )
	{
		qCritical( "Couldn't open SDL-audio: %s\n", SDL_GetError() );
		return;
	}

	m_outConvertEndian = ( m_audioHandle.format != actual.format );
#endif


	_success_ful = true;

#ifdef LMMS_HAVE_SDL2
	// Workaround for a race condition that causes SDL to segfault
	SDL_Delay(50);

	m_inputAudioHandle = m_audioHandle;
	m_inputAudioHandle.callback = sdlInputAudioCallback;

	const auto inputDevice = ConfigManager::inst()->value(SectionSDL, InputDeviceSDL).toStdString();
	const bool isDefaultInput = inputDevice.empty();

	// Try with the configured device
	const auto inputDeviceCStr = isDefaultInput ? nullptr : inputDevice.c_str();
	m_inputDevice = SDL_OpenAudioDevice (inputDeviceCStr, 1, &m_inputAudioHandle, &actual, 0);

	// If we did not get a device ID try again with the default device if we did not try that before
	if (m_inputDevice == 0 && !isDefaultInput)
	{
		m_inputDevice = SDL_OpenAudioDevice(nullptr, 1, &m_inputAudioHandle, &actual, 0);
	}

	if (m_inputDevice != 0) {
		m_supportsCapture = true;
	} else {
		m_supportsCapture = false;
		qWarning ( "Couldn't open SDL capture device: %s\n", SDL_GetError ());
	}

#endif
}




AudioSdl::~AudioSdl()
{
	stopProcessing();

#ifdef LMMS_HAVE_SDL2
	if (m_inputDevice != 0)
		SDL_CloseAudioDevice(m_inputDevice);
	if (m_outputDevice != 0)
		SDL_CloseAudioDevice(m_outputDevice);
#else
	SDL_CloseAudio();
	delete[] m_convertedBuf;
#endif

	SDL_Quit();

	delete[] m_outBuf;
}




void AudioSdl::startProcessing()
{
	m_stopped = false;

#ifdef LMMS_HAVE_SDL2
	SDL_PauseAudioDevice (m_outputDevice, 0);
	SDL_PauseAudioDevice (m_inputDevice, 0);
#else
	SDL_PauseAudio( 0 );
#endif
}




void AudioSdl::stopProcessing()
{
#ifdef LMMS_HAVE_SDL2
	if( SDL_GetAudioDeviceStatus(m_outputDevice) == SDL_AUDIO_PLAYING )
#else
	if( SDL_GetAudioStatus() == SDL_AUDIO_PLAYING )
#endif
	{
#ifdef LMMS_HAVE_SDL2
		SDL_LockAudioDevice (m_inputDevice);
		SDL_LockAudioDevice (m_outputDevice);

		m_stopped = true;

		SDL_PauseAudioDevice (m_inputDevice,	1);
		SDL_PauseAudioDevice (m_outputDevice,	1);

		SDL_UnlockAudioDevice (m_inputDevice);
		SDL_UnlockAudioDevice (m_outputDevice);
#else
		SDL_LockAudio();
		m_stopped = true;
		SDL_PauseAudio( 1 );
		SDL_UnlockAudio();
#endif

	}
}

void AudioSdl::sdlAudioCallback( void * _udata, Uint8 * _buf, int _len )
{
	auto _this = static_cast<AudioSdl*>(_udata);

	_this->sdlAudioCallback( _buf, _len );
}




void AudioSdl::sdlAudioCallback( Uint8 * _buf, int _len )
{
	if( m_stopped )
	{
		memset( _buf, 0, _len );
		return;
	}

	// SDL2: process float samples
#ifdef LMMS_HAVE_SDL2
	while( _len )
	{
		if( m_currentBufferFramePos == 0 )
		{
			// frames depend on the sample rate
			const fpp_t frames = getNextBuffer( m_outBuf );
			if( !frames )
			{
				memset( _buf, 0, _len );
				return;
			}
			m_currentBufferFramesCount = frames;

		}
		const uint min_frames_count = std::min(_len/sizeof(SampleFrame),
										  m_currentBufferFramesCount
										- m_currentBufferFramePos);

		memcpy( _buf, m_outBuf + m_currentBufferFramePos, min_frames_count*sizeof(SampleFrame) );
		_buf += min_frames_count*sizeof(SampleFrame);
		_len -= min_frames_count*sizeof(SampleFrame);
		m_currentBufferFramePos += min_frames_count;

		m_currentBufferFramePos %= m_currentBufferFramesCount;
	}
#else
	while( _len )
	{
		if( m_convertedBufPos == 0 )
		{
			// frames depend on the sample rate
			const fpp_t frames = getNextBuffer( m_outBuf );
			if( !frames )
			{
				m_stopped = true;
				memset( _buf, 0, _len );
				return;
			}
			m_convertedBufSize = frames * channels()
						* sizeof( int_sample_t );

			convertToS16(m_outBuf, frames, reinterpret_cast<int_sample_t*>(m_convertedBuf), m_outConvertEndian);
		}
		const int min_len = std::min(_len, m_convertedBufSize
							- m_convertedBufPos);
		memcpy( _buf, m_convertedBuf + m_convertedBufPos, min_len );
		_buf += min_len;
		_len -= min_len;
		m_convertedBufPos += min_len;
		m_convertedBufPos %= m_convertedBufSize;
	}
#endif // LMMS_HAVE_SDL2
}

#ifdef LMMS_HAVE_SDL2

void AudioSdl::sdlInputAudioCallback(void *_udata, Uint8 *_buf, int _len) {
	auto _this = static_cast<AudioSdl*>(_udata);

	_this->sdlInputAudioCallback( _buf, _len );
}

void AudioSdl::sdlInputAudioCallback(Uint8 *_buf, int _len) {
	auto samples_buffer = (SampleFrame*)_buf;
	fpp_t frames = _len / sizeof ( SampleFrame );

	audioEngine()->pushInputFrames (samples_buffer, frames);
}

#endif

QString AudioSdl::setupWidget::s_systemDefaultDevice = QObject::tr("[System Default]");

AudioSdl::setupWidget::setupWidget( QWidget * _parent ) :
	AudioDeviceSetupWidget( AudioSdl::name(), _parent )
{
	QFormLayout * form = new QFormLayout(this);
	form->setRowWrapPolicy(QFormLayout::WrapLongRows);

	m_playbackDeviceComboBox = new QComboBox(this);

	populatePlaybackDeviceComboBox();

	form->addRow(tr("Playback device"), m_playbackDeviceComboBox);

	m_inputDeviceComboBox = new QComboBox(this);

	populateInputDeviceComboBox();

	form->addRow(tr("Input device"), m_inputDeviceComboBox);
}




void AudioSdl::setupWidget::saveSettings()
{
	const auto currentPlaybackDevice = m_playbackDeviceComboBox->currentText();
	if (currentPlaybackDevice == s_systemDefaultDevice)
	{
		// Represent the default input device with an empty string
		ConfigManager::inst()->setValue(SectionSDL, PlaybackDeviceSDL, "");
	}
	else if (!currentPlaybackDevice.isEmpty())
	{
		ConfigManager::inst()->setValue(SectionSDL, PlaybackDeviceSDL, currentPlaybackDevice);
	}

	const auto currentInputDevice = m_inputDeviceComboBox->currentText();
	if (currentInputDevice == s_systemDefaultDevice)
	{
		// Represent the default input device with an empty string
		ConfigManager::inst()->setValue(SectionSDL, InputDeviceSDL, "");
	}
	else if (!currentInputDevice.isEmpty())
	{
		ConfigManager::inst()->setValue(SectionSDL, InputDeviceSDL, currentInputDevice);
	}
}

void AudioSdl::setupWidget::populatePlaybackDeviceComboBox()
{
#ifdef LMMS_HAVE_SDL2
	m_playbackDeviceComboBox->addItem(s_systemDefaultDevice);

	QStringList playbackDevices;
	const int numberOfPlaybackDevices = SDL_GetNumAudioDevices(0);
	for (int i = 0; i < numberOfPlaybackDevices; ++i)
	{
		const QString deviceName = SDL_GetAudioDeviceName(i, 0);
		playbackDevices.append(deviceName);
	}

	playbackDevices.sort();

	m_playbackDeviceComboBox->addItems(playbackDevices);

	const auto playbackDevice = ConfigManager::inst()->value(SectionSDL, PlaybackDeviceSDL);
	m_playbackDeviceComboBox->setCurrentText(playbackDevice.isEmpty() ? s_systemDefaultDevice : playbackDevice);
#endif
}

void AudioSdl::setupWidget::populateInputDeviceComboBox()
{
#ifdef LMMS_HAVE_SDL2
	m_inputDeviceComboBox->addItem(s_systemDefaultDevice);

	QStringList inputDevices;
	const int numberOfInputDevices = SDL_GetNumAudioDevices(1);
	for (int i = 0; i < numberOfInputDevices; ++i)
	{
		const QString deviceName = SDL_GetAudioDeviceName(i, 1);
		inputDevices.append(deviceName);
	}

	inputDevices.sort();

	m_inputDeviceComboBox->addItems(inputDevices);

	// Set the current device to the one in the configuration
	const auto inputDevice = ConfigManager::inst()->value(SectionSDL, InputDeviceSDL);
	m_inputDeviceComboBox->setCurrentText(inputDevice.isEmpty() ? s_systemDefaultDevice : inputDevice);
#endif
}


} // namespace lmms

#endif // LMMS_HAVE_SDL

