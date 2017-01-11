/*
 * oveReader.cpp - support for importing Overture files
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

#include "oveReader.h"

oveReader::oveReader( TrackContainer* tc ) :
	commonReader( tc, commonReader::tr( "Importing %1 file..." ).arg( "Overture" ) ),
	m_seq( new drumstick::QOve )
{
	// Connect to slots.
	connect( m_seq, SIGNAL( signalOVEError( QString ) ),
		 this, SLOT( errorHandler( QString ) ) );

	connect( m_seq, SIGNAL( signalOVEHeader( int, int ) ),
		 this, SLOT( fileHeader( int, int ) ) );

	connect( m_seq, SIGNAL( signalOVENoteOn( int, long, int, int, int ) ),
		 this, SLOT( noteOnEvent( int, long, int, int, int ) ) );

	connect( m_seq, SIGNAL( signalOVENoteOff( int, long, int, int, int ) ),
		 this, SLOT( noteOffEvent( int, long, int, int, int ) ) );

	connect( m_seq, SIGNAL( signalOVECtlChange( int, long, int, int, int ) ),
		 this, SLOT( ctlChangeEvent( int, long, int, int, int ) ) );

	connect( m_seq, SIGNAL( signalOVEPitchBend( int, long, int, int ) ),
		 this, SLOT( ctlChangeEvent( int, long, int, int, int ) ) );

	connect( m_seq, SIGNAL( signalOVEProgram( int, long, int, int ) ),
		 this, SLOT( programEvent( int, long, int, int ) ) );

	connect( m_seq, SIGNAL( signalOVETimeSig( int, long, int, int ) ),
		 this, SLOT( timeSigEvent( int, long, int, int ) ) );

	connect( m_seq, SIGNAL( signalOVETempo( long, int ) ),
		 this, SLOT( tempoEvent( long, int ) ) );

	connect( m_seq, SIGNAL( signalOVETrackPatch( int, int, int ) ),
		 this, SLOT( trackPatch( int, int, int ) ) );

	connect( m_seq, SIGNAL( signalOVETrackVol( int, int, int ) ),
		 this, SLOT( trackVol( int, int, int ) ) );

	connect( m_seq, SIGNAL( signalOVETrackBank( int, int, int ) ),
		 this, SLOT( trackBank( int, int, int ) ) );

	connect( m_seq, SIGNAL( signalOVENewTrack( QString, int, int, int, int, int, bool, bool, bool ) ),
		 this, SLOT( trackStart( QString, int, int, int, int, int, bool, bool, bool ) ) );

}

oveReader::~oveReader()
{
	printf( "destroy oveReader\n" );
	delete m_seq;
}

void oveReader::read( QString &fileName )
{
	m_seq->readFromFile( fileName );
}

// Slots below.
void oveReader::timeSigEvent( int bar, long tick, int num, int den )
{
	timeSigHandler( tick, num, den );
}

void oveReader::tempoEvent( long tick, int tempo )
{
	tempoHandler( tick, tempo / 100 );
}

void oveReader::errorHandler( const QString &errorStr )
{
	printf( "MidiImport::readSMF(): got error %s\n",
		errorStr.toStdString().c_str()  );
}

void oveReader::ctlChangeEvent( int track, long tick, int chan, int ctl, int value )
{
	CCHandler( tick, track, ctl, value );
}

void oveReader::pitchBendEvent( int track, long tick, int chan, int value )
{
	CCHandler( tick, track, pitchBendEventId, value );
}

void oveReader::noteOnEvent( int track, long tick, int chan, int pitch, int vol )
{

	if( vol )
	{
		insertNoteEvent( tick, chan, pitch, vol, track );
	}
	else
	{
		addNoteEvent( tick, chan, pitch, track );
	}
}

void oveReader::noteOffEvent( int track, long tick, int chan, int pitch, int vol )
{
	addNoteEvent( tick, chan, pitch, track );
}

void oveReader::programEvent( int track, long tick, int chan, int patch )
{
	programHandler( tick, chan, patch, track );
}

void oveReader::trackPatch( int track, int chan, int patch )
{
	programHandler( 0, chan, patch, track );
}

void oveReader::trackBank( int track, int chan, int bank )
{
	CCHandler( 0, track, bankEventId, bank );
}

void oveReader::trackStart( const QString &name, int track, int channel, int pitch, int velocity, int port, bool selected, bool muted, bool loop )
{
	trackStartHandler();
	textHandler( 3, name, track );
}

void oveReader::trackVol( int track, int chan, int vol )
{
	CCHandler( 0, track, volumeEventId, vol );
}

void oveReader::fileHeader( int quarter, int trackCount )
{
	timeBaseHandler( quarter );
	pd.setMaximum( trackCount + preTrackSteps );
}
