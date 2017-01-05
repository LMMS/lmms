/*
 * smfMidiCC.cpp - support for importing MIDI files
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QApplication>

#include "TrackContainer.h"
#include "AutomationTrack.h"
#include "AutomationPattern.h"
#include "MidiTime.h"

#include "smfMidiCC.h"

smfMidiCC::smfMidiCC() :
	at( NULL ),
	ap( NULL ),
	lastPos( 0 )
{ }

smfMidiCC & smfMidiC::create( TrackContainer* tc, QString tn )
{
	if( !at )
	{
		// Keep LMMS responsive, for now the import runs 
		// in the main thread. This should probably be 
		// removed if that ever changes.
		qApp->processEvents();
		at = dynamic_cast<AutomationTrack *>( Track::create( Track::AutomationTrack, tc ) );
	}
	if( tn != "") {
		at->setName( tn );
	}
	return *this;
}


void smfMidiCC::clear()
{
    at = NULL;
	ap = NULL;
	lastPos = 0;
}

smfMidiCC & smfMidiCC::putValue( MidiTime time, AutomatableModel * objModel, float value )
{
	if( !ap || time > lastPos + DefaultTicksPerTact )
	{
		MidiTime pPos = MidiTime( time.getTact(), 0 );
		ap = dynamic_cast<AutomationPattern*>(
			at->createTCO(0) );
		ap->movePosition( pPos );
		ap->addObject( objModel );
	}

	lastPos = time;
	time = time - ap->startPosition();
	ap->putValue( time, value, false );
	ap->changeLength( MidiTime( time.getTact() + 1, 0 ) ); 

	return *this;
}
