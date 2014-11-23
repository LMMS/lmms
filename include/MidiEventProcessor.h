/*
 * MidiEventProcessor.h - base-class for midi-processing classes
 *
 * Copyright (c) 2005-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef MIDI_EVENT_PROCESSOR_H
#define MIDI_EVENT_PROCESSOR_H

#include "MidiEvent.h"
#include "MidiTime.h"


// all classes being able to process MIDI-events should inherit from this
class MidiEventProcessor
{
public:
	MidiEventProcessor()
	{
	}

	virtual ~MidiEventProcessor()
	{
	}

	// to be implemented by inheriting classes
	virtual void processInEvent( const MidiEvent& event, const MidiTime& time = MidiTime(), f_cnt_t offset = 0 ) = 0;
	virtual void processOutEvent( const MidiEvent& event, const MidiTime& time = MidiTime(), f_cnt_t offset = 0 ) = 0;

} ;

#endif
