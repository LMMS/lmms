/*
 * MidiAlsaRaw.h - MIDI client for RawMIDI via ALSA
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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

#ifndef _MIDI_ALSA_RAW_H
#define _MIDI_ALSA_RAW_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_ALSA

#include <alsa/asoundlib.h>

#include <QtCore/QThread>

#include "MidiClient.h"


struct pollfd;
class QLineEdit;


class MidiAlsaRaw : public MidiClientRaw , public QThread
{
public:
	MidiAlsaRaw();
	virtual ~MidiAlsaRaw();

	static QString probeDevice();


	inline static QString name()
	{
		return QT_TRANSLATE_NOOP( "setupWidget",
			"ALSA Raw-MIDI (Advanced Linux Sound Architecture)" );
	}


	class setupWidget : public MidiClientRaw::setupWidget
	{
	public:
		setupWidget( QWidget * _parent );
		virtual ~setupWidget();

		virtual void saveSettings();

	private:
		QLineEdit * m_device;

	} ;


protected:
	virtual void sendByte( const unsigned char c );
	virtual void run();


private:
	snd_rawmidi_t * m_input, * * m_inputp;
	snd_rawmidi_t * m_output, * * m_outputp;
	int m_npfds;
	pollfd * m_pfds;

	volatile bool m_quit;

} ;

#endif

#endif
