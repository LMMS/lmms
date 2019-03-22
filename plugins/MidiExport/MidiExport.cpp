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

extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT midiexport_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"MIDI Export",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"Filter for exporting MIDI-files from LMMS" ),
	"Mohamed Abdel Maksoud <mohamed at amaksoud.com> and "
	"Hyunjin Song <tteu.ingog/at/gmail.com>",
	0x0100,
	Plugin::ExportFilter,
	NULL,
	NULL,
	NULL
} ;

}


MidiExport::MidiExport() : ExportFilter( &midiexport_plugin_descriptor)
{
}




MidiExport::~MidiExport()
{
}



bool MidiExport::tryExport(const TrackContainer::TrackList &tracks,
			const TrackContainer::TrackList &tracks_BB,
			int tempo, int masterPitch, const QString &filename)
{
	QFile f(filename);
	f.open(QIODevice::WriteOnly);
	QDataStream midiout(&f);

	InstrumentTrack* instTrack;
	BBTrack* bbTrack;
	QDomElement element;


	int nTracks = 0;
	uint8_t buffer[BUFFER_SIZE];
	uint32_t size;

	for (const Track* track : tracks) if (track->type() == Track::InstrumentTrack) nTracks++;
	for (const Track* track : tracks_BB) if (track->type() == Track::InstrumentTrack) nTracks++;

	// midi header
	MidiFile::MIDIHeader header(nTracks);
	size = header.writeToBuffer(buffer);
	midiout.writeRawData((char *)buffer, size);

	std::vector<std::vector<std::pair<int,int>>> plists;

	// midi tracks
	for (Track* track : tracks)
	{
		DataFile dataFile(DataFile::SongProject);
		MTrack mtrack;

		if (track->type() == Track::InstrumentTrack)
		{

			mtrack.addName(track->name().toStdString(), 0);
			//mtrack.addProgramChange(0, 0);
			mtrack.addTempo(tempo, 0);

			instTrack = dynamic_cast<InstrumentTrack *>(track);
			element = instTrack->saveState(dataFile, dataFile.content());

			int base_pitch = 0;
			double base_volume = 1.0;
			int base_time = 0;

			MidiNoteVector pat;

			for (QDomNode n = element.firstChild(); !n.isNull(); n = n.nextSibling())
			{

				if (n.nodeName() == "instrumenttrack")
				{
					QDomElement it = n.toElement();
					// transpose +12 semitones, workaround for #1857
					base_pitch = (69 - it.attribute("basenote", "57").toInt());
					if (it.attribute("usemasterpitch", "1").toInt())
					{
						base_pitch += masterPitch;
					}
					base_volume = LocaleHelper::toDouble(it.attribute("volume", "100"))/100.0;
				}

				if (n.nodeName() == "pattern")
				{
					base_time = n.toElement().attribute("pos", "0").toInt();
					writePattern(pat, n, base_pitch, base_volume, base_time);
				}

			}
			ProcessBBNotes(pat, INT_MAX);
			writePatternToTrack(mtrack, pat);
			size = mtrack.writeToBuffer(buffer);
			midiout.writeRawData((char *)buffer, size);
		}

		if (track->type() == Track::BBTrack)
		{
			bbTrack = dynamic_cast<BBTrack *>(track);
			element = bbTrack->saveState(dataFile, dataFile.content());

			std::vector<std::pair<int,int>> plist;
			for (QDomNode n = element.firstChild(); !n.isNull(); n = n.nextSibling())
			{

				if (n.nodeName() == "bbtco")
				{
					QDomElement it = n.toElement();
					int pos = it.attribute("pos", "0").toInt();
					int len = it.attribute("len", "0").toInt();
					plist.push_back(std::pair<int,int>(pos, pos+len));
				}
			}
			std::sort(plist.begin(), plist.end());
			plists.push_back(plist);

		}
	} // for each track

	// midi tracks in BB tracks
	for (Track* track : tracks_BB)
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

		for (QDomNode n = element.firstChild(); !n.isNull(); n = n.nextSibling())
		{
			if (n.nodeName() == "instrumenttrack")
			{
				QDomElement it = n.toElement();
				// transpose +12 semitones, workaround for #1857
				base_pitch = (69 - it.attribute("basenote", "57").toInt());
				if (it.attribute("usemasterpitch", "1").toInt())
				{
					base_pitch += masterPitch;
				}
				base_volume = LocaleHelper::toDouble(it.attribute("volume", "100")) / 100.0;
			}

			if (n.nodeName() == "pattern")
			{
				std::vector<std::pair<int,int>> &plist = *itr;

				MidiNoteVector nv, pat;
				writePattern(pat, n, base_pitch, base_volume, 0);

				// workaround for nested BBTCOs
				int pos = 0;
				int len = n.toElement().attribute("steps", "1").toInt() * 12;
				for (auto it = plist.begin(); it != plist.end(); ++it)
				{
					while (!st.empty() && st.back().second <= it->first)
					{
						writeBBPattern(pat, nv, len, st.back().first, pos, st.back().second);
						pos = st.back().second;
						st.pop_back();
					}

					if (!st.empty() && st.back().second <= it->second)
					{
						writeBBPattern(pat, nv, len, st.back().first, pos, it->first);
						pos = it->first;
						while (!st.empty() && st.back().second <= it->second)
						{
							st.pop_back();
						}
					}

					st.push_back(*it);
					pos = it->first;
				}

				while (!st.empty())
				{
					writeBBPattern(pat, nv, len, st.back().first, pos, st.back().second);
					pos = st.back().second;
					st.pop_back();
				}

				ProcessBBNotes(nv, pos);
				writePatternToTrack(mtrack, nv);
				++itr;
			}
		}
		size = mtrack.writeToBuffer(buffer);
		midiout.writeRawData((char *)buffer, size);
	}

	return true;

}



void MidiExport::writePattern(MidiNoteVector &pat, QDomNode n,
				int base_pitch, double base_volume, int base_time)
{
	// TODO interpret steps="12" muted="0" type="1" name="Piano1"  len="2592"
	for (QDomNode nn = n.firstChild(); !nn.isNull(); nn = nn.nextSibling())
	{
		QDomElement note = nn.toElement();
		if (note.attribute("len", "0") == "0") continue;
		// TODO interpret pan="0" fxch="0" pitchrange="1"
		MidiNote mnote;
		mnote.pitch = qMax(0, qMin(127, note.attribute("key", "0").toInt() + base_pitch));
		 // Map from LMMS volume to MIDI velocity
		mnote.volume = qMin(qRound(base_volume * LocaleHelper::toDouble(note.attribute("vol", "100")) * (127.0 / 200.0)), 127);
		mnote.time = base_time + note.attribute("pos", "0").toInt();
		mnote.duration = note.attribute("len", "0").toInt();
		pat.push_back(mnote);
	}
}



void MidiExport::writePatternToTrack(MTrack &mtrack, MidiNoteVector &nv)
{
	for (auto it = nv.begin(); it != nv.end(); ++it)
	{
		mtrack.addNote(it->pitch, it->volume, it->time / 48.0, it->duration / 48.0);
	}
}



void MidiExport::writeBBPattern(MidiNoteVector &src, MidiNoteVector &dst,
				int len, int base, int start, int end)
{
	if (start >= end) { return; }
	start -= base;
	end -= base;
	std::sort(src.begin(), src.end());
	for (auto it = src.begin(); it != src.end(); ++it)
	{
		for (int time = it->time  + ceil((start - it->time) / len)
				* len; time < end; time += len)
		{
			MidiNote note;
			note.duration = it->duration;
			note.pitch = it->pitch;
			note.time = base + time;
			note.volume = it->volume;
			dst.push_back(note);
		}
	}
}



void MidiExport::ProcessBBNotes(MidiNoteVector &nv, int cutPos)
{
	std::sort(nv.begin(), nv.end());
	int cur = INT_MAX, next = INT_MAX;
	for (auto it = nv.rbegin(); it != nv.rend(); ++it)
	{
		if (it->time < cur)
		{
			next = cur;
			cur = it->time;
		}
		if (it->duration < 0)
		{
			it->duration = qMin(qMin(-it->duration, next - cur), cutPos - it->time);
		}
	}
}



void MidiExport::error()
{
	//qDebug() << "MidiExport error: " << m_error ;
}



extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model *, void * _data )
{
	return new MidiExport();
}


}

