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

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_PORTAUDIO

#include <iostream>

#include "AudioEngine.h"
#include "AudioPortAudio.h"
#include "ConfigManager.h"

namespace {
constexpr auto configTag = "audioportaudio";

constexpr auto outputDeviceAttribute = "outputdevice";
constexpr auto outputDeviceBackendAttribute = "outputbackend";
constexpr auto outputDeviceChannelsAttribute = "outputchannels";

constexpr auto inputDeviceAttribute = "inputdevice";
constexpr auto inputDeviceBackendAttribute = "inputbackend";
constexpr auto inputDeviceChannelsAttribute = "inputchannels";
} // namespace

namespace lmms {
AudioPortAudio::AudioPortAudio(AudioEngine* engine)
	: AudioDevice(DEFAULT_CHANNELS, engine)
	, m_outBuf(engine->framesPerPeriod())
{
	if (Pa_Initialize() != paNoError) { throw std::runtime_error{"PortAudio: could not initialize"}; }

	const auto outputDeviceName = ConfigManager::inst()->value(configTag, outputDeviceAttribute);
	const auto outputDeviceBackend = ConfigManager::inst()->value(configTag, outputDeviceBackendAttribute);
	const auto outputDeviceChannels = ConfigManager::inst()->value(configTag, outputDeviceChannelsAttribute);

	const auto inputDeviceName = ConfigManager::inst()->value(configTag, inputDeviceAttribute);
	const auto inputDeviceBackend = ConfigManager::inst()->value(configTag, inputDeviceBackendAttribute);
	const auto inputDeviceChannels = ConfigManager::inst()->value(configTag, inputDeviceChannelsAttribute);

	auto outputDevice = paNoDevice;
	auto inputDevice = paNoDevice;

	const auto deviceCount = Pa_GetDeviceCount();
	if (deviceCount < 0) { throw std::runtime_error{"PortAudio: no available devices"}; }

	auto outputDeviceInfo = static_cast<const PaDeviceInfo*>(nullptr);
	auto inputDeviceInfo = static_cast<const PaDeviceInfo*>(nullptr);

	auto outputBackendInfo = static_cast<const PaHostApiInfo*>(nullptr);
	auto inputBackendInfo = static_cast<const PaHostApiInfo*>(nullptr);

	for (auto i = 0; i < deviceCount; ++i)
	{
		const auto deviceInfo = Pa_GetDeviceInfo(i);
		const auto backendInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);

		if (outputDeviceName == deviceInfo->name && outputDeviceBackend == backendInfo->name)
		{
			outputDevice = i;
			outputDeviceInfo = deviceInfo;
			outputBackendInfo = backendInfo;
		}

		if (inputDeviceName == deviceInfo->name && inputDeviceBackend == backendInfo->name)
		{
			inputDevice = i;
			inputDeviceInfo = deviceInfo;
			inputBackendInfo = backendInfo;
		}
	}

	outputDevice = outputDevice == paNoDevice ? Pa_GetDefaultOutputDevice() : outputDevice;
	outputDeviceInfo = Pa_GetDeviceInfo(outputDevice);

	inputDevice = inputDevice == paNoDevice ? Pa_GetDefaultInputDevice() : inputDevice;
	inputDeviceInfo = Pa_GetDeviceInfo(inputDevice);

	if (outputDevice == paNoDevice || inputDevice == paNoDevice)
	{
		throw std::runtime_error{"PortAudio: could not load input and output device"};
	}

	auto outputDeviceChannelCount = outputDeviceChannels.toInt();
	if (outputDeviceChannelCount == 0) { outputDeviceChannelCount = DEFAULT_CHANNELS; }

	auto inputDeviceChannelCount = inputDeviceChannels.toInt();
	if (inputDeviceChannelCount == 0) { inputDeviceChannelCount = DEFAULT_CHANNELS; }

	const auto sampleRate = static_cast<double>(engine->outputSampleRate());
	const auto framesPerBuffer = engine->framesPerPeriod();
	const auto latency = framesPerBuffer / sampleRate;

	const auto outputParameters = PaStreamParameters{.device = outputDevice,
		.channelCount = outputDeviceChannelCount,
		.sampleFormat = paFloat32,
		.suggestedLatency = latency,
		.hostApiSpecificStreamInfo = nullptr};

	const auto inputParameters = PaStreamParameters{.device = inputDevice,
		.channelCount = inputDeviceChannelCount,
		.sampleFormat = paFloat32,
		.suggestedLatency = latency,
		.hostApiSpecificStreamInfo = nullptr};

	const auto err = Pa_OpenStream(&m_paStream, &inputParameters, &outputParameters, sampleRate, framesPerBuffer,
		paNoFlag, &processCallback, this);

	if (err != paNoError)
	{
		throw std::runtime_error{std::string{"PortAudio: could not open stream, "} + Pa_GetErrorText(err)};
	}

	ConfigManager::inst()->setValue(configTag, outputDeviceBackendAttribute, outputBackendInfo->name);
	ConfigManager::inst()->setValue(configTag, outputDeviceAttribute, outputDeviceInfo->name);
	ConfigManager::inst()->setValue(
		configTag, outputDeviceChannelsAttribute, QString::number(outputDeviceChannelCount));

	ConfigManager::inst()->setValue(configTag, inputDeviceBackendAttribute, inputBackendInfo->name);
	ConfigManager::inst()->setValue(configTag, inputDeviceAttribute, inputDeviceInfo->name);
	ConfigManager::inst()->setValue(configTag, inputDeviceChannelsAttribute, QString::number(inputDeviceChannelCount));
}

AudioPortAudio::~AudioPortAudio()
{
	stopProcessing();
	Pa_CloseStream(m_paStream);
	Pa_Terminate();
}

void AudioPortAudio::startProcessing()
{
	if (const auto err = Pa_StartStream(m_paStream); err != paNoError)
	{
		std::cerr << "PortAudio: could not start stream,  " << Pa_GetErrorText(err) << '\n';
	}
}

void AudioPortAudio::stopProcessing()
{
	if (const auto err = Pa_StopStream(m_paStream); err != paNoError)
	{
		std::cerr << "PortAudio: could not stop stream,  " << Pa_GetErrorText(err) << '\n';
	}
}

int AudioPortAudio::processCallback(const void* input, void* output, unsigned long frameCount,
	const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData)
{
	(void)input;
	(void)timeInfo;
	(void)statusFlags;

	const auto outputBuffer = static_cast<float*>(output);
	const auto device = static_cast<AudioPortAudio*>(userData);

	std::fill_n(outputBuffer, frameCount * device->channels(), 0.f);

	for (auto frame = std::size_t{0}; frame < frameCount; ++frame)
	{
		if (device->m_outBufPos == 0 && device->getNextBuffer(device->m_outBuf.data()) == 0) { return paComplete; }

		if (device->channels() == 1) { outputBuffer[frame] = device->m_outBuf[device->m_outBufPos].average(); }
		else
		{
			outputBuffer[frame * device->channels()] = device->m_outBuf[device->m_outBufPos][0];
			outputBuffer[frame * device->channels() + 1] = device->m_outBuf[device->m_outBufPos][1];
		}

		device->m_outBufPos = frame % device->m_outBuf.size();
	}

	return paContinue;
}
} // namespace lmms

namespace lmms::gui {
AudioPortAudioSetupWidget::AudioPortAudioSetupWidget(QWidget* parent)
	: AudioDeviceSetupWidget(AudioPortAudio::name(), parent)
{
}

void AudioPortAudioSetupWidget::saveSettings()
{
}
} // namespace lmms::gui

#endif // LMMS_HAVE_PORTAUDIO
