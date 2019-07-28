/*
 * MidiDummy.h - dummy MIDI-driver
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

#ifndef MIDI_DUMMY_H
#define MIDI_DUMMY_H

#include "MidiClient.h"


class MidiDummy : public MidiClientRaw
{
public:
	MidiDummy()
	{
	}
	virtual ~MidiDummy()
	{
	}

	inline static QString name()
	{
		return( QT_TRANSLATE_NOOP( "MidiSetupWidget",
			"Dummy (no MIDI support)" ) );
	}

	inline static QString probeDevice()
	{
		return QString(); // no midi device name
	}

	inline static QString configSection()
	{
		return QString(); // no configuration settings
	}


protected:
	void sendByte( const unsigned char ) override
	{
	}

} ;


#endif
