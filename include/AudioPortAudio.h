/*
 * AudioPortAudio.h - device-class that performs PCM-output via PortAudio
 *
 * Copyright (c) 2008 Csaba Hruska <csaba.hruska/at/gmail.com>
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

#ifndef LMMS_AUDIO_PORTAUDIO_H
#define LMMS_AUDIO_PORTAUDIO_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_PORTAUDIO

#include <QComboBox>
#include <QFormLayout>
#include <QString>
#include <QWidget>
#include <portaudio.h>

#include "AudioDevice.h"
#include "AudioDeviceSetupWidget.h"
#include "LcdSpinBox.h"

namespace lmms {
class AudioPortAudio : public AudioDevice
{
public:
	AudioPortAudio(AudioEngine* engine);
	~AudioPortAudio() override;

	void startProcessing() override;
	void stopProcessing() override;

	static auto name() -> QString { return QT_TRANSLATE_NOOP("AudioDeviceSetupWidget", "PortAudio"); }

private:
	static int processCallback(const void* input, void* output, unsigned long frameCount,
		const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData);

	PaStream* m_paStream = nullptr;
	std::vector<SampleFrame> m_outBuf;
	std::size_t m_outBufPos = 0;
};
} // namespace lmms

namespace lmms::gui {
class AudioPortAudioSetupWidget : public AudioDeviceSetupWidget
{
public:
	AudioPortAudioSetupWidget(QWidget* parent);
	~AudioPortAudioSetupWidget();

	void show() override;
	void saveSettings() override;

private:
	void updateBackends(bool updateInput = true, bool updateOutput = true);
	void updateDevices(bool updateInput = true, bool updateOutput = true);
	void updateChannels(bool updateInput = true, bool updateOutput = true);

	QComboBox* m_outputDeviceComboBox = nullptr;
	QComboBox* m_outputBackendComboBox = nullptr;
	IntModel m_outputChannelsModel;
	LcdSpinBox* m_outputChannelsSpinBox = nullptr;

	QComboBox* m_inputDeviceComboBox = nullptr;
	QComboBox* m_inputBackendComboBox = nullptr;
	IntModel m_inputChannelsModel;
	LcdSpinBox* m_inputChannelsSpinBox = nullptr;
};
} // namespace lmms::gui

#endif // LMMS_HAVE_PORTAUDIO

#endif // LMMS_AUDIO_PORTAUDIO_H
