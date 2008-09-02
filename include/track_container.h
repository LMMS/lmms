/*
 * track_container.h - base-class for all track-containers like Song-Editor,
 *                     BB-Editor...
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _TRACK_CONTAINER_H
#define _TRACK_CONTAINER_H

#include <QReadWriteLock>

#include "track.h"
#include "journalling_object.h"


class automationPattern;
class instrumentTrack;
class trackContainerView;


class EXPORT trackContainer : public model, public journallingObject
{
	Q_OBJECT
public:
	typedef QVector<track *> trackList;

	trackContainer( void );
	virtual ~trackContainer();

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );

	virtual void loadSettings( const QDomElement & _this );


	virtual automationPattern * tempoAutomationPattern( void )
	{
		return( NULL );
	}

	int countTracks( track::TrackTypes _tt = track::NumTrackTypes ) const;


	virtual void updateAfterTrackAdd( void );
	void addTrack( track * _track );
	void removeTrack( track * _track );

	void clearAllTracks( void );

	/*
	trackList & tracks( void )
	{
		return( m_tracks );
	}
	*/

	const trackList & tracks( void ) const
	{
		return( m_tracks );
	}

	static const QString classNodeName( void )
	{
		return( "trackcontainer" );
	}


signals:
	void trackAdded( track * _track );

protected:
	mutable QReadWriteLock m_tracksMutex;

private:
	trackList m_tracks;


	friend class trackContainerView;
	friend class track;

} ;


class dummyTrackContainer : public trackContainer
{
public:
	dummyTrackContainer( void );

	virtual ~dummyTrackContainer()
	{
	}

	virtual QString nodeName( void ) const
	{
		return( "dummytrackcontainer" );
	}

	instrumentTrack * dummyInstrumentTrack( void )
	{
		return( m_dummyInstrumentTrack );
	}


private:
	instrumentTrack * m_dummyInstrumentTrack;

} ;


#endif
