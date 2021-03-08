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

#include <QVariant>
#include <VocalInstrumentTrack.h>

#include "AutomationPattern.h"
#include "AutomationTrack.h"
#include "BBTrack.h"
#include "BBTrackContainer.h"
#include "ConfigManager.h"
#include "Engine.h"
#include "InstrumentTrack.h"
#include "SampleTrack.h"
#include "Song.h"


/*! \brief Create a new (empty) track object
 *
 *  The track object is the whole track, linking its contents, its
 *  automation, name, type, and so forth.
 *
 * \param type The type of track (Song Editor or Beat+Bassline Editor)
 * \param tc The track Container object to encapsulate in this track.
 *
 * \todo check the definitions of all the properties - are they OK?
 */
Track::Track( TrackTypes type, TrackContainer * tc ) :
	Model( tc ),                   /*!< The track Model */
	m_trackContainer( tc ),        /*!< The track container object */
	m_type( type ),                /*!< The track type */
	m_name(),                       /*!< The track's name */
	m_mutedModel( false, this, tr( "Mute" ) ), /*!< For controlling track muting */
	m_soloModel( false, this, tr( "Solo" ) ), /*!< For controlling track soloing */
	m_simpleSerializingMode( false ),
	m_trackContentObjects(),        /*!< The track content objects (segments) */
	m_color( 0, 0, 0 ),
	m_hasColor( false )
{
	m_trackContainer->addTrack( this );
	m_height = -1;
}




/*! \brief Destroy this track
 *
 *  If the track container is a Beat+Bassline container, step through
 *  its list of tracks and remove us.
 *
 *  Then delete the TrackContentObject's contents, remove this track from
 *  the track container.
 *
 *  Finally step through this track's automation and forget all of them.
 */
Track::~Track()
{
	lock();
	emit destroyedTrack();

	while( !m_trackContentObjects.isEmpty() )
	{
		delete m_trackContentObjects.last();
	}

	m_trackContainer->removeTrack( this );
	unlock();
}




/*! \brief Create a track based on the given track type and container.
 *
 *  \param tt The type of track to create
 *  \param tc The track container to attach to
 */
Track * Track::create( TrackTypes tt, TrackContainer * tc )
{
	Engine::mixer()->requestChangeInModel();

	Track * t = NULL;

	switch( tt )
	{
		case InstrumentTrack: t = new ::InstrumentTrack( tc ); break;
		case BBTrack: t = new ::BBTrack( tc ); break;
		case SampleTrack: t = new ::SampleTrack( tc ); break;
//		case EVENT_TRACK:
//		case VIDEO_TRACK:
		case AutomationTrack: t = new ::AutomationTrack( tc ); break;
		case HiddenAutomationTrack:
						t = new ::AutomationTrack( tc, true ); break;
		case VocalTrack:
						t = new VocalInstrumentTrack(tc); break;
		default: break;
	}

	if( tc == Engine::getBBTrackContainer() && t )
	{
		t->createTCOsForBB( Engine::getBBTrackContainer()->numOfBBs()
									- 1 );
	}

	tc->updateAfterTrackAdd();

	Engine::mixer()->doneChangeInModel();

	return t;
}




/*! \brief Create a track inside TrackContainer from track type in a QDomElement and restore state from XML
 *
 *  \param element The QDomElement containing the type of track to create
 *  \param tc The track container to attach to
 */
Track * Track::create( const QDomElement & element, TrackContainer * tc )
{
	Engine::mixer()->requestChangeInModel();

	Track * t = create(
		static_cast<TrackTypes>( element.attribute( "type" ).toInt() ),
									tc );
	if( t != NULL )
	{
		t->restoreState( element );
	}

	Engine::mixer()->doneChangeInModel();

	return t;
}




/*! \brief Clone a track from this track
 *
 */
Track* Track::clone()
{
	QDomDocument doc;
	QDomElement parent = doc.createElement("clone");
	saveState(doc, parent);
	Track* t = create(parent.firstChild().toElement(), m_trackContainer);

	AutomationPattern::resolveAllIDs();
	return t;
}






/*! \brief Save this track's settings to file
 *
 *  We save the track type and its muted state and solo state, then append the track-
 *  specific settings.  Then we iterate through the trackContentObjects
 *  and save all their states in turn.
 *
 *  \param doc The QDomDocument to use to save
 *  \param element The The QDomElement to save into
 *  \todo Does this accurately describe the parameters?  I think not!?
 *  \todo Save the track height
 */
void Track::saveSettings( QDomDocument & doc, QDomElement & element )
{
	if( !m_simpleSerializingMode )
	{
		element.setTagName( "track" );
	}
	element.setAttribute( "type", type() );
	element.setAttribute( "name", name() );
	m_mutedModel.saveSettings( doc, element, "muted" );
	m_soloModel.saveSettings( doc, element, "solo" );
	// Save the mutedBeforeSolo value so we can recover the muted state if any solo was active (issue 5562)
	element.setAttribute( "mutedBeforeSolo", int(m_mutedBeforeSolo) );

	if( m_height >= MINIMAL_TRACK_HEIGHT )
	{
		element.setAttribute( "trackheight", m_height );
	}
	
	if( m_hasColor )
	{
		element.setAttribute( "color", m_color.name() );
	}
	
	QDomElement tsDe = doc.createElement( nodeName() );
	// let actual track (InstrumentTrack, bbTrack, sampleTrack etc.) save
	// its settings
	element.appendChild( tsDe );
	saveTrackSpecificSettings( doc, tsDe );

	if( m_simpleSerializingMode )
	{
		m_simpleSerializingMode = false;
		return;
	}

	// now save settings of all TCO's
	for( tcoVector::const_iterator it = m_trackContentObjects.begin();
				it != m_trackContentObjects.end(); ++it )
	{
		( *it )->saveState( doc, element );
	}
}




/*! \brief Load the settings from a file
 *
 *  We load the track's type and muted state and solo state, then clear out our
 *  current TrackContentObject.
 *
 *  Then we step through the QDomElement's children and load the
 *  track-specific settings and trackContentObjects states from it
 *  one at a time.
 *
 *  \param element the QDomElement to load track settings from
 *  \todo Load the track height.
 */
void Track::loadSettings( const QDomElement & element )
{
	if( element.attribute( "type" ).toInt() != type() )
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

	if( element.hasAttribute( "color" ) )
	{
		m_color.setNamedColor( element.attribute( "color" ) );
		m_hasColor = true;
	}

	if( m_simpleSerializingMode )
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
		m_simpleSerializingMode = false;
		return;
	}

	while( !m_trackContentObjects.empty() )
	{
		delete m_trackContentObjects.front();
//		m_trackContentObjects.erase( m_trackContentObjects.begin() );
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
				TrackContentObject * tco = createTCO(
								TimePos( 0 ) );
				tco->restoreState( node.toElement() );
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




/*! \brief Add another TrackContentObject into this track
 *
 *  \param tco The TrackContentObject to attach to this track.
 */
TrackContentObject * Track::addTCO( TrackContentObject * tco )
{
	m_trackContentObjects.push_back( tco );

	emit trackContentObjectAdded( tco );

	return tco;		// just for convenience
}




/*! \brief Remove a given TrackContentObject from this track
 *
 *  \param tco The TrackContentObject to remove from this track.
 */
void Track::removeTCO( TrackContentObject * tco )
{
	tcoVector::iterator it = std::find( m_trackContentObjects.begin(),
					m_trackContentObjects.end(),
					tco );
	if( it != m_trackContentObjects.end() )
	{
		m_trackContentObjects.erase( it );
		if( Engine::getSong() )
		{
			Engine::getSong()->updateLength();
			Engine::getSong()->setModified();
		}
	}
}


/*! \brief Remove all TCOs from this track */
void Track::deleteTCOs()
{
	while( ! m_trackContentObjects.isEmpty() )
	{
		delete m_trackContentObjects.first();
	}
}


/*! \brief Return the number of trackContentObjects we contain
 *
 *  \return the number of trackContentObjects we currently contain.
 */
int Track::numOfTCOs()
{
	return m_trackContentObjects.size();
}




/*! \brief Get a TrackContentObject by number
 *
 *  If the TCO number is less than our TCO array size then fetch that
 *  numbered object from the array.  Otherwise we warn the user that
 *  we've somehow requested a TCO that is too large, and create a new
 *  TCO for them.
 *  \param tcoNum The number of the TrackContentObject to fetch.
 *  \return the given TrackContentObject or a new one if out of range.
 *  \todo reject TCO numbers less than zero.
 *  \todo if we create a TCO here, should we somehow attach it to the
 *     track?
 */
TrackContentObject * Track::getTCO( int tcoNum )
{
	if( tcoNum < m_trackContentObjects.size() )
	{
		return m_trackContentObjects[tcoNum];
	}
	printf( "called Track::getTCO( %d ), "
			"but TCO %d doesn't exist\n", tcoNum, tcoNum );
	return createTCO( tcoNum * TimePos::ticksPerBar() );

}




/*! \brief Determine the given TrackContentObject's number in our array.
 *
 *  \param tco The TrackContentObject to search for.
 *  \return its number in our array.
 */
int Track::getTCONum( const TrackContentObject * tco )
{
//	for( int i = 0; i < getTrackContentWidget()->numOfTCOs(); ++i )
	tcoVector::iterator it = std::find( m_trackContentObjects.begin(),
					m_trackContentObjects.end(),
					tco );
	if( it != m_trackContentObjects.end() )
	{
/*		if( getTCO( i ) == _tco )
		{
			return i;
		}*/
		return it - m_trackContentObjects.begin();
	}
	qWarning( "Track::getTCONum(...) -> _tco not found!\n" );
	return 0;
}




/*! \brief Retrieve a list of trackContentObjects that fall within a period.
 *
 *  Here we're interested in a range of trackContentObjects that intersect
 *  the given time period.
 *
 *  We return the TCOs we find in order by time, earliest TCOs first.
 *
 *  \param tcoV The list to contain the found trackContentObjects.
 *  \param start The MIDI start time of the range.
 *  \param end   The MIDI endi time of the range.
 */
void Track::getTCOsInRange( tcoVector & tcoV, const TimePos & start,
							const TimePos & end )
{
	for( TrackContentObject* tco : m_trackContentObjects )
	{
		int s = tco->startPosition();
		int e = tco->endPosition();
		if( ( s <= end ) && ( e >= start ) )
		{
			// TCO is within given range
			// Insert sorted by TCO's position
			tcoV.insert(std::upper_bound(tcoV.begin(), tcoV.end(), tco, TrackContentObject::comparePosition),
						tco);
		}
	}
}




/*! \brief Swap the position of two trackContentObjects.
 *
 *  First, we arrange to swap the positions of the two TCOs in the
 *  trackContentObjects list.  Then we swap their start times as well.
 *
 *  \param tcoNum1 The first TrackContentObject to swap.
 *  \param tcoNum2 The second TrackContentObject to swap.
 */
void Track::swapPositionOfTCOs( int tcoNum1, int tcoNum2 )
{
	qSwap( m_trackContentObjects[tcoNum1],
					m_trackContentObjects[tcoNum2] );

	const TimePos pos = m_trackContentObjects[tcoNum1]->startPosition();

	m_trackContentObjects[tcoNum1]->movePosition(
			m_trackContentObjects[tcoNum2]->startPosition() );
	m_trackContentObjects[tcoNum2]->movePosition( pos );
}




void Track::createTCOsForBB( int bb )
{
	while( numOfTCOs() < bb + 1 )
	{
		TimePos position = TimePos( numOfTCOs(), 0 );
		TrackContentObject * tco = createTCO( position );
		tco->changeLength( TimePos( 1, 0 ) );
	}
}




/*! \brief Move all the trackContentObjects after a certain time later by one bar.
 *
 *  \param pos The time at which we want to insert the bar.
 *  \todo if we stepped through this list last to first, and the list was
 *    in ascending order by TCO time, once we hit a TCO that was earlier
 *    than the insert time, we could fall out of the loop early.
 */
void Track::insertBar( const TimePos & pos )
{
	// we'll increase the position of every TCO, positioned behind pos, by
	// one bar
	for( tcoVector::iterator it = m_trackContentObjects.begin();
				it != m_trackContentObjects.end(); ++it )
	{
		if( ( *it )->startPosition() >= pos )
		{
			( *it )->movePosition( (*it)->startPosition() +
						TimePos::ticksPerBar() );
		}
	}
}




/*! \brief Move all the trackContentObjects after a certain time earlier by one bar.
 *
 *  \param pos The time at which we want to remove the bar.
 */
void Track::removeBar( const TimePos & pos )
{
	// we'll decrease the position of every TCO, positioned behind pos, by
	// one bar
	for( tcoVector::iterator it = m_trackContentObjects.begin();
				it != m_trackContentObjects.end(); ++it )
	{
		if( ( *it )->startPosition() >= pos )
		{
			(*it)->movePosition((*it)->startPosition() - TimePos::ticksPerBar());
		}
	}
}




/*! \brief Return the length of the entire track in bars
 *
 *  We step through our list of TCOs and determine their end position,
 *  keeping track of the latest time found in ticks.  Then we return
 *  that in bars by dividing by the number of ticks per bar.
 */
bar_t Track::length() const
{
	// find last end-position
	tick_t last = 0;
	for( tcoVector::const_iterator it = m_trackContentObjects.begin();
				it != m_trackContentObjects.end(); ++it )
	{
		if( Engine::getSong()->isExporting() &&
				( *it )->isMuted() )
		{
			continue;
		}

		const tick_t cur = ( *it )->endPosition();
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
	for( TrackContainer::TrackList::const_iterator it = tl.begin();
							it != tl.end(); ++it )
	{
		if( *it != this )
		{
			if( ( *it )->m_soloModel.value() )
			{
				soloBefore = true;
				break;
			}
		}
	}

	const bool solo = m_soloModel.value();
	// Should we use the new behavior of solo or the older/legacy one?
	const bool soloLegacyBehavior = ConfigManager::inst()->value("app", "sololegacybehavior", "0").toInt();

	for( TrackContainer::TrackList::const_iterator it = tl.begin();
							it != tl.end(); ++it )
	{
		if( solo )
		{
			// save mute-state in case no track was solo before
			if( !soloBefore )
			{
				( *it )->m_mutedBeforeSolo = ( *it )->isMuted();
			}
			// Don't mute AutomationTracks (keep their original state) unless we are on the sololegacybehavior mode
			if( *it == this )
			{
				( *it )->setMuted( false );
			}
			else if( soloLegacyBehavior || ( *it )->type() != AutomationTrack )
			{
				( *it )->setMuted( true );
			}
			if( *it != this )
			{
				( *it )->m_soloModel.setValue( false );
			}
		}
		else if( !soloBefore )
		{
			// Unless we are on the sololegacybehavior mode, only restores the
			// mute state if the track isn't an Automation Track
			if( soloLegacyBehavior || ( *it )->type() != AutomationTrack )
			{
				( *it )->setMuted( ( *it )->m_mutedBeforeSolo );
			}
		}
	}
}

void Track::trackColorChanged( QColor & c )
{
	for (int i = 0; i < numOfTCOs(); i++)
	{
		m_trackContentObjects[i]->updateColor();
	}
	m_hasColor = true;
	m_color = c;
}

void Track::trackColorReset()
{
	for (int i = 0; i < numOfTCOs(); i++)
	{
		m_trackContentObjects[i]->updateColor();
	}
	m_hasColor = false;
}


BoolModel *Track::getMutedModel()
{
	return &m_mutedModel;
}

