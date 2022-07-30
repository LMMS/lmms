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

#ifndef AUDIO_JACK_H
#define AUDIO_JACK_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_JACK
#ifndef LMMS_HAVE_WEAKJACK
#include <jack/jack.h>
#else
#include "weak_libjack.h"
#endif

#include <atomic>
#include <QVector>

#include "AudioDevice.h"
#include "AudioDeviceSetupWidget.h"

class QLineEdit;

namespace lmms
{

class MidiJack;

namespace gui
{
class LcdSpinBox;
}


class AudioJack : public QObject, public AudioDevice
{
	Q_OBJECT
public:
	AudioJack( bool & _success_ful, AudioEngine* audioEngine );
	~AudioJack() override;

	// this is to allow the jack midi connection to use the same jack client connection
	// the jack callback is handled here, we call the midi client so that it can read
	// it's midi data during the callback
	AudioJack * addMidiClient(MidiJack *midiClient);
	void removeMidiClient() { m_midiClient = nullptr; }
	jack_client_t * jackClient() {return m_client;};

	inline static QString name()
	{
		return QT_TRANSLATE_NOOP( "AudioDeviceSetupWidget",
			"JACK (JACK Audio Connection Kit)" );
	}


class setupWidget : public gui::AudioDeviceSetupWidget
	{
	public:
		setupWidget( QWidget * _parent );
		~setupWidget() override;

		void saveSettings() override;

	private:
		QLineEdit * m_clientName;
		gui::LcdSpinBox * m_channels;

	} ;


private slots:
	void restartAfterZombified();


private:
	bool initJackClient();

	void startProcessing() override;
	void stopProcessing() override;
	void applyQualitySettings() override;

	void registerPort( AudioPort * _port ) override;
	void unregisterPort( AudioPort * _port ) override;
	void renamePort( AudioPort * _port ) override;

	int processCallback( jack_nframes_t _nframes, void * _udata );

	static int staticProcessCallback( jack_nframes_t _nframes,
							void * _udata );
	static void shutdownCallback( void * _udata );


	jack_client_t * m_client;

	bool m_active;
	std::atomic<bool> m_stopped;

	std::atomic<MidiJack *> m_midiClient;
	QVector<jack_port_t *> m_outputPorts;
	jack_default_audio_sample_t * * m_tempOutBufs;
	surroundSampleFrame * m_outBuf;

	f_cnt_t m_framesDoneInCurBuf;
	f_cnt_t m_framesToDoInCurBuf;


#ifdef AUDIO_PORT_SUPPORT
	struct StereoPort
	{
		jack_port_t * ports[2];
	} ;

	using JackPortMap = QMap<AudioPort *, StereoPort>;
	JackPortMap m_portMap;
#endif

signals:
	void zombified();

} ;

} // namespace lmms

#endif // LMMS_HAVE_JACK

#endif
