/*
 * timeline.h - class timeLine, representing a time-line with position marker
 *
 * Copyright (c) 2004-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _TIMELINE_H
#define _TIMELINE_H

#include <QtGui/QWidget>

#include "song_editor.h"


class QPixmap;
class nStateButton;
class textFloat;


class timeLine : public QWidget, public journallingObject
{
	Q_OBJECT
public:
	enum autoScrollStates
	{
		AUTOSCROLL_ENABLED, AUTOSCROLL_DISABLED
	} ;

	enum loopPointStates
	{
		LOOP_POINTS_DISABLED, LOOP_POINTS_ENABLED
	} ;

	enum behaviourAtStopStates
	{
		BACK_TO_ZERO, BACK_TO_START, KEEP_STOP_POSITION
	} ;


	timeLine( int _xoff, int _yoff, float _ppt, songEditor::playPos & _pos,
				const midiTime & _begin, QWidget * _parent );
	virtual ~timeLine();

	inline songEditor::playPos & pos( void )
	{
		return( m_pos );
	}

	behaviourAtStopStates behaviourAtStop( void ) const
	{
		return( m_behaviourAtStop );
	}

	bool loopPointsEnabled( void ) const
	{
		return( m_loopPoints == LOOP_POINTS_ENABLED );
	}

	inline const midiTime & loopBegin( void ) const
	{
		return( ( m_loopPos[0] < m_loopPos[1] ) ?
						m_loopPos[0] : m_loopPos[1] );
	}

	inline const midiTime & loopEnd( void ) const
	{
		return( ( m_loopPos[0] > m_loopPos[1] ) ?
						m_loopPos[0] : m_loopPos[1] );
	}

	inline void savePos( const midiTime & _pos )
	{
		m_savedPos = _pos;
	}
	inline const midiTime & savedPos( void ) const
	{
		return( m_savedPos );
	}

	inline void setPixelsPerTact( float _ppt )
	{
		m_ppt = _ppt;
		update();
	}

	void addToolButtons( QWidget * _tool_bar );


	virtual void FASTCALL saveSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void FASTCALL loadSettings( const QDomElement & _this );
	inline virtual QString nodeName( void ) const
	{
		return( "timeline" );
	}


public slots:
	void updatePosition( const midiTime & );
	void updatePosition( void )
	{
		updatePosition( midiTime() );
	}
	void toggleAutoScroll( int _n );
	void toggleLoopPoints( int _n );
	void toggleBehaviourAtStop( int _n );


protected:
	virtual void paintEvent( QPaintEvent * _pe );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseMoveEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );


private:
	inline int markerX( const midiTime & _t ) const
	{
		return( m_xOffset + static_cast<int>( ( _t - m_begin ) *
							m_ppt / 64.0f ) );
	}


	static QPixmap * s_timeLinePixmap;
	static QPixmap * s_posMarkerPixmap;
	static QPixmap * s_loopPointPixmap;
	static QPixmap * s_loopPointDisabledPixmap;

	autoScrollStates m_autoScroll;
	loopPointStates m_loopPoints;
	behaviourAtStopStates m_behaviourAtStop;

	bool m_changedPosition;

	int m_xOffset;
	int m_posMarkerX;
	float m_ppt;
	songEditor::playPos & m_pos;
	const midiTime & m_begin;
	midiTime m_loopPos[2];

	midiTime m_savedPos;


	textFloat * m_hint;


	enum actions
	{
		NONE, MOVE_POS_MARKER, MOVE_LOOP_BEGIN, MOVE_LOOP_END
	} m_action;

	int m_moveXOff;


signals:
	void positionChanged( const midiTime & _t );
	void loopPointStateLoaded( int _n );

} ;


#endif
