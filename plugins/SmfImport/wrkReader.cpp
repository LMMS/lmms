/*
 * wrkReader.cpp - support for importing Cakewalk files
 *
 * Copyright (c) 2016-2017 Tony Chyi <tonychee1989/at/gmail.com>
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

#include <QString>
#include <QApplication>
#include <QMessageBox>
#include <QProgressDialog>
#include <drumstick.h>

#include "GuiApplication.h"
#include "TrackContainer.h"
#include "Engine.h"
#include "Song.h"
#include "AutomationPattern.h"
#include "MidiTime.h"
#include "MainWindow.h"

#include "SmfMidiCC.h"
#include "SmfMidiChannel.h"

#include "wrkReader.h"

wrkReader::wrkReader( TrackContainer* tc ) :
	commonReader( tc, commonReader::tr( "Importing %1 file..." ).arg( "Cakewalk" ) ),
	m_seq( new drumstick::QWrk )
{
	// Connect to slots.
	connect( m_seq, SIGNAL( signalWRKError( QString ) ),
		 this, SLOT( errorHandler( QString ) ) );
	connect( m_seq, SIGNAL( signalWRKTimeBase( int ) ),
		 this, SLOT( timeBase( int ) ) );
	connect( m_seq, SIGNAL( signalWRKNote( int, long, int, int, int, int ) ),
		 this, SLOT( noteEvent( int, long, int, int, int, int ) ) );
	connect( m_seq, SIGNAL( signalWRKCtlChange( int, long, int, int, int ) ),
		 this, SLOT( ctlChangeEvent( int, long, int, int, int ) ) );
	connect( m_seq, SIGNAL( signalWRKPitchBend( int, long, int, int ) ),
		 this, SLOT( pitchBendEvent( int, long, int, int ) ) );
	connect( m_seq, SIGNAL( signalWRKProgram( int, long, int, int ) ),
		 this, SLOT( programEvent( int, long, int, int ) ) );
	connect( m_seq, SIGNAL( signalWRKTimeSig( int, int, int ) ),
		 this, SLOT( timeSigEvent( int, int, int ) ) );
	connect( m_seq, SIGNAL( signalWRKTempo( long, int ) ),
		 this, SLOT( tempoEvent( long, int ) ) );
	connect( m_seq, SIGNAL( signalWRKTrackVol( int, int ) ),
		 this, SLOT( trackVol( int, int ) ) );
	connect( m_seq, SIGNAL( signalWRKTrackBank( int, int ) ),
		 this, SLOT( trackBank( int, int ) ) );
	connect( m_seq, SIGNAL( signalWRKTrackName( int, const QString ) ),
		 this, SLOT() );
}

wrkReader::~wrkReader()
{
	printf( "destroy wrkReader\n" );
	delete m_seq;
}

void wrkReader::read( QString &fileName )
{
	m_seq->readFromFile( fileName );
}

// Slots below.

void wrkReader::timeSigEvent( int bar, int num, int den )
{
	int tick = bar * num * ticksPerBeat;
	timeSigHandler( tick, num, den );
}

void wrkReader::tempoEvent( long time, int tempo )
{
	tempoHandler( time, tempo / 100 );
}

void wrkReader::errorHandler( const QString &errorStr )
{
	printf( "MidiImport::readSMF(): got error %s\n",
		errorStr.toStdString().c_str()  );
}

void wrkReader::ctlChangeEvent( int track, long tick, int chan, int ctl, int value )
{
	CCHandler( tick, track, ctl, value );
}

void wrkReader::pitchBendEvent( int track, long tick, int chan, int value )
{
	CCHandler( tick, track, pitchBendEventId, value );
}

void wrkReader::noteEvent( int track, long time, int chan, int pitch, int vol, int dur )
{
	addNoteEvent( time, chan, pitch, vol, dur, track );
}

void wrkReader::programEvent( int track, long tick, int chan, int patch )
{
	programHandler( tick, chan, patch, track );
}

void wrkReader::trackPatch( int track, int patch )
{
	//programHandler(0, chan, patch, track);
}

void wrkReader::trackBank( int track, int bank )
{
	//CCHandler(0, track, 0, bank);
}

void wrkReader::trackName( int track, QString name )
{
	trackStartHandler();
	textHandler( 3, name, track );
}

void wrkReader::trackVol( int track, int vol )
{
	CCHandler( 0, track, volumeEventId, vol );
}

void wrkReader::timeBase( int quarter )
{
	timeBaseHandler( quarter );
}
