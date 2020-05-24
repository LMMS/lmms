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

#include "MidiFile.h"
#include "DataFile.h"
#include "ExportFilter.h"

using std::pair;
using std::vector;

/*---------------------------------------------------------------------------*/

//! MIDI exporting base class
class MidiExport : public ExportFilter
{
private:
	//! A single MIDI note
	struct Note
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
		inline bool operator<(const Note &b) const
		{
			return m_time < b.m_time;
		}
	};

	/*-----------------------------------------------------------------------*/

	//! A pattern of MIDI notes
	class Pattern
	{
	private:
		//! List of actual notes
		vector<Note> m_notes;

	public:
		//! Append notes from root node to pattern
		void write(const QDomNode &root,
				int basePitch, double baseVolume, int baseTime);

		//! Adjust special duration BB notes by resizing them accordingly
		void processBbNotes(int cutPos);

		//! Add pattern notes to MIDI file track
		void writeToTrack(MidiFile::Track &mTrack) const;

		//! Write sorted notes to a explicitly repeating BB pattern
		void writeToBb(Pattern &bbPat,
				int len, int base, int start, int end);
	};

	/*-----------------------------------------------------------------------*/

	//! MIDI file object to work with
	MidiFile *m_file;

	//! Song global tempo
	int m_tempo;

	//! Song master pitch
	int m_masterPitch;

	//! Current (incremental) track channel number for non drum tracks
	uint8_t m_channel = 0;

	//! DataFile to be used by Qt elements
	DataFile m_dataFile = DataFile(DataFile::SongProject);

	//! Matrix containing (start, end) pairs for BB objects
	vector<vector<pair<int, int>>> m_plists;

public:
	//! Explicit constructor for setting plugin descriptor
	MidiExport();

	//! \brief Export tracks from a project to a .mid extension MIDI file
	//! \param tracks Normal instrument tracks
	//! \param tracksBB Beat + Bassline tracks
	//! \param tempo Song global tempo
	//! \param masterPitch Song master pitch
	//! \param filename Name of file to be saved
	//! \return If operation was successful
	bool tryExport(const TrackContainer::TrackList &tracks,
			const TrackContainer::TrackList &tracksBB,
			int tempo, int masterPitch, const QString &filename);

private:
	//! Process a given instrument track
	void processTrack(Track *track, size_t channelID, bool isBB=false);

	//! Build a repeating pattern from a normal one and write to MIDI track
	void writeBbPattern(Pattern &pat, const QDomElement &patElem,
			uint8_t channelID, MidiFile::Track &midiTrack);

	//! Process a given BB track
	void processBbTrack(Track *track);

	//! Necessary for lmms_plugin_main()
	PluginView *instantiateView(QWidget *) { return nullptr; }
} ;

/*---------------------------------------------------------------------------*/

#endif
