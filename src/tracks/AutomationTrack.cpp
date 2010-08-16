/*
 * AutomationTrack.cpp - AutomationTrack handles automation of objects without
 *                       a track
 *
 * Copyright (c) 2008-2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2006-2008 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
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

#include "AutomationTrack.h"
#include "AutomationPattern.h"
#include "engine.h"
#include "embed.h"
#include "ProjectJournal.h"
#include "string_pair_drag.h"
#include "track_container_view.h"
#include "track_label_button.h"


AutomationTrack::AutomationTrack( trackContainer * _tc, bool _hidden ) :
	track( _hidden ? HiddenAutomationTrack : track::AutomationTrack, _tc )
{
	setName( tr( "Automation track" ) );
}




AutomationTrack::~AutomationTrack()
{
}




bool AutomationTrack::play( const midiTime & _start, const fpp_t _frames,
				const f_cnt_t _frame_base, Sint16 _tco_num )
{
	if( isMuted() )
	{
		return false;
	}

	tcoVector tcos;
	if( _tco_num >= 0 )
	{
		trackContentObject * tco = getTCO( _tco_num );
		tcos.push_back( tco );
	}
	else
	{
		getTCOsInRange( tcos, _start, _start + static_cast<int>(
					_frames / engine::framesPerTick()) );
	}

	for( tcoVector::iterator it = tcos.begin(); it != tcos.end(); ++it )
	{
		AutomationPattern * p = dynamic_cast<AutomationPattern *>( *it );
		if( p == NULL || ( *it )->isMuted() )
		{
			continue;
		}
		midiTime cur_start = _start;
		if( _tco_num < 0 )
		{
			cur_start -= p->startPosition();
		}
		p->processMidiTime( cur_start );
	}
	return false;
}




trackView * AutomationTrack::createView( trackContainerView * _tcv )
{
	return new AutomationTrackView( this, _tcv );
}




trackContentObject * AutomationTrack::createTCO( const midiTime & )
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





AutomationTrackView::AutomationTrackView( AutomationTrack * _at,
						trackContainerView * _tcv ) :
	trackView( _at, _tcv )
{
	setFixedHeight( 32 );
	trackLabelButton * tlb = new trackLabelButton( this,
						getTrackSettingsWidget() );
	tlb->setIcon( embed::getIconPixmap( "automation_track" ) );
	tlb->move( 3, 1 );
	tlb->show();
	setModel( _at );
}




AutomationTrackView::~AutomationTrackView()
{
}




void AutomationTrackView::dragEnterEvent( QDragEnterEvent * _dee )
{
	stringPairDrag::processDragEnterEvent( _dee, "automatable_model" );
}




void AutomationTrackView::dropEvent( QDropEvent * _de )
{
	QString type = stringPairDrag::decodeKey( _de );
	QString val = stringPairDrag::decodeValue( _de );
	if( type == "automatable_model" )
	{
		AutomatableModel * mod = dynamic_cast<AutomatableModel *>(
				engine::projectJournal()->
					journallingObject( val.toInt() ) );
		if( mod != NULL )
		{
			midiTime pos = midiTime( getTrackContainerView()->
							currentPosition() +
				( _de->pos().x() -
					getTrackContentWidget()->x() ) *
						midiTime::ticksPerTact() /
		static_cast<int>( getTrackContainerView()->pixelsPerTact() ) )
				.toNearestTact();

			if( pos.getTicks() < 0 )
			{
				pos.setTicks( 0 );
			}

			trackContentObject * tco = getTrack()->createTCO( pos );
			AutomationPattern * pat = dynamic_cast<AutomationPattern *>( tco );
			pat->addObject( mod );
			pat->movePosition( pos );
		}
	}

	update();
}



