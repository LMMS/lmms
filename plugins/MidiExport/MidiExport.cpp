/*
 * MidiExport.cpp - support for Exporting MIDI files
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

#include "MidiExport.h"

#include <stack>

#include "Instrument.h"
#include "BBTrack.h"
#include "InstrumentTrack.h"
#include "LocaleHelper.h"
#include "plugin_export.h"

using std::stack;
using std::sort;

extern "C"
{

//! Standardized plugin descriptor for MIDI exporter
Plugin::Descriptor PLUGIN_EXPORT midiexport_plugin_descriptor =
{
	STRINGIFY(PLUGIN_NAME),
	"MIDI Export",
	QT_TRANSLATE_NOOP("pluginBrowser",
		"Filter for exporting MIDI-files from LMMS"),
	"Mohamed Abdel Maksoud <mohamed at amaksoud.com> and "
		"Hyunjin Song <tteu.ingog/at/gmail.com>",
	0x0100,
	Plugin::ExportFilter,
	NULL,
	NULL,
	NULL
};

} // extern "C"

/*---------------------------------------------------------------------------*/

void MidiExport::Pattern::write(const QDomNode &root,
		int basePitch, double baseVolume, int baseTime)
{
	// TODO interpret steps="12" muted="0" type="1" name="Piano1" len="259"
	for (QDomNode node = root.firstChild(); not node.isNull();
			node = node.nextSibling())
	{
		QDomElement element = node.toElement();

		// Ignore zero-length notes
		if (element.attribute("len", "0") == "0") continue;

		// Adjust note attributes based on base measures
		Note note;
		int pitch = element.attribute("key", "0").toInt() + basePitch;
		note.m_pitch = qBound(0, pitch, 127);
		double volume =
				LocaleHelper::toDouble(element.attribute("vol", "100"));
		volume *= baseVolume * (127.0 / 200.0);
		note.m_volume = qMin(qRound(volume), 127);
		note.m_time = baseTime + element.attribute("pos", "0").toInt();
		note.m_duration = element.attribute("len", "0").toInt();

		// Append note to vector
		m_notes.push_back(note);
	}
}

void MidiExport::Pattern::writeToTrack(MidiFile::Track &mTrack) const
{
	for (const Note &note : m_notes) {
		mTrack.addNote(note.m_pitch, note.m_volume,
				note.m_time / 48.0, note.m_duration / 48.0);
	}
}

void MidiExport::Pattern::processBbNotes(int cutPos)
{
	// Sort in reverse order
	sort(m_notes.rbegin(), m_notes.rend());

	int cur = INT_MAX, next = INT_MAX;
	for (Note &note : m_notes)
	{
		if (note.m_time < cur)
		{
			// Set last two notes positions
			next = cur;
			cur = note.m_time;
		}
		if (note.m_duration < 0)
		{
			// Note should have positive duration that neither
			// overlaps next one nor exceeds cutPos
			note.m_duration = qMin(-note.m_duration, next - cur);
			note.m_duration = qMin(note.m_duration, cutPos - note.m_time);
		}
	}
}

void MidiExport::Pattern::writeToBb(Pattern &bbPat,
		int len, int base, int start, int end)
{
	// Avoid misplaced start and end positions
	if (start >= end) { return; }

	// Adjust positions relatively to base pos
	start -= base;
	end -= base;

	sort(m_notes.begin(), m_notes.end());
	for (Note note : m_notes)
	{
		// Insert periodically repeating notes from <t0> and spaced
		// by <len> to mimic BB pattern behavior
		int t0 = note.m_time + ceil((start - note.m_time) / len) * len;
		for (int time = t0;	time < end; time += len)
		{
			note.m_time = base + time;
			bbPat.m_notes.push_back(note);
		}
	}
}

/*---------------------------------------------------------------------------*/

MidiExport::MidiExport() :
		ExportFilter(&midiexport_plugin_descriptor) {}

/*---------------------------------------------------------------------------*/

bool MidiExport::tryExport(const TrackContainer::TrackList &tracks,
		const TrackContainer::TrackList &tracksBB,
		int tempo, int masterPitch, const QString &filename)
{
	// Count total number of tracks
	uint16_t nTracks = 0;
	for (auto trackList : {tracks, tracksBB})
	{
		for (const Track *track : trackList)
		{
			if (track->type() == Track::InstrumentTrack) { nTracks++; }
		}
	}
	// Create MIDI file object
	MidiFile file(filename, nTracks);
	m_file = &file;

	// Write header info
	m_file->m_header.writeToBuffer();

	// Set tempo and pitch properties
	m_tempo = tempo;
	m_masterPitch = masterPitch;

	// Iterate through "normal" tracks
	uint8_t channel_id = 0;
	for (Track *track : tracks)
	{
		if (track->type() == Track::InstrumentTrack)
		{
			processTrack(track, channel_id);
		}
		else if (track->type() == Track::BBTrack)
		{
			processBbTrack(track);
		}
	}
	// Iterate through BB tracks
	for (Track *track : tracksBB)
	{
		processTrack(track, channel_id, true);
	}
	// Write all buffered data to stream
	m_file->writeAllToStream();

	// Always returns success... for now?
	return true;
}

void MidiExport::processTrack(Track *track, uint8_t &channelID, bool isBB)
{
	// Cast track as a instrument one and save info from it to element
	InstrumentTrack *instTrack = dynamic_cast<InstrumentTrack *>(track);
	QDomElement root = instTrack->saveState(m_dataFile, m_dataFile.content());

	// Create track with incremental id
	MidiFile::Track &midiTrack = m_file->m_tracks[channelID++];

	// Add info about tempo and track name
	midiTrack.addTempo(m_tempo, 0);
	midiTrack.addName(track->name().toStdString(), 0);

	// If the current track is a Sf2 Player one, set the current
	// patch for to the exporting track. Note that this only works
	// decently if the current bank is a GM 1~128 one (which would be
	// needed as the default either way for successful import).
	uint8_t patch = 0;
	QString instName = instTrack->instrumentName();
	if (instName == "Sf2 Player")
	{
		class Instrument *inst = instTrack->instrument();
		patch = inst->childModel("patch")->value<uint8_t>();
	}
	midiTrack.addProgramChange(patch, 0);

	// ---- Instrument track ---- //
	QDomNode trackNode = root.firstChildElement("instrumenttrack");
	QDomElement trackElem = trackNode.toElement();
	// Transpose +12 semitones (workaround for #1857)
	int basePitch = trackElem.attribute("basenote", "57").toInt();
	basePitch = 69 - basePitch;
	// Adjust to masterPitch if enabled
	if (trackElem.attribute("usemasterpitch", "1").toInt())
	{
		basePitch += m_masterPitch;
	}
	// Volume ranges in [0.0, 2.0]
	double baseVolume = LocaleHelper::toDouble(
			trackElem.attribute("volume", "100")) / 100.0;

	// ---- Pattern ---- //
	QDomNode patternNode = root.firstChildElement("pattern");
	QDomElement patElem = patternNode.toElement();
	Pattern pat;
	if (not isBB)
	{
		// Base time == initial position
		int baseTime = patElem.attribute("pos", "0").toInt();

		// Write track notes to pattern
		pat.write(patternNode, basePitch, baseVolume, baseTime);

		// Write pattern info to MIDI file track
		pat.processBbNotes(INT_MAX);
		pat.writeToTrack(midiTrack);
	}
	else
	{
		// Write to-be repeated BB notes to pattern
		// (notice base time of 0)
		pat.write(patternNode, basePitch, baseVolume, 0);
	}

	// Write track data to buffer
	midiTrack.writeToBuffer();
}

void MidiExport::writeBbPattern(Pattern &pat, const QDomElement &patElem,
		uint8_t channelID, MidiFile::Track &midiTrack)
{
	// Workaround for nested BBTCOs
	int pos = 0;
	int len = 12 * patElem.attribute("steps", "1").toInt();

	// Iterate through BBTCO pairs of current list
	// TODO: This *may* need some corrections?
	const vector<pair<int,int>> &plist = m_plists[channelID - 1];
	stack<pair<int, int>> st;
	Pattern bbPat;
	for (const pair<int, int> &p : plist)
	{
		while (not st.empty() and st.top().second <= p.first)
		{
			pat.writeToBb(bbPat, len, st.top().first, pos, st.top().second);
			pos = st.top().second;
			st.pop();
		}
		if (not st.empty() and st.top().second <= p.second)
		{
			pat.writeToBb(bbPat, len, st.top().first, pos, p.first);
			pos = p.first;
			while (not st.empty() and st.top().second <= p.second)
			{
				st.pop();
			}
		}
		st.push(p);
		pos = p.first;
	}
	while (not st.empty())
	{
		pat.writeToBb(bbPat, len, st.top().first, pos, st.top().second);
		pos = st.top().second;
		st.pop();
	}
	// Write pattern info to MIDI file track
	bbPat.processBbNotes(pos);
	bbPat.writeToTrack(midiTrack);
}

void MidiExport::processBbTrack(Track *track)
{
	// Cast track as a BB one and save info from it to element
	BBTrack *bbTrack = dynamic_cast<BBTrack *>(track);
	QDomElement root = bbTrack->saveState(m_dataFile, m_dataFile.content());

	// Build lists of (start, end) pairs from BB note objects
	vector<pair<int,int>> plist;
	for (QDomNode bbtcoNode = root.firstChildElement("bbtco");
			not bbtcoNode.isNull();
			bbtcoNode = bbtcoNode.nextSiblingElement("bbtco"))
	{
		QDomElement bbtcoElem = bbtcoNode.toElement();
		int start = bbtcoElem.attribute("pos", "0").toInt();
		int end = start + bbtcoElem.attribute("len", "0").toInt();
		plist.push_back(pair<int,int>(start, end));
	}
	// Sort list in ascending order and append it to matrix
	sort(plist.begin(), plist.end());
	m_plists.push_back(plist);
}

/*---------------------------------------------------------------------------*/

extern "C"
{

//! Necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main(Model *, void * _data)
{
	return new MidiExport();
}

} // extern "C"
