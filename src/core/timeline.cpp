/*
 * playpos_marker.cpp - class timeLine, representing a time-line with
 *                      position marker
 *
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "qt3support.h"

#ifdef QT4

#include <QPainter>
#include <QApplication>
#include <QMouseEvent>
#include <QLayout>
#include <QTimer>

#else

#include <qpainter.h>
#include <qapplication.h>
#include <qlayout.h>
#include <qtimer.h>

#endif


#include "timeline.h"
#include "embed.h"
#include "templates.h"
#include "nstate_button.h"



QPixmap * timeLine::s_timeLinePixmap = NULL;
QPixmap * timeLine::s_posMarkerPixmap = NULL;
QPixmap * timeLine::s_loopPointPixmap = NULL;
QPixmap * timeLine::s_loopPointDisabledPixmap = NULL;


timeLine::timeLine( const int _xoff, const int _yoff, const float _ppt,
			songEditor::playPos & _pos, const midiTime & _begin,
							QWidget * _parent ) :
	QWidget( _parent ),
	m_autoScroll( AUTOSCROLL_ENABLED ),
	m_loopPoints( LOOP_POINTS_DISABLED ),
	m_behaviourAtStop( BACK_TO_ZERO ),
	m_changedPosition( TRUE ),
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

	if( s_loopPointDisabledPixmap == NULL)
	{
		s_loopPointDisabledPixmap = new QPixmap( embed::getIconPixmap(
						"loop_point_disabled" ) );
	}

	move( 0, _yoff );
	setFixedHeight( s_timeLinePixmap->height() );

	m_xOffset -= s_posMarkerPixmap->width() / 2;

#ifndef QT4
	setBackgroundMode( Qt::NoBackground );
#endif

	m_pos.m_timeLine = this;

	updatePosition();
	QTimer * update_timer = new QTimer( this );
	connect( update_timer, SIGNAL( timeout() ), this,
					SLOT( checkForUpdatedPosition() ) );
	update_timer->start( 50 );
}




timeLine::~timeLine()
{
	m_pos.m_timeLine = NULL;
}




void timeLine::addToolButtons( QWidget * _tool_bar )
{
	nStateButton * m_autoScroll = new nStateButton( _tool_bar );
	m_autoScroll->setGeneralToolTip( tr( "Enable/disable "
							"auto-scrolling" ) );
	m_autoScroll->addState( embed::getIconPixmap( "autoscroll_on" ) );
	m_autoScroll->addState( embed::getIconPixmap( "autoscroll_off" ) );
	connect( m_autoScroll, SIGNAL( changedState( int ) ), this,
					SLOT( toggleAutoScroll( int ) ) );

	nStateButton * m_loopPoints = new nStateButton( _tool_bar );
	m_loopPoints->setGeneralToolTip( tr( "Enable/disable loop-points" ) );
	m_loopPoints->addState( embed::getIconPixmap( "loop_points_off" ) );
	m_loopPoints->addState( embed::getIconPixmap( "loop_points_on" ) );
	connect( m_loopPoints, SIGNAL( changedState( int ) ), this,
					SLOT( toggleLoopPoints( int ) ) );

	nStateButton * m_behaviourAtStop = new nStateButton( _tool_bar );
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
	connect( m_behaviourAtStop, SIGNAL( changedState( int ) ), this,
					SLOT( toggleBehaviourAtStop( int ) ) );

	QBoxLayout * layout = dynamic_cast<QBoxLayout *>( _tool_bar->layout() );
	layout->addWidget( m_autoScroll );
	layout->addWidget( m_loopPoints );
	layout->addWidget( m_behaviourAtStop );
}




void timeLine::updatePosition( const midiTime & )
{
	const int new_x = markerX( m_pos );

	if( new_x != m_posMarkerX )
	{
		m_posMarkerX = new_x;
		m_changedPosition = TRUE;
		if( m_autoScroll == AUTOSCROLL_ENABLED )
		{
			emit positionChanged( m_pos );
		}
	}
}




void timeLine::toggleAutoScroll( int _n )
{
	m_autoScroll = static_cast<autoScrollStates>( _n );
}




void timeLine::toggleLoopPoints( int _n )
{
	m_loopPoints = static_cast<loopPointStates>( _n );
	update();
}




void timeLine::toggleBehaviourAtStop( int _n )
{
	m_behaviourAtStop = static_cast<behaviourAtStopStates>( _n );
}




void timeLine::checkForUpdatedPosition( void )
{
	if( m_changedPosition == TRUE )
	{
		repaint();
		m_changedPosition = FALSE;
	}
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

	const QPixmap & lpoint = loopPointsEnabled() ?
						*s_loopPointPixmap :
						*s_loopPointDisabledPixmap;
	p.drawPixmap( markerX( m_loopPos[0] ), 7, lpoint );
	p.drawPixmap( markerX( m_loopPos[1] ), 7, lpoint );


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
			p.setPen( QColor( 224, 224, 224 ) );
			p.drawText( x + static_cast<int>( i * m_ppt ) + 1, 15,
						QString::number( tact_num ) );
			p.setPen( QColor( 0, 0, 0 ) );
			p.drawText( x + static_cast<int>( i * m_ppt ), 14,
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
			m_loopPos[0] = t.getTact() * 64;
			update();
			break;

		case MOVE_LOOP_END:
			m_loopPos[1] = t.getTact() * 64;
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

