/*
 * TempoTrack.cpp - implementation of tempo track
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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


#include "TempoTrack.h"
#include "Track.h"
#include "TrackLabelButton.h"
#include "Engine.h"
#include "embed.h"

TempoTrack::TempoTrack(  TrackContainer* tc ) :
	AutomationTrack( tc, false, true )
{
	setName( tr( "Tempo Track" ) );
}


TempoTrack::~TempoTrack()
{
}


ProcessHandle * TempoTrack::getProcessHandle()
{
	return NULL;
}


TrackView * TempoTrack::createView( TrackContainerView * tcv )
{
	return new TempoTrackView( this, tcv );
}


TrackContentObject * TempoTrack::createTCO( const MidiTime & _pos )
{
	return new TempoPattern( this );
}


void TempoTrack::saveTrackSpecificSettings( QDomDocument & _doc, QDomElement & _parent )
{
}


void TempoTrack::loadTrackSpecificSettings( const QDomElement & _this )
{
}



TempoPattern::TempoPattern( TempoTrack * tt ) :
	AutomationPattern( tt )
{
	setTempoPattern( true );
}


TempoPattern::TempoPattern( const TempoPattern & tpToCopy ) :
	AutomationPattern( tpToCopy )
{
	setTempoPattern( true );
}


TempoPattern::~TempoPattern()
{
}


TrackContentObjectView * TempoPattern::createView( TrackView * tv )
{
	return new TempoPatternView( this, tv );
}


TempoPatternView::TempoPatternView( TempoPattern * pat, TrackView * parent ) :
	AutomationPatternView( pat, parent )
{
}


TempoPatternView::~TempoPatternView()
{
}


TempoTrackView::TempoTrackView( TempoTrack * tt, TrackContainerView * tcv ) :
	TrackView( tt, tcv )
{
	setFixedHeight( 32 );
	TrackLabelButton * tlb = new TrackLabelButton( this, getTrackSettingsWidget() );
	tlb->setIcon( embed::getIconPixmap( "tempo_sync" ) );
	tlb->move( 3, 1 );
	tlb->show();
	setModel( tt );
}


TempoTrackView::~TempoTrackView()
{
}


void TempoTrackView::dragEnterEvent( QDragEnterEvent * _dee )
{}
void TempoTrackView::dropEvent( QDropEvent * _de )
{}
