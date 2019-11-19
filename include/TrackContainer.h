/*
 * TrackContainer.h - base-class for all track-containers like Song-Editor,
 *                    BB-Editor...
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef TRACK_CONTAINER_H
#define TRACK_CONTAINER_H

#include <QtCore/QReadWriteLock>

#include "Track.h"
#include "JournallingObject.h"


class AutomationPattern;
class InstrumentTrack;
class TrackContainerView;


class LMMS_EXPORT TrackContainer : public Model, public JournallingObject
{
	Q_OBJECT
public:
	typedef QVector<Track *> TrackList;
	enum TrackContainerTypes
	{
		BBContainer,
		SongContainer
	} ;

	TrackContainer();
	virtual ~TrackContainer();

	void saveSettings( QDomDocument & _doc, QDomElement & _parent ) override;

	void loadSettings( const QDomElement & _this ) override;


	virtual AutomationPattern * tempoAutomationPattern()
	{
		return NULL;
	}

	int countTracks( Track::TrackTypes _tt = Track::NumTrackTypes ) const;


	void addTrack( Track * _track );
	void removeTrack( Track * _track );

	virtual void updateAfterTrackAdd();

	void clearAllTracks();

	const TrackList & tracks() const
	{
		return m_tracks;
	}

	bool isEmpty() const;

	static const QString classNodeName()
	{
		return "trackcontainer";
	}

	inline void setType( TrackContainerTypes newType )
	{
		m_TrackContainerType = newType;
	}

	inline TrackContainerTypes type() const
	{
		return m_TrackContainerType;
	}

	virtual AutomatedValueMap automatedValuesAt(MidiTime time, int tcoNum = -1) const;

signals:
	void trackAdded( Track * _track );

protected:
	static AutomatedValueMap automatedValuesFromTracks(const TrackList &tracks, MidiTime timeStart, int tcoNum = -1);

	mutable QReadWriteLock m_tracksMutex;

private:
	TrackList m_tracks;

	TrackContainerTypes m_TrackContainerType;


	friend class TrackContainerView;
	friend class Track;

} ;


class DummyTrackContainer : public TrackContainer
{
public:
	DummyTrackContainer();

	virtual ~DummyTrackContainer()
	{
	}

	QString nodeName() const override
	{
		return "DummyTrackContainer";
	}

	InstrumentTrack * dummyInstrumentTrack()
	{
		return m_dummyInstrumentTrack;
	}


private:
	InstrumentTrack * m_dummyInstrumentTrack;

} ;


#endif
