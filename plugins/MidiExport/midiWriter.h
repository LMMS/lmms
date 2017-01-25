/*
 * midiWriter.h - export backend.
 *
 * Copyright (c) 2016-2017 Tony Chyi <tonychee1989/at/gmail.com>
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

#ifndef MIDI_WRITER_H
#define MIDI_WRITER_H

#include <QObject>
#include <drumstick/qsmf.h>

#include "AutomationPattern.h"
#include "Engine.h"
#include "MeterModel.h"
#include "Midi.h"
#include "MidiEvent.h"
#include "TrackContainer.h"

#define EXPORTED_BY QString( "Exported by LMMS" )
#define GM_SYSEX QByteArray( "\x7e\x7f\x09\x01" ) + (char) end_of_sysex

const int DefaultPitchMultiply = 2;
// If TimSig = 4/4, set midi clock = 24;
const int DefaultMidiClockPerMetronomeClick = 24;
const int DefaultMidiDivision = 960;
// Channel 10 in SMF is drum kit.
const int DrumChannel = 9;

class midiWriter: public QObject
{
	Q_OBJECT
public:
	midiWriter( const TrackContainer::TrackList &tracks );
	~midiWriter();
	void writeFile( const QString &fileName );

public slots:
	void writeTrackEvent( int track );

private:
	/*
	 * Write event to file here. But need to delete event after write,
	 * otherwise it will cause memory leak.
	 */
	void writeEventToFile();

	// All methods below will create new MidiEvent, MUST delete it later!
	void insertTempoEvent();
	void insertTimeSigEvent();
	void insertNoteEvent( InstrumentTrack *track );

	void insertCCEvent( InstrumentTrack *track );
	void insertProgramEvent( InstrumentTrack *track );
	void insertPitchEvent( InstrumentTrack *track );

	void allocateChannel(InstrumentTrack *track );
	int intSqrt( int n );

	const TrackContainer::TrackList &m_tl;
	drumstick::QSmf *m_seq;

	QMultiMap< long, MidiEvent* > EventList;
	QMap< int, int > PitchBendMultiply;
	QList< int > InstrumentTracks;
	QList< int > AutomationTracks;
	int8_t ChannelProg[ MidiChannelCount ];

	AutomationPattern* tempoPat;
	AutomationPattern* timeSigNumPat;
	AutomationPattern* timeSigDenPat;

	double tickRate;
	int8_t currentChannel;

	// LMMS just only export pitch bend range.
	bool flagRpnPitchBendRangeSent;
	bool flagCurrentTrackHasNotes;
};

#endif
