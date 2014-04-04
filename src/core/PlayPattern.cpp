/*
 * PlayPattern.cpp - PlayPattern strategy.
 *
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "PlayPattern.h"
#include "TrackContainer.h"
#include "track.h"
#include "pattern.h"


  void PlayPattern::process(song *const aSong){
 
 			if( aSong->m_patternToPlay != NULL )
			{
				aSong->m_patternToPlay->getTrack()->
						getTCONum( aSong->m_patternToPlay );
				aSong->m_tracklist.push_back(
						aSong->m_patternToPlay->getTrack() );
			}
			
}
