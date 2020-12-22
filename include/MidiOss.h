/*
 * MidiOss.h - OSS raw MIDI client
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef MIDI_OSS_H
#define MIDI_OSS_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_OSS

#include <QtCore/QThread>
#include <QtCore/QFile>

#include "MidiClient.h"



class MidiOss : public QThread, public MidiClientRaw
{
	Q_OBJECT
public:
	MidiOss();
	virtual ~MidiOss();

	static QString probeDevice();


	inline static QString name()
	{
		return( QT_TRANSLATE_NOOP( "MidiSetupWidget",
			"OSS Raw-MIDI (Open Sound System)" ) );
	}

	inline static QString configSection()
	{
		return "midioss";
	}

protected:
	void sendByte( const unsigned char c ) override;
	void run() override;


private:
	QFile m_midiDev;

	volatile bool m_quit;

} ;

#endif


#endif
