/*
 * MidiExport.h - support for Exporting MIDI-files
 *
 * Copyright (c) 2015 Mohamed Abdel Maksoud <mohamed at amaksoud.com>
 * Copyright (c) 2017 Hyunjin Song <tteu.ingog/at/gmail.com>
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

#ifndef _MIDI_EXPORT_H
#define _MIDI_EXPORT_H

#include <QString>

#include "ExportFilter.h"
#include "MidiFile.hpp"

using std::vector;

/*---------------------------------------------------------------------------*/

//! Size of the buffer used for exporting info
constexpr size_t BUFFER_SIZE = 50*1024;

//! A single note
struct MidiNote
{
	//! The pitch (tone), which can be lower or higher
	uint8_t pitch;

	//! Volume (loudness)
	uint8_t volume;

	//! Absolute time (from song start) when the note starts playing
	int time;

	//! For how long the note plays
	int duration;

	//! Sort notes by time
	inline bool operator<(const MidiNote &b) const
	{
		return time < b.time;
	}
};

// Helper vector typedefs
typedef vector<MidiNote> MidiNoteVector;
typedef vector<MidiNote>::iterator MidiNoteIterator;

/*---------------------------------------------------------------------------*/

//! MIDI exporting base class
class MidiExport: public ExportFilter
{
	typedef MidiFile::MIDITrack<BUFFER_SIZE> MTrack;

private:
	struct Pattern;
	void error();

public:
	MidiExport();
	~MidiExport();

	virtual PluginView *instantiateView(QWidget *)
	{
		return nullptr;
	}

	//! Export a list of tracks with tempo and master pitch
	//  to designated filename. Return if operation was successful
	bool tryExport(const TrackContainer::TrackList &tracks,
			const TrackContainer::TrackList &tracksBB,
			int tempo, int masterPitch, const QString &filename);
} ;

/*---------------------------------------------------------------------------*/

#endif
