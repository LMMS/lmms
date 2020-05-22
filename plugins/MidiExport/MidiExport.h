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

#include "MidiFile.hpp"
#include "DataFile.h"
#include "ExportFilter.h"

using std::pair;
using std::vector;

/*---------------------------------------------------------------------------*/

//! A single note
struct MidiNote
{
	//! The pitch (tone), which can be lower or higher
	uint8_t m_pitch;

	//! Volume (loudness)
	uint8_t m_volume;

	//! Absolute time (from song start) when the note starts playing
	int m_time;

	//! For how long the note plays
	int m_duration;

	//! Sort notes by time
	inline bool operator<(const MidiNote &b) const
	{
		return m_time < b.m_time;
	}
};

/*---------------------------------------------------------------------------*/

//! MIDI exporting base class
class MidiExport : public ExportFilter
{
private:
	//! Represents a pattern of notes
	struct Pattern
	{
		//! Vector of actual notes
		vector<MidiNote> m_notes;

		//! Append notes from root node to pattern
		void write(const QDomNode &root,
				int basePitch, double baseVolume, int baseTime);

		//! Add pattern notes to MIDI file track
		void writeToTrack(MidiFile::Track &mTrack) const;

		//! Adjust special duration BB notes by resizing them accordingly
		void processBBNotes(int cutPos);

		//! Write sorted notes to a explicitly repeating BB pattern
		void writeToBB(Pattern &bbPat,
				int len, int base, int start, int end);
	};

	//! DataFile to be used by Qt elements
	DataFile m_dataFile = DataFile(DataFile::SongProject);

	MidiFile *m_file;

	//! Song tempo
	int m_tempo;

	//! Song master pitch
	int m_masterPitch;

	//! Matrix containing (start, end) pairs for BB objects
	vector<vector<pair<int, int>>> m_plists;

	void foo(Track *track, uint8_t &channelID, bool isBB=false);
	void goo(Track *track);

public:
	MidiExport();
	~MidiExport();

	virtual PluginView *instantiateView(QWidget *)
	{
		return nullptr;
	}

	//! Export normal and BB tracks from a project with designated
	//  tempo and master pitch to a file indicated by \param filename
	//  \return If operation was successful
	bool tryExport(const TrackContainer::TrackList &tracks,
			const TrackContainer::TrackList &tracksBB,
			int tempo, int masterPitch, const QString &filename);
} ;

/*---------------------------------------------------------------------------*/

#endif
