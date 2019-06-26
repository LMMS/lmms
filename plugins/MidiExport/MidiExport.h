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


const int BUFFER_SIZE = 50*1024;
typedef MidiFile::MIDITrack<BUFFER_SIZE> MTrack;

struct MidiNote
{
	int time;
	uint8_t pitch;
	int duration;
	uint8_t volume;

	inline bool operator<(const MidiNote &b) const
	{
		return this->time < b.time;
	}
} ;

typedef std::vector<MidiNote> MidiNoteVector;
typedef std::vector<MidiNote>::iterator MidiNoteIterator;



class MidiExport: public ExportFilter
{
// 	Q_OBJECT
public:
	MidiExport();
	~MidiExport();

	virtual PluginView *instantiateView(QWidget *)
	{
		return nullptr;
	}

	virtual bool tryExport(const TrackContainer::TrackList &tracks,
				const TrackContainer::TrackList &tracks_BB,
				int tempo, int masterPitch, const QString &filename);
	
private:
	void writePattern(MidiNoteVector &pat, QDomNode n,
				int base_pitch, double base_volume, int base_time);
	void writePatternToTrack(MTrack &mtrack, MidiNoteVector &nv);
	void writeBBPattern(MidiNoteVector &src, MidiNoteVector &dst,
				int len, int base, int start, int end);
	void ProcessBBNotes(MidiNoteVector &nv, int cutPos);

	void error();


} ;


#endif
