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
#include "LcdSpinBox.h"

namespace {
enum class Direction
{
	Input,
	Output
};

constexpr auto tag()
{
	return "audioportaudio";
}
constexpr auto backendAttribute()
{
	return "backend";
}

constexpr auto deviceNameAttribute(Direction direction)
{
	switch (direction)
	{
	case Direction::Input:
		return "inputdevice";
	case Direction::Output:
		return "outputdevice";
	}

	return "";
}

constexpr auto channelsAttribute(Direction direction)
{
	switch (direction)
	{
	case Direction::Input:
		return "inputchannels";
	case Direction::Output:
		return "outputchannels";
	}

	return "";
}

int maxChannels(const PaDeviceInfo* info, Direction direction)
{
	switch (direction)
	{
	case Direction::Input:
		return info->maxInputChannels;
	case Direction::Output:
		return info->maxOutputChannels;
	}

	return 0;
}

QString numChannelsFromConfig(Direction direction)
{
	using namespace lmms;
	const auto defaultNumChannels = QString::number(DEFAULT_CHANNELS);
	const auto numChannels = ConfigManager::inst()->value(tag(), channelsAttribute(direction), defaultNumChannels);
	return numChannels;
}

PaStreamParameters createStreamParameters(PaDeviceIndex index, double suggestedLatency, Direction direction)
{
	using namespace lmms;

	return PaStreamParameters{.device = index,
		.channelCount = numChannelsFromConfig(direction).toInt(),
		.sampleFormat = paFloat32,
		.suggestedLatency = suggestedLatency,
		.hostApiSpecificStreamInfo = nullptr};
}

PaDeviceIndex findDeviceFromConfig(Direction direction)
{
	using namespace lmms;

	const auto backendName = ConfigManager::inst()->value(tag(), backendAttribute());
	const auto deviceName = ConfigManager::inst()->value(tag(), deviceNameAttribute(direction));

	auto deviceIndex = paNoDevice;

	for (auto i = 0, deviceCount = Pa_GetDeviceCount(); i < deviceCount; ++i)
	{
		const auto deviceInfo = Pa_GetDeviceInfo(i);
		const auto backendInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);

		if (deviceInfo->name == deviceName && backendInfo->name == backendName)
		{
			deviceIndex = i;
			break;
		}
	}

	if (deviceIndex == paNoDevice)
	{
		switch (direction)
		{
		case Direction::Input:
			deviceIndex = Pa_GetDefaultInputDevice();
			break;
		case Direction::Output:
			deviceIndex = Pa_GetDefaultOutputDevice();
			break;
		}
	}

	return deviceIndex;
}

class PortAudioInitializationGuard
{
public:
	PortAudioInitializationGuard()
		: m_error(Pa_Initialize())
	{
		if (m_error != paNoError) { throw std::runtime_error{"PortAudio: could not initialize"}; }
	}

	~PortAudioInitializationGuard()
	{
		if (m_error == paNoError) { Pa_Terminate(); }
	}

	PortAudioInitializationGuard(const PortAudioInitializationGuard&) = default;
	PortAudioInitializationGuard(PortAudioInitializationGuard&&) = delete;
	PortAudioInitializationGuard& operator=(const PortAudioInitializationGuard&) = default;
	PortAudioInitializationGuard& operator=(PortAudioInitializationGuard&&) = delete;

private:
	PaError m_error = paNoError;
};
} // namespace

namespace lmms {
AudioPortAudio::AudioPortAudio(AudioEngine* engine)
	: AudioDevice(DEFAULT_CHANNELS, engine)
	, m_outBuf(engine->framesPerPeriod())
{
	static auto s_initGuard = PortAudioInitializationGuard{};

	auto inputDevice = findDeviceFromConfig(Direction::Input);
	m_supportsCapture = inputDevice != paNoDevice;

	auto outputDevice = findDeviceFromConfig(Direction::Output);
	if (outputDevice == paNoDevice) { throw std::runtime_error{"PortAudio: could not load output device"}; }

	const auto framesPerBuffer = engine->framesPerPeriod();
	auto inputStreamParameters = createStreamParameters(inputDevice, 0., Direction::Input);
	auto outputStreamParameters = createStreamParameters(outputDevice, 0., Direction::Output);

	auto sampleRateSuggestions = SUPPORTED_SAMPLERATES;
	const auto currentSampleRate = ConfigManager::inst()->value("audioengine", "samplerate").toInt();

	if (currentSampleRate != 0)
	{
		// Prioritize the current sample rate if it is supported
		std::stable_partition(sampleRateSuggestions.begin(), sampleRateSuggestions.end(),
			[&](const auto rate) { return rate == currentSampleRate; });
	}

	auto openStreamErr = PaError{paNoError};
	for (auto i = std::size_t{0}; i < sampleRateSuggestions.size(); ++i)
	{
		const auto sampleRate = sampleRateSuggestions[i];
		const auto suggestedLatency = static_cast<double>(framesPerBuffer) / sampleRate;

		inputStreamParameters.suggestedLatency = suggestedLatency;
		outputStreamParameters.suggestedLatency = suggestedLatency;

		openStreamErr = Pa_OpenStream(&m_paStream, inputDevice == paNoDevice ? nullptr : &inputStreamParameters,
			&outputStreamParameters, sampleRate, framesPerBuffer, paNoFlag, &processCallback, this);

		if (openStreamErr == paNoError)
		{
			setSampleRate(sampleRate);

			// TODO: We should also be setting the input channel count
			setChannels(outputStreamParameters.channelCount);

			ConfigManager::inst()->setValue("audioengine", "samplerate", QString::number(sampleRate));
			break;
		}
	}

	if (openStreamErr != paNoError)
	{
		throw std::runtime_error{"PortAudio: failure to open stream - " + std::string{Pa_GetErrorText(openStreamErr)}};
	}
}

AudioPortAudio::~AudioPortAudio()
{
	stopProcessing();

	if (const auto err = Pa_CloseStream(m_paStream); err != paNoError)
	{
		std::cerr << "PortAudio: failure to close stream - " << Pa_GetErrorText(err) << '\n';
	}
}

void AudioPortAudio::startProcessing()
{
	if (const auto err = Pa_StartStream(m_paStream); err != paNoError)
	{
		std::cerr << "PortAudio: failure to start stream - " << Pa_GetErrorText(err) << '\n';
	}
}

void AudioPortAudio::stopProcessing()
{
	if (const auto err = Pa_StopStream(m_paStream); err != paNoError)
	{
		std::cerr << "PortAudio: failure to stop stream - " << Pa_GetErrorText(err) << '\n';
	}
}

int AudioPortAudio::processCallback(const void*, void* output, unsigned long frameCount,
	const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void* userData)
{
	const auto outputBuffer = static_cast<float*>(output);
	const auto device = static_cast<AudioPortAudio*>(userData);

	for (auto frame = std::size_t{0}; frame < frameCount; ++frame)
	{
		if (device->m_outBufPos == 0 && device->getNextBuffer(device->m_outBuf.data()) == 0)
		{
			std::fill(outputBuffer + frame * device->channels(), outputBuffer + frameCount * device->channels(), 0.f);
			return paComplete;
		}

		if (device->channels() == 1)
		{
			outputBuffer[frame] = device->m_outBuf[device->m_outBufPos].average();
		}
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

class AudioPortAudioSetupWidget::DeviceSelectorWidget : public QGroupBox
{
public:
	DeviceSelectorWidget(const QString& deviceLabel, Direction direction, QWidget* parent = nullptr)
		: QGroupBox{parent}
		, m_deviceComboBox{new QComboBox{this}}
		, m_channelSpinBox{new LcdSpinBox{1, this}}
	{
		m_channelSpinBox->setModel(&m_channelModel);

		const auto layout = new QFormLayout{this};
		layout->addRow(deviceLabel, m_deviceComboBox);
		layout->addRow(tr("Channels"), m_channelSpinBox);

		connect(m_deviceComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this,
			[this, direction](int index) { refreshChannels(m_deviceComboBox->itemData(index).toInt(), direction); });
	}

	void refreshFromConfig(PaHostApiIndex backendIndex, Direction direction)
	{
		using namespace lmms;

		m_deviceComboBox->clear();

		for (auto i = 0, deviceCount = Pa_GetDeviceCount(); i < deviceCount; ++i)
		{
			const auto deviceInfo = Pa_GetDeviceInfo(i);

			if (maxChannels(deviceInfo, direction) > 0 && deviceInfo->hostApi == backendIndex)
			{
				m_deviceComboBox->addItem(deviceInfo->name, i);
			}
		}

		const auto selectedDeviceName = ConfigManager::inst()->value(tag(), deviceNameAttribute(direction));
		const auto selectedDeviceIndex = std::max(0, m_deviceComboBox->findText(selectedDeviceName));
		m_deviceComboBox->setCurrentIndex(selectedDeviceIndex);
	}

	void refreshChannels(PaDeviceIndex deviceIndex, Direction direction)
	{
		const auto maxChannelCount = maxChannels(Pa_GetDeviceInfo(deviceIndex), direction);
		m_channelModel.setRange(1, static_cast<float>(maxChannelCount));
		m_channelModel.setValue(static_cast<float>(numChannelsFromConfig(direction).toInt()));
		m_channelSpinBox->setNumDigits(QString::number(maxChannelCount).length());
	}

	void saveToConfig(Direction direction)
	{
		ConfigManager::inst()->setValue(tag(), deviceNameAttribute(direction), m_deviceComboBox->currentText());
		ConfigManager::inst()->setValue(tag(), channelsAttribute(direction), QString::number(m_channelModel.value()));
	}

private:
	QComboBox* m_deviceComboBox = nullptr;
	lmms::gui::LcdSpinBox* m_channelSpinBox = nullptr;
	lmms::IntModel m_channelModel;
};

AudioPortAudioSetupWidget::AudioPortAudioSetupWidget(QWidget* parent)
	: AudioDeviceSetupWidget{AudioPortAudio::name(), parent}
	, m_backendComboBox{new QComboBox{this}}
	, m_inputDevice{new DeviceSelectorWidget{tr("Input device"), Direction::Input}}
	, m_outputDevice(new DeviceSelectorWidget{tr("Output device"), Direction::Output})
{
	constexpr auto formVerticalSpacing = 10;
	const auto form = new QFormLayout{this};
	form->setRowWrapPolicy(QFormLayout::WrapLongRows);
	form->setVerticalSpacing(formVerticalSpacing);

	form->addRow(tr("Backend"), m_backendComboBox);
	form->addRow(m_outputDevice);
	form->addRow(m_inputDevice);

	connect(m_backendComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
		m_inputDevice->refreshFromConfig(m_backendComboBox->itemData(index).toInt(), Direction::Input);
		m_outputDevice->refreshFromConfig(m_backendComboBox->itemData(index).toInt(), Direction::Output);
	});
}

void AudioPortAudioSetupWidget::show()
{
	static auto s_initGuard = PortAudioInitializationGuard{};

	if (m_backendComboBox->count() == 0)
	{
		for (auto i = 0, backendCount = Pa_GetHostApiCount(); i < backendCount; ++i)
		{
			m_backendComboBox->addItem(Pa_GetHostApiInfo(i)->name, i);
		}

		const auto selectedBackendName = ConfigManager::inst()->value(tag(), backendAttribute());
		const auto selectedBackendIndex = std::max(0, m_backendComboBox->findText(selectedBackendName));
		m_backendComboBox->setCurrentIndex(selectedBackendIndex);
	}

	AudioDeviceSetupWidget::show();
}

void AudioPortAudioSetupWidget::saveSettings()
{
	ConfigManager::inst()->setValue(tag(), backendAttribute(), m_backendComboBox->currentText());
	m_inputDevice->saveToConfig(Direction::Input);
	m_outputDevice->saveToConfig(Direction::Output);
}
} // namespace lmms::gui

#endif // LMMS_HAVE_PORTAUDIO
