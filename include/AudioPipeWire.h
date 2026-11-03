/*
 * AudioPipeWire.h - Audio driver for PipeWire
 *
 * Copyright (c) 2024- Michael Gregorius
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

#ifndef LMMS_AUDIO_PIPEWIRE_H
#define LMMS_AUDIO_PIPEWIRE_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_PIPEWIRE

#include "AudioDevice.h"
#include "AudioDeviceSetupWidget.h"

#include <pipewire/pipewire.h>

#include <QObject>

#include <atomic>
#include <cstdint>
#include <vector>

class QToolButton;

namespace lmms
{

class AudioPipeWire : public QObject, public AudioDevice
{
	Q_OBJECT
public:
	AudioPipeWire(bool& successful, AudioEngine* audioEngine);
	~AudioPipeWire() override;

	inline static QString name()
	{
		return QT_TRANSLATE_NOOP( "AudioDeviceSetupWidget", "PipeWire" );
	}

	void processOutputStreams();
	void processCaptureStreams();

	// Called on the PipeWire thread when a stream's format has been negotiated.
	void onFormatNegotiated(bool capture, uint32_t rate);

	class setupWidget : public gui::AudioDeviceSetupWidget
	{
	public:
		setupWidget(QWidget* parent);
		~setupWidget() override;

		void saveSettings() override;

	private:
		QToolButton* m_outputDevice;
		QToolButton* m_inputDevice;
	};

private:
	void startProcessingImpl() override;
	void stopProcessingImpl() override;

private:
	pw_thread_loop* m_loop = nullptr;
	pw_context* m_context = nullptr;
    pw_core* m_core = nullptr;
    pw_stream* m_outputStream = nullptr;
	pw_stream* m_captureStream = nullptr;

	std::vector<SampleFrame> m_captureBuffer;

	// 0 until the respective stream has negotiated its format
	std::atomic<uint32_t> m_playbackRate = 0;
	std::atomic<uint32_t> m_captureRate = 0;

	// This is a scratch space like buffer that is used by PipeWire to create POD (plain old data) objects on the stack.
	uint8_t m_podBuilderBuffer[1024];
};

} // namespace lmms

#endif // LMMS_HAVE_PIPEWIRE

#endif // LMMS_AUDIO_PIPEWIRE_H
