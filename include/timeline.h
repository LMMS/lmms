/*
 * timeline.h - class timeLine, representing a time-line with position marker
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox@users.sourceforge.net>
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


#ifndef _TIMELINE_H
#define _TIMELINE_H

#include "qt3support.h"

#ifdef QT4

#include <QWidget>

#else

#include <qwidget.h>

#endif


#include "song_editor.h"


class QPixmap;
class nStateButton;


class timeLine : public QWidget
{
	Q_OBJECT
public:
	timeLine( int _xoff, int _yoff, float _ppt, songEditor::playPos & _pos,
				const midiTime & _begin, QWidget * _parent );
	~timeLine();

	inline songEditor::playPos & pos( void )
	{
		return( m_pos );
	}

	enum behaviourAtStopStates
	{
		BACK_TO_ZERO, BACK_TO_START, KEEP_STOP_POSITION
	} ;


	behaviourAtStopStates behaviourAtStop( void ) const;

	bool loopPointsEnabled( void ) const;
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


public slots:
	void updatePosition( const midiTime & = 0 );
	void toggleLoopPoints( int _n );


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

	nStateButton * m_autoScroll;
	nStateButton * m_loopPoints;
	nStateButton * m_behaviourAtStop;

	int m_xOffset;
	int m_posMarkerX;
	float m_ppt;
	songEditor::playPos & m_pos;
	const midiTime & m_begin;
	midiTime m_loopPos[2];

	midiTime m_savedPos;


	enum actions
	{
		NONE, MOVE_POS_MARKER, MOVE_LOOP_BEGIN, MOVE_LOOP_END
	} m_action;

	int m_moveXOff;


	enum autoScrollStates
	{
		AUTOSCROLL_ENABLED, AUTOSCROLL_DISABLED
	} ;

	enum loopPointStates
	{
		LOOP_POINTS_DISABLED, LOOP_POINTS_ENABLED
	} ;


signals:
	void positionChanged( const midiTime & _t );

} ;


#endif
