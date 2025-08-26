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


#include "MidiFile.h"
#include "DataFile.h"
#include "ExportFilter.h"
#include "Note.h"

class QDomNode;

namespace lmms
{

/*---------------------------------------------------------------------------*/

//! MIDI exporting base class
class MidiExport : public ExportFilter
{
private:
	//! A single MIDI note
	struct Note
	{
		using Type = ::lmms::Note::Type;

		//! The pitch (tone), which can be lower or higher
		uint8_t m_pitch = 0;

		//! Volume (loudness)
		uint8_t m_volume = 0;

		//! Absolute time (from song start) when the note starts playing
		int m_time = 0;

		//! For how long the note plays
		int m_duration = 0;

		Type type = Type::Regular;

		//! Sort notes by time
		inline bool operator<(const Note &b) const
		{
			return m_time < b.m_time;
		}
	};

	/*-----------------------------------------------------------------------*/

	//! A clip of MIDI notes
	class Clip
	{
	private:
		//! List of actual notes
		std::vector<Note> m_notes;

	public:
		//! Append notes from root node to clip
		void write(const QDomNode &root,
				int basePitch, double baseVolume, int baseTime);

		//! Adjust special duration pattern clip notes by resizing them accordingly
		void processPatternNotes(int cutPos);

		//! Add clip notes to MIDI file track
		void writeToTrack(MidiFile::Track &mTrack) const;

		//! Write sorted notes to a explicitly repeating pattern clip
		void writeToPattern(Clip &patternClip,
				int len, int base, int start, int end);
	};

	/*-----------------------------------------------------------------------*/

	//! MIDI file object to work with
	MidiFile* m_file = nullptr;

	//! Song global tempo
	int m_tempo = 0;

	//! Song master pitch
	int m_masterPitch = 0;

	//! Current (incremental) track channel number for non drum tracks
	uint8_t m_channel = 0;

	//! DataFile to be used by Qt elements
	DataFile m_dataFile = DataFile(DataFile::Type::SongProject);

	//! Matrix containing (start, end) pairs for pattern objects
	std::vector<std::vector<std::pair<int, int>>> m_plists;

public:
	//! Explicit constructor for setting plugin descriptor
	MidiExport();
	~MidiExport() override = default;

	// Default Beat Length in ticks for step notes
	// TODO: The beat length actually varies per note, however the method that
	// calculates it (InstrumentTrack::beatLen) requires a NotePlayHandle to do
	// so. While we don't figure out a way to hold the beat length of each note
	// on its member variables, we will use a default value as a beat length that
	// will be used as an upper limit of the midi note length. This doesn't worsen
	// the current logic used for MidiExport because right now the beat length is
	// not even considered during the generation of the MIDI.
	static constexpr int DefaultBeatLength = 1500;

	//! \brief Export tracks from a project to a .mid extension MIDI file
	//! \param tracks Normal instrument tracks
	//! \param patternStoreTracks PatternStore tracks
	//! \param tempo Song global tempo
	//! \param masterPitch Song master pitch
	//! \param filename Name of file to be saved
	//! \return If operation was successful
	bool tryExport(const TrackContainer::TrackList &tracks,
			const TrackContainer::TrackList &patternStoreTracks,
			int tempo, int masterPitch, const QString &filename) override;

private:
	//! Process a given instrument track
	void processTrack(Track *track, size_t channelID, bool isPattern=false);

	//! Build a repeating clip from a normal one and write to MIDI track
	void writePatternClip(Clip &clip, const QDomElement &clipElem,
			uint8_t channelID, MidiFile::Track &midiTrack);

	//! Process a given pattern track
	void processPatternTrack(Track *track);

	//! Necessary for lmms_plugin_main()
	gui::PluginView *instantiateView(QWidget *) override { return nullptr; }
} ;

/*---------------------------------------------------------------------------*/

} // namespace lmms

#endif
