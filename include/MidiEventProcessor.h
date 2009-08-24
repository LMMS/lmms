/*
 * MidiEventProcessor.h - base-class for midi-processing classes
 *
 * Copyright (c) 2005-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _MIDI_EVENT_PROCESSOR_H
#define _MIDI_EVENT_PROCESSOR_H

#include <QtCore/QCoreApplication>

#include "note.h"
#include "AutomatableModel.h"


// all classes being able to process MIDI-events should inherit from this
class MidiEventProcessor
{
public:
	inline MidiEventProcessor() :
		m_baseNoteModel( DefaultKey,
					0,
					KeysPerOctave * NumOctaves - 1,
					NULL,
					QCoreApplication::tr( "Base note" ) )
	{
	}

	virtual inline ~MidiEventProcessor()
	{
	}

	IntModel * baseNoteModel()
	{
		return &m_baseNoteModel;
	}

	const IntModel * baseNoteModel() const
	{
		return &m_baseNoteModel;
	}


	// to be implemented by inheriting classes
	virtual void processInEvent( const midiEvent & _me,
						const midiTime & _time ) = 0;
	virtual void processOutEvent( const midiEvent & _me,
						const midiTime & _time ) = 0;

private:
	IntModel m_baseNoteModel;

} ;

#endif
