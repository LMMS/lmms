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
	m_startCrossfadeLength(0),
	m_endCrossfadeLength(0),
	m_autoCrossfade{true},
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
	deleteCrossfades();
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
	updateCrossfades();
}


void Clip::updateCrossfades()
{
	if (m_autoCrossfade)
	{
		// If the left or right crossfade isn't valid anymore, remove them
		if (m_leftCrossfadeClip && (
			m_leftCrossfadeClip->endPosition() <= startPosition()
			|| m_leftCrossfadeClip->startPosition() >= startPosition()
			|| !m_leftCrossfadeClip->autoCrossfade()))
		{
			m_leftCrossfadeClip->setEndCrossfadeLength(0);
			m_leftCrossfadeClip->m_rightCrossfadeClip = nullptr;
			setStartCrossfadeLength(0);
			m_leftCrossfadeClip = nullptr;
		}
		if (m_rightCrossfadeClip && (
			m_rightCrossfadeClip->startPosition() >= endPosition()
			|| m_rightCrossfadeClip->endPosition() <= endPosition()
			|| !m_rightCrossfadeClip->autoCrossfade()))
		{
			m_rightCrossfadeClip->setStartCrossfadeLength(0);
			m_rightCrossfadeClip->m_leftCrossfadeClip = nullptr;
			setEndCrossfadeLength(0);
			m_rightCrossfadeClip = nullptr;
		}
		// If there isn't a left or right crossfade yet, add them
		if (!m_rightCrossfadeClip || !m_leftCrossfadeClip)
		{
			for (Clip* clip: m_track->getClips())
			{
				if (clip && clip != this && clip->autoCrossfade())
				{
					if (clip->startPosition() <= startPosition() && clip->endPosition() >= endPosition()) { continue; }
					if (clip->startPosition() >= startPosition() && clip->endPosition() <= endPosition()) { continue; }
					if (clip->startPosition() < endPosition() && clip->endPosition() > endPosition() && !m_rightCrossfadeClip && !clip->m_leftCrossfadeClip)
					{
						clip->m_leftCrossfadeClip = this;
						m_rightCrossfadeClip = clip;
					}
					else if (clip->endPosition() > startPosition() && clip->startPosition() < startPosition() && !m_leftCrossfadeClip && !clip->m_rightCrossfadeClip)
					{
						clip->m_rightCrossfadeClip = this;
						m_leftCrossfadeClip = clip;
					}
				}
			}
		}
		// If we do have a left/right crossfade, update the length.
		if (m_leftCrossfadeClip)
		{
			const TimePos overlap = m_leftCrossfadeClip->endPosition() - startPosition();
			m_leftCrossfadeClip->setEndCrossfadeLength(overlap);
			setStartCrossfadeLength(overlap);
			emit m_leftCrossfadeClip->crossfadesChanged();
		}
		if (m_rightCrossfadeClip)
		{
			const TimePos overlap = endPosition() - m_rightCrossfadeClip->startPosition();
			m_rightCrossfadeClip->setStartCrossfadeLength(overlap);
			setEndCrossfadeLength(overlap);
			emit m_rightCrossfadeClip->crossfadesChanged();
		}
	}
	emit crossfadesChanged();
}

void Clip::deleteCrossfades()
{
	if (m_leftCrossfadeClip)
	{
		m_leftCrossfadeClip->m_rightCrossfadeClip = nullptr;
		m_leftCrossfadeClip->setEndCrossfadeLength(0);
		m_leftCrossfadeClip = nullptr;
	}
	if (m_rightCrossfadeClip)
	{
		m_rightCrossfadeClip->m_leftCrossfadeClip = nullptr;
		m_leftCrossfadeClip->setStartCrossfadeLength(0);
		m_rightCrossfadeClip = nullptr;
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
	updateCrossfades();
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
