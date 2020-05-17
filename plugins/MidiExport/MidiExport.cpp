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

#include <stack>

#include "MidiExport.h"

#include "BBTrack.h"
#include "InstrumentTrack.h"
#include "LocaleHelper.h"
#include "plugin_export.h"

using std::pair;
using std::vector;
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

// Constructor and destructor
MidiExport::MidiExport() :
		ExportFilter(&midiexport_plugin_descriptor) {}
MidiExport::~MidiExport() {}

/*---------------------------------------------------------------------------*/

//! Represents a pattern of notes
struct MidiExport::Pattern
{
	//! Vector of actual notes
	MidiNoteVector notes;

	//! Append notes from root node to pattern
	void write(const QDomNode root,
			int basePitch, double baseVolume, int baseTime)
	{
		// TODO interpret steps="12" muted="0" type="1" name="Piano1" len="259"
		for (QDomNode node = root.firstChild(); not node.isNull();
				node = root.nextSibling())
		{
			QDomElement note = node.toElement();

			// Ignore zero-length notes
			if (note.attribute("len", "0") == "0") continue;

			// Adjust note attributes based on base measures
			MidiNote mnote;
			int pitch = note.attribute("key", "0").toInt() + basePitch;
			mnote.pitch = qBound(0, pitch, 127);
			double volume =
					LocaleHelper::toDouble(note.attribute("vol", "100"));
			volume *= baseVolume * (127.0 / 200.0);
			mnote.volume = qMin(qRound(volume), 127);
			mnote.time = baseTime + note.attribute("pos", "0").toInt();
			mnote.duration = note.attribute("len", "0").toInt();

			// Append note to vector
			notes.push_back(mnote);
		}
	}

	//! Add pattern notes to MIDI file track
	void writeToTrack(MTrack &mtrack) const
	{
		for (const MidiNote &n : notes) {
			mtrack.addNote(n.pitch, n.volume,
					n.time / 48.0, n.duration / 48.0);
		}
	}

	//! Adjust special duration BB notes by resizing them accordingly
	void processBBNotes(int cutPos)
	{
		// Sort in reverse order
		sort(notes.rbegin(), notes.rend());

		int cur = INT_MAX, next = INT_MAX;
		for (MidiNote &n : notes)
		{
			if (n.time < cur)
			{
				// Set last two notes positions
				next = cur;
				cur = n.time;
			}
			if (n.duration < 0)
			{
				// Note should have positive duration that neither
				// overlaps next one nor exceeds cutPos
				n.duration = qMin(-n.duration, next - cur);
				n.duration = qMin(n.duration, cutPos - n.time);
			}
		}
	}

	//! Write sorted notes to a BB explictly repeating pattern
	void writeToBB(Pattern &bbPat, int len, int base, int start, int end)
	{
		// Avoid misplaced start and end positions
		if (start >= end) return;

		// Adjust positions relatively to base pos
		start -= base;
		end -= base;

		sort(notes.begin(), notes.end());
		for (MidiNote note : notes)
		{
			// TODO
			int t0 = note.time + ceil((start - note.time) / len) * len;
			for (int time = t0;	time < end; time += len)
			{
				note.time = base + time;
				bbPat.notes.push_back(note);
			}
		}
	}
};

/*---------------------------------------------------------------------------*/

//! Export a list of normal tracks and a list of BB ones, with
//  global tempo and master pitch, to a MIDI file indicated by <filename>
bool MidiExport::tryExport(const TrackContainer::TrackList &tracks,
			const TrackContainer::TrackList &tracksBB,
			int tempo, int masterPitch, const QString &filename)
{
	// Open designated blank MIDI file (and data stream) for writing
	QFile f(filename);
	f.open(QIODevice::WriteOnly);
	QDataStream midiout(&f);

	// Count total number of tracks
	int nTracks = 0;
	for (const Track *track : tracks)
	{
		if (track->type() == Track::InstrumentTrack) nTracks++;
	}
	for (const Track *track : tracksBB)
	{
		if (track->type() == Track::InstrumentTrack) nTracks++;
	}

	// Write MIDI header data to stream
	MidiFile::MIDIHeader header(nTracks);
	uint8_t buffer[BUFFER_SIZE];
	size_t size = header.writeToBuffer(buffer);
	midiout.writeRawData((char *) buffer, size);

	// Matrix containing (start, end) pairs for BB objects
	vector<vector<pair<int, int>>> plists;

	// Iterate through "normal" tracks
	QDomElement element;
	for (Track *track : tracks)
	{
		MTrack mtrack;
		DataFile dataFile(DataFile::SongProject);

		if (track->type() == Track::InstrumentTrack)
		{
			// Firstly, add info about tempo and track name (at time 0)
			mtrack.addName(track->name().toStdString(), 0);
			// mtrack.addProgramChange(0, 0);
			mtrack.addTempo(tempo, 0);

			// Cast track as a instrument one and save info from it to element
			InstrumentTrack *instTrack =
					dynamic_cast<InstrumentTrack *>(track);
			element = instTrack->saveState(dataFile, dataFile.content());

			// Get track info and then update pattern
			int basePitch;
			double baseVolume;
			Pattern pat;
			for (QDomNode node = element.firstChild(); not node.isNull();
					node = node.nextSibling())
			{
				QDomElement e = node.toElement();

				if (node.nodeName() == "instrumenttrack")
				{
					// Transpose +12 semitones (workaround for #1857).
					// Adjust to masterPitch if enabled
					basePitch = e.attribute("basenote", "57").toInt();
					basePitch = 69 - basePitch;
					if (e.attribute("usemasterpitch", "1").toInt())
					{
						basePitch += masterPitch;
					}
					// Volume ranges in [0, 2]
					baseVolume = LocaleHelper::toDouble(
							e.attribute("volume", "100"))/100.0;
				}
				else if (node.nodeName() == "pattern")
				{
					// Base time == initial position
					int baseTime = e.attribute("pos", "0").toInt();

					// Write track notes to pattern
					pat.write(node, basePitch, baseVolume, baseTime);
				}

			}
			// Write pattern info to MIDI file track, and then to stream
			pat.processBBNotes(INT_MAX);
			pat.writeToTrack(mtrack);
			size = mtrack.writeToBuffer(buffer);
			midiout.writeRawData((char *) buffer, size);
		}

		else if (track->type() == Track::BBTrack)
		{
			// Cast track as a BB one and save info from it to element
			BBTrack *bbTrack = dynamic_cast<BBTrack *>(track);
			element = bbTrack->saveState(dataFile, dataFile.content());

			// Build lists of (start, end) pairs from BB note objects
			vector<pair<int,int>> plist;
			for (QDomNode node = element.firstChild(); not node.isNull();
					node = node.nextSibling())
			{
				if (node.nodeName() == "bbtco")
				{
					QDomElement e = node.toElement();
					int start = e.attribute("pos", "0").toInt();
					int end = start + e.attribute("len", "0").toInt();
					plist.push_back(pair<int,int>(start, end));
				}
			}
			// Sort list in ascending order and append it to matrix
			sort(plist.begin(), plist.end());
			plists.push_back(plist);
		}
	}

	// Iterate through BB tracks
	for (Track* track : tracksBB)
	{
		MTrack mtrack;
		DataFile dataFile(DataFile::SongProject);

		// Cast track as a instrument one and save info from it to element
		if (track->type() != Track::InstrumentTrack) continue;
		InstrumentTrack *instTrack = dynamic_cast<InstrumentTrack *>(track);
		element = instTrack->saveState(dataFile, dataFile.content());

		// Add usual info to start of track
		mtrack.addName(track->name().toStdString(), 0);
		// mtrack.addProgramChange(0, 0);
		mtrack.addTempo(tempo, 0);

		// Get track info and then update pattern(s)
		int basePitch;
		double baseVolume;
		int i = 0;
		for (QDomNode node = element.firstChild(); not node.isNull();
				node = node.nextSibling())
		{
			QDomElement e = node.toElement();

			if (node.nodeName() == "instrumenttrack")
			{
				// Transpose +12 semitones (workaround for #1857).
				// Adjust to masterPitch if enabled
				basePitch = e.attribute("basenote", "57").toInt();
				basePitch = 69 - basePitch;
				if (e.attribute("usemasterpitch", "1").toInt())
				{
					basePitch += masterPitch;
				}
				// Volume ranges in [0, 2]
				baseVolume = LocaleHelper::toDouble(
						e.attribute("volume", "100"))/100.0;
			}
			else if (node.nodeName() == "pattern")
			{
				// Write to-be repeated BB notes to pattern
				// (notice base time of 0)
				Pattern pat, bbPat;
				pat.write(node, basePitch, baseVolume, 0);

				// Workaround for nested BBTCOs
				int pos = 0;
				int len = 12 * e.attribute("steps", "1").toInt();

				// Iterate through BBTCO pairs of current list
				// TODO: Rewrite or completely refactor this
				vector<pair<int,int>> &plist = plists[i++];
				stack<pair<int, int>> st;
				for (pair<int, int> &p : plist)
				{
					while (not st.empty() and st.top().second <= p.first)
					{
						pat.writeToBB(bbPat,
								len, st.top().first, pos, st.top().second);
						pos = st.top().second;
						st.pop();
					}
					if (not st.empty() and st.top().second <= p.second)
					{
						pat.writeToBB(bbPat,
								len, st.top().first, pos, p.first);
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
					pat.writeToBB(bbPat,
							len, st.top().first, pos, st.top().second);
					pos = st.top().second;
					st.pop();
				}
				// Write pattern info to MIDI file track
				bbPat.processBBNotes(pos);
				bbPat.writeToTrack(mtrack);

				// Increment matrix line index
				i++;
			}
		}
		// Write track data to stream
		size = mtrack.writeToBuffer(buffer);
		midiout.writeRawData((char *) buffer, size);
	}

	// Always returns success... for now?
	return true;
}

/*---------------------------------------------------------------------------*/

extern "C"
{

// Necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main(Model *, void * _data)
{
	return new MidiExport();
}

} // extern "C"
