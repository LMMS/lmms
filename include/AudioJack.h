/*
 * AudioJack.h - support for JACK-transport
 *
 * Copyright (c) 2005-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_AUDIO_JACK_H
#define LMMS_AUDIO_JACK_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_JACK
#ifndef LMMS_HAVE_WEAKJACK
#include <jack/jack.h>
#else
#include <weak_libjack.h>
#endif

#include <atomic>
#include <vector>
#ifdef AUDIO_BUS_HANDLE_SUPPORT
#include <QMap>
#endif

#include "AudioDevice.h"
#include "AudioDeviceSetupWidget.h"
#ifdef AUDIO_BUS_HANDLE_SUPPORT
#include "AudioBusHandle.h"
#endif

class QLineEdit;
class QMenu;
class QToolButton;

namespace lmms
{

class MidiJack;


class AudioJack : public QObject, public AudioDevice
{
	Q_OBJECT
public:
	AudioJack(bool& successful, AudioEngine* audioEngine);
	~AudioJack() override;

	// this is to allow the jack midi connection to use the same jack client connection
	// the jack callback is handled here, we call the midi client so that it can read
	// it's midi data during the callback
	AudioJack* addMidiClient(MidiJack* midiClient);
	void removeMidiClient() { m_midiClient = nullptr; }
	jack_client_t* jackClient() { return m_client; };

	inline static QString name()
	{
		return QT_TRANSLATE_NOOP("AudioDeviceSetupWidget", "JACK (JACK Audio Connection Kit)");
	}

	class setupWidget : public gui::AudioDeviceSetupWidget
	{
	public:
		setupWidget(QWidget* parent);
		void saveSettings() override;

	private:
		std::vector<std::string> getAudioPortNames(JackPortFlags portFlags) const;
		std::vector<std::string> getAudioInputNames() const;
		std::vector<std::string> getAudioOutputNames() const;
		static QMenu* buildMenu(QToolButton* toolButton, const std::vector<std::string>& names, const QString& filteredLMMSClientName);

	private:
		QLineEdit* m_clientName;
		// Because we do not have access to a JackAudio driver instance we have to be our own client to display inputs and outputs...
		jack_client_t* m_client;

		std::vector<QToolButton*> m_outputDevices;
		std::vector<QToolButton*> m_inputDevices;
	};

private slots:
	void restartAfterZombified();

private:
	bool initJackClient();
	void resizeInputBuffer(jack_nframes_t nframes);

	void attemptToConnect(size_t index, const char *lmms_port_type, const char *source_port, const char *destination_port);
	void attemptToReconnectOutput(size_t outputIndex, const QString& targetPort);
	void attemptToReconnectInput(size_t inputIndex, const QString& sourcePort);

	void startProcessing() override;
	void stopProcessing() override;

	void registerPort(AudioBusHandle* port) override;
	void unregisterPort(AudioBusHandle* port) override;
	void renamePort(AudioBusHandle* port) override;

	int processCallback(jack_nframes_t nframes);

	static int staticProcessCallback(jack_nframes_t nframes, void* udata);
	static void shutdownCallback(void* _udata);

	jack_client_t* m_client;

	bool m_active;
	std::atomic<bool> m_stopped;

	std::atomic<MidiJack*> m_midiClient;
	std::vector<jack_port_t*> m_outputPorts;
	std::vector<jack_port_t*> m_inputPorts;
	jack_default_audio_sample_t** m_tempOutBufs;
	std::vector<SampleFrame> m_inputFrameBuffer;
	SampleFrame* m_outBuf;

	f_cnt_t m_framesDoneInCurBuf;
	f_cnt_t m_framesToDoInCurBuf;

#ifdef AUDIO_BUS_HANDLE_SUPPORT
	struct StereoPort
	{
		jack_port_t* ports[2];
	};

	using JackPortMap = QMap<AudioBusHandle*, StereoPort>;
	JackPortMap m_portMap;
#endif

signals:
	void zombified();
};

} // namespace lmms

#endif // LMMS_HAVE_JACK

#endif // LMMS_AUDIO_JACK_H
