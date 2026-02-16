/*
 * MidiExport.cpp - support for Exporting MIDI files
 *
 * Copyright (c) 2015 Mohamed Abdel Maksoud <mohamed at amaksoud.com>
 * Copyright (c) 2017 Hyunjin Song <tteu.ingog/at/gmail.com>
 * Copyright (c) 2020 EmoonX
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

#include <algorithm>
#include <stack>

#include "Instrument.h"
#include "DataFile.h"
#include "InstrumentTrack.h"
#include "LocaleHelper.h"
#include "PatternTrack.h"
#include "plugin_export.h"

namespace lmms
{

extern "C"
{

//! Standardized plugin descriptor for MIDI exporter
Plugin::Descriptor PLUGIN_EXPORT midiexport_plugin_descriptor =
{
	LMMS_STRINGIFY(PLUGIN_NAME),
	"MIDI Export",
	QT_TRANSLATE_NOOP("PluginBrowser",
		"Filter for exporting MIDI-files from LMMS"),
	"Mohamed Abdel Maksoud <mohamed at amaksoud.com> and "
		"Hyunjin Song <tteu.ingog/at/gmail.com>",
	0x0100,
	Plugin::Type::ExportFilter,
	nullptr,
	nullptr,
	nullptr,
};

PLUGIN_EXPORT Plugin* lmms_plugin_main(Model*, void*)
{
	return new MidiExport{};
}

} // extern "C"

void MidiExport::Clip::write(const QDomNode& root,
	int basePitch, double baseVolume, int baseTime)
{
	// TODO: interpret steps="12" muted="0" type="1" name="Piano1" len="259"
	for (QDomNode node = root.firstChild(); !node.isNull();
		node = node.nextSibling())
	{
		QDomElement element = node.toElement();

		// Ignore zero-length notes
		if (element.attribute("len", "0") == "0") { continue; }

		// Adjust note attributes based on base measures
		const int pitch = basePitch + element.attribute("key", "0").toInt();
		double volume = LocaleHelper::toDouble(element.attribute("vol", "100"));
		volume *= baseVolume * (127.0 / 200.0);

		auto note = Note {
			.pitch = std::clamp<std::uint8_t>(pitch, 0, 127),
			.volume = std::min<std::uint8_t>(std::lround(volume), 127),
			.time = baseTime + element.attribute("pos", "0").toInt(),
			.duration = element.attribute("len", "0").toInt(),
			.type = static_cast<Note::Type>(element.attribute("type", "0").toInt())
		};

		// Append note to vector
		m_notes.push_back(note);
	}
}

void MidiExport::Clip::writeToTrack(MidiFile::Track& mTrack) const
{
	for (const Note& note : m_notes)
	{
		mTrack.addNote(note.pitch, note.volume,
			note.time / 48.0, note.duration / 48.0);
	}
}

void MidiExport::Clip::processPatternNotes(int cutPos)
{
	// Sort in reverse order
	std::sort(m_notes.rbegin(), m_notes.rend());

	int cur = INT_MAX;
	int next = INT_MAX;
	for (Note& note : m_notes)
	{
		if (note.time < cur)
		{
			// Set last two notes positions
			next = cur;
			cur = note.time;
		}
		if (note.type == Note::Type::Step)
		{
			// Note should have positive duration that neither
			// overlaps next one nor exceeds cutPos
			note.duration = qMin(qMin(DefaultBeatLength, next - cur), cutPos - note.time);
		}
	}
}

void MidiExport::Clip::writeToPattern(Clip& patternClip,
	int len, int base, int start, int end)
{
	// Avoid misplaced start and end positions
	if (start >= end) { return; }

	// Adjust positions relatively to base pos
	start -= base;
	end -= base;

	std::sort(m_notes.begin(), m_notes.end());
	for (Note note : m_notes)
	{
		// Insert periodically repeating notes from <t0> and spaced
		// by <len> to mimic pattern clip behavior
		int t0 = note.time + ceil((start - note.time) / len) * len;
		for (int time = t0;	time < end; time += len)
		{
			note.time = base + time;
			patternClip.m_notes.push_back(note);
		}
	}
}

MidiExport::MidiExport()
	: ExportFilter(&midiexport_plugin_descriptor)
{}

bool MidiExport::tryExport(const TrackContainer::TrackList& tracks,
	const TrackContainer::TrackList& patternStoreTracks,
	int tempo, int masterPitch, const QString& filename)
{
	// Count number of instrument (and PatternStore) tracks
	const auto numTracks = std::ranges::count_if(tracks, [](auto t) {
		return t->type() == Track::Type::Instrument;
	}) + patternStoreTracks.size();

	// Write header info
	auto file = MidiFile(filename, numTracks);
	file.m_header.writeToBuffer();

	// Iterate through "normal" tracks
	std::size_t trackIdx = 0;
	for (Track* track : tracks)
	{
		assert(track != nullptr);
		if (track->type() == Track::Type::Instrument)
		{
			processTrack(*track, file.m_tracks[trackIdx++], tempo, masterPitch);
		}
		else if (track->type() == Track::Type::Pattern)
		{
			processPatternTrack(*track);
		}
	}

	// Iterate through PatternStore tracks
	for (Track* track : patternStoreTracks)
	{
		assert(track != nullptr);
		processTrack(*track, file.m_tracks[trackIdx++], tempo, masterPitch, true);
	}

	// Write all buffered data to stream
	file.writeAllToStream();

	// Always returns success... for now?
	return true;
}

void MidiExport::processTrack(Track& track, MidiFile::Track& midiTrack,
	int tempo, int masterPitch, bool isPattern)
{
	// Cast track as a instrument one and save info from it to element
	InstrumentTrack& instTrack = dynamic_cast<InstrumentTrack&>(track);
	QDomElement root = instTrack.saveState(m_dataFile, m_dataFile.content());

	// Set the channel of the next MIDI file track object
	if (m_channel == 9) { ++m_channel; }
	midiTrack.channel = m_channel++;

	// Add info about tempo and track name
	midiTrack.addTempo(tempo, 0);
	midiTrack.addName(track.name().toStdString(), 0);

	// If the current track is a Sf2 Player one, set the current
	// patch to the exporting track. Note that this only works
	// decently if the current bank is a GM 1~128 one (which would be
	// needed as the default either way for successful import).
	// Pattern tracks are always bank 128 (see MidiImport), patch 0.
	std::uint8_t patch = 0;
	QString instName = instTrack.instrumentName();
	if (instName == "Sf2 Player")
	{
		Instrument* inst = instTrack.instrument();
		const auto bank = inst->childModel("bank")->value<std::uint8_t>();
		if (bank == 128)
		{
			// Drum Sf2 track, so set its channel to 10
			// (and reverse counter increment)
			midiTrack.channel = 9;
			m_channel--;
		}
		else { patch = inst->childModel("patch")->value<std::uint8_t>(); }
	}
	midiTrack.addProgramChange(patch, 0);

	// ---- Instrument track ---- //
	QDomNode trackNode = root.firstChildElement("instrumenttrack");
	QDomElement trackElem = trackNode.toElement();
	int basePitch = 69 - trackElem.attribute("basenote", "69").toInt();
	// Adjust to masterPitch if enabled
	if (trackElem.attribute("usemasterpitch", "1").toInt())
	{
		basePitch += masterPitch;
	}
	// Volume ranges in [0.0, 2.0]
	double baseVolume = LocaleHelper::toDouble(
		trackElem.attribute("volume", "100")) / 100.0;

	// ---- Clips ---- //
	std::uint8_t patternId = 0;
	for (QDomNode clipNode = root.firstChildElement("midiclip");
		!clipNode.isNull();
		clipNode = clipNode.nextSiblingElement("midiclip"))
	{
		QDomElement clipElem = clipNode.toElement();
		Clip clip;
		if (!isPattern)
		{
			// Base time == initial position
			int baseTime = clipElem.attribute("pos", "0").toInt();

			// Write track notes to clip
			clip.write(clipNode, basePitch, baseVolume, baseTime);

			// Write clip info to MIDI file track
			clip.processPatternNotes(INT_MAX);
			clip.writeToTrack(midiTrack);
		}
		else
		{
			// Write to-be repeated pattern clip notes to clip
			// (notice base time of 0)
			clip.write(clipNode, basePitch, baseVolume, 0);

			// Write clip to track
			writePatternClip(clip, clipElem, patternId++, midiTrack);
		}
	}

	// Write track data to buffer
	midiTrack.writeToBuffer();
}

void MidiExport::writePatternClip(Clip& clip, const QDomElement& clipElem,
	std::uint8_t patternIdx, MidiFile::Track& midiTrack)
{
	// Workaround for nested PatternClips
	int pos = 0;
	int len = 12 * clipElem.attribute("steps", "1").toInt();

	// Iterate through PatternClip pairs of current list
	// TODO: This *may* need some corrections?
	const std::vector<std::pair<int,int>>& plist = m_plists[patternIdx];
	std::stack<std::pair<int, int>> st;
	Clip patternClip;
	for (const std::pair<int, int>& p : plist)
	{
		while (!st.empty() && st.top().second <= p.first)
		{
			clip.writeToPattern(patternClip, len, st.top().first, pos, st.top().second);
			pos = st.top().second;
			st.pop();
		}
		if (!st.empty() && st.top().second <= p.second)
		{
			clip.writeToPattern(patternClip, len, st.top().first, pos, p.first);
			pos = p.first;
			while (!st.empty() && st.top().second <= p.second)
			{
				st.pop();
			}
		}
		st.push(p);
		pos = p.first;
	}
	while (!st.empty())
	{
		clip.writeToPattern(patternClip, len, st.top().first, pos, st.top().second);
		pos = st.top().second;
		st.pop();
	}
	// Write clip info to MIDI file track
	patternClip.processPatternNotes(pos);
	patternClip.writeToTrack(midiTrack);
}

void MidiExport::processPatternTrack(Track& track)
{
	// Cast track as a pattern track and save info from it to element
	PatternTrack& patternTrack = dynamic_cast<PatternTrack&>(track);
	QDomElement root = patternTrack.saveState(m_dataFile, m_dataFile.content());

	// Build lists of (start, end) pairs from pattern clip note objects
	std::vector<std::pair<int, int>> plist;
	for (QDomNode patternClipNode = root.firstChildElement("patternclip");
		!patternClipNode.isNull();
		patternClipNode = patternClipNode.nextSiblingElement("patternclip"))
	{
		QDomElement patternClipElem = patternClipNode.toElement();
		int start = patternClipElem.attribute("pos", "0").toInt();
		int end = start + patternClipElem.attribute("len", "0").toInt();
		plist.emplace_back(start, end);
	}

	// Sort list in ascending order and append it to matrix
	std::sort(plist.begin(), plist.end());
	m_plists.emplace_back(std::move(plist));
}

} // namespace lmms
