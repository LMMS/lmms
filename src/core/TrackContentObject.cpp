/*
 * TrackContentObject.cpp - implementation of TrackContentObject class
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

#include "TrackContentObject.h"

#include <QDomDocument>

#include "AutomationEditor.h"
#include "AutomationPattern.h"
#include "Engine.h"
#include "GuiApplication.h"
#include "Song.h"


/*! \brief Create a new TrackContentObject
 *
 *  Creates a new track content object for the given track.
 *
 * \param _track The track that will contain the new object
 */
TrackContentObject::TrackContentObject( Track * track ) :
	Model( track ),
	m_track( track ),
	m_startPosition(),
	m_length(),
	m_mutedModel( false, this, tr( "Mute" ) ),
	m_selectViewOnCreate( false ),
	m_color( 128, 128, 128 ),
	m_useCustomClipColor( false )
{
	if( getTrack() )
	{
		getTrack()->addTCO( this );
	}
	setJournalling( false );
	movePosition( 0 );
	changeLength( 0 );
	setJournalling( true );
}




/*! \brief Destroy a TrackContentObject
 *
 *  Destroys the given track content object.
 *
 */
TrackContentObject::~TrackContentObject()
{
	emit destroyedTCO();

	if( getTrack() )
	{
		getTrack()->removeTCO( this );
	}
}




/*! \brief Move this TrackContentObject's position in time
 *
 *  If the track content object has moved, update its position.  We
 *  also add a journal entry for undo and update the display.
 *
 * \param _pos The new position of the track content object.
 */
void TrackContentObject::movePosition( const TimePos & pos )
{
	TimePos newPos = qMax(0, pos.getTicks());
	if (m_startPosition != newPos)
	{
		Engine::mixer()->requestChangeInModel();
		m_startPosition = newPos;
		Engine::mixer()->doneChangeInModel();
		Engine::getSong()->updateLength();
		emit positionChanged();
	}
}




/*! \brief Change the length of this TrackContentObject
 *
 *  If the track content object's length has changed, update it.  We
 *  also add a journal entry for undo and update the display.
 *
 * \param _length The new length of the track content object.
 */
void TrackContentObject::changeLength( const TimePos & length )
{
	m_length = length;
	Engine::getSong()->updateLength();
	emit lengthChanged();
}




bool TrackContentObject::comparePosition(const TrackContentObject *a, const TrackContentObject *b)
{
	return a->startPosition() < b->startPosition();
}




/*! \brief Copies the state of a TrackContentObject to another TrackContentObject
 *
 *  This method copies the state of a TCO to another TCO
 */
void TrackContentObject::copyStateTo( TrackContentObject *src, TrackContentObject *dst )
{
	// If the node names match we copy the state
	if( src->nodeName() == dst->nodeName() ){
		QDomDocument doc;
		QDomElement parent = doc.createElement( "StateCopy" );
		src->saveState( doc, parent );

		const TimePos pos = dst->startPosition();
		dst->restoreState( parent.firstChild().toElement() );
		dst->movePosition( pos );

		AutomationPattern::resolveAllIDs();
		GuiApplication::instance()->automationEditor()->m_editor->updateAfterPatternChange();
	}
}




/*! \brief Mutes this TrackContentObject
 *
 *  Restore the previous state of this track content object.  This will
 *  restore the position or the length of the track content object
 *  depending on what was changed.
 *
 * \param _je The journal entry to undo
 */
void TrackContentObject::toggleMute()
{
	m_mutedModel.setValue( !m_mutedModel.value() );
	emit dataChanged();
}




TimePos TrackContentObject::startTimeOffset() const
{
	return m_startTimeOffset;
}




void TrackContentObject::setStartTimeOffset( const TimePos &startTimeOffset )
{
	m_startTimeOffset = startTimeOffset;
}

// Update TCO color if it follows the track color
void TrackContentObject::updateColor()
{
	if( ! m_useCustomClipColor )
	{
		emit trackColorChanged();
	}
}


void TrackContentObject::useCustomClipColor( bool b )
{
	m_useCustomClipColor = b;
	updateColor();
}


bool TrackContentObject::hasColor()
{
	return usesCustomClipColor() || getTrack()->useColor();
}

