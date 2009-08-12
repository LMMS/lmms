/*
 * DummyMidiEventProcessor.h - implementation of a MidiEventProcessor that
 *                             does nothing besides updating the baseNoteModel.
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

#ifndef _DUMMY_MIDI_EVENT_PROCESSOR_H
#define _DUMMY_MIDI_EVENT_PROCESSOR_H

#include "MidiEventProcessor.h"

class DummyMidiEventProcessor : public MidiEventProcessor
{
public:
	inline DummyMidiEventProcessor() :
		MidiEventProcessor(),
		m_updateBaseNote( false )
	{
	}
	
	virtual inline ~DummyMidiEventProcessor()
	{
	}

	inline void setUpdateBaseNote( bool _updateBaseNote )
	{
		m_updateBaseNote = _updateBaseNote;
	}

	virtual void processInEvent( const midiEvent & _me,
				    const midiTime & _time )
	{
		if( m_updateBaseNote )
		{
			baseNoteModel()->setValue( _me.key() );
		}
	}

	virtual void processOutEvent( const midiEvent & _me,
				     const midiTime & _time )
	{
		// nop
	}

private:
	bool m_updateBaseNote;

} ;

#endif
