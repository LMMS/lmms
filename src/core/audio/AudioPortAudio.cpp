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

#include "AudioEngine.h"
#include "AudioPortAudio.h"
#include "ConfigManager.h"

namespace {
constexpr auto configTag = "audioportaudio";

constexpr auto configOutputDeviceAttribute = "outputdevice";
constexpr auto configOutputDeviceBackendAttribute = "outputbackend";
constexpr auto configOutputDeviceChannelsAttribute = "outputchannels";

constexpr auto configInputDeviceAttribute = "inputdevice";
constexpr auto configInputDeviceBackendAttribute = "inputbackend";
constexpr auto configInputDeviceChannelsAttribute = "inputchannels";
} // namespace

namespace lmms {
AudioPortAudio::AudioPortAudio(AudioEngine* engine)
	: AudioDevice(DEFAULT_CHANNELS, engine)
	, m_outBuf(engine->framesPerPeriod())
{
	if (Pa_Initialize() != paNoError) { throw std::runtime_error{"PortAudio: could not initialize"}; }

	const auto configOutputDeviceValue = ConfigManager::inst()->value(configTag, configOutputDeviceAttribute);
	const auto configOutputDeviceBackendValue = ConfigManager::inst()->value(configTag, configOutputDeviceBackendAttribute);
	const auto configOutputDeviceChannelsValue = ConfigManager::inst()->value(configTag, configOutputDeviceChannelsAttribute);

	const auto configInputDeviceValue = ConfigManager::inst()->value(configTag, configInputDeviceAttribute);
	const auto configInputDeviceBackendValue = ConfigManager::inst()->value(configTag, configInputDeviceBackendAttribute);
	const auto configInputDeviceChannelsValue = ConfigManager::inst()->value(configTag, configInputDeviceChannelsAttribute);

	auto outputDeviceIndex = paNoDevice;
	auto inputDeviceIndex = paNoDevice;

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

		if (configOutputDeviceValue == deviceInfo->name && configOutputDeviceBackendValue == backendInfo->name)
		{
			outputDeviceIndex = i;
			outputDeviceInfo = deviceInfo;
			outputBackendInfo = backendInfo;
		}

		if (configInputDeviceValue == deviceInfo->name && configInputDeviceBackendValue == backendInfo->name)
		{
			inputDeviceIndex = i;
			inputDeviceInfo = deviceInfo;
			inputBackendInfo = backendInfo;
		}
	}

	if (outputDeviceIndex == paNoDevice)
	{
		outputDeviceIndex = Pa_GetDefaultOutputDevice();
		outputDeviceInfo = Pa_GetDeviceInfo(outputDeviceIndex);
		outputBackendInfo = outputDeviceInfo == nullptr ? nullptr : Pa_GetHostApiInfo(outputDeviceInfo->hostApi);
	}

	if (inputDeviceIndex == paNoDevice)
	{
		inputDeviceIndex = Pa_GetDefaultInputDevice();
		inputDeviceInfo = Pa_GetDeviceInfo(inputDeviceIndex);
		inputBackendInfo = inputDeviceInfo == nullptr ? nullptr : Pa_GetHostApiInfo(inputDeviceInfo->hostApi);
	}

	if (outputDeviceIndex == paNoDevice)
	{
		throw std::runtime_error{"PortAudio: could not find output device"};
	}

	if (inputDeviceIndex != paNoDevice)
	{
		m_supportsCapture = true;
	}

	auto outputDeviceChannelCount = configOutputDeviceChannelsValue.toInt();
	if (outputDeviceChannelCount == 0) { outputDeviceChannelCount = DEFAULT_CHANNELS; }

	auto inputDeviceChannelCount = configInputDeviceChannelsValue.toInt();
	if (inputDeviceChannelCount == 0) { inputDeviceChannelCount = DEFAULT_CHANNELS; }

	const auto sampleRate = static_cast<double>(engine->outputSampleRate());
	const auto framesPerBuffer = engine->framesPerPeriod();
	const auto latency = framesPerBuffer / sampleRate;

	const auto outputParameters = PaStreamParameters{.device = outputDeviceIndex,
		.channelCount = outputDeviceChannelCount,
		.sampleFormat = paFloat32,
		.suggestedLatency = latency,
		.hostApiSpecificStreamInfo = nullptr};

	const auto inputParameters = PaStreamParameters{.device = inputDeviceIndex,
		.channelCount = inputDeviceChannelCount,
		.sampleFormat = paFloat32,
		.suggestedLatency = latency,
		.hostApiSpecificStreamInfo = nullptr};

	const auto err = Pa_OpenStream(&m_paStream, inputDeviceIndex == paNoDevice ? nullptr : &inputParameters,
		&outputParameters, sampleRate, framesPerBuffer, paNoFlag, &processCallback, this);

	if (err != paNoError)
	{
		throw std::runtime_error{std::string{"PortAudio: could not open stream, "} + Pa_GetErrorText(err)};
	}

	ConfigManager::inst()->setValue(configTag, configOutputDeviceAttribute, outputDeviceInfo->name);
	ConfigManager::inst()->setValue(configTag, configOutputDeviceBackendAttribute, outputBackendInfo->name);
	ConfigManager::inst()->setValue(configTag, configOutputDeviceChannelsAttribute, QString::number(outputDeviceChannelCount));

	ConfigManager::inst()->setValue(configTag, configInputDeviceAttribute, inputDeviceInfo->name);
	ConfigManager::inst()->setValue(configTag, configInputDeviceBackendAttribute, inputBackendInfo->name);
	ConfigManager::inst()->setValue(configTag, configInputDeviceChannelsAttribute, QString::number(inputDeviceChannelCount));
}

AudioPortAudio::~AudioPortAudio()
{
	stopProcessing();
	Pa_CloseStream(m_paStream);
	Pa_Terminate();
}

void AudioPortAudio::startProcessing()
{
	Pa_StartStream(m_paStream);
}

void AudioPortAudio::stopProcessing()
{
	Pa_StopStream(m_paStream);
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
