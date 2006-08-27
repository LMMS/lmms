/*
 * automation_track.h - declaration of class automationTrack, which handles
 *                      automation of objects without a track
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */


#ifndef _AUTOMATION_TRACK_H
#define _AUTOMATION_TRACK_H

#include "track.h"


class automationTrack : public QWidget, public track
{
	Q_OBJECT
public:
	automationTrack( trackContainer * _tc );
	virtual ~automationTrack();


private:
	inline QString nodeName( void ) const
	{
		return( "automation-track" );
	}

	virtual trackTypes type( void ) const;

	virtual bool FASTCALL play( const midiTime & _start,
						const fpab_t _frames,
						const f_cnt_t _frame_base,
							Sint16 _tco_num = -1 );

	virtual trackContentObject * FASTCALL createTCO( const midiTime &
									_pos );

	virtual void FASTCALL saveTrackSpecificSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void FASTCALL loadTrackSpecificSettings( const QDomElement &
									_this );

} ;


#endif
