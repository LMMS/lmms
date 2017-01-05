/*
 * smfMidiChannel.h - support for importing MIDI files
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

#ifndef SMF_MIDI_CHANNEL_H
#define SMF_MIDI_CHANNEL_H

#include <QApplication>

#include "InstrumentTrack.h"
#include "Pattern.h"
#include "Instrument.h"
#include "MidiTime.h"




class SmfMidiChannel
{

public:
	SmfMidiChannel();
	
	InstrumentTrack * it;
	Pattern* p;
	Instrument * it_inst;
	bool isSF2; 
	bool hasNotes;
	MidiTime lastEnd;
	QString trackName;
	
	SmfMidiChannel * create( TrackContainer* tc, QString tn );

	void addNote( Note & n );

};

#endif
