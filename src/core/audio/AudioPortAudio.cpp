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
#include <iostream>

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
	, m_outBuf(std::make_unique<SampleFrame[]>(engine->framesPerPeriod()))
	, m_outBufPos(0)
	, m_outBufSize(engine->framesPerPeriod())
{
	successful = false;

	PaError err = Pa_Initialize();
	if( err != paNoError ) {
		std::cerr << "Couldn't initialize PortAudio: " << Pa_GetErrorText(err) << '\n';
		m_wasPAInitError = true;
		return;
	}

	const QString& backend = ConfigManager::inst()->value( "audioportaudio", "backend" );
	const QString& device = ConfigManager::inst()->value( "audioportaudio", "device" );

	PaDeviceIndex inputDeviceIndex = Pa_GetDefaultInputDevice();
	PaDeviceIndex outputDeviceIndex = Pa_GetDefaultOutputDevice();
	for( int i = 0; i < Pa_GetDeviceCount(); ++i )
	{
		const auto deviceInfo = Pa_GetDeviceInfo(i);
		if (deviceInfo->name == device && Pa_GetHostApiInfo(deviceInfo->hostApi)->name == backend)
		{
			inputDeviceIndex = i;
			outputDeviceIndex = i;
		}
	}

	PaStreamParameters* inputParameters = nullptr;
	PaStreamParameters* outputParameters = nullptr;

	if (outputDeviceIndex >= 0)
	{
		m_outputParameters.device = outputDeviceIndex;
		m_outputParameters.channelCount = channels();
		m_outputParameters.sampleFormat = paFloat32;
		m_outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputDeviceIndex)->defaultLowOutputLatency;
		m_outputParameters.hostApiSpecificStreamInfo = nullptr;
		outputParameters = &m_outputParameters;
	}
	else { return; }

	if (inputDeviceIndex >= 0)
	{
		m_inputParameters.device = inputDeviceIndex;
		m_inputParameters.channelCount = channels();
		m_inputParameters.sampleFormat = paFloat32;
		m_inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputDeviceIndex)->defaultLowInputLatency;
		m_inputParameters.hostApiSpecificStreamInfo = nullptr;
		inputParameters = &m_inputParameters;
		m_supportsCapture = true;
	}
	else { m_supportsCapture = false; }

	err = Pa_OpenStream(&m_paStream,
		inputParameters,
		outputParameters,
		sampleRate(),
		engine->framesPerPeriod(),
		paNoFlag, 
		processCallback,
		this);

	if( err != paNoError )
	{
		std::cerr << "Couldn't open PortAudio: " << Pa_GetErrorText(err) << '\n';
		return;
	}

	const auto inputDeviceInfo = Pa_GetDeviceInfo(inputDeviceIndex);
	const auto inputDeviceName = inputDeviceInfo->name;
	const auto inputDeviceBackend = Pa_GetHostApiInfo(inputDeviceInfo->hostApi)->name;

	const auto outputDeviceInfo = Pa_GetDeviceInfo(outputDeviceIndex);
	const auto outputDeviceName = outputDeviceInfo->name;
	const auto outputDeviceBackend = Pa_GetHostApiInfo(outputDeviceInfo->hostApi)->name;

	std::cout << "Input device: " << inputDeviceName << ", backend: " << inputDeviceBackend << '\n';
	std::cout << "Output device: " << outputDeviceName << ", backend: " << outputDeviceBackend << '\n';

	successful = true;
}




AudioPortAudio::~AudioPortAudio()
{
	stopProcessing();

	if( !m_wasPAInitError )
	{
		Pa_Terminate();
	}
}




void AudioPortAudio::startProcessing()
{
	PaError err = Pa_StartStream( m_paStream );
	if( err != paNoError )
	{
		std::cerr << "Failed to start PortAudio stream: " << Pa_GetErrorText(err) << '\n';
	}
}




void AudioPortAudio::stopProcessing()
{
	if( m_paStream && Pa_IsStreamActive( m_paStream ) )
	{
		PaError err = Pa_StopStream( m_paStream );
		if (err != paNoError) { std::cerr << "Failed to stop PortAudio stream: " << Pa_GetErrorText(err) << '\n'; }
	}
}


int AudioPortAudio::processCallback(const float* inputBuffer, float* outputBuffer, f_cnt_t framesPerBuffer)
{
	if (supportsCapture()) { audioEngine()->pushInputFrames((SampleFrame*)inputBuffer, framesPerBuffer); }

	while (framesPerBuffer)
	{
		if( m_outBufPos == 0 )
		{
			const fpp_t frames = getNextBuffer(m_outBuf.get());
			if( !frames )
			{
				memset(outputBuffer, 0, framesPerBuffer * channels() * sizeof(float));
				return paContinue;
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
		std::cerr << "Couldn't initialize PortAudio: " << Pa_GetErrorText(err) << '\n';
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
		std::cerr << "Couldn't initialize PortAudio: " << Pa_GetErrorText(err) << '\n';
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
		std::cerr << "Couldn't initialize PortAudio: " << Pa_GetErrorText(err) << '\n';
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



