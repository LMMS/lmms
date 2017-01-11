/*
 * SmfMidiChannel.cpp - support for importing MIDI files
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

#include "InstrumentTrack.h"
#include "Pattern.h"
#include "Instrument.h"
#include "MidiTime.h"
#include "ConfigManager.h"

#include "SmfMidiChannel.h"

SmfMidiChannel::SmfMidiChannel() :
	it( NULL ),
	p( NULL ),
	it_inst( NULL ),
	isSF2( false ),
	hasNotes( false ),
	lastEnd( 0 )
{ }

SmfMidiChannel * SmfMidiChannel::create( TrackContainer* tc, QString tn )
{
	if( !it ) {
		// Keep LMMS responsive
		qApp->processEvents();
		it = dynamic_cast<InstrumentTrack *>( Track::create( Track::InstrumentTrack, tc ) );

#ifdef LMMS_HAVE_FLUIDSYNTH
		it_inst = it->loadInstrument( "sf2player" );
		
		if( it_inst )
		{
			isSF2 = true;
			it_inst->loadFile( ConfigManager::inst()->defaultSoundfont() );
			it_inst->childModel( "bank" )->setValue( 0 );
			it_inst->childModel( "patch" )->setValue( 0 );
		}
		else
		{
			it_inst = it->loadInstrument( "patman" );
		}	
#else
		it_inst = it->loadInstrument( "patman" );
#endif
		trackName = tn;
		if( trackName != "") {
			it->setName( tn );
		}
		lastEnd = 0;
		// General MIDI default
		it->pitchRangeModel()->setInitValue( 2 );
	}
	return this;
}

void SmfMidiChannel::addNote( Note & n )
{
	if( !p || n.pos() > lastEnd + DefaultTicksPerTact )
	{
		MidiTime pPos = MidiTime( n.pos().getTact(), 0 );
		p = dynamic_cast<Pattern*>( it->createTCO( 0 ) );
		p->movePosition( pPos );
	}
	hasNotes = true;
	lastEnd = n.pos() + n.length();
	n.setPos( n.pos( p->startPosition() ) );
    p->addNote( n, false );
}

void SmfMidiChannel::setName( QString tn )
{
    if ( !tn.length() )
    {
        it->setName( QT_TRANSLATE_NOOP("TrackContainer", "Track") );
        return;
    }
    it->setName( tn );
}
