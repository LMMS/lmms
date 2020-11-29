/*
 * MidiSndio.h - base-class that implements sndio midi support
 *
 * Copyright (c) 2010-2016 jackmsr@openbsd.net
 * Copyright (c) 2016-2017 David Carlier <devnexen@gmail.com>
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

#ifndef _MIDI_SNDIO_H
#define _MIDI_SNDIO_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_SNDIO

#include <QtCore/QThread>
#include <QtCore/QFile>

#include <sndio.h>

#include "MidiClient.h"


class MidiSndio : public QThread, public MidiClientRaw
{
	Q_OBJECT
public:
	MidiSndio( void );
	virtual ~MidiSndio();

	static QString probeDevice(void);

	inline static QString name(void)
	{
		return QT_TRANSLATE_NOOP("MidiSetupWidget", "sndio MIDI");
	}

	inline static QString configSection()
	{
		return "MidiSndio";
	}


protected:
	void sendByte(const unsigned char c) override;
	void run(void) override;

private:
	struct mio_hdl *m_hdl;
	volatile bool m_quit;
} ;

#endif	/* LMMS_HAVE_SNDIO */

#endif	/* _MIDI_SNDIO_H */
