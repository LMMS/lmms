/*
 * Track.h - declaration of Track class
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

#ifndef TRACK_H
#define TRACK_H


#include <QtCore/QVector>
#include <QColor>

#include "AutomatableModel.h"
#include "JournallingObject.h"
#include "lmms_basics.h"


class TimePos;
class TrackContainer;
class TrackContainerView;
class TrackContentObject;
class TrackView;


/*! The minimum track height in pixels
 *
 * Tracks can be resized by shift-dragging anywhere inside the track
 * display.  This sets the minimum size in pixels for a track.
 */
const int MINIMAL_TRACK_HEIGHT = 32;
const int DEFAULT_TRACK_HEIGHT = 32;

char const *const FILENAME_FILTER = "[\\0000-\x1f\"*/:<>?\\\\|\x7f]";


//! Base-class for all tracks
class LMMS_EXPORT Track : public Model, public JournallingObject
{
	Q_OBJECT
	MM_OPERATORS
	mapPropertyFromModel(bool,isMuted,setMuted,m_mutedModel);
	mapPropertyFromModel(bool,isSolo,setSolo,m_soloModel);
public:
	typedef QVector<TrackContentObject *> tcoVector;

	enum TrackTypes
	{
		InstrumentTrack,
		BBTrack,
		SampleTrack,
		EventTrack,
		VideoTrack,
		AutomationTrack,
		HiddenAutomationTrack,
		VocalTrack,
		NumTrackTypes
	} ;

	Track( TrackTypes type, TrackContainer * tc );
	virtual ~Track();

	static Track * create( TrackTypes tt, TrackContainer * tc );
	static Track * create( const QDomElement & element,
							TrackContainer * tc );
	Track * clone();


	// pure virtual functions
	TrackTypes type() const
	{
		return m_type;
	}

	virtual bool play( const TimePos & start, const fpp_t frames,
						const f_cnt_t frameBase, int tcoNum = -1 ) = 0;


	virtual TrackView * createView( TrackContainerView * view ) = 0;
	virtual TrackContentObject * createTCO( const TimePos & pos ) = 0;

	virtual void saveTrackSpecificSettings( QDomDocument & doc,
						QDomElement & parent ) = 0;
	virtual void loadTrackSpecificSettings( const QDomElement & element ) = 0;


	void saveSettings( QDomDocument & doc, QDomElement & element ) override;
	void loadSettings( const QDomElement & element ) override;

	void setSimpleSerializing()
	{
		m_simpleSerializingMode = true;
	}

	// -- for usage by TrackContentObject only ---------------
	TrackContentObject * addTCO( TrackContentObject * tco );
	void removeTCO( TrackContentObject * tco );
	// -------------------------------------------------------
	void deleteTCOs();

	int numOfTCOs();
	TrackContentObject * getTCO( int tcoNum );
	int getTCONum(const TrackContentObject* tco );

	const tcoVector & getTCOs() const
	{
		return m_trackContentObjects;
	}
	void getTCOsInRange( tcoVector & tcoV, const TimePos & start,
							const TimePos & end );
	void swapPositionOfTCOs( int tcoNum1, int tcoNum2 );

	void createTCOsForBB( int bb );


	void insertBar( const TimePos & pos );
	void removeBar( const TimePos & pos );

	bar_t length() const;


	inline TrackContainer* trackContainer() const
	{
		return m_trackContainer;
	}

	// name-stuff
	virtual const QString & name() const
	{
		return m_name;
	}

	QString displayName() const override
	{
		return name();
	}

	using Model::dataChanged;

	inline int getHeight()
	{
		return m_height >= MINIMAL_TRACK_HEIGHT
			? m_height
			: DEFAULT_TRACK_HEIGHT;
	}
	inline void setHeight( int height )
	{
		m_height = height;
	}

	void lock()
	{
		m_processingLock.lock();
	}
	void unlock()
	{
		m_processingLock.unlock();
	}
	bool tryLock()
	{
		return m_processingLock.tryLock();
	}
	
	QColor color()
	{
		return m_color;
	}
	bool useColor()
	{
		return m_hasColor;
	}
	
	BoolModel* getMutedModel();

public slots:
	virtual void setName( const QString & newName )
	{
		m_name = newName;
		emit nameChanged();
	}

	void toggleSolo();

	void trackColorChanged( QColor & c );
	void trackColorReset();

private:
	TrackContainer* m_trackContainer;
	TrackTypes m_type;
	QString m_name;
	int m_height;

protected:
	BoolModel m_mutedModel;
private:
	BoolModel m_soloModel;
	bool m_mutedBeforeSolo;

	bool m_simpleSerializingMode;

	tcoVector m_trackContentObjects;

	QMutex m_processingLock;
	
	QColor m_color;
	bool m_hasColor;

	friend class TrackView;


signals:
	void destroyedTrack();
	void nameChanged();
	void trackContentObjectAdded( TrackContentObject * );

} ;



#endif
