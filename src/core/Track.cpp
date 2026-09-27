/*
 * Track.cpp - implementation of Track class
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

/** \file Track.cpp
 *  \brief Implementation of Track class
 */


#include "Track.h"

#include <QDomElement>
#include <QVariant>

#include "AutomationClip.h"
#include "AutomationTrack.h"
#include "ConfigManager.h"
#include "Engine.h"
#include "InstrumentTrack.h"
#include "PatternStore.h"
#include "PatternTrack.h"
#include "SampleTrack.h"
#include "Song.h"


namespace lmms
{

Track::Track(Type type, TrackContainer* tc)
	: Model(tc)
	, m_trackContainer(tc)
	, m_type(type)
	, m_name()
	, m_mutedModel(false, this, tr("Mute"))
	, m_soloModel(false, this, tr("Solo"))
	, m_clips()
{	
	m_trackContainer->addTrack( this );
	m_height = -1;
}




Track::~Track()
{
	lock();
	emit destroyedTrack();

	while (!m_clips.empty())
	{
		delete m_clips.back();
	}

	m_trackContainer->removeTrack( this );
	unlock();
}




Track * Track::create( Type tt, TrackContainer * tc )
{
	Engine::audioEngine()->requestChangeInModel();

	Track * t = nullptr;

	switch( tt )
	{
		case Type::Instrument: t = new class InstrumentTrack( tc ); break;
		case Type::Pattern: t = new class PatternTrack( tc ); break;
		case Type::Sample: t = new class SampleTrack( tc ); break;
//		case Type::Event:
//		case Type::Video:
		case Type::Automation: t = new class AutomationTrack( tc ); break;
		case Type::HiddenAutomation:
						t = new class AutomationTrack( tc, true ); break;
		default: break;
	}

	if (tc == Engine::patternStore() && t)
	{
		t->createClipsForPattern(Engine::patternStore()->numOfPatterns() - 1);
	}

	tc->updateAfterTrackAdd();

	Engine::audioEngine()->doneChangeInModel();

	return t;
}




Track * Track::create( const QDomElement & element, TrackContainer * tc )
{
	Engine::audioEngine()->requestChangeInModel();

	Track * t = create(
		static_cast<Type>( element.attribute( "type" ).toInt() ),
									tc );
	if( t != nullptr )
	{
		t->restoreState( element );
	}

	Engine::audioEngine()->doneChangeInModel();

	return t;
}




Track* Track::clone()
{
	// Save track to temporary XML and load it to create a new identical track
	QDomDocument doc;
	QDomElement parent = doc.createElement("clonedtrack");
	saveState(doc, parent);
	Track* t = create(parent.firstChild().toElement(), m_trackContainer);

	AutomationClip::resolveAllIDs();
	return t;
}


void Track::saveTrack(QDomDocument& doc, QDomElement& element, bool presetMode)
{
	if (!presetMode)
	{
		element.setTagName( "track" );
	}
	element.setAttribute( "type", static_cast<int>(type()) );
	element.setAttribute( "name", name() );
	m_mutedModel.saveSettings( doc, element, "muted" );
	m_soloModel.saveSettings( doc, element, "solo" );
	// Save the mutedBeforeSolo value so we can recover the muted state if any solo was active (issue 5562)
	element.setAttribute( "mutedBeforeSolo", int(m_mutedBeforeSolo) );

	if( m_height >= MINIMAL_TRACK_HEIGHT )
	{
		element.setAttribute( "trackheight", m_height );
	}
	
	if (m_color.has_value())
	{
		element.setAttribute("color", m_color->name());
	}
	
	QDomElement tsDe = doc.createElement( nodeName() );
	// let actual track (InstrumentTrack, PatternTrack, SampleTrack etc.) save its settings
	element.appendChild( tsDe );
	saveTrackSpecificSettings(doc, tsDe, presetMode);

	if (presetMode)
	{
		return;
	}

	// now save settings of all Clip's
	for (const auto& clip : m_clips)
	{
		clip->saveState(doc, element);
	}
}


void Track::loadTrack(const QDomElement& element, bool presetMode)
{
	if( static_cast<Type>(element.attribute( "type" ).toInt()) != type() )
	{
		qWarning( "Current track-type does not match track-type of "
							"settings-node!\n" );
	}

	setName( element.hasAttribute( "name" ) ? element.attribute( "name" ) :
			element.firstChild().toElement().attribute( "name" ) );

	m_mutedModel.loadSettings( element, "muted" );
	m_soloModel.loadSettings( element, "solo" );
	// Get the mutedBeforeSolo value so we can recover the muted state if any solo was active.
	// Older project files that didn't have this attribute will set the value to false (issue 5562)
	m_mutedBeforeSolo = QVariant( element.attribute( "mutedBeforeSolo", "0" ) ).toBool();

	if (element.hasAttribute("color"))
	{
		setColor(QColor{element.attribute("color")});
	}

	if (presetMode)
	{
		QDomNode node = element.firstChild();
		while( !node.isNull() )
		{
			if( node.isElement() && node.nodeName() == nodeName() )
			{
				loadTrackSpecificSettings( node.toElement() );
				break;
			}
			node = node.nextSibling();
		}

		return;
	}

	{
		auto guard = Engine::audioEngine()->requestChangesGuard();
		deleteClips();
	}

	QDomNode node = element.firstChild();
	while( !node.isNull() )
	{
		if( node.isElement() )
		{
			if( node.nodeName() == nodeName() )
			{
				loadTrackSpecificSettings( node.toElement() );
			}
			else if( node.nodeName() != "muted"
			&& node.nodeName() != "solo"
			&& !node.toElement().attribute( "metadata" ).toInt() )
			{
				Clip * clip = createClip(
								TimePos( 0 ) );
				clip->restoreState( node.toElement() );
			}
		}
		node = node.nextSibling();
	}

	int storedHeight = element.attribute( "trackheight" ).toInt();
	if( storedHeight >= MINIMAL_TRACK_HEIGHT )
	{
		m_height = storedHeight;
	}
}

void Track::savePreset(QDomDocument & doc, QDomElement & element)
{
	saveTrack(doc, element, true);
}

void Track::loadPreset(const QDomElement & element)
{
	loadTrack(element, true);
}

void Track::saveSettings(QDomDocument& doc, QDomElement& element)
{
	// Assume that everything should be saved if we are called through SerializingObject::saveSettings
	saveTrack(doc, element, false);
}

void Track::loadSettings(const QDomElement& element)
{
	// Assume that everything should be loaded if we are called through SerializingObject::loadSettings 
	loadTrack(element, false);
}




Clip * Track::addClip( Clip * clip )
{
	m_clips.push_back( clip );

	emit clipAdded( clip );

	return clip; // just for convenience
}




void Track::removeClip( Clip * clip )
{
	clipVector::iterator it = std::find( m_clips.begin(), m_clips.end(), clip );
	if( it != m_clips.end() )
	{
		m_clips.erase( it );
		if( Engine::getSong() )
		{
			Engine::getSong()->updateLength();
			Engine::getSong()->setModified();
		}
	}
}


void Track::deleteClips()
{
	while (!m_clips.empty())
	{
		delete m_clips.front();
	}
}




auto Track::getClip(std::size_t clipNum) -> Clip*
{
	if( clipNum < m_clips.size() )
	{
		return m_clips[clipNum];
	}
	printf( "called Track::getClip( %zu ), "
			"but Clip %zu doesn't exist\n", clipNum, clipNum );
	return createClip( clipNum * TimePos::ticksPerBar() );

}




int Track::getClipNum( const Clip * clip )
{
//	for( int i = 0; i < getTrackContentWidget()->numOfClips(); ++i )
	clipVector::iterator it = std::find( m_clips.begin(), m_clips.end(), clip );
	if( it != m_clips.end() )
	{
/*		if( getClip( i ) == _clip )
		{
			return i;
		}*/
		return it - m_clips.begin();
	}
	qWarning( "Track::getClipNum(...) -> _clip not found!\n" );
	return 0;
}




void Track::getClipsInRange( clipVector & clipV, const TimePos & start,
							const TimePos & end )
{
	for( Clip* clip : m_clips )
	{
		int s = clip->startPosition();
		int e = clip->endPosition();
		if( ( s <= end ) && ( e >= start ) )
		{
			// Clip is within given range
			// Insert sorted by Clip's position
			clipV.insert(std::upper_bound(clipV.begin(), clipV.end(), clip, Clip::comparePosition),
						clip);
		}
	}
}




void Track::swapPositionOfClips( int clipNum1, int clipNum2 )
{
	qSwap( m_clips[clipNum1], m_clips[clipNum2] );

	const TimePos pos = m_clips[clipNum1]->startPosition();

	m_clips[clipNum1]->movePosition( m_clips[clipNum2]->startPosition() );
	m_clips[clipNum2]->movePosition( pos );
}




void Track::createClipsForPattern(int pattern)
{
	while( numOfClips() < pattern + 1 )
	{
		TimePos position = TimePos( numOfClips(), 0 );
		Clip * clip = createClip( position );
		clip->changeLength( TimePos( 1, 0 ) );
	}
}




//! @todo If we stepped through this list last to first, and the list was in ascending order by Clip time, once we hit
//! a Clip that was earlier than the insert time, we could fall out of the loop early.
void Track::insertBar( const TimePos & pos )
{
	// we'll increase the position of every Clip, positioned behind pos, by
	// one bar
	for (const auto& clip : m_clips)
	{
		if (clip->startPosition() >= pos)
		{
			clip->movePosition(clip->startPosition() + TimePos::ticksPerBar());
		}
	}
}




void Track::removeBar( const TimePos & pos )
{
	// we'll decrease the position of every Clip, positioned behind pos, by
	// one bar
	for (const auto& clip : m_clips)
	{
		if (clip->startPosition() >= pos)
		{
			clip->movePosition(clip->startPosition() - TimePos::ticksPerBar());
		}
	}
}




bar_t Track::length() const
{
	// find last end-position
	tick_t last = 0;
	for (const auto& clip : m_clips)
	{
		if (Engine::getSong()->isExporting() && clip->isMuted())
		{
			continue;
		}

		const tick_t cur = clip->endPosition();
		if( cur > last )
		{
			last = cur;
		}
	}

	return last / TimePos::ticksPerBar();
}



void Track::toggleSolo()
{
	const TrackContainer::TrackList & tl = m_trackContainer->tracks();

	bool soloBefore = false;
	for (const auto& track : tl)
	{
		if (track != this)
		{
			if (track->m_soloModel.value())
			{
				soloBefore = true;
				break;
			}
		}
	}

	const bool solo = m_soloModel.value();
	// Should we use the new behavior of solo or the older/legacy one?
	const bool soloLegacyBehavior = ConfigManager::inst()->value("app", "sololegacybehavior", "0").toInt();

	for (const auto& track : tl)
	{
		if (solo)
		{
			// save mute-state in case no track was solo before
			if (!soloBefore)
			{
				track->m_mutedBeforeSolo = track->isMuted();
			}
			// Don't mute AutomationTracks (keep their original state) unless we are on the sololegacybehavior mode
			if (track == this)
			{
				track->setMuted(false);
			}
			else if (soloLegacyBehavior || track->type() != Type::Automation)
			{
				track->setMuted(true);
			}
			if (track != this)
			{
				track->m_soloModel.setValue(false);
			}
		}
		else if (!soloBefore)
		{
			// Unless we are on the sololegacybehavior mode, only restores the
			// mute state if the track isn't an Automation Track
			if (soloLegacyBehavior || track->type() != Type::Automation)
			{
				track->setMuted(track->m_mutedBeforeSolo);
			}
		}
	}
}

void Track::setColor(const std::optional<QColor>& color)
{
	m_color = color;
	emit colorChanged();
}

BoolModel *Track::getMutedModel()
{
	return &m_mutedModel;
}

void Track::setName(const QString& newName)
{
	if (m_name != newName)
	{
		m_name = newName;

		if (auto song = Engine::getSong())
		{
			song->setModified();
		}
		
		emit nameChanged();
	}
}

} // namespace lmms
