/*
 * MidiJack.h - MIDI client for Jack
 *
 * Copyright (c) 2015 Shane Ambler <develop/at/shaneware.biz>
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

#ifndef MIDIJACK_H
#define MIDIJACK_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_JACK
#ifndef LMMS_HAVE_WEAKJACK
#include <jack/jack.h>
#include <jack/midiport.h>
#else
#include "weak_libjack.h"
#endif

#include <QtCore/QThread>
#include <QMutex>
#include <QtCore/QFile>

#include "MidiClient.h"
#include "AudioJack.h"

#define	JACK_MIDI_BUFFER_MAX 64 /* events */

class QLineEdit;

class MidiJack : public QThread, public MidiClientRaw
{
        Q_OBJECT
public:
	MidiJack();
	virtual ~MidiJack();

	jack_client_t* jackClient();

	static QString probeDevice();

	inline static QString name()
	{
		return( QT_TRANSLATE_NOOP( "MidiSetupWidget",
			"Jack-MIDI" ) );
	}

	void JackMidiWrite(jack_nframes_t nframes);
	void JackMidiRead(jack_nframes_t nframes);


	inline static QString configSection()
	{
		return "MidiJack";
	}


protected:
	virtual void sendByte( const unsigned char c );
	virtual void run();


private:
	AudioJack *m_jackAudio;
	jack_client_t *m_jackClient;
	jack_port_t *m_input_port;
	jack_port_t *m_output_port;
	uint8_t m_jack_buffer[JACK_MIDI_BUFFER_MAX * 4];

	void JackMidiOutEvent(uint8_t *buf, uint8_t len);
	void lock();
	void unlock();

	void getPortInfo( const QString& sPortName, int& nClient, int& nPort );

	volatile bool m_quit;

};

#endif // LMMS_HAVE_JACK

#endif // MIDIJACK_H
