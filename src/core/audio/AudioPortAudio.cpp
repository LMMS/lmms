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
	: AudioDevice(DEFAULT_CHANNELS, engine)
	, m_paStream(nullptr)
	, m_outBuf(engine->framesPerPeriod())
	, m_outBufPos(0)
{
	auto configBackendName = ConfigManager::inst()->value("audioportaudio", "backend");
	auto configDeviceName = ConfigManager::inst()->value("audioportaudio", "device");
	auto configChannelCount = ConfigManager::inst()->value("audioportaudio", "channels");

	auto backendIndex = Pa_GetDefaultHostApi();
	auto backendInfo = Pa_GetHostApiInfo(backendIndex);
	const auto backendCount = Pa_GetHostApiCount();

	auto outputDeviceIndex = backendInfo == nullptr ? paNoDevice : backendInfo->defaultOutputDevice;
	auto outputDeviceInfo = Pa_GetDeviceInfo(outputDeviceIndex);

	for (auto i = 0; i < backendCount; ++i)
	{
		const auto currentBackendInfo = Pa_GetHostApiInfo(i);
		if (currentBackendInfo->name == configBackendName)
		{
			backendIndex = i;
			backendInfo = currentBackendInfo;
		}

		for (auto j = 0; j < currentBackendInfo->deviceCount; ++j)
		{
			const auto currentDeviceIndex = Pa_HostApiDeviceIndexToDeviceIndex(i, j);
			const auto currentDeviceInfo = Pa_GetDeviceInfo(currentDeviceIndex);

			if (currentDeviceInfo->name == configDeviceName)
			{
				outputDeviceIndex = currentDeviceIndex;
				outputDeviceInfo = currentDeviceInfo;
			}
		}
	}

	if (backendInfo == nullptr || outputDeviceInfo == nullptr)
	{
		std::cerr << "Could not find a valid PortAudio backend and/or device\n";
		successful = false;
		return;
	}
	else
	{
		const auto channelCount = std::min(configChannelCount.toInt(), outputDeviceInfo->maxOutputChannels);
		ConfigManager::inst()->setValue("audioportaudio", "channels", QString::number(channelCount));
		setChannels(channelCount);

		ConfigManager::inst()->setValue("audioportaudio", "backend", backendInfo->name);
		ConfigManager::inst()->setValue("audioportaudio", "device", outputDeviceInfo->name);
	}

	const auto outputParameters = PaStreamParameters{.device = outputDeviceIndex,
		.channelCount = channels(),
		.sampleFormat = paFloat32,
		.suggestedLatency = outputDeviceInfo->defaultLowOutputLatency,
		.hostApiSpecificStreamInfo = nullptr};

	auto err = Pa_IsFormatSupported(nullptr, &outputParameters, sampleRate());

	if (err != paFormatIsSupported)
	{
		std::cerr << "Failed to support PortAudio format: " << Pa_GetErrorText(err) << '\n';

		if (outputDeviceInfo != nullptr)
		{
			std::cerr << "Output device max channel count: " << outputDeviceInfo->maxOutputChannels << '\n';
			std::cerr << "Output device default sample rate: " << outputDeviceInfo->defaultSampleRate << '\n';
			std::cerr << "Output device name: " << outputDeviceInfo->name;
		}

		if (backendInfo != nullptr)
		{
			std::cerr << "Backend: " << backendInfo->name << '\n';
			std::cerr << "Backend device count: " << backendInfo->deviceCount << '\n';
		}

		std::cerr << "Output sample rate: " << sampleRate() << '\n';
		std::cerr << "Output device index: " << outputParameters.device << '\n';
		std::cerr << "Output channel count: " << outputParameters.channelCount << '\n';
		std::cerr << "Output sample format: " << outputParameters.sampleFormat << '\n';
		std::cerr << "Output suggested latency: " << outputParameters.suggestedLatency << '\n';
		std::cerr << "Output host API stream info?: " << (outputParameters.hostApiSpecificStreamInfo == nullptr ? "N" : "Y") << '\n';
	}

	err = Pa_OpenStream(&m_paStream, nullptr, &outputParameters, sampleRate(), engine->framesPerPeriod(),
		paNoFlag, processCallback, this);

	if (err != paNoError)
	{
		std::cerr << "Failed to open PortAudio stream: " << Pa_GetErrorText(err) << '\n';
		successful = false;
		return;
	}

	if (const auto streamInfo = Pa_GetStreamInfo(m_paStream); streamInfo != nullptr)
	{
		std::cout << "Stream sample rate: " << streamInfo->sampleRate << '\n';
		std::cout << "Stream input latency: " << streamInfo->inputLatency << '\n';
		std::cout << "Stream output latency: " << streamInfo->outputLatency << '\n';
	}

	successful = true;
}

AudioPortAudio::~AudioPortAudio()
{
	stopProcessing();

	const auto err = Pa_CloseStream(m_paStream);
	if (err != paNoError) { std::cerr << "Failed to close PortAudio stream: " << Pa_GetErrorText(err) << '\n'; }
}

void AudioPortAudio::startProcessing()
{
	const auto err = Pa_StartStream(m_paStream);
	if (err != paNoError) { std::cerr << "Failed to start PortAudio stream: " << Pa_GetErrorText(err) << '\n'; }
}

void AudioPortAudio::stopProcessing()
{
	const auto err = Pa_StopStream(m_paStream);
	if (err != paNoError) { std::cerr << "Failed to stop PortAudio stream: " << Pa_GetErrorText(err) << '\n'; }
}

int AudioPortAudio::processCallback(const float* inputBuffer, float* outputBuffer, f_cnt_t framesPerBuffer)
{
	std::fill_n(outputBuffer, framesPerBuffer * channels(), 0.f);

	for (auto frame = std::size_t{0}; frame < framesPerBuffer; ++frame)
	{
		if (m_outBufPos == 0 && getNextBuffer(m_outBuf.data()) == 0) { return paComplete; }

		if (channels() == 1)
		{
			outputBuffer[frame] = m_outBuf[m_outBufPos].average();
		}
		else
		{
			outputBuffer[frame * channels()] = m_outBuf[m_outBufPos][0];
			outputBuffer[frame * channels() + 1] = m_outBuf[m_outBufPos][1];
		}

		m_outBufPos = frame % m_outBuf.size();
	}

	return paContinue;
}

int AudioPortAudio::processCallback(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void* arg)
{
	auto _this = static_cast<AudioPortAudio*>(arg);
	return _this->processCallback(
		static_cast<const float*>(inputBuffer), static_cast<float*>(outputBuffer), framesPerBuffer);
}

void gui::AudioPortAudioSetupWidget::updateBackends()
{
	const auto initGuard = PortAudioInitializationGuard{};
	m_backendComboBox->clear();

	for (auto i = 0; i < Pa_GetHostApiCount(); ++i)
	{
		const auto backendInfo = Pa_GetHostApiInfo(i);
		m_backendComboBox->addItem(backendInfo->name, i);
	}

	const auto backend = ConfigManager::inst()->value("audioportaudio", "backend");
	const auto index = std::max(0, m_backendComboBox->findText(backend));
	m_backendComboBox->setCurrentIndex(index);
}

void gui::AudioPortAudioSetupWidget::updateDevices()
{
	const auto initGuard = PortAudioInitializationGuard{};
	m_deviceComboBox->clear();

	const auto backendIndex = m_backendComboBox->currentData().toInt();
	const auto backendInfo = Pa_GetHostApiInfo(backendIndex);
	if (backendInfo == nullptr) { return; }

	for (auto i = 0; i < backendInfo->deviceCount; ++i)
	{
		const auto deviceIndex = Pa_HostApiDeviceIndexToDeviceIndex(backendIndex, i);
		const auto deviceInfo = Pa_GetDeviceInfo(deviceIndex);

		if (deviceInfo->hostApi == backendIndex)
		{
			m_deviceComboBox->addItem(deviceInfo->name, deviceIndex);
		}
	}

	const auto device = ConfigManager::inst()->value("audioportaudio", "device");
	const auto index = std::max(0, m_deviceComboBox->findText(device));
	m_deviceComboBox->setCurrentIndex(index);
}

void gui::AudioPortAudioSetupWidget::updateChannels()
{
	const auto initGuard = PortAudioInitializationGuard{};

	const auto deviceInfo = Pa_GetDeviceInfo(m_deviceComboBox->currentData().toInt());
	if (deviceInfo == nullptr) { return; }

	const auto channels = ConfigManager::inst()->value("audioportaudio", "channels");
	m_channelModel.setRange(1, deviceInfo->maxOutputChannels);
	m_channelModel.setValue(std::min(channels.toInt(), deviceInfo->maxOutputChannels));
	m_channelSpinBox->setNumDigits(QString::number(deviceInfo->maxOutputChannels).length());
}

gui::AudioPortAudioSetupWidget::AudioPortAudioSetupWidget(QWidget* _parent)
	: AudioDeviceSetupWidget(AudioPortAudio::name(), _parent)
{
	const auto form = new QFormLayout(this);
	form->setRowWrapPolicy(QFormLayout::WrapLongRows);

	m_backendComboBox = new QComboBox(this);
	form->addRow(tr("Backend"), m_backendComboBox);

	m_deviceComboBox = new QComboBox(this);
	form->addRow(tr("Device"), m_deviceComboBox);

	m_channelSpinBox = new LcdSpinBox(1, this);
	m_channelSpinBox->setModel(&m_channelModel);
	form->addRow(tr("Channels"), m_channelSpinBox);

	connect(m_backendComboBox, qOverload<int>(&QComboBox::activated), [&] {
		updateDevices();
		updateChannels();
	});

	connect(m_deviceComboBox, qOverload<int>(&QComboBox::activated), [&] { updateChannels(); });
}

void gui::AudioPortAudioSetupWidget::saveSettings()
{
	ConfigManager::inst()->setValue("audioportaudio", "backend", m_backendComboBox->currentText());
	ConfigManager::inst()->setValue("audioportaudio", "device", m_deviceComboBox->currentText());
	ConfigManager::inst()->setValue("audioportaudio", "channels", QString::number(m_channelSpinBox->value<int>()));
}

void gui::AudioPortAudioSetupWidget::show()
{
	if (m_backendComboBox->count() == 0)
	{
		updateBackends();
		updateDevices();
		updateChannels();
	}

	AudioDeviceSetupWidget::show();
}

PortAudioInitializationGuard::PortAudioInitializationGuard()
	: m_error(Pa_Initialize())
{
	if (m_error != paNoError) { std::cerr << "Failed to initialize PortAudio: " << Pa_GetErrorText(m_error) << '\n'; }
}

PortAudioInitializationGuard::~PortAudioInitializationGuard()
{
	if (m_error == paNoError)
	{
		const auto err = Pa_Terminate();
		if (err != paNoError) { std::cerr << "Failed to terminate PortAudio: " << Pa_GetErrorText(err) << '\n'; }
	}
}
} // namespace lmms

#endif // LMMS_HAVE_PORTAUDIO
