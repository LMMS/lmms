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

#include <QObject>
#include "ComboBoxModel.h"

#include <portaudio.h>
#include "AudioDevice.h"
#include "AudioDeviceSetupWidget.h"

namespace lmms
{

namespace gui {

class ComboBox;
class LcdSpinBox;

class AudioPortAudioSetupWidget : public gui::AudioDeviceSetupWidget
{
public:
	AudioPortAudioSetupWidget(QWidget* _parent);
	~AudioPortAudioSetupWidget() override;

	void updateBackends();
	void updateDevices();
	void updateChannels();

	void saveSettings() override;
	void show() override;

private:
	ComboBox* m_backend;
	ComboBox* m_device;
	ComboBoxModel m_backendModel;
	ComboBoxModel m_deviceModel;
};
} // namespace gui

class AudioPortAudio : public AudioDevice
{
public:
	AudioPortAudio(bool& successful, AudioEngine* engine);
	~AudioPortAudio() override;

	inline static QString name()
	{
		return QT_TRANSLATE_NOOP( "AudioDeviceSetupWidget", "PortAudio" );
	}

private:
	void startProcessing() override;
	void stopProcessing() override;

	int processCallback(const float* _inputBuffer, float* _outputBuffer, f_cnt_t _framesPerBuffer);
	static int processCallback(const void* _inputBuffer, void* _outputBuffer, unsigned long _framesPerBuffer,
		const PaStreamCallbackTimeInfo* _timeInfo, PaStreamCallbackFlags _statusFlags, void* arg);

	PaStream * m_paStream;
	PaStreamParameters m_outputParameters;
	PaStreamParameters m_inputParameters;

	bool m_wasPAInitError;

	SampleFrame* m_outBuf;
	std::size_t m_outBufPos;
	fpp_t m_outBufSize;

	bool m_stopped;

} ;

#endif // LMMS_HAVE_PORTAUDIO

} // namespace lmms

#endif // LMMS_AUDIO_PORTAUDIO_H
