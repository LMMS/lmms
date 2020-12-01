/*
 * BBTrack.h - class BBTrack, a wrapper for using bbEditor
 *              (which is a singleton-class) as track
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


#ifndef BB_TRACK_H
#define BB_TRACK_H

#include <QtCore/QObject>
#include <QtCore/QMap>
#include <QStaticText>

#include "Track.h"

class TrackLabelButton;
class TrackContainer;


class BBTCO : public TrackContentObject
{
public:
	BBTCO( Track * _track );
	virtual ~BBTCO() = default;

	void saveSettings( QDomDocument & _doc, QDomElement & _parent ) override;
	void loadSettings( const QDomElement & _this ) override;
	inline QString nodeName() const override
	{
		return( "bbtco" );
	}

	int bbTrackIndex();

	TrackContentObjectView * createView( TrackView * _tv ) override;

private:


	friend class BBTCOView;

} ;



class BBTCOView : public TrackContentObjectView
{
	Q_OBJECT
public:
	BBTCOView( TrackContentObject * _tco, TrackView * _tv );
	virtual ~BBTCOView() = default;


public slots:
	void update() override;

protected slots:
	void openInBBEditor();
	void resetName();
	void changeName();


protected:
	void paintEvent( QPaintEvent * pe ) override;
	void mouseDoubleClickEvent( QMouseEvent * _me ) override;
	void constructContextMenu( QMenu * ) override;


private:
	BBTCO * m_bbTCO;
	QPixmap m_paintPixmap;
	
	QStaticText m_staticTextName;
} ;




class LMMS_EXPORT BBTrack : public Track
{
	Q_OBJECT
public:
	BBTrack( TrackContainer* tc );
	virtual ~BBTrack();

	virtual bool play( const TimePos & _start, const fpp_t _frames,
						const f_cnt_t _frame_base, int _tco_num = -1 ) override;
	TrackView * createView( TrackContainerView* tcv ) override;
	TrackContentObject* createTCO(const TimePos & pos) override;

	virtual void saveTrackSpecificSettings( QDomDocument & _doc,
							QDomElement & _parent ) override;
	void loadTrackSpecificSettings( const QDomElement & _this ) override;

	static BBTrack * findBBTrack( int _bb_num );
	static void swapBBTracks( Track * _track1, Track * _track2 );

	int index()
	{
		return s_infoMap[this];
	}

	bool automationDisabled( Track * _track )
	{
		return( m_disabledTracks.contains( _track ) );
	}
	void disableAutomation( Track * _track )
	{
		m_disabledTracks.append( _track );
	}
	void enableAutomation( Track * _track )
	{
		m_disabledTracks.removeAll( _track );
	}

protected:
	inline QString nodeName() const override
	{
		return( "bbtrack" );
	}


private:
	QList<Track *> m_disabledTracks;

	typedef QMap<BBTrack *, int> infoMap;
	static infoMap s_infoMap;

	friend class BBTrackView;

} ;



class BBTrackView : public TrackView
{
	Q_OBJECT
public:
	BBTrackView( BBTrack* bbt, TrackContainerView* tcv );
	virtual ~BBTrackView();

	bool close() override;

	const BBTrack * getBBTrack() const
	{
		return( m_bbTrack );
	}


public slots:
	void clickedTrackLabel();


private:
	BBTrack * m_bbTrack;
	TrackLabelButton * m_trackLabel;

} ;



#endif
