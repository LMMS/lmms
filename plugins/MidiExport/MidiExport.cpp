/*
 * MidiExport.cpp - support for importing MIDI files
 *
 * Author: Mohamed Abdel Maksoud <mohamed at amaksoud.com>
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
#include "Engine.h"
#include "TrackContainer.h"
#include "InstrumentTrack.h"


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT midiexport_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"MIDI Export",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"Filter for exporting MIDI-files from LMMS" ),
	"Mohamed Abdel Maksoud <mohamed at amaksoud.com>",
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
			const TrackContainer::TrackList &tracks_BB, int tempo, const QString &filename)
{
	QFile f(filename);
	f.open(QIODevice::WriteOnly);
	QDataStream midiout(&f);

	InstrumentTrack* instTrack;
	QDomElement element;


	int nTracks = 0;
	uint8_t buffer[BUFFER_SIZE];
	uint32_t size;

	for( const Track* track : tracks ) if( track->type() == Track::InstrumentTrack ) nTracks++;

	// midi header
	MidiFile::MIDIHeader header(nTracks);
	size = header.writeToBuffer(buffer);
	midiout.writeRawData((char *)buffer, size);

	// midi tracks
	for( Track* track : tracks )
	{
		DataFile dataFile( DataFile::SongProject );
		MidiFile::MIDITrack<BUFFER_SIZE> mtrack;
	
		if( track->type() != Track::InstrumentTrack ) continue;

		//qDebug() << "exporting " << track->name();
	
	
		mtrack.addName(track->name().toStdString(), 0);
		//mtrack.addProgramChange(0, 0);
		mtrack.addTempo(tempo, 0);
	
		instTrack = dynamic_cast<InstrumentTrack *>( track );
		element = instTrack->saveState( dataFile, dataFile.content() );
	
		// instrumentTrack
		//     - instrumentTrack
		//     - pattern
		int   base_pitch   = 0;
		double base_volume = 1.0;
		int    base_time = 0;
	
	
		for(QDomNode n = element.firstChild(); !n.isNull(); n = n.nextSibling())
		{
			//QDomText txt = n.toText();
			//qDebug() << ">> child node " << n.nodeName();
		
			if (n.nodeName() == "instrumenttrack")
			{
				// TODO interpret pan="0" fxch="0" usemasterpitch="1" pitchrange="1" pitch="0" basenote="57"
				QDomElement it = n.toElement();
				base_pitch  = it.attribute("pitch", "0").toInt();
				base_volume = it.attribute("volume", "100").toDouble()/100.0;
			}
		
			if (n.nodeName() == "pattern")
			{
				base_time = n.toElement().attribute("pos", "0").toInt();

				writePattern(mtrack, n, base_pitch, base_volume, base_time);
			}
		
		}
		size = mtrack.writeToBuffer(buffer);
		midiout.writeRawData((char *)buffer, size);
	} // for each track
	
	return true;

}



void MidiExport::writePattern(MidiFile::MIDITrack<BUFFER_SIZE> &mtrack, QDomNode n, int base_pitch, double base_volume, int base_time)
{
	// TODO interpret steps="12" muted="0" type="1" name="Piano1"  len="2592"
	for(QDomNode nn = n.firstChild(); !nn.isNull(); nn = nn.nextSibling())
	{
		QDomElement note = nn.toElement();
		if (note.attribute("len", "0") == "0" || note.attribute("vol", "0") == "0") continue;
		#if 0
		qDebug() << ">>>> key " << note.attribute("key", "0")
			<< " " << note.attribute("len", "0") << " @"
			<< note.attribute("pos", "0");
		#endif
		mtrack.addNote(
			note.attribute("key", "0").toInt()+base_pitch
			, 100 * base_volume * (note.attribute("vol", "100").toDouble()/100)
			, (base_time+note.attribute("pos", "0").toDouble())/48
			, (note.attribute("len", "0")).toDouble()/48);
	}
}



void MidiExport::error()
{
	//qDebug() << "MidiExport error: " << m_error ;
}



extern "C"
{

// necessary for getting instance out of shared lib
Plugin * PLUGIN_EXPORT lmms_plugin_main( Model *, void * _data )
{
	return new MidiExport();
}


}

