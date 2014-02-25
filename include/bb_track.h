/*
 * bb_track.h - class bbTrack, a wrapper for using bbEditor
 *              (which is a singleton-class) as track
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef BB_TRACK_H
#define BB_TRACK_H

#include <QtCore/QObject>
#include <QtCore/QMap>

#include "track.h"

class trackLabelButton;
class TrackContainer;


class bbTCO : public trackContentObject
{
public:
	bbTCO( track * _track, unsigned int _color = 0 );
	virtual ~bbTCO();

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );
	inline virtual QString nodeName() const
	{
		return( "bbtco" );
	}

	inline unsigned int color() const
	{
		return( m_color );
	}
	inline static unsigned int defaultColor()
	{
		return qRgb( 128, 182, 175 );
	}

	virtual trackContentObjectView * createView( trackView * _tv );

private:
	unsigned int m_color;


	friend class bbTCOView;

} ;



class bbTCOView : public trackContentObjectView
{
	Q_OBJECT
public:
	bbTCOView( trackContentObject * _tco, trackView * _tv );
	virtual ~bbTCOView();

	QColor color() const
	{
		return( m_bbTCO->m_color );
	}
	void setColor( QColor _new_color );


protected slots:
	void openInBBEditor();
	void resetName();
	void changeName();
	void changeColor();


protected:
	void paintEvent( QPaintEvent * );
	void mouseDoubleClickEvent( QMouseEvent * _me );
	virtual void constructContextMenu( QMenu * );


private:
	bbTCO * m_bbTCO;

} ;




class EXPORT bbTrack : public track
{
	Q_OBJECT
public:
	bbTrack( TrackContainer* tc );
	virtual ~bbTrack();

	virtual bool play( const MidiTime & _start, const fpp_t _frames,
						const f_cnt_t _frame_base, int _tco_num = -1 );
	virtual trackView * createView( TrackContainerView* tcv );
	virtual trackContentObject * createTCO( const MidiTime & _pos );

	virtual void saveTrackSpecificSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void loadTrackSpecificSettings( const QDomElement & _this );

	static bbTrack * findBBTrack( int _bb_num );
	static int numOfBBTrack( track * _track );
	static void swapBBTracks( track * _track1, track * _track2 );

	bool automationDisabled( track * _track )
	{
		return( m_disabledTracks.contains( _track ) );
	}
	void disableAutomation( track * _track )
	{
		m_disabledTracks.append( _track );
	}
	void enableAutomation( track * _track )
	{
		m_disabledTracks.removeAll( _track );
	}


protected:
	inline virtual QString nodeName() const
	{
		return( "bbtrack" );
	}


private:
	QList<track *> m_disabledTracks;

	typedef QMap<bbTrack *, int> infoMap;
	static infoMap s_infoMap;


	friend class bbTrackView;

} ;



class bbTrackView : public trackView
{
	Q_OBJECT
public:
	bbTrackView( bbTrack* bbt, TrackContainerView* tcv );
	virtual ~bbTrackView();

	virtual bool close();

	const bbTrack * getBBTrack() const
	{
		return( m_bbTrack );
	}


public slots:
	void clickedTrackLabel();


private:
	bbTrack * m_bbTrack;
	trackLabelButton * m_trackLabel;

} ;



#endif
