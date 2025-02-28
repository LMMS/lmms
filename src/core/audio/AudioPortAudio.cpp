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
constexpr auto configBackendAttribute = "backend";

constexpr auto configOutputDeviceAttribute = "outputdevice";
constexpr auto configOutputDeviceChannelsAttribute = "outputchannels";

constexpr auto configInputDeviceAttribute = "inputdevice";
constexpr auto configInputDeviceChannelsAttribute = "inputchannels";
} // namespace

namespace lmms {
AudioPortAudio::AudioPortAudio(AudioEngine* engine)
	: AudioDevice(DEFAULT_CHANNELS, engine)
	, m_outBuf(engine->framesPerPeriod())
{
	if (Pa_Initialize() != paNoError)
	{
		throw std::runtime_error{"PortAudio: could not initialize - " + std::string(Pa_GetErrorText(Pa_Initialize()))};
	}

	const auto configBackendValue = ConfigManager::inst()->value(configTag, configBackendAttribute);

	const auto configOutputDeviceValue = ConfigManager::inst()->value(configTag, configOutputDeviceAttribute);
	const auto configOutputDeviceChannelsValue = ConfigManager::inst()->value(configTag, configOutputDeviceChannelsAttribute);
	
	const auto configInputDeviceValue = ConfigManager::inst()->value(configTag, configInputDeviceAttribute);
	const auto configInputDeviceChannelsValue = ConfigManager::inst()->value(configTag, configInputDeviceChannelsAttribute);
	
	auto outputDeviceIndex = paNoDevice;
	auto inputDeviceIndex = paNoDevice;
	
	const auto deviceCount = Pa_GetDeviceCount();
	if (deviceCount < 0) { throw std::runtime_error{"PortAudio: no available devices"}; }
	
	auto outputDeviceInfo = static_cast<const PaDeviceInfo*>(nullptr);
	auto inputDeviceInfo = static_cast<const PaDeviceInfo*>(nullptr);

	for (auto i = 0; i < deviceCount; ++i)
	{
		const auto currentDeviceInfo = Pa_GetDeviceInfo(i);
		const auto currentBackendInfo = Pa_GetHostApiInfo(currentDeviceInfo->hostApi);

		if (configOutputDeviceValue == currentDeviceInfo->name && configBackendValue == currentBackendInfo->name)
		{
			outputDeviceIndex = i;
			outputDeviceInfo = currentDeviceInfo;
		}

		if (configInputDeviceValue == currentDeviceInfo->name && configBackendValue == currentBackendInfo->name)
		{
			inputDeviceIndex = i;
			inputDeviceInfo = currentDeviceInfo;
		}
	}

	if (outputDeviceIndex == paNoDevice)
	{
		outputDeviceIndex = Pa_GetDefaultOutputDevice();
		outputDeviceInfo = Pa_GetDeviceInfo(outputDeviceIndex);
	}

	if (inputDeviceIndex == paNoDevice)
	{
		inputDeviceIndex = Pa_GetDefaultInputDevice();
		inputDeviceInfo = Pa_GetDeviceInfo(inputDeviceIndex);
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

	setChannels(outputDeviceChannelCount);

	const auto backendInfo = Pa_GetHostApiInfo(outputDeviceInfo->hostApi);
	ConfigManager::inst()->setValue(configTag, configBackendAttribute, backendInfo->name);

	ConfigManager::inst()->setValue(configTag, configOutputDeviceAttribute, outputDeviceInfo->name);
	ConfigManager::inst()->setValue(configTag, configOutputDeviceChannelsAttribute, QString::number(outputDeviceChannelCount));

	ConfigManager::inst()->setValue(configTag, configInputDeviceAttribute, inputDeviceInfo->name);
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
AudioPortAudioSetupWidget::AudioPortAudioSetupWidget(QWidget* parent)
	: AudioDeviceSetupWidget(AudioPortAudio::name(), parent)
{
	if (Pa_Initialize() != paNoError)
	{
		throw std::runtime_error{"PortAudio: could not initialize - " + std::string(Pa_GetErrorText(Pa_Initialize()))};
	}

	const auto form = new QFormLayout(this);
	form->setRowWrapPolicy(QFormLayout::WrapLongRows);

	const auto outputGroup = new QHBoxLayout();
	const auto inputGroup = new QHBoxLayout();

	m_backendComboBox = new QComboBox();

	m_outputDeviceComboBox = new QComboBox();
	m_outputChannelsSpinBox = new LcdSpinBox(1, nullptr);
	m_outputChannelsSpinBox->setModel(&m_outputChannelsModel);

	m_inputDeviceComboBox = new QComboBox();
	m_inputChannelsSpinBox = new LcdSpinBox(1, nullptr);
	m_inputChannelsSpinBox->setModel(&m_inputChannelsModel);

	outputGroup->addWidget(m_outputDeviceComboBox);
	outputGroup->addWidget(m_outputChannelsSpinBox);

	inputGroup->addWidget(m_inputDeviceComboBox);
	inputGroup->addWidget(m_inputChannelsSpinBox);

	form->addRow(tr("Backend"), m_backendComboBox);
	form->addRow(tr("Output device"), outputGroup);
	form->addRow(tr("Input device"), inputGroup);

	connect(m_backendComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this,
		[&] { updateDevices(); });

	connect(m_outputDeviceComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this,
		[&] { updateChannels(false, true); });

	connect(m_inputDeviceComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this,
		[&] { updateChannels(true, false); });
}

AudioPortAudioSetupWidget::~AudioPortAudioSetupWidget()
{
	Pa_Terminate();
}

void AudioPortAudioSetupWidget::show()
{
	updateBackends();
	updateDevices();
	
	const auto configBackendValue = ConfigManager::inst()->value(configTag, configBackendAttribute);
	
	const auto configOutputDeviceValue = ConfigManager::inst()->value(configTag, configOutputDeviceAttribute);
	const auto configOutputDeviceChannelsValue = ConfigManager::inst()->value(configTag, configOutputDeviceChannelsAttribute);
	
	const auto configInputDeviceValue = ConfigManager::inst()->value(configTag, configInputDeviceAttribute);
	const auto configInputDeviceChannelsValue = ConfigManager::inst()->value(configTag, configInputDeviceChannelsAttribute);
	
	const auto backendIndex = std::max(0, m_backendComboBox->findText(configBackendValue));
	m_backendComboBox->setCurrentIndex(backendIndex);

	const auto inputDeviceIndex = std::max(0, m_inputDeviceComboBox->findText(configInputDeviceValue));
	m_inputDeviceComboBox->setCurrentIndex(inputDeviceIndex);
	m_inputDeviceComboBox->setCurrentIndex(inputDeviceIndex);
	m_inputChannelsModel.setValue(configInputDeviceChannelsValue.toInt());

	const auto outputDeviceIndex = std::max(0, m_outputDeviceComboBox->findText(configOutputDeviceValue));
	m_outputDeviceComboBox->setCurrentIndex(outputDeviceIndex);
	m_outputChannelsModel.setValue(configOutputDeviceChannelsValue.toInt());

	AudioDeviceSetupWidget::show();
}

void AudioPortAudioSetupWidget::updateBackends()
{
	const auto backendCount = Pa_GetHostApiCount();
	if (backendCount < 0) { return; }

	for (auto i = 0; i < backendCount; ++i)
	{
		const auto backendInfo = Pa_GetHostApiInfo(i);
		m_backendComboBox->addItem(backendInfo->name, i);
	}
}

void AudioPortAudioSetupWidget::updateDevices(bool updateInput, bool updateOutput)
{
	const auto deviceCount = Pa_GetDeviceCount();
	if (deviceCount < 0) { return; }

	if (updateOutput) { m_outputDeviceComboBox->clear(); }
	if (updateInput) { m_inputDeviceComboBox->clear(); }

	const auto backend = m_backendComboBox->currentData();
	if (!backend.isValid() && updateOutput) { return; }

	const auto backendIndex = backend.toInt();

	for (auto i = 0; i < deviceCount; ++i)
	{
		const auto deviceInfo = Pa_GetDeviceInfo(i);

		if (deviceInfo->maxOutputChannels > 0 && deviceInfo->hostApi == backendIndex && updateOutput)
		{
			m_outputDeviceComboBox->addItem(deviceInfo->name, i);
		}

		if (deviceInfo->maxInputChannels > 0 && deviceInfo->hostApi == backendIndex && updateInput)
		{
			m_inputDeviceComboBox->addItem(deviceInfo->name, i);
		}
	}
}

void AudioPortAudioSetupWidget::updateChannels(bool updateInput, bool updateOutput)
{	
	if (updateOutput)
	{
		const auto selectedOutputDevice = m_outputDeviceComboBox->currentData();
		if (!selectedOutputDevice.isValid()) { return; }
		
		const auto selectedOutputDeviceIndex = selectedOutputDevice.toInt();
		const auto outputDeviceInfo = Pa_GetDeviceInfo(selectedOutputDeviceIndex);

		m_outputChannelsModel.setRange(1, outputDeviceInfo->maxOutputChannels);
		m_outputChannelsSpinBox->setNumDigits(QString::number(outputDeviceInfo->maxOutputChannels).length());
	}
	
	if (updateInput)
	{
		const auto selectedInputDevice = m_inputDeviceComboBox->currentData();
		if (!selectedInputDevice.isValid() && updateInput) { return; }
	
		const auto selectedInputDeviceIndex = selectedInputDevice.toInt();
		const auto inputDeviceInfo = Pa_GetDeviceInfo(selectedInputDeviceIndex);

		m_inputChannelsModel.setRange(1, inputDeviceInfo->maxInputChannels);
		m_inputChannelsSpinBox->setNumDigits(QString::number(inputDeviceInfo->maxInputChannels).length());
	}
}

void AudioPortAudioSetupWidget::saveSettings()
{
	ConfigManager::inst()->setValue(configTag, configBackendAttribute, m_backendComboBox->currentText());

	ConfigManager::inst()->setValue(configTag, configOutputDeviceAttribute, m_outputDeviceComboBox->currentText());
	ConfigManager::inst()->setValue(configTag, configOutputDeviceChannelsAttribute, QString::number(m_outputChannelsModel.value()));

	ConfigManager::inst()->setValue(configTag, configInputDeviceAttribute, m_inputDeviceComboBox->currentText());
	ConfigManager::inst()->setValue(configTag, configInputDeviceChannelsAttribute, QString::number(m_inputChannelsModel.value()));
}
} // namespace lmms::gui

#endif // LMMS_HAVE_PORTAUDIO
