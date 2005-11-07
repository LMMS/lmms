/*
 * playpos_marker.cpp - class timeLine, representing a time-line with
 *                      position marker
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "qt3support.h"

#ifdef QT4

#include <QPainter>
#include <QApplication>
#include <QMouseEvent>

#else

#include <qpainter.h>
#include <qapplication.h>

#endif


#include "timeline.h"
#include "nstate_button.h"
#include "embed.h"
#include "templates.h"



QPixmap * timeLine::s_timeLinePixmap = NULL;
QPixmap * timeLine::s_posMarkerPixmap = NULL;
QPixmap * timeLine::s_loopPointPixmap = NULL;


timeLine::timeLine( const int _xoff, const int _yoff, const float _ppt,
			songEditor::playPos & _pos, const midiTime & _begin,
							QWidget * _parent ) :
	QWidget( _parent ),
	m_xOffset( _xoff ),
	m_posMarkerX( 0 ),
	m_ppt( _ppt ),
	m_pos( _pos ),
	m_begin( _begin ),
	m_savedPos( -1 ),
	m_action( NONE ),
	m_moveXOff( 0 )
{
	m_loopPos[0] = 0;
	m_loopPos[1] = 64;

	if( s_timeLinePixmap == NULL)
	{
		s_timeLinePixmap = new QPixmap( embed::getIconPixmap(
								"timeline" ) );
	}
	if( s_posMarkerPixmap == NULL)
	{
		s_posMarkerPixmap = new QPixmap( embed::getIconPixmap(
							"playpos_marker" ) );
	}
	if( s_loopPointPixmap == NULL)
	{
		s_loopPointPixmap = new QPixmap( embed::getIconPixmap(
							"loop_point" ) );
	}

	move( 0, _yoff );
	setFixedHeight( s_timeLinePixmap->height() );

	m_xOffset -= s_posMarkerPixmap->width() / 2;


	m_autoScroll = new nStateButton( this );
	m_autoScroll->move( 3, 3 );
	m_autoScroll->setGeneralToolTip( tr( "Enable/disable "
							"auto-scrolling" ) );
	m_autoScroll->addState( embed::getIconPixmap( "autoscroll_on" ) );
	m_autoScroll->addState( embed::getIconPixmap( "autoscroll_off" ) );

	m_loopPoints = new nStateButton( this );
	m_loopPoints->move( 20, 3 );
	m_loopPoints->setGeneralToolTip( tr( "Enable/disable loop-points" ) );
	m_loopPoints->addState( embed::getIconPixmap( "loop_points_off" ) );
	m_loopPoints->addState( embed::getIconPixmap( "loop_points_on" ) );
	connect( m_loopPoints, SIGNAL( stateChanged( int ) ), this,
					SLOT( toggleLoopPoints( int ) ) );

	m_behaviourAtStop = new nStateButton( this );
	m_behaviourAtStop ->move( 37, 3 );
	m_behaviourAtStop ->addState( embed::getIconPixmap( "back_to_zero" ),
					tr( "After stopping go back to begin" )
									);
	m_behaviourAtStop ->addState( embed::getIconPixmap(
							"back_to_start" ),
					tr( "After stopping go back to "
						"position at which playing was "
						"started" ) );
	m_behaviourAtStop ->addState( embed::getIconPixmap(
						"keep_stop_position" ),
					tr( "After stopping keep position" ) );

#ifndef QT4
	setBackgroundMode( Qt::NoBackground );
#endif

	m_pos.m_timeLine = this;

	updatePosition();
}




timeLine::~timeLine()
{
	m_pos.m_timeLine = NULL;
}




timeLine::behaviourAtStopStates timeLine::behaviourAtStop( void ) const
{
	return( static_cast<behaviourAtStopStates>(
						m_behaviourAtStop->state() ) );
}




bool timeLine::loopPointsEnabled( void ) const
{
	return( m_loopPoints->state() == LOOP_POINTS_ENABLED );
}




void timeLine::updatePosition( const midiTime & )
{
	const int new_x = markerX( m_pos );

	if( new_x != m_posMarkerX )
	{
		m_posMarkerX = new_x;
#ifndef QT4
		qApp->lock();
#endif
		paintEvent( NULL );
#ifndef QT4
		qApp->unlock();
#endif
		if( m_autoScroll->state() == AUTOSCROLL_ENABLED )
		{
			emit positionChanged( m_pos );
		}
	}
}




void timeLine::toggleLoopPoints( int _n )
{
	update();
}




void timeLine::paintEvent( QPaintEvent * )
{
#ifdef QT4
	QPainter p( this );
#else
	QPixmap draw_pm( rect().size() );

	QPainter p( &draw_pm, this );
#endif

	for( int x = 0; x < width(); x += s_timeLinePixmap->width() )
	{
		p.drawPixmap( x, 0, *s_timeLinePixmap );
	}
	p.setClipRect( m_xOffset, 0, width() - m_xOffset, height() );
	p.setPen( QColor( 0, 0, 0 ) );

	if( m_loopPoints->state() == LOOP_POINTS_ENABLED )
	{
		p.drawPixmap( markerX( m_loopPos[0] ), 7,
							*s_loopPointPixmap );
		p.drawPixmap( markerX( m_loopPos[1] ), 7,
							*s_loopPointPixmap );
	}


	tact tact_num = m_begin.getTact();
	int x = m_xOffset + s_posMarkerPixmap->width() / 2 -
			( ( static_cast<Sint32>( m_begin * m_ppt ) / 64 ) %
						static_cast<int>( m_ppt ) );

	for( int i = 0; x + i * m_ppt < width(); ++i )
	{
		++tact_num;
		if( ( tact_num - 1 ) %
			tMax( 1, static_cast<int>( 64.0f / m_ppt ) ) == 0 )
		{
			p.drawText( x + static_cast<int>( i * m_ppt ), 16,
						QString::number( tact_num ) );
		}
	}

	p.drawPixmap( m_posMarkerX, 4, *s_posMarkerPixmap );

#ifndef QT4
	// and blit all the drawn stuff on the screen...
	bitBlt( this, rect().topLeft(), &draw_pm );
#endif
}




void timeLine::mousePressEvent( QMouseEvent * _me )
{
	if( _me->x() < m_xOffset )
	{
		return;
	}
	if( _me->button() == Qt::RightButton )
	{
		if( m_loopPoints->state() != LOOP_POINTS_ENABLED )
		{
			return;
		}
		if( _me->x() >= markerX( loopBegin() ) &&
				_me->x() <= markerX( loopBegin() ) +
						s_loopPointPixmap->width() )
		{
			m_action = MOVE_LOOP_BEGIN;
			m_moveXOff = s_loopPointPixmap->width() / 2;
		}
		else if( _me->x() >= markerX( loopEnd() ) &&
				_me->x() <= markerX( loopEnd() ) +
						s_loopPointPixmap->width() )
		{
			m_action = MOVE_LOOP_END;
			m_moveXOff = s_loopPointPixmap->width() / 2;
		}
	}
	else
	{
		m_action = MOVE_POS_MARKER;
		if( _me->x() - m_xOffset < s_posMarkerPixmap->width() )
		{
			m_moveXOff = _me->x() - m_xOffset;
		}
		else
		{
			m_moveXOff = s_posMarkerPixmap->width() / 2;
		}
	}
	mouseMoveEvent( _me );
}




void timeLine::mouseMoveEvent( QMouseEvent * _me )
{
	const midiTime t = m_begin + static_cast<Sint32>( tMax( _me->x() -
				    m_xOffset - m_moveXOff, 0 ) * 64 / m_ppt );
	switch( m_action )
	{
		case MOVE_POS_MARKER:
			m_pos.setTact( t.getTact() );
			m_pos.setTact64th( t.getTact64th() );
			m_pos.setCurrentFrame( 0 );
			updatePosition();
			break;

		case MOVE_LOOP_BEGIN:
			m_loopPos[0] = t;
			update();
			break;

		case MOVE_LOOP_END:
			m_loopPos[1] = t;
			update();
			break;

		default:
			break;
	}
}




void timeLine::mouseReleaseEvent( QMouseEvent * _me )
{
	m_action = NONE;
}




#include "timeline.moc"

