/*
 * Piano.h - declaration of class Piano
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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

#ifndef _PIANO_H
#define _PIANO_H

#include "note.h"
#include "Model.h"

class MidiEventProcessor;

class Piano : public Model
{
public:
	enum KeyTypes
	{
		WhiteKey,
		BlackKey
	} ;

	Piano( MidiEventProcessor * _mep );
	virtual ~Piano();

	void setKeyState( int _key, bool _on = false );

	void handleKeyPress( int _key );
	void handleKeyRelease( int _key );


private:
	MidiEventProcessor * m_midiEvProc;
	bool m_pressedKeys[NumKeys];


	friend class PianoView;

} ;

#endif

