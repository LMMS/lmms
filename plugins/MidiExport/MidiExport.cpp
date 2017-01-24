/*
 * MidiExport.cpp - support for importing MIDI files
 *
 * Author: Mohamed Abdel Maksoud <mohamed at amaksoud.com>
 *
 * This file is part of LMMS - http://lmms.io
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
#include <drumstick.h>

#include "MidiExport.h"
#include "midiWriter.h"
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



bool MidiExport::tryExport( const TrackContainer::TrackList &tracks, int tempo, const QString &filename  )
{
	midiWriter s_mr( tracks );
	s_mr.writeFile( filename );
	return true;
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

