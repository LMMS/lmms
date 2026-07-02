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

#include "TrackContainer.h"
#include "DataFile.h"
#include "InstrumentTrack.h"
#include "LocaleHelper.h"
#include "PatternTrack.h"

#include "plugin_export.h"

namespace lmms
{


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT midiexport_plugin_descriptor =
{
	LMMS_STRINGIFY( PLUGIN_NAME ),
	"MIDI Export",
	QT_TRANSLATE_NOOP( "PluginBrowser",
				"Filter for exporting MIDI-files from LMMS" ),
	"Mohamed Abdel Maksoud <mohamed at amaksoud.com> and "
	"Hyunjin Song <tteu.ingog/at/gmail.com>",
	0x0100,
	Plugin::Type::ExportFilter,
	nullptr,
	nullptr,
	nullptr,
} ;

}


MidiExport::MidiExport() : ExportFilter( &midiexport_plugin_descriptor)
{
}




bool MidiExport::tryExport(const TrackContainer::TrackList &tracks,
			const TrackContainer::TrackList &patternStoreTracks,
			int tempo, int masterPitch, const QString &filename)
{
	QFile f(filename);
	f.open(QIODevice::WriteOnly);
	QDataStream midiout(&f);

	QDomElement element;


	int nTracks = 0;
	auto buffer = std::array<uint8_t, BUFFER_SIZE>{};

	for (const Track* track : tracks) if (track->type() == Track::Type::Instrument) nTracks++;
	for (const Track* track : patternStoreTracks) if (track->type() == Track::Type::Instrument) nTracks++;

	// midi header
	MidiFile::MIDIHeader header(nTracks);
	uint32_t size = header.writeToBuffer(buffer.data());
	midiout.writeRawData((char *)buffer.data(), size);

	std::vector<std::vector<std::pair<int,int>>> plists;

	// midi tracks
	for (Track* track : tracks)
	{
		DataFile dataFile(DataFile::Type::SongProject);
		MTrack mtrack;

		if (track->type() == Track::Type::Instrument)
		{

			mtrack.addName(track->name().toStdString(), 0);
			//mtrack.addProgramChange(0, 0);
			mtrack.addTempo(tempo, 0);

			auto instTrack = dynamic_cast<InstrumentTrack *>(track);
			element = instTrack->saveState(dataFile, dataFile.content());

			int base_pitch = 0;
			double base_volume = 1.0;
			int base_time = 0;

			MidiNoteVector midiClip;

			for (QDomNode n = element.firstChild(); !n.isNull(); n = n.nextSibling())
			{

				if (n.nodeName() == "instrumenttrack")
				{
					QDomElement it = n.toElement();
					base_pitch = (69 - it.attribute("basenote", "69").toInt());
					if (it.attribute("usemasterpitch", "1").toInt())
					{
						base_pitch += masterPitch;
					}
					base_volume = LocaleHelper::toDouble(it.attribute("volume", "100"))/100.0;
				}

				if (n.nodeName() == "midiclip")
				{
					base_time = n.toElement().attribute("pos", "0").toInt();
					writeMidiClip(midiClip, n, base_pitch, base_volume, base_time);
				}

			}
			processPatternNotes(midiClip, INT_MAX);
			writeMidiClipToTrack(mtrack, midiClip);
			size = mtrack.writeToBuffer(buffer.data());
			midiout.writeRawData((char *)buffer.data(), size);
		}

		if (track->type() == Track::Type::Pattern)
		{
			auto patternTrack = dynamic_cast<PatternTrack*>(track);
			element = patternTrack->saveState(dataFile, dataFile.content());

			std::vector<std::pair<int,int>> plist;
			for (QDomNode n = element.firstChild(); !n.isNull(); n = n.nextSibling())
			{

				if (n.nodeName() == "patternclip")
				{
					QDomElement it = n.toElement();
					int pos = it.attribute("pos", "0").toInt();
					int len = it.attribute("len", "0").toInt();
					plist.emplace_back(pos, pos+len);
				}
			}
			std::sort(plist.begin(), plist.end());
			plists.push_back(plist);

		}
	} // for each track

	// for each instrument in the pattern editor
	for (Track* track : patternStoreTracks)
	{
		DataFile dataFile(DataFile::Type::SongProject);
		MTrack mtrack;

		// begin at the first pattern track (first pattern)
		auto itr = plists.begin();

		std::vector<std::pair<int,int>> st;

		if (track->type() != Track::Type::Instrument) continue;

		mtrack.addName(track->name().toStdString(), 0);
		//mtrack.addProgramChange(0, 0);
		mtrack.addTempo(tempo, 0);

		auto instTrack = dynamic_cast<InstrumentTrack *>(track);
		element = instTrack->saveState(dataFile, dataFile.content());

		int base_pitch = 0;
		double base_volume = 1.0;

		// for each pattern in the pattern editor
		for (QDomNode n = element.firstChild(); !n.isNull(); n = n.nextSibling())
		{
			if (n.nodeName() == "instrumenttrack")
			{
				QDomElement it = n.toElement();
				base_pitch = (69 - it.attribute("basenote", "69").toInt());
				if (it.attribute("usemasterpitch", "1").toInt())
				{
					base_pitch += masterPitch;
				}
				base_volume = LocaleHelper::toDouble(it.attribute("volume", "100")) / 100.0;
			}

			if (n.nodeName() == "midiclip")
			{
				std::vector<std::pair<int,int>> &plist = *itr;

				MidiNoteVector nv, midiClip;
				writeMidiClip(midiClip, n, base_pitch, base_volume, 0);


				// FIXME better variable names and comments
				int pos = 0;
				int len = n.toElement().attribute("steps", "1").toInt() * 12;

				// for each pattern clip of the current pattern track (in song editor)
				for (const auto& position : plist)
				{
					const auto& [start, end] = position;
					while (!st.empty() && st.back().second <= start)
					{
						writePatternClip(midiClip, nv, len, st.back().first, pos, st.back().second);
						pos = st.back().second;
						st.pop_back();
					}

					if (!st.empty() && st.back().second <= end)
					{
						writePatternClip(midiClip, nv, len, st.back().first, pos, start);
						pos = start;
						while (!st.empty() && st.back().second <= end)
						{
							st.pop_back();
						}
					}

					st.push_back(position);
					pos = start;
				}

				while (!st.empty())
				{
					writePatternClip(midiClip, nv, len, st.back().first, pos, st.back().second);
					pos = st.back().second;
					st.pop_back();
				}

				processPatternNotes(nv, pos);
				writeMidiClipToTrack(mtrack, nv);

				// next pattern track
				++itr;
			}
		}
		size = mtrack.writeToBuffer(buffer.data());
		midiout.writeRawData((char *)buffer.data(), size);
	}

	return true;

}



void MidiExport::writeMidiClip(MidiNoteVector &midiClip, const QDomNode& n,
				int base_pitch, double base_volume, int base_time)
{
	// TODO interpret steps="12" muted="0" type="1" name="Piano1"  len="2592"
	for (QDomNode nn = n.firstChild(); !nn.isNull(); nn = nn.nextSibling())
	{
		QDomElement note = nn.toElement();
		if (note.attribute("len", "0") == "0") continue;
		// TODO interpret pan="0" mixch="0" pitchrange="1"
		MidiNote mnote;
		mnote.pitch = qMax(0, qMin(127, note.attribute("key", "0").toInt() + base_pitch));
		 // Map from LMMS volume to MIDI velocity
		mnote.volume = qMin(qRound(base_volume * LocaleHelper::toDouble(note.attribute("vol", "100")) * (127.0 / 200.0)), 127);
		mnote.time = base_time + note.attribute("pos", "0").toInt();
		mnote.duration = note.attribute("len", "0").toInt();
		mnote.type = static_cast<Note::Type>(note.attribute("type", "0").toInt());
		midiClip.push_back(mnote);
	}
}



void MidiExport::writeMidiClipToTrack(MTrack &mtrack, MidiNoteVector &nv)
{
	for (const auto& note : nv)
	{
		mtrack.addNote(note.pitch, note.volume, note.time / 48.0, note.duration / 48.0);
	}
}



void MidiExport::writePatternClip(MidiNoteVector& src, MidiNoteVector& dst,
				int len, int base, int start, int end)
{
	if (start >= end) { return; }
	start -= base;
	end -= base;
	std::sort(src.begin(), src.end());
	for (const auto& srcNote : src)
	{
		for (int time = srcNote.time + ceil((start - srcNote.time) / len) * len; time < end; time += len)
		{
			MidiNote note;
			note.duration = srcNote.duration;
			note.pitch = srcNote.pitch;
			note.time = base + time;
			note.volume = srcNote.volume;
			note.type = srcNote.type;
			dst.push_back(note);
		}
	}
}



void MidiExport::processPatternNotes(MidiNoteVector& nv, int cutPos)
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
		if (it->type == Note::Type::Step)
		{
			it->duration = qMin(qMin(DefaultBeatLength, next - cur), cutPos - it->time);
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


} // namespace lmms
