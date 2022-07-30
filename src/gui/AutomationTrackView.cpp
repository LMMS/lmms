/*
 * AutomationTrackView.cpp
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
 
 #include "AutomationTrackView.h"
 
 #include "AutomationPattern.h"
 #include "embed.h"
 #include "Engine.h"
 #include "ProjectJournal.h"
 #include "StringPairDrag.h"
 #include "TrackContainerView.h"
 #include "TrackLabelButton.h"
 

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
		if( mod != nullptr )
		{
			TimePos pos = TimePos( trackContainerView()->
							currentPosition() +
				( _de->pos().x() -
					getTrackContentWidget()->x() ) *
						TimePos::ticksPerBar() /
		static_cast<int>( trackContainerView()->pixelsPerBar() ) )
				.toAbsoluteBar();

			if( pos.getTicks() < 0 )
			{
				pos.setTicks( 0 );
			}

			TrackContentObject * tco = getTrack()->createTCO( pos );
			AutomationPattern * pat = dynamic_cast<AutomationPattern *>( tco );
			pat->addObject( mod );
		}
	}

	update();
}