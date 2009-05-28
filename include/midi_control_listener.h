/*
 * midi_control_listener.h - listens to MIDI source(s) for commands that
 *                           start/stops LMMS' transportation,
 *                           or toggles loop
 *
 * Copyright (c) 2009 Achim Settelmeier <lmms/at/m1.sirlab.de>
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

#ifndef _MIDI_CONTROL_LISTENER_H
#define _MIDI_CONTROL_LISTENER_H

#include "midi_event_processor.h"
#include "midi_port.h"
#include "note.h"

class MidiControlListener : public MidiEventProcessor
{
public:
	typedef enum
	{
		ActionNone = 0,
		ActionPlay,
		ActionStop
	} EventAction;
	typedef QMap<int, EventAction> ActionMap;

	MidiControlListener();
	virtual ~MidiControlListener();

	virtual void processInEvent( const midiEvent & _me,
						const midiTime & _time );
	virtual void processOutEvent( const midiEvent & _me,
				     const midiTime & _time )
	{
	}

private:
	void act( EventAction _action );


	midiPort m_port;

	bool m_controlKeyPressed;  // flag, whether the control key is pressed

	// configuration
	bool m_useControlKey;      // true: use control key (two key sequence); false: single key sequence
	int m_controlKey;          // number of the control key (0 - NumKeys)
	int m_controlChannel;      // number of channel (0 - 15) or -1 for all channels
	ActionMap m_actionMapKeys;
	ActionMap m_actionMapControllers;
} ;

#endif

