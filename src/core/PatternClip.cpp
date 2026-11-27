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

namespace lmms
{


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
	element.setAttribute("off", startTimeOffset());
	element.setAttribute( "muted", isMuted() );
	element.setAttribute("autoresize", QString::number(getAutoResize()));
	if (const auto& c = color())
	{
		element.setAttribute("color", c->name());
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
	setAutoResize(element.attribute("autoresize", "1").toInt());
	setStartTimeOffset(element.attribute("off").toInt());
	if (static_cast<bool>(element.attribute("muted").toInt()) != isMuted())
	{
		toggleMute();
	}
	
	if (element.hasAttribute("color"))
	{
		const auto parseColor = [](const QString& raw) -> QColor
		{
			// first attempt - parse color from string (usually a hex color string)
			const auto att1 = QColor{raw};
			if (att1.isValid()) { return att1; }

			// the rest of this function is trying to parse an uint, and then a color from that
			bool ok = false;
			const auto asUint = raw.toUInt(&ok);
			if (!ok) { return QColor{}; } // return "invalid color"

			// hopefully the value here should be valid
			return QColor{asUint};
		};

		// Pre-1.3 projects used to sometimes have a "usestyle" attribute in pattern clips.
		//
		// We can only load the color if it does not exist, or its value is zero.
		//
		// TODO: explain properly what "usestyle" used to be in old versions
		if (!element.hasAttribute("usestyle") || element.attribute("usestyle").toUInt() == 0)
		{
			setColor(parseColor(element.attribute("color")));
		}
	}
}



int PatternClip::patternIndex()
{
	return dynamic_cast<PatternTrack*>(getTrack())->patternIndex();
}



gui::ClipView* PatternClip::createView(gui::TrackView* tv)
{
	return new gui::PatternClipView(this, tv);
}


} // namespace lmms
