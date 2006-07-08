#ifndef SINGLE_SOURCE_COMPILE

/*
 * automation_track.cpp - automationTrack handles automation of objects without
 *                        a track
 *
 * Copyright (c) 2006 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#include "automation_track.h"




automationTrack::automationTrack( trackContainer * _tc ) :
	track( _tc, FALSE )
{
}




automationTrack::~automationTrack()
{
}




track::trackTypes automationTrack::type( void ) const
{
	return( AUTOMATION_TRACK );
}




bool automationTrack::play( const midiTime & _start,
					const f_cnt_t _start_frame,
					const fpab_t _frames,
					const f_cnt_t _frame_base,
							Sint16 _tco_num )
{
	sendMidiTime( _start );
	return( FALSE );
}




trackContentObject * automationTrack::createTCO( const midiTime & )
{
	return( NULL );
}




void automationTrack::saveTrackSpecificSettings( QDomDocument & _doc,
							QDomElement & _this )
{
}




void automationTrack::loadTrackSpecificSettings( const QDomElement & _this )
{
}




#include "automation_track.moc"


#endif
