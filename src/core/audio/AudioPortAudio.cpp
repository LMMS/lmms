/*
 * AudioPortAudio.cpp - device-class that performs PCM-output via PortAudio
 *
 * Copyright (c) 2008 Csaba Hruska <csaba.hruska/at/gmail.com>
 * Copyright (c) 2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "AudioPortAudio.h"

#ifdef LMMS_HAVE_PORTAUDIO

#include <QFormLayout>

#include "Engine.h"
#include "ConfigManager.h"
#include "ComboBox.h"
#include "AudioEngine.h"

namespace lmms
{

AudioPortAudio::AudioPortAudio(bool& successful, AudioEngine* engine)
	: AudioDevice(std::clamp<ch_cnt_t>(ConfigManager::inst()->value("audioportaudio", "channels").toInt(),
					  DEFAULT_CHANNELS, DEFAULT_CHANNELS),
		  engine)
	, m_paStream(nullptr)
	, m_wasPAInitError(false)
	, m_outBuf(new SampleFrame[audioEngine()->framesPerPeriod()])
	, m_outBufPos(0)
{
	successful = false;
	m_outBufSize = audioEngine()->framesPerPeriod();

	PaError err = Pa_Initialize();
	
	if( err != paNoError ) {
		printf( "Couldn't initialize PortAudio: %s\n", Pa_GetErrorText( err ) );
		m_wasPAInitError = true;
		return;
	}

	if( Pa_GetDeviceCount() <= 0 )
	{
		return;
	}
	
	const QString& backend = ConfigManager::inst()->value( "audioportaudio", "backend" );
	const QString& device = ConfigManager::inst()->value( "audioportaudio", "device" );
		
	PaDeviceIndex inDevIdx = -1;
	PaDeviceIndex outDevIdx = -1;
	for( int i = 0; i < Pa_GetDeviceCount(); ++i )
	{
		const auto di = Pa_GetDeviceInfo(i);
		if( di->name == device &&
			Pa_GetHostApiInfo( di->hostApi )->name == backend )
		{
			inDevIdx = i;
			outDevIdx = i;
		}
	}

	if( inDevIdx < 0 )
	{
		inDevIdx = Pa_GetDefaultInputDevice();
	}
	
	if( outDevIdx < 0 )
	{
		outDevIdx = Pa_GetDefaultOutputDevice();
	}

	if( inDevIdx < 0 || outDevIdx < 0 )
	{
		return;
	}

	const int samples = audioEngine()->framesPerPeriod();
	
	m_outputParameters.device = outDevIdx;
	m_outputParameters.channelCount = channels();
	m_outputParameters.sampleFormat = paFloat32;
	m_outputParameters.suggestedLatency = Pa_GetDeviceInfo(outDevIdx)->defaultLowOutputLatency;
	m_outputParameters.hostApiSpecificStreamInfo = nullptr;

	m_inputParameters.device = inDevIdx;
	m_inputParameters.channelCount = DEFAULT_CHANNELS;
	m_inputParameters.sampleFormat = paFloat32;
	m_inputParameters.suggestedLatency = Pa_GetDeviceInfo(inDevIdx)->defaultLowInputLatency;
	m_inputParameters.hostApiSpecificStreamInfo = nullptr;

	err = Pa_OpenStream(&m_paStream, supportsCapture() ? &m_inputParameters : nullptr, &m_outputParameters,
		sampleRate(), samples, paNoFlag, processCallback, this);

	if( err == paInvalidDevice && sampleRate() < 48000 )
	{
		printf("Pa_OpenStream() failed with 44,1 KHz, trying again with 48 KHz\n");
		// some backends or drivers do not allow 32 bit floating point data
		// with a samplerate of 44100 Hz
		setSampleRate( 48000 );
		err = Pa_OpenStream(&m_paStream, supportsCapture() ? &m_inputParameters : nullptr, &m_outputParameters,
			sampleRate(), samples, paNoFlag, processCallback, this);
	}

	if( err != paNoError )
	{
		printf( "Couldn't open PortAudio: %s\n", Pa_GetErrorText( err ) );
		return;
	}

	printf( "Input device: '%s' backend: '%s'\n", Pa_GetDeviceInfo( inDevIdx )->name, Pa_GetHostApiInfo( Pa_GetDeviceInfo( inDevIdx )->hostApi )->name );
	printf( "Output device: '%s' backend: '%s'\n", Pa_GetDeviceInfo( outDevIdx )->name, Pa_GetHostApiInfo( Pa_GetDeviceInfo( outDevIdx )->hostApi )->name );

	successful = true;
}




AudioPortAudio::~AudioPortAudio()
{
	stopProcessing();

	if( !m_wasPAInitError )
	{
		Pa_Terminate();
	}
	delete[] m_outBuf;
}




void AudioPortAudio::startProcessing()
{
	m_stopped = false;
	PaError err = Pa_StartStream( m_paStream );
	
	if( err != paNoError )
	{
		m_stopped = true;
		printf( "PortAudio error: %s\n", Pa_GetErrorText( err ) );
	}
}




void AudioPortAudio::stopProcessing()
{
	if( m_paStream && Pa_IsStreamActive( m_paStream ) )
	{
		m_stopped = true;
		PaError err = Pa_StopStream( m_paStream );
	
		if( err != paNoError )
		{
			printf( "PortAudio error: %s\n", Pa_GetErrorText( err ) );
		}
	}
}


int AudioPortAudio::processCallback(const float* inputBuffer, float* outputBuffer, f_cnt_t framesPerBuffer)
{
	if (supportsCapture()) { audioEngine()->pushInputFrames((SampleFrame*)inputBuffer, framesPerBuffer); }

	if( m_stopped )
	{
		memset(outputBuffer, 0, framesPerBuffer * channels() * sizeof(float));
		return paComplete;
	}

	while (framesPerBuffer)
	{
		if( m_outBufPos == 0 )
		{
			const fpp_t frames = getNextBuffer( m_outBuf );
			if( !frames )
			{
				m_stopped = true;
				memset(outputBuffer, 0, framesPerBuffer * channels() * sizeof(float));
				return paComplete;
			}
			m_outBufSize = frames;
		}
		const auto min_len = std::min(framesPerBuffer, m_outBufSize - m_outBufPos);

		for( fpp_t frame = 0; frame < min_len; ++frame )
		{
			for( ch_cnt_t chnl = 0; chnl < channels(); ++chnl )
			{
				(outputBuffer + frame * channels())[chnl] = AudioEngine::clip(m_outBuf[frame][chnl]);
			}
		}

		outputBuffer += min_len * channels();
		framesPerBuffer -= min_len;
		m_outBufPos += min_len;
		m_outBufPos %= m_outBufSize;
	}

	return paContinue;
}

int AudioPortAudio::processCallback(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* arg)
{
	Q_UNUSED(timeInfo);
	Q_UNUSED(statusFlags);

	auto _this = static_cast<AudioPortAudio*>(arg);
	return _this->processCallback(
		static_cast<const float*>(inputBuffer), static_cast<float*>(outputBuffer), framesPerBuffer);
}




void gui::AudioPortAudioSetupWidget::updateBackends()
{
	PaError err = Pa_Initialize();
	if( err != paNoError ) {
		printf( "Couldn't initialize PortAudio: %s\n", Pa_GetErrorText( err ) );
		return;
	}

	for( int i = 0; i < Pa_GetHostApiCount(); ++i )
	{
		const auto hi = Pa_GetHostApiInfo(i);
		m_backendModel.addItem( hi->name );
	}

	Pa_Terminate();
}




void gui::AudioPortAudioSetupWidget::updateDevices()
{
	PaError err = Pa_Initialize();
	if( err != paNoError ) {
		printf( "Couldn't initialize PortAudio: %s\n", Pa_GetErrorText( err ) );
		return;
	}

	const QString& backend = m_backendModel.currentText();
	int hostApi = 0;
	for( int i = 0; i < Pa_GetHostApiCount(); ++i )
	{
		const auto hi = Pa_GetHostApiInfo(i);
		if( backend == hi->name )
		{
			hostApi = i;
			break;
		}
	}

	m_deviceModel.clear();
	for( int i = 0; i < Pa_GetDeviceCount(); ++i )
	{
		const auto di = Pa_GetDeviceInfo(i);
		if( di->hostApi == hostApi )
		{
			m_deviceModel.addItem( di->name );
		}
	}
	Pa_Terminate();
}




void gui::AudioPortAudioSetupWidget::updateChannels()
{
	PaError err = Pa_Initialize();
	if( err != paNoError ) {
		printf( "Couldn't initialize PortAudio: %s\n", Pa_GetErrorText( err ) );
		return;
	}
	Pa_Terminate();
}

gui::AudioPortAudioSetupWidget::AudioPortAudioSetupWidget(QWidget* _parent)
	: AudioDeviceSetupWidget(AudioPortAudio::name(), _parent)
{
	using gui::ComboBox;

	QFormLayout * form = new QFormLayout(this);

	m_backend = new ComboBox( this, "BACKEND" );
	form->addRow(tr("Backend"), m_backend);

	m_device = new ComboBox( this, "DEVICE" );
	form->addRow(tr("Device"), m_device);

	connect(&m_backendModel, &ComboBoxModel::dataChanged, this, &AudioPortAudioSetupWidget::updateDevices);
	connect(&m_deviceModel, &ComboBoxModel::dataChanged, this, &AudioPortAudioSetupWidget::updateChannels);

	m_backend->setModel(&m_backendModel);
	m_device->setModel(&m_deviceModel);
}




gui::AudioPortAudioSetupWidget::~AudioPortAudioSetupWidget()
{
	disconnect(&m_backendModel, &ComboBoxModel::dataChanged, this, &AudioPortAudioSetupWidget::updateDevices);
	disconnect(&m_deviceModel, &ComboBoxModel::dataChanged, this, &AudioPortAudioSetupWidget::updateChannels);
}




void gui::AudioPortAudioSetupWidget::saveSettings()
{
	ConfigManager::inst()->setValue("audioportaudio", "backend", m_backendModel.currentText());
	ConfigManager::inst()->setValue("audioportaudio", "device", m_deviceModel.currentText());
}




void gui::AudioPortAudioSetupWidget::show()
{
	if (m_backendModel.size() == 0)
	{
		// populate the backend model the first time we are shown
		updateBackends();

		const QString& backend = ConfigManager::inst()->value(
			"audioportaudio", "backend" );
		const QString& device = ConfigManager::inst()->value(
			"audioportaudio", "device" );

		int i = std::max(0, m_backendModel.findText(backend));
		m_backendModel.setValue(i);

		updateDevices();

		i = std::max(0, m_deviceModel.findText(device));
		m_deviceModel.setValue(i);
	}

	AudioDeviceSetupWidget::show();
}

} // namespace lmms


#endif // LMMS_HAVE_PORTAUDIO



