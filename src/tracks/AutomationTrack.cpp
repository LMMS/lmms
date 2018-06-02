/*
 * AutomationTrack.cpp - AutomationTrack handles automation of objects without
 *                       a track
 *
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2006-2008 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
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

#include "AutomationTrack.h"
#include "AutomationPattern.h"
#include "Engine.h"
#include "embed.h"
#include "ProjectJournal.h"
#include "StringPairDrag.h"
#include "TrackContainerView.h"
#include "TrackLabelButton.h"

#include <QMenu>

AutomationTrack::AutomationTrack( TrackContainer* tc, bool _hidden ) :
	Track( _hidden ? HiddenAutomationTrack : Track::AutomationTrack, tc )
{
	setName( tr( "Automation track" ) );
}

bool AutomationTrack::play( const MidiTime & time_start, const fpp_t _frames,
							const f_cnt_t _frame_base, int _tco_num )
{
	return false;
}




TrackView * AutomationTrack::createView( TrackContainerView* tcv )
{
	return new AutomationTrackView( this, tcv );
}




TrackContentObject * AutomationTrack::createTCO( const MidiTime & )
{
	return new AutomationPattern( this );
}




void AutomationTrack::saveTrackSpecificSettings( QDomDocument & _doc,
							QDomElement & _this )
{
}




void AutomationTrack::loadTrackSpecificSettings( const QDomElement & _this )
{
	// just in case something somehow wrent wrong...
	if( type() == HiddenAutomationTrack )
	{
		setMuted( false );
	}
}





AutomationTrackView::AutomationTrackView( AutomationTrack * _at, TrackContainerView* tcv ) :
	TrackView( _at, tcv )
{
        setFixedHeight( 32 );
	TrackLabelButton * tlb = new TrackLabelButton( this,
						getTrackSettingsWidget() );
	tlb->setIcon( embed::getIconPixmap( "automation_track" ) );
	tlb->move( 3, 1 );
	tlb->show();
	setModel( _at );
}

void AutomationTrackView::dragEnterEvent( QDragEnterEvent * _dee )
{
	StringPairDrag::processDragEnterEvent( _dee, "automatable_model" );
}




void AutomationTrackView::dropEvent( QDropEvent * _de )
{
	QString type = StringPairDrag::decodeKey( _de );
	QString val = StringPairDrag::decodeValue( _de );
	if( type == "automatable_model" )
	{
		AutomatableModel * mod = dynamic_cast<AutomatableModel *>(
				Engine::projectJournal()->
					journallingObject( val.toInt() ) );
		if( mod != NULL )
		{
			MidiTime pos = MidiTime( trackContainerView()->
							currentPosition() +
				( _de->pos().x() -
					getTrackContentWidget()->x() ) *
						MidiTime::ticksPerTact() /
		static_cast<int>( trackContainerView()->pixelsPerTact() ) )
				.toAbsoluteTact();

			if( pos.getTicks() < 0 )
			{
				pos.setTicks( 0 );
			}

			TrackContentObject * tco = getTrack()->createTCO( pos );
			AutomationPattern * pat = dynamic_cast<AutomationPattern *>( tco );
			pat->addObject( mod );
			pat->movePosition( pos );
		}
	}

	update();
}

void AutomationTrackView::updateTrackOperationsWidgetMenu(TrackOperationsWidget *trackOperations) {
	TrackView::updateTrackOperationsWidgetMenu (trackOperations);

	auto toMenu = trackOperations->trackOps ()->menu ();

	toMenu->addAction( tr( "Turn all recording on" ), this, SLOT( recordingOn() ) );
	toMenu->addAction( tr( "Turn all recording off" ), this, SLOT( recordingOff() ) );
}

void AutomationTrackView::recordingOn() {
	const Track::tcoVector & tcov = getTrack()->getTCOs();
	for( Track::tcoVector::const_iterator it = tcov.begin(); it != tcov.end(); ++it )
	{
		AutomationPattern * ap = dynamic_cast<AutomationPattern *>( *it );
		if( ap ) { ap->setRecording( true ); }
	}
	update();
}


void AutomationTrackView::recordingOff() {
	const Track::tcoVector & tcov = getTrack()->getTCOs();
	for( Track::tcoVector::const_iterator it = tcov.begin(); it != tcov.end(); ++it )
	{
		AutomationPattern * ap = dynamic_cast<AutomationPattern *>( *it );
		if( ap ) { ap->setRecording( false ); }
	}
	update();
}


