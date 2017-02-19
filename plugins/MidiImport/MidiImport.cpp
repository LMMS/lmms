/*
 * MidiImport.cpp - support for importing MIDI files
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "MidiImport.h"
#include "TrackContainer.h"
#include "InstrumentTrack.h"
#include "AutomationTrack.h"
#include "AutomationPattern.h"
#include "ConfigManager.h"
#include "Pattern.h"
#include "Instrument.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "MidiTime.h"
#include "debug.h"
#include "embed.h"
#include "Song.h"

#include "smfMidiCC.h"
#include "smfMidiChannel.h"
#include "midiReader.h"

#define makeID(_c0, _c1, _c2, _c3) \
		( 0 | \
		( ( _c0 ) | ( ( _c1 ) << 8 ) | ( ( _c2 ) << 16 ) | ( ( _c3 ) << 24 ) ) )



extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT midiimport_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"MIDI Import",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"Filter for importing MIDI-files into LMMS" ),
	"Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>",
	0x0100,
	Plugin::ImportFilter,
	NULL,
	NULL,
	NULL
} ;

}


MidiImport::MidiImport( const QString & _file ) :
	ImportFilter( _file, &midiimport_plugin_descriptor ),
	m_events(),
	m_timingDivision( 0 )
{

}




MidiImport::~MidiImport()
{

}




bool MidiImport::tryImport( TrackContainer* tc )
{
	if( openFile() == false )
	{
		return false;
	}

#ifdef LMMS_HAVE_FLUIDSYNTH
	if( gui != NULL &&
		ConfigManager::inst()->defaultSoundfont().isEmpty() )
	{
		QMessageBox::information( gui->mainWindow(),
			tr( "Setup incomplete" ),
			tr( "You do not have set up a default soundfont in "
				"the settings dialog (Edit->Settings). "
				"Therefore no sound will be played back after "
				"importing this MIDI file. You should download "
				"a General MIDI soundfont, specify it in "
				"settings dialog and try again." ) );
	}
#else
	if( gui )
	{
		QMessageBox::information( gui->mainWindow(),
			tr( "Setup incomplete" ),
			tr( "You did not compile LMMS with support for "
				"SoundFont2 player, which is used to add default "
				"sound to imported MIDI files. "
				"Therefore no sound will be played back after "
				"importing this MIDI file." ) );
	}
#endif

	switch( readID() )
	{
		case makeID( 'M', 'T', 'h', 'd' ):
			printf( "MidiImport::tryImport(): found MThd\n");
			return readSMF( tc );

		case makeID( 'R', 'I', 'F', 'F' ):
			printf( "MidiImport::tryImport(): found RIFF\n");
			return readRIFF( tc );

		default:
			printf( "MidiImport::tryImport(): not a Standard MIDI "
								"file\n" );
			return false;
	}
}

bool MidiImport::readSMF( TrackContainer* tc )
{
	QString filename = file().fileName();
	closeFile();

	midiReader mr(tc);
	mr.read(filename);

	return true;
}




bool MidiImport::readRIFF( TrackContainer* tc )
{
	// skip file length
	skip( 4 );

	// check file type ("RMID" = RIFF MIDI)
	if( readID() != makeID( 'R', 'M', 'I', 'D' ) )
	{
invalid_format:
			qWarning( "MidiImport::readRIFF(): invalid file format" );
			return false;
	}

	// search for "data" chunk
	while( 1 )
	{
		const int id = readID();
		const int len = read32LE();
		if( file().atEnd() )
		{
data_not_found:
				qWarning( "MidiImport::readRIFF(): data chunk not found" );
				return false;
		}
		if( id == makeID( 'd', 'a', 't', 'a' ) )
		{
				break;
		}
		if( len < 0 )
		{
				goto data_not_found;
		}
		skip( ( len + 1 ) & ~1 );
	}

	// the "data" chunk must contain data in SMF format
	if( readID() != makeID( 'M', 'T', 'h', 'd' ) )
	{
		goto invalid_format;
	}
	return readSMF( tc );
}




void MidiImport::error()
{
	printf( "MidiImport::readTrack(): invalid MIDI data (offset %#x)\n",
						(unsigned int) file().pos() );
}


extern "C"
{

// necessary for getting instance out of shared lib
Plugin * PLUGIN_EXPORT lmms_plugin_main( Model *, void * _data )
{
	return new MidiImport( QString::fromUtf8(
									static_cast<const char *>( _data ) ) );
}


}

