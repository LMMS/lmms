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

#include <QComboBox>
#include <QFormLayout>
#include <iostream>

#include "AudioEngine.h"
#include "ConfigManager.h"
#include "Engine.h"

namespace lmms {

AudioPortAudio::AudioPortAudio(bool& successful, AudioEngine* engine)
	: AudioDevice(ConfigManager::inst()->value("audioportaudio", "channels", QString::number(DEFAULT_CHANNELS)).toInt(), engine)
	, m_paStream(nullptr)
	, m_outBuf(std::make_unique<SampleFrame[]>(engine->framesPerPeriod()))
	, m_outBufPos(0)
	, m_outBufSize(engine->framesPerPeriod())
{
	Pa_Initialize();
	const QString& backend = ConfigManager::inst()->value("audioportaudio", "backend");
	const QString& device = ConfigManager::inst()->value("audioportaudio", "device");

	auto inputDeviceIndex = Pa_GetDefaultInputDevice();
	auto outputDeviceIndex = Pa_GetDefaultOutputDevice();
	const auto deviceCount = Pa_GetDeviceCount();

	for (auto i = 0; i < deviceCount; ++i)
	{
		const auto deviceInfo = Pa_GetDeviceInfo(i);
		if (deviceInfo->name == device && Pa_GetHostApiInfo(deviceInfo->hostApi)->name == backend)
		{
			inputDeviceIndex = i;
			outputDeviceIndex = i;
			break;
		}
	}

	const auto latency = static_cast<double>(audioEngine()->framesPerPeriod()) / sampleRate();

	auto outputParameters = PaStreamParameters{};
	auto inputParameters = PaStreamParameters{};

	outputParameters.device = outputDeviceIndex;
	outputParameters.channelCount = channels();
	outputParameters.sampleFormat = paFloat32;
	outputParameters.suggestedLatency = latency;
	outputParameters.hostApiSpecificStreamInfo = nullptr;

	inputParameters.device = inputDeviceIndex;
	inputParameters.channelCount = channels();
	inputParameters.sampleFormat = paFloat32;
	inputParameters.suggestedLatency = latency;
	inputParameters.hostApiSpecificStreamInfo = nullptr;

	auto err = Pa_OpenStream(&m_paStream, &inputParameters, &outputParameters, sampleRate(), engine->framesPerPeriod(),
		paNoFlag, processCallback, this);

	if (err != paNoError)
	{
		std::cerr << "Couldn't open PortAudio: " << Pa_GetErrorText(err) << '\n';
		successful = false;
		return;
	}

	m_supportsCapture = true;
	successful = true;
}

AudioPortAudio::~AudioPortAudio()
{
	stopProcessing();
	Pa_Terminate();
}

void AudioPortAudio::startProcessing()
{
	PaError err = Pa_StartStream(m_paStream);
	if (err != paNoError) { std::cerr << "Failed to start PortAudio stream: " << Pa_GetErrorText(err) << '\n'; }
}

void AudioPortAudio::stopProcessing()
{
	if (m_paStream && Pa_IsStreamActive(m_paStream))
	{
		PaError err = Pa_StopStream(m_paStream);
		if (err != paNoError) { std::cerr << "Failed to stop PortAudio stream: " << Pa_GetErrorText(err) << '\n'; }
	}
}

int AudioPortAudio::processCallback(const float* inputBuffer, float* outputBuffer, f_cnt_t framesPerBuffer)
{
	if (supportsCapture()) { audioEngine()->pushInputFrames((SampleFrame*)inputBuffer, framesPerBuffer); }

	while (framesPerBuffer)
	{
		if (m_outBufPos == 0)
		{
			m_outBufSize = getNextBuffer(m_outBuf.get());
			if (m_outBufSize == 0)
			{
				std::fill_n(outputBuffer, framesPerBuffer * channels(), 0.0f);
				return paComplete;
			}
		}

		const auto minLen = std::min(framesPerBuffer, m_outBufSize - m_outBufPos);
		for (auto sample = std::size_t{0}; sample < framesPerBuffer * channels(); ++sample)
		{
			outputBuffer[sample] = m_outBuf[sample / channels()][sample % channels()];
		}

		outputBuffer += minLen * channels();
		framesPerBuffer -= minLen;
		m_outBufPos += minLen;
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
	if (err != paNoError)
	{
		std::cerr << "Couldn't initialize PortAudio: " << Pa_GetErrorText(err) << '\n';
		return;
	}

	for (int i = 0; i < Pa_GetHostApiCount(); ++i)
	{
		const auto hi = Pa_GetHostApiInfo(i);
		m_backendComboBox->addItem(hi->name);
	}

	Pa_Terminate();
}

void gui::AudioPortAudioSetupWidget::updateDevices()
{
	PaError err = Pa_Initialize();
	if (err != paNoError)
	{
		std::cerr << "Couldn't initialize PortAudio: " << Pa_GetErrorText(err) << '\n';
		return;
	}

	const QString& backend = m_backendComboBox->currentText();
	int hostApi = 0;
	for (int i = 0; i < Pa_GetHostApiCount(); ++i)
	{
		const auto hi = Pa_GetHostApiInfo(i);
		if (backend == hi->name)
		{
			hostApi = i;
			break;
		}
	}

	m_deviceComboBox->clear();
	for (int i = 0; i < Pa_GetDeviceCount(); ++i)
	{
		const auto di = Pa_GetDeviceInfo(i);
		if (di->hostApi == hostApi) { m_deviceComboBox->addItem(di->name); }
	}
	Pa_Terminate();
}

void gui::AudioPortAudioSetupWidget::updateChannels()
{
	PaError err = Pa_Initialize();
	if (err != paNoError)
	{
		std::cerr << "Couldn't initialize PortAudio: " << Pa_GetErrorText(err) << '\n';
		return;
	}
	Pa_Terminate();
}

gui::AudioPortAudioSetupWidget::AudioPortAudioSetupWidget(QWidget* _parent)
	: AudioDeviceSetupWidget(AudioPortAudio::name(), _parent)
{
	QFormLayout* form = new QFormLayout(this);
	form->setRowWrapPolicy(QFormLayout::WrapLongRows);

	m_backendComboBox = new QComboBox(this);
	form->addRow(tr("Backend"), m_backendComboBox);

	m_deviceComboBox = new QComboBox(this);
	form->addRow(tr("Device"), m_deviceComboBox);
}

void gui::AudioPortAudioSetupWidget::saveSettings()
{
	ConfigManager::inst()->setValue("audioportaudio", "backend", m_backendComboBox->currentText());
	ConfigManager::inst()->setValue("audioportaudio", "device", m_deviceComboBox->currentText());
}

void gui::AudioPortAudioSetupWidget::show()
{
	if (m_backendComboBox->count() == 0)
	{
		// populate the backend model the first time we are shown
		updateBackends();

		const QString& backend = ConfigManager::inst()->value("audioportaudio", "backend");
		const QString& device = ConfigManager::inst()->value("audioportaudio", "device");

		const auto backendIndex = std::max(0, m_backendComboBox->findText(backend));
		m_backendComboBox->setCurrentIndex(backendIndex);

		updateDevices();

		const auto deviceIndex = std::max(0, m_deviceComboBox->findText(device));
		m_deviceComboBox->setCurrentIndex(deviceIndex);
	}

	AudioDeviceSetupWidget::show();
}

} // namespace lmms

#endif // LMMS_HAVE_PORTAUDIO
