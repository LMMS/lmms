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

struct DeviceSpec
{
	int maxChannels() const
	{
		switch (direction)
		{
		case Direction::Input:
			return deviceInfo->maxInputChannels;
		case Direction::Output:
			return deviceInfo->maxOutputChannels;
		}

		return 0;
	}

	PaStreamParameters createStreamParameters(double suggestedLatency) const
	{
		using namespace lmms;

		const auto defaultNumChannels = QString::number(DEFAULT_CHANNELS);
		const auto numChannels = ConfigManager::inst()->value(tag(), channelsAttribute(direction), defaultNumChannels);

		return PaStreamParameters{.device = index,
			.channelCount = numChannels.toInt(),
			.sampleFormat = paFloat32,
			.suggestedLatency = suggestedLatency,
			.hostApiSpecificStreamInfo = nullptr};
	}

	static DeviceSpec loadFromIndex(PaDeviceIndex index, Direction direction)
	{
		const auto deviceInfo = Pa_GetDeviceInfo(index);
		const auto backendInfo = Pa_GetHostApiInfo(index);
		const auto deviceSpec
			= DeviceSpec{.index = index, .deviceInfo = deviceInfo, .backendInfo = backendInfo, .direction = direction};
		return deviceSpec;
	}

	static DeviceSpec loadFromConfig(Direction direction)
	{
		using namespace lmms;

		const auto backendName = ConfigManager::inst()->value(tag(), backendAttribute());
		const auto deviceName = ConfigManager::inst()->value(tag(), deviceNameAttribute(direction));

		auto deviceIndex = paNoDevice;
		auto deviceInfo = static_cast<const PaDeviceInfo*>(nullptr);
		auto backendInfo = static_cast<const PaHostApiInfo*>(nullptr);

		for (auto i = 0, deviceCount = Pa_GetDeviceCount(); i < deviceCount; ++i)
		{
			deviceInfo = Pa_GetDeviceInfo(i);
			backendInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);

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

			deviceInfo = Pa_GetDeviceInfo(deviceIndex);
			backendInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);
		}

		return DeviceSpec{
			.index = deviceIndex, .deviceInfo = deviceInfo, .backendInfo = backendInfo, .direction = direction};
	}

	bool valid() const { return index != paNoDevice; }

	const PaDeviceIndex index = paNoDevice;
	const PaDeviceInfo* deviceInfo = nullptr;
	const PaHostApiInfo* backendInfo = nullptr;
	const Direction direction;
};

} // namespace

namespace lmms {
AudioPortAudio::AudioPortAudio(AudioEngine* engine)
	: AudioDevice(DEFAULT_CHANNELS, engine)
	, m_outBuf(engine->framesPerPeriod())
{
	if (Pa_Initialize() != paNoError) { throw std::runtime_error{"PortAudio: could not initialize"}; }

	auto inputDevice = DeviceSpec::loadFromConfig(Direction::Input);
	m_supportsCapture = inputDevice.valid();

	auto outputDevice = DeviceSpec::loadFromConfig(Direction::Output);
	if (!outputDevice.valid()) { throw std::runtime_error{"PortAudio: could not load output device"}; }

	const auto sampleRate = engine->baseSampleRate();
	const auto framesPerBuffer = engine->framesPerPeriod();
	const auto suggestedLatency = static_cast<double>(framesPerBuffer) / sampleRate;

	auto inputStreamParameters = inputDevice.createStreamParameters(suggestedLatency);
	auto outputStreamParameters = outputDevice.createStreamParameters(suggestedLatency);

	if (const auto err = Pa_OpenStream(&m_paStream, inputDevice.valid() ? &inputStreamParameters : nullptr,
			&outputStreamParameters, sampleRate, framesPerBuffer, paNoFlag, &processCallback, this))
	{
		throw std::runtime_error{std::string{"PortAudio: could not open stream, "} + Pa_GetErrorText(err)};
	}
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

int AudioPortAudio::processCallback(const void*, void* output, unsigned long frameCount,
	const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void* userData)
{
	const auto outputBuffer = static_cast<float*>(output);
	const auto device = static_cast<AudioPortAudio*>(userData);

	std::fill_n(outputBuffer, frameCount * device->channels(), 0.f);

	for (auto frame = std::size_t{0}; frame < frameCount; ++frame)
	{
		if (device->m_outBufPos == 0 && device->getNextBuffer(device->m_outBuf.data()) == 0) { return paComplete; }

		if (device->channels() == 1)
		{
			outputBuffer[frame] = device->m_outBuf[device->m_outBufPos].average();
			continue;
		}

		outputBuffer[frame * device->channels()] = device->m_outBuf[device->m_outBufPos][0];
		outputBuffer[frame * device->channels() + 1] = device->m_outBuf[device->m_outBufPos][1];
		device->m_outBufPos = frame % device->m_outBuf.size();
	}

	return paContinue;
}
} // namespace lmms

namespace lmms::gui {

class AudioPortAudioSetupWidget::DeviceSpecWidget : public QGroupBox
{
public:
	DeviceSpecWidget(Direction direction, QWidget* parent = nullptr)
		: QGroupBox{parent}
		, m_direction(direction)
	{
        m_deviceComboBox = new QComboBox{this};
        m_channelSpinBox = new LcdSpinBox{1, this};

		const auto layout = new QFormLayout{this};
        auto rowHeader = "";

        switch (direction)
        {
		case Direction::Input:
            rowHeader = "Input device";
            break;
		case Direction::Output:
            rowHeader = "Output device";
			break;
		}

		m_channelSpinBox->setModel(&m_channelModel);
		layout->addRow(tr(rowHeader), m_deviceComboBox);
		layout->addRow(tr("Channels"), m_channelSpinBox);
	}

	void refreshFromConfig(PaHostApiIndex backendIndex)
	{
		using namespace lmms;

		m_deviceComboBox->clear();

		for (auto i = 0, deviceCount = Pa_GetDeviceCount(); i < deviceCount; ++i)
		{
			const auto deviceInfo = Pa_GetDeviceInfo(i);
			const auto canAddInputDevice = m_direction == Direction::Input && deviceInfo->maxInputChannels > 0;
			const auto canAddOutputDevice = m_direction == Direction::Output && deviceInfo->maxOutputChannels > 0;

			if ((canAddInputDevice || canAddOutputDevice) && deviceInfo->hostApi == backendIndex)
			{
				m_deviceComboBox->addItem(deviceInfo->name, i);
			}
		}

		const auto selectedDeviceName = ConfigManager::inst()->value(tag(), deviceNameAttribute(m_direction));
		const auto selectedDeviceIndex = std::max(0, m_deviceComboBox->findText(selectedDeviceName));
		m_deviceComboBox->setCurrentIndex(selectedDeviceIndex);

		const auto defaultNumChannels = QString::number(DEFAULT_CHANNELS);
		const auto selectedNumChannels
			= ConfigManager::inst()->value(tag(), channelsAttribute(m_direction), defaultNumChannels);
		m_channelModel.setValue(selectedNumChannels.toInt());

		const auto deviceIndex = m_deviceComboBox->currentData().toInt();
		const auto deviceSpec = DeviceSpec::loadFromIndex(deviceIndex, m_direction);

		m_channelModel.setRange(1, deviceSpec.maxChannels());
		m_channelSpinBox->setNumDigits(QString::number(deviceSpec.maxChannels()).length());
	}

	void saveToConfig()
	{
		ConfigManager::inst()->setValue(tag(), deviceNameAttribute(m_direction), m_deviceComboBox->currentText());
		ConfigManager::inst()->setValue(tag(), channelsAttribute(m_direction), QString::number(m_channelModel.value()));
	}

private:
	QComboBox* m_deviceComboBox = nullptr;
	lmms::gui::LcdSpinBox* m_channelSpinBox = nullptr;
	lmms::IntModel m_channelModel;
	Direction m_direction;
};

AudioPortAudioSetupWidget::AudioPortAudioSetupWidget(QWidget* parent)
	: AudioDeviceSetupWidget{AudioPortAudio::name(), parent}
{
	if (Pa_Initialize() != paNoError) { throw std::runtime_error{"PortAudio: could not initialize"}; }

	const auto form = new QFormLayout{this};
	form->setRowWrapPolicy(QFormLayout::WrapLongRows);
	form->setVerticalSpacing(10);

	m_backendComboBox = new QComboBox{};
	for (auto i = 0, backendCount = Pa_GetHostApiCount(); i < backendCount; ++i)
	{
		m_backendComboBox->addItem(Pa_GetHostApiInfo(i)->name, i);
	}

	const auto selectedBackendName = ConfigManager::inst()->value(tag(), backendAttribute());
	const auto selectedBackendIndex = std::max(0, m_backendComboBox->findText(selectedBackendName));
	m_backendComboBox->setCurrentIndex(selectedBackendIndex);

	m_inputDevice = new DeviceSpecWidget{Direction::Input};
	m_outputDevice = new DeviceSpecWidget{Direction::Output};

	form->addRow(tr("Backend"), m_backendComboBox);
	form->addRow(m_outputDevice);
	form->addRow(m_inputDevice);

	const auto onBackendIndexChanged = [&] {
		m_inputDevice->refreshFromConfig(m_backendComboBox->currentData().toInt());
		m_outputDevice->refreshFromConfig(m_backendComboBox->currentData().toInt());
	};

	onBackendIndexChanged();
	connect(m_backendComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, onBackendIndexChanged);
}

AudioPortAudioSetupWidget::~AudioPortAudioSetupWidget()
{
	Pa_Terminate();
}

void AudioPortAudioSetupWidget::saveSettings()
{
	ConfigManager::inst()->setValue(tag(), backendAttribute(), m_backendComboBox->currentText());
	m_inputDevice->saveToConfig();
	m_outputDevice->saveToConfig();
}
} // namespace lmms::gui

#endif // LMMS_HAVE_PORTAUDIO
