/*
 * bb_track.h - class bbTrack, a wrapper for using bbEditor
 *              (which is a singleton-class) as track
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#ifndef _BB_TRACK_H
#define _BB_TRACK_H

#include "qt3support.h"

#ifdef QT4

#include <QObject>
#include <QMap>

#else

#include <qobject.h>
#include <qmap.h>

#endif


#include "track.h"

class nameLabel;
class trackContainer;


class bbTCO : public trackContentObject
{
	Q_OBJECT
public:
	bbTCO( track * _track, const QColor & _c = QColor() );
	virtual ~bbTCO();

	virtual void FASTCALL saveSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void FASTCALL loadSettings( const QDomElement & _this );
	inline virtual QString nodeName( void ) const
	{
		return( "bbtco" );
	}

	const QColor & color( void ) const
	{
		return( m_color );
	}


protected slots:
	void openInBBEditor( bool _c );
	void openInBBEditor( void );
	void resetName( void );
	void changeName( void );
	void changeColor( void );


protected:
	void paintEvent( QPaintEvent * );
	void mouseDoubleClickEvent( QMouseEvent * _me );
	virtual void constructContextMenu( QMenu * );


private:
	QString m_name;
	QColor m_color;

} ;



class bbTrack : public QObject, public track
{
	Q_OBJECT
public:
	bbTrack( trackContainer * _tc );
	virtual ~bbTrack();

	virtual trackTypes type( void ) const;
	virtual bool FASTCALL play( const midiTime & _start,
					Uint32 _start_frame, Uint32 _frames,
					Uint32 _frame_base,
							Sint16 _tco_num = -1 );
	virtual trackContentObject * FASTCALL createTCO( const midiTime &
									_pos );

	virtual void FASTCALL saveTrackSpecificSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void FASTCALL loadTrackSpecificSettings( const QDomElement &
									_this );

	static bbTrack * FASTCALL findBBTrack( csize _bb_num );
	static csize FASTCALL numOfBBTrack( track * _track );
	static void FASTCALL swapBBTracks( track * _track1, track * _track2 );

	inline nameLabel * trackLabel( void )
	{
		return( m_trackLabel );
	}


public slots:
	void clickedTrackLabel( void );


protected:
	inline virtual QString nodeName( void ) const
	{
		return( "bbtrack" );
	}


private:
	nameLabel * m_trackLabel;

	typedef QMap<bbTrack *, csize> infoMap;
	static infoMap s_infoMap;

} ;


#endif
