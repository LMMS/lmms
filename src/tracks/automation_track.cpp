#ifndef SINGLE_SOURCE_COMPILE

/*
 * automation_track.cpp - automationTrack handles automation of objects without
 *                        a track
 *
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "automation_track.h"
#include "automation_pattern.h"
#include "embed.h"
#include "name_label.h"




automationTrack::automationTrack( trackContainer * _tc, bool _hidden ) :
	track( _hidden ? HiddenAutomationTrack : AutomationTrack, _tc )
{
}




automationTrack::~automationTrack()
{
}




bool automationTrack::play( const midiTime & _start, const fpp_t _frames,
				const f_cnt_t _frame_base, Sint16 _tco_num )
{
	tcoVector tcos;
	getTCOsInRange( tcos, _start, _start + static_cast<int>(
					_frames / engine::framesPerTick() ) );
	for( tcoVector::iterator it = tcos.begin(); it != tcos.end(); ++it )
	{
		automationPattern * p =
				dynamic_cast<automationPattern *>( *it );
		if( p == NULL || ( *it )->isMuted() )
		{
			continue;
		}
		p->processMidiTime( _start - p->startPosition() );
	}
	return( FALSE );
}




trackView * automationTrack::createView( trackContainerView * _tcv )
{
	return( new automationTrackView( this, _tcv ) );
}




trackContentObject * automationTrack::createTCO( const midiTime & )
{
	return( new automationPattern( this ) );
}




void automationTrack::saveTrackSpecificSettings( QDomDocument & _doc,
							QDomElement & _this )
{
}




void automationTrack::loadTrackSpecificSettings( const QDomElement & _this )
{
}





automationTrackView::automationTrackView( automationTrack * _at,
						trackContainerView * _tcv ) :
	trackView( _at, _tcv )
{
	setFixedHeight( 32 );
	m_trackLabel = new nameLabel( _at->name(), getTrackSettingsWidget() );
	m_trackLabel->setPixmap( embed::getIconPixmap( "automation" ) );
	m_trackLabel->setGeometry( 1, 1, DEFAULT_SETTINGS_WIDGET_WIDTH - 2,
									29 );
	m_trackLabel->show();
	connect( m_trackLabel, SIGNAL( nameChanged( const QString & ) ),
			_at, SLOT( setName( const QString & ) ) );
	setModel( _at );
}




automationTrackView::~automationTrackView()
{
}




#endif
