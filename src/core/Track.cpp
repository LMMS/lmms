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

/*! \brief Create a new (empty) track object
 *
 *  The track object is the whole track, linking its contents, its
 *  automation, name, type, and so forth.
 *
 * \param type The type of track (Song Editor or Pattern Editor)
 * \param tc The track Container object to encapsulate in this track.
 *
 * \todo check the definitions of all the properties - are they OK?
 */
Track::Track( Type type, TrackContainer * tc ) :
	Model( tc ),                   /*!< The track Model */
	m_trackContainer( tc ),        /*!< The track container object */
	m_type( type ),                /*!< The track type */
	m_name(),                       /*!< The track's name */
	m_mutedModel( false, this, tr( "Mute" ) ), /*!< For controlling track muting */
	m_soloModel( false, this, tr( "Solo" ) ), /*!< For controlling track soloing */
	m_clips()        /*!< The clips (segments) */
{	
	m_trackContainer->addTrack( this );
	m_height = -1;
}




/*! \brief Destroy this track
 *
 *  Delete the clips and remove this track from the track container.
 */
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




/*! \brief Create a track based on the given track type and container.
 *
 *  \param tt The type of track to create
 *  \param tc The track container to attach to
 */
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




/*! \brief Create a track inside TrackContainer from track type in a QDomElement and restore state from XML
 *
 *  \param element The QDomElement containing the type of track to create
 *  \param tc The track container to attach to
 */
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




/*! \brief Clone a track from this track
 *
 */
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


/*! \brief Save this track's settings to file
 *
 *  We save the track type and its muted state and solo state, then append the track-
 *  specific settings.  Then we iterate through the clips
 *  and save all their states in turn.
 *
 *  \param doc The QDomDocument to use to save
 *  \param element The The QDomElement to save into
 *  \param presetMode Describes whether to save the track as a preset or not.
 *  \todo Does this accurately describe the parameters?  I think not!?
 *  \todo Save the track height
 */
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

/*! \brief Load the settings from a file
 *
 *  We load the track's type and muted state and solo state, then clear out our
 *  current Clip.
 *
 *  Then we step through the QDomElement's children and load the
 *  track-specific settings and clip states from it
 *  one at a time.
 *
 *  \param element the QDomElement to load track settings from
 *  \param presetMode Indicates if a preset or a full track is loaded
 *  \todo Load the track height.
 */
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




/*! \brief Add another Clip into this track
 *
 *  \param clip The Clip to attach to this track.
 */
Clip * Track::addClip( Clip * clip )
{
	m_clips.push_back( clip );

	emit clipAdded( clip );

	return clip; // just for convenience
}




/*! \brief Remove a given Clip from this track
 *
 *  \param clip The Clip to remove from this track.
 */
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


/*! \brief Remove all Clips from this track */
void Track::deleteClips()
{
	while (!m_clips.empty())
	{
		delete m_clips.front();
	}
}


/*! \brief Return the number of clips we contain
 *
 *  \return the number of clips we currently contain.
 */
int Track::numOfClips()
{
	return m_clips.size();
}




/*! \brief Get a Clip by number
 *
 *  If the Clip number is less than our Clip array size then fetch that
 *  numbered object from the array.  Otherwise we warn the user that
 *  we've somehow requested a Clip that is too large, and create a new
 *  Clip for them.
 *  \param clipNum The number of the Clip to fetch.
 *  \return the given Clip or a new one if out of range.
 *  \todo reject Clip numbers less than zero.
 *  \todo if we create a Clip here, should we somehow attach it to the
 *     track?
 */
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




/*! \brief Determine the given Clip's number in our array.
 *
 *  \param clip The Clip to search for.
 *  \return its number in our array.
 */
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




/*! \brief Retrieve a list of clips that fall within a period.
 *
 *  Here we're interested in a range of clips that intersect
 *  the given time period.
 *
 *  We return the Clips we find in order by time, earliest Clips first.
 *
 *  \param clipV The list to contain the found clips.
 *  \param start The MIDI start time of the range.
 *  \param end   The MIDI endi time of the range.
 */
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




/*! \brief Swap the position of two clips.
 *
 *  First, we arrange to swap the positions of the two Clips in the
 *  clips list.  Then we swap their start times as well.
 *
 *  \param clipNum1 The first Clip to swap.
 *  \param clipNum2 The second Clip to swap.
 */
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




/*! \brief Move all the clips after a certain time later by one bar.
 *
 *  \param pos The time at which we want to insert the bar.
 *  \todo if we stepped through this list last to first, and the list was
 *    in ascending order by Clip time, once we hit a Clip that was earlier
 *    than the insert time, we could fall out of the loop early.
 */
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




/*! \brief Move all the clips after a certain time earlier by one bar.
 *
 *  \param pos The time at which we want to remove the bar.
 */
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




/*! \brief Return the length of the entire track in bars
 *
 *  We step through our list of Clips and determine their end position,
 *  keeping track of the latest time found in ticks.  Then we return
 *  that in bars by dividing by the number of ticks per bar.
 */
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



/*! \brief Invert the track's solo state.
 *
 *  We have to go through all the tracks determining if any other track
 *  is already soloed.  Then we have to save the mute state of all tracks,
 *  and set our mute state to on and all the others to off.
 */
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
