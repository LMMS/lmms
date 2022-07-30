/*
 * PatternClip.cpp - implementation of class PatternClip
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 
#include "PatternClip.h"
 
#include <QDomElement>
 
#include "Engine.h"
#include "PatternClipView.h"
#include "PatternStore.h"
#include "PatternTrack.h"


PatternClip::PatternClip(Track* track) :
	Clip(track)
{
	bar_t t = Engine::patternStore()->lengthOfPattern(patternIndex());
	if( t > 0 )
	{
		saveJournallingState( false );
		changeLength( TimePos( t, 0 ) );
		restoreJournallingState();
	}
	setAutoResize( false );
}

void PatternClip::saveSettings(QDomDocument& doc, QDomElement& element)
{
	element.setAttribute( "name", name() );
	if( element.parentNode().nodeName() == "clipboard" )
	{
		element.setAttribute( "pos", -1 );
	}
	else
	{
		element.setAttribute( "pos", startPosition() );
	}
	element.setAttribute( "len", length() );
	element.setAttribute( "muted", isMuted() );
	if( usesCustomClipColor() )
	{
		element.setAttribute( "color", color().name() );
	}
}




void PatternClip::loadSettings(const QDomElement& element)
{
	setName( element.attribute( "name" ) );
	if( element.attribute( "pos" ).toInt() >= 0 )
	{
		movePosition( element.attribute( "pos" ).toInt() );
	}
	changeLength( element.attribute( "len" ).toInt() );
	if( element.attribute( "muted" ).toInt() != isMuted() )
	{
		toggleMute();
	}
	
	// for colors saved in 1.3-onwards
	if( element.hasAttribute( "color" ) && !element.hasAttribute( "usestyle" ) )
	{
		useCustomClipColor( true );
		setColor( element.attribute( "color" ) );
	}
	
	// for colors saved before 1.3
	else if(element.hasAttribute("color"))
	{
		setColor(QColor(element.attribute("color").toUInt()));
		
		// usestyle attribute is no longer used
	}
	else
	{
		useCustomClipColor(false);
	}
}



int PatternClip::patternIndex()
{
	return dynamic_cast<PatternTrack*>(getTrack())->patternIndex();
}



ClipView* PatternClip::createView(TrackView* tv)
{
	return new PatternClipView(this, tv);
}
