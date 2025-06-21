/*
 * Clip.cpp - implementation of Clip class
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

#include "Clip.h"

#include <QDomDocument>

#include "AutomationEditor.h"
#include "AutomationClip.h"
#include "Engine.h"
#include "GuiApplication.h"
#include "Song.h"
#include "Track.h"
#include "TrackContainer.h"


namespace lmms
{

/*! \brief Create a new Clip
 *
 *  Creates a new clip for the given track.
 *
 * \param _track The track that will contain the new object
 */
Clip::Clip( Track * track ) :
	Model( track ),
	m_track( track ),
	m_startPosition(),
	m_length(),
	m_mutedModel( false, this, tr( "Mute" ) ),
	m_selectViewOnCreate{false}
{
	if( getTrack() )
	{
		getTrack()->addClip( this );
	}
	setJournalling( false );
	movePosition( 0 );
	changeLength( 0 );
	setJournalling( true );
}


/*! \brief Copy a Clip
 *
 *  Creates a duplicate clip of the one provided.
 *
 * \param other The clip object which will be copied.
 */
Clip::Clip(const Clip& other):
	Model(other.m_track),
	m_track(other.m_track),
	m_name(other.m_name),
	m_startPosition(other.m_startPosition),
	m_length(other.m_length),
	m_startTimeOffset(other.m_startTimeOffset),
	m_mutedModel(other.m_mutedModel.value(), this, tr( "Mute" )),
	m_autoResize(other.m_autoResize),
	m_selectViewOnCreate{other.m_selectViewOnCreate},
	m_color(other.m_color)
{
	if (getTrack())
	{
		getTrack()->addClip(this);
	}
}

/*! \brief Destroy a Clip
 *
 *  Destroys the given clip.
 *
 */
Clip::~Clip()
{
	emit destroyedClip();

	if( getTrack() )
	{
		getTrack()->removeClip( this );
	}
}




/*! \brief Move this Clip's position in time
 *
 *  If the clip has moved, update its position.  We
 *  also add a journal entry for undo and update the display.
 *
 * \param _pos The new position of the clip.
 */
void Clip::movePosition( const TimePos & pos )
{
	TimePos newPos = std::max(0, pos.getTicks());
	if (m_startPosition != newPos)
	{
		Engine::audioEngine()->requestChangeInModel();
		m_startPosition = newPos;
		Engine::audioEngine()->doneChangeInModel();
		Engine::getSong()->updateLength();
		emit positionChanged();
	}
}




/*! \brief Change the length of this Clip
 *
 *  If the clip's length has changed, update it.  We
 *  also add a journal entry for undo and update the display.
 *
 * \param _length The new length of the clip.
 */
void Clip::changeLength( const TimePos & length )
{
	m_length = length;
	Engine::getSong()->updateLength();
	emit lengthChanged();
}




bool Clip::comparePosition(const Clip *a, const Clip *b)
{
	return a->startPosition() < b->startPosition();
}




/*! \brief Copies the state of a Clip to another Clip
 *
 *  This method copies the state of a Clip to another Clip
 */
void Clip::copyStateTo( Clip *src, Clip *dst )
{
	// If the node names match we copy the state
	if( src->nodeName() == dst->nodeName() ){
		QDomDocument doc;
		QDomElement parent = doc.createElement( "StateCopy" );
		src->saveState( doc, parent );

		const TimePos pos = dst->startPosition();
		dst->restoreState( parent.firstChild().toElement() );
		dst->movePosition( pos );

		AutomationClip::resolveAllIDs();
		gui::getGUI()->automationEditor()->m_editor->updateAfterClipChange();
	}
}

bool Clip::hasTrackContainer() const
{
	return getTrack() != nullptr && getTrack()->trackContainer() != nullptr;
}

bool Clip::isInPattern() const
{
	return hasTrackContainer()
		&& getTrack()->trackContainer()->type() == TrackContainer::Type::Pattern;
}

bool Clip::manuallyResizable() const
{
	return !isInPattern();
}



/*! \brief Mutes this Clip
 *
 *  Restore the previous state of this clip. This will
 *  restore the position or the length of the clip
 *  depending on what was changed.
 *
 * \param _je The journal entry to undo
 */
void Clip::toggleMute()
{
	m_mutedModel.setValue( !m_mutedModel.value() );
	emit dataChanged();
}




TimePos Clip::startTimeOffset() const
{
	return m_startTimeOffset;
}




void Clip::setStartTimeOffset( const TimePos &startTimeOffset )
{
	m_startTimeOffset = startTimeOffset;
}

void Clip::setColor(const std::optional<QColor>& color)
{
	m_color = color;
	emit colorChanged();
}

} // namespace lmms
