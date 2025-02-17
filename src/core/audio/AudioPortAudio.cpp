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
	, m_outBuf(engine->framesPerPeriod())
	, m_outBufPos(0)
{
	const auto backend = ConfigManager::inst()->value("audioportaudio", "backend");
	const auto device = ConfigManager::inst()->value("audioportaudio", "device");

	auto outputDeviceIndex = Pa_GetDefaultOutputDevice();
	auto outputDeviceInfo = Pa_GetDeviceInfo(outputDeviceIndex);
	const auto deviceCount = Pa_GetDeviceCount();

	for (auto i = 0; i < deviceCount; ++i)
	{
		const auto deviceInfo = Pa_GetDeviceInfo(i);
		const auto currentBackendName = Pa_GetHostApiInfo(deviceInfo->hostApi)->name;
		const auto currentDeviceName = deviceInfo->name;

		if (currentBackendName == backend && currentDeviceName == device)
		{
			outputDeviceIndex = i;
			outputDeviceInfo = deviceInfo;
			break;
		}
	}

	const auto outputParameters = PaStreamParameters{.device = outputDeviceIndex,
		.channelCount = channels(),
		.sampleFormat = paFloat32,
		.suggestedLatency = outputDeviceInfo->defaultLowOutputLatency,
		.hostApiSpecificStreamInfo = nullptr};

	const auto err = Pa_OpenStream(&m_paStream, nullptr, &outputParameters, sampleRate(), engine->framesPerPeriod(),
		paNoFlag, processCallback, this);

	if (err != paNoError)
	{
		std::cerr << "Failed to open PortAudio stream: " << Pa_GetErrorText(err) << '\n';
		successful = false;
		return;
	}

	successful = true;
}

AudioPortAudio::~AudioPortAudio()
{
	stopProcessing();

	const auto err = Pa_CloseStream(&m_paStream);
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

	for (auto i = 0; i < Pa_GetDeviceCount(); ++i)
	{
		const auto deviceInfo = Pa_GetDeviceInfo(i);
		if (deviceInfo->hostApi == m_backendComboBox->currentData().toInt())
		{
			m_deviceComboBox->addItem(deviceInfo->name, i);
		}
	}

	const auto device = ConfigManager::inst()->value("audioportaudio", "device");
	const auto index = std::max(0, m_deviceComboBox->findText(device));
	m_deviceComboBox->setCurrentIndex(index);
}

void gui::AudioPortAudioSetupWidget::updateChannels()
{
	const auto initGuard = PortAudioInitializationGuard{};

	const auto channels = ConfigManager::inst()->value("audioportaudio", "channels");
	const auto deviceInfo = Pa_GetDeviceInfo(m_deviceComboBox->currentData().toInt());
	const auto maxOutputChannels = deviceInfo == nullptr ? DEFAULT_CHANNELS : deviceInfo->maxOutputChannels;

	m_channelModel.setRange(1, maxOutputChannels);
	m_channelModel.setValue(std::min(channels.isEmpty() ? DEFAULT_CHANNELS : channels.toInt(), maxOutputChannels));
	m_channelSpinBox->setNumDigits(QString::number(maxOutputChannels).length());
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
