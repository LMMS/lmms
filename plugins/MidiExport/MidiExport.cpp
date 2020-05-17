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

#include <QDomDocument>
#include <QDir>
#include <QApplication>
#include <QMessageBox>
#include <QProgressDialog>

#include "MidiExport.h"

#include "lmms_math.h"
#include "TrackContainer.h"
#include "BBTrack.h"
#include "InstrumentTrack.h"
#include "LocaleHelper.h"
#include "plugin_export.h"

using std::sort;

extern "C"
{

//! Standardized plugin descriptor for MIDI exporter
Plugin::Descriptor PLUGIN_EXPORT midiexport_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
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

MidiExport::MidiExport() : ExportFilter( &midiexport_plugin_descriptor) {}
MidiExport::~MidiExport() {}

/*---------------------------------------------------------------------------*/

struct MidiExport::Pattern
{
	MidiNoteVector notes;

	void write(QDomNode element,
			int base_pitch, double base_volume, int base_time)
	{
		// TODO interpret steps="12" muted="0" type="1" name="Piano1"  len="2592"
		for (QDomNode node = element.firstChild(); !node.isNull();
				node = node.nextSibling())
		{
			QDomElement note = node.toElement();
			if (note.attribute("len", "0") == "0") continue;
			// TODO interpret pan="0" fxch="0" pitchrange="1"

			MidiNote mnote;
			int pitch = note.attribute("key", "0").toInt() + base_pitch;
			mnote.pitch = qBound(0, pitch, 127);
			 // Map from LMMS volume to MIDI velocity
			double volume = LocaleHelper::toDouble(note.attribute("vol", "100"));
			volume *= base_volume * (127.0 / 200.0);
			mnote.volume = qMin(qRound(volume), 127);
			mnote.time = base_time + note.attribute("pos", "0").toInt();
			mnote.duration = note.attribute("len", "0").toInt();

			notes.push_back(mnote);
		}
	}

	void writeToTrack(MTrack &mtrack)
	{
		for (MidiNote &n : notes) {
			mtrack.addNote(n.pitch, n.volume,
					n.time / 48.0, n.duration / 48.0);
		}
	}

	//! Adjust negative duration BB notes by resizing them positively
	void processBBNotes(int cutPos)
	{
		// Sort in reverse order
		sort(notes.rbegin(), notes.rend());

		int cur = INT_MAX, next = INT_MAX;
		for (MidiNote &n : notes)
		{
			if (n.time < cur)
			{
				next = cur;
				cur = n.time;
			}
			if (n.duration < 0)
			{
				n.duration = qMin(-n.duration, next - cur);
				n.duration = qMin(n.duration, cutPos - n.time);
			}
		}
	}
};

/*---------------------------------------------------------------------------*/

bool MidiExport::tryExport(const TrackContainer::TrackList &tracks,
			const TrackContainer::TrackList &tracksBB,
			int tempo, int masterPitch, const QString &filename)
{
	// Open .mid file, from where to be exported, in write mode
	QFile f(filename);
	f.open(QIODevice::WriteOnly);
	QDataStream midiout(&f);

	InstrumentTrack* instTrack;
	BBTrack* bbTrack;
	QDomElement element;

	uint8_t buffer[BUFFER_SIZE];

	// Count total number of tracks
	uint8_t nTracks = 0;
	for (const Track *track : tracks)
	{
		if (track->type() == Track::InstrumentTrack) nTracks++;
	}
	for (const Track *track : tracksBB)
	{
		if (track->type() == Track::InstrumentTrack) nTracks++;
	}

	// Write MIDI header data to file
	MidiFile::MIDIHeader header(nTracks);
	size_t size = header.writeToBuffer(buffer);
	midiout.writeRawData((char *)buffer, size);

	std::vector<std::vector<std::pair<int,int>>> plists;

	// Iterate through "normal" tracks
	for (Track *track : tracks)
	{
		DataFile dataFile(DataFile::SongProject);
		MTrack mtrack;

		if (track->type() == Track::InstrumentTrack)
		{
			// Firstly, add info about tempo and track name (at time 0)
			mtrack.addName(track->name().toStdString(), 0);
			//mtrack.addProgramChange(0, 0);
			mtrack.addTempo(tempo, 0);

			instTrack = dynamic_cast<InstrumentTrack *>(track);
			element = instTrack->saveState(dataFile, dataFile.content());

			int base_pitch = 0;
			double base_volume = 1.0;
			int base_time = 0;

			Pattern pat;

			for (QDomNode node = element.firstChild(); not node.isNull();
					node = node.nextSibling())
			{
				if (node.nodeName() == "instrumenttrack")
				{
					QDomElement it = node.toElement();
					// transpose +12 semitones, workaround for #1857
					base_pitch = (69 - it.attribute("basenote", "57").toInt());
					if (it.attribute("usemasterpitch", "1").toInt())
					{
						base_pitch += masterPitch;
					}
					base_volume = LocaleHelper::toDouble(
							it.attribute("volume", "100"))/100.0;
				}
				else if (node.nodeName() == "pattern")
				{
					base_time = node.toElement().attribute("pos", "0").toInt();
					pat.write(node, base_pitch, base_volume, base_time);
				}

			}
			pat.processBBNotes(INT_MAX);
			pat.writeToTrack(mtrack);
			size = mtrack.writeToBuffer(buffer);
			midiout.writeRawData((char *) buffer, size);
		}

		else if (track->type() == Track::BBTrack)
		{
			bbTrack = dynamic_cast<BBTrack *>(track);
			element = bbTrack->saveState(dataFile, dataFile.content());

			std::vector<std::pair<int,int>> plist;
			for (QDomNode node = element.firstChild(); not node.isNull();
					node = node.nextSibling())
			{
				if (node.nodeName() == "bbtco")
				{
					QDomElement it = node.toElement();
					int pos = it.attribute("pos", "0").toInt();
					int len = it.attribute("len", "0").toInt();
					plist.push_back(std::pair<int,int>(pos, pos+len));
				}
			}
			std::sort(plist.begin(), plist.end());
			plists.push_back(plist);
		}
	}

	// Iterate through BB tracks
	for (Track* track : tracksBB)
	{
		DataFile dataFile(DataFile::SongProject);
		MTrack mtrack;

		auto itr = plists.begin();
		std::vector<std::pair<int,int>> st;

		if (track->type() != Track::InstrumentTrack) continue;

		mtrack.addName(track->name().toStdString(), 0);
		//mtrack.addProgramChange(0, 0);
		mtrack.addTempo(tempo, 0);

		instTrack = dynamic_cast<InstrumentTrack *>(track);
		element = instTrack->saveState(dataFile, dataFile.content());

		int base_pitch = 0;
		double base_volume = 1.0;

		for (QDomNode node = element.firstChild(); not node.isNull();
				node = node.nextSibling())
		{
			if (node.nodeName() == "instrumenttrack")
			{
				QDomElement it = node.toElement();
				// transpose +12 semitones, workaround for #1857
				base_pitch = (69 - it.attribute("basenote", "57").toInt());
				if (it.attribute("usemasterpitch", "1").toInt())
				{
					base_pitch += masterPitch;
				}
				base_volume = LocaleHelper::toDouble(
						it.attribute("volume", "100"))/100.0;
			}
			else if (node.nodeName() == "pattern")
			{
				std::vector<std::pair<int,int>> &plist = *itr;

				Pattern pat, bbPat;
				pat.write(node, base_pitch, base_volume, 0);

				// workaround for nested BBTCOs
				int pos = 0;
				int len = node.toElement().attribute("steps", "1").toInt();
				len *= 12;
				for (pair<int, int> &p : plist)
				{
					while (not st.empty() and st.back().second <= p.first)
					{
						writeBBPattern(pat, bbPat,
								len, st.back().first, pos, st.back().second);
						pos = st.back().second;
						st.pop_back();
					}
					if (not st.empty() and st.back().second <= p.second)
					{
						writeBBPattern(pat, bbPat,
								len, st.back().first, pos, p.first);
						pos = p.first;
						while (not st.empty() and st.back().second <= p.second)
						{
							st.pop_back();
						}
					}
					st.push_back(p);
					pos = p.first;
				}
				while (not st.empty())
				{
					writeBBPattern(pat, bbPat,
							len, st.back().first, pos, st.back().second);
					pos = st.back().second;
					st.pop_back();
				}
				bbPat.processBBNotes(pos);
				bbPat.writeToTrack(mtrack);
				++itr;
			}
		}
		size = mtrack.writeToBuffer(buffer);
		midiout.writeRawData((char *) buffer, size);
	}

	// Always returns true
	return true;
}

/*---------------------------------------------------------------------------*/

void MidiExport::writeBBPattern(Pattern &src, Pattern &dst,
				int len, int base, int start, int end)
{
	if (start >= end) { return; }
	start -= base;
	end -= base;
	std::sort(src.notes.begin(), src.notes.end());
	for (MidiNote note : src.notes)
	{
		for (int time = note.time + ceil((start - note.time) / len) * len;
				time < end; time += len)
		{
			note.time += base;
			dst.notes.push_back(note);
		}
	}
}

/*---------------------------------------------------------------------------*/

extern "C"
{

// Necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model *, void * _data )
{
	return new MidiExport();
}

}
