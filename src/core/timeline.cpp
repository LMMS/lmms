#ifndef SINGLE_SOURCE_COMPILE

/*
 * timeline.cpp - class timeLine, representing a time-line with position marker
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


#include <Qt/QtXml>
#include <QtCore/QTimer>
#include <QtGui/QApplication>
#include <QtGui/QLayout>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>


#include "timeline.h"
#include "embed.h"
#include "engine.h"
#include "templates.h"
#include "nstate_button.h"
#include "main_window.h"
#include "text_float.h"
#include "automatable_model_templates.h"



QPixmap * timeLine::s_timeLinePixmap = NULL;
QPixmap * timeLine::s_posMarkerPixmap = NULL;
QPixmap * timeLine::s_loopPointPixmap = NULL;
QPixmap * timeLine::s_loopPointDisabledPixmap = NULL;


timeLine::timeLine( const int _xoff, const int _yoff, const float _ppt,
			song::playPos & _pos, const midiTime & _begin,
							QWidget * _parent ) :
	QWidget( _parent ),
	m_autoScroll( AutoScrollEnabled ),
	m_loopPoints( LoopPointsDisabled ),
	m_behaviourAtStop( BackToZero ),
	m_changedPosition( TRUE ),
	m_xOffset( _xoff ),
	m_posMarkerX( 0 ),
	m_ppt( _ppt ),
	m_pos( _pos ),
	m_begin( _begin ),
	m_savedPos( -1 ),
	m_hint( NULL ),
	m_action( NoAction ),
	m_moveXOff( 0 )
{
	m_loopPos[0] = 0;
	m_loopPos[1] = 64;

	if( s_timeLinePixmap == NULL )
	{
		s_timeLinePixmap = new QPixmap( embed::getIconPixmap(
								"timeline" ) );
	}
	if( s_posMarkerPixmap == NULL )
	{
		s_posMarkerPixmap = new QPixmap( embed::getIconPixmap(
							"playpos_marker" ) );
	}
	if( s_loopPointPixmap == NULL )
	{
		s_loopPointPixmap = new QPixmap( embed::getIconPixmap(
							"loop_point" ) );
	}

	if( s_loopPointDisabledPixmap == NULL )
	{
		s_loopPointDisabledPixmap = new QPixmap( embed::getIconPixmap(
						"loop_point_disabled" ) );
	}

	move( 0, _yoff );
	setFixedHeight( s_timeLinePixmap->height() );

	m_xOffset -= s_posMarkerPixmap->width() / 2;

	m_pos.m_timeLine = this;

	QTimer * update_timer = new QTimer( this );
	connect( update_timer, SIGNAL( timeout() ),
					this, SLOT( updatePosition() ) );
	update_timer->start( 50 );
}




timeLine::~timeLine()
{
	if( engine::getSongEditor() )
	{
		m_pos.m_timeLine = NULL;
	}
	delete m_hint;
}




void timeLine::addToolButtons( QWidget * _tool_bar )
{
	nStateButton * autoScroll = new nStateButton( _tool_bar );
	autoScroll->setGeneralToolTip( tr( "Enable/disable auto-scrolling" ) );
	autoScroll->addState( embed::getIconPixmap( "autoscroll_on" ) );
	autoScroll->addState( embed::getIconPixmap( "autoscroll_off" ) );
	connect( autoScroll, SIGNAL( changedState( int ) ), this,
					SLOT( toggleAutoScroll( int ) ) );

	nStateButton * loopPoints = new nStateButton( _tool_bar );
	loopPoints->setGeneralToolTip( tr( "Enable/disable loop-points" ) );
	loopPoints->addState( embed::getIconPixmap( "loop_points_off" ) );
	loopPoints->addState( embed::getIconPixmap( "loop_points_on" ) );
	connect( loopPoints, SIGNAL( changedState( int ) ), this,
					SLOT( toggleLoopPoints( int ) ) );
	connect( this, SIGNAL( loopPointStateLoaded( int ) ), loopPoints,
					SLOT( changeState( int ) ) );

	nStateButton * behaviourAtStop = new nStateButton( _tool_bar );
	behaviourAtStop->addState( embed::getIconPixmap( "back_to_zero" ),
					tr( "After stopping go back to begin" )
									);
	behaviourAtStop->addState( embed::getIconPixmap( "back_to_start" ),
					tr( "After stopping go back to "
						"position at which playing was "
						"started" ) );
	behaviourAtStop->addState( embed::getIconPixmap( "keep_stop_position" ),
					tr( "After stopping keep position" ) );
	connect( behaviourAtStop, SIGNAL( changedState( int ) ), this,
					SLOT( toggleBehaviourAtStop( int ) ) );

	QBoxLayout * layout = dynamic_cast<QBoxLayout *>( _tool_bar->layout() );
	layout->addWidget( autoScroll );
	layout->addWidget( loopPoints );
	layout->addWidget( behaviourAtStop );
}




void timeLine::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setAttribute( "lp0pos", (int) loopBegin() );
	_this.setAttribute( "lp1pos", (int) loopEnd() );
	_this.setAttribute( "lpstate", m_loopPoints );
}




void timeLine::loadSettings( const QDomElement & _this )
{
	m_loopPos[0] = _this.attribute( "lp0pos" ).toInt();
	m_loopPos[1] = _this.attribute( "lp1pos" ).toInt();
	m_loopPoints = static_cast<LoopPointStates>(
					_this.attribute( "lpstate" ).toInt() );
	update();
	emit loopPointStateLoaded( m_loopPoints );
}




void timeLine::updatePosition( const midiTime & )
{
	const int new_x = markerX( m_pos );

	if( new_x != m_posMarkerX )
	{
		m_posMarkerX = new_x;
		m_changedPosition = TRUE;
		if( m_autoScroll == AutoScrollEnabled )
		{
			emit positionChanged( m_pos );
		}
		update();
	}
}




void timeLine::toggleAutoScroll( int _n )
{
	m_autoScroll = static_cast<AutoScrollStates>( _n );
}




void timeLine::toggleLoopPoints( int _n )
{
	m_loopPoints = static_cast<LoopPointStates>( _n );
	update();
}




void timeLine::toggleBehaviourAtStop( int _n )
{
	m_behaviourAtStop = static_cast<BehaviourAtStopStates>( _n );
}




void timeLine::paintEvent( QPaintEvent * )
{
	QPainter p( this );

	for( int x = 0; x < width(); x += s_timeLinePixmap->width() )
	{
		p.drawPixmap( x, 0, *s_timeLinePixmap );
	}
	p.setClipRect( m_xOffset, 0, width() - m_xOffset, height() );
	p.setPen( QColor( 0, 0, 0 ) );

	const QPixmap & lpoint = loopPointsEnabled() ?
						*s_loopPointPixmap :
						*s_loopPointDisabledPixmap;
	p.drawPixmap( markerX( loopBegin() ), 7, lpoint );
	p.drawPixmap( markerX( loopEnd() ), 7, lpoint );


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
}




void timeLine::mousePressEvent( QMouseEvent * _me )
{
	if( _me->x() < m_xOffset )
	{
		return;
	}
	if( _me->button() == Qt::LeftButton )
	{
		m_action = MovePositionMarker;
		if( _me->x() - m_xOffset < s_posMarkerPixmap->width() )
		{
			m_moveXOff = _me->x() - m_xOffset;
		}
		else
		{
			m_moveXOff = s_posMarkerPixmap->width() / 2;
		}
	}
	else
	{
		const midiTime t = m_begin +
				static_cast<Sint32>( _me->x() * 64 / m_ppt );
		m_action = MoveLoopBegin;
		if( m_loopPos[0] > m_loopPos[1]  )
		{
			qSwap( m_loopPos[0], m_loopPos[1] );
		}
		if( _me->button() == Qt::RightButton )
		{
			m_action = MoveLoopEnd;
		}
		m_loopPos[( m_action == MoveLoopBegin ) ? 0 : 1] = t;
	}
	if( m_action == MoveLoopBegin || m_action == MoveLoopEnd )
	{
		delete m_hint;
		m_hint = textFloat::displayMessage( tr( "Hint" ),
					tr( "Press <Ctrl> to disable magnetic "
							"loop-points." ),
					embed::getIconPixmap( "hint" ), 0 );
	}
	mouseMoveEvent( _me );
}




void timeLine::mouseMoveEvent( QMouseEvent * _me )
{
	const midiTime t = m_begin + static_cast<Sint32>( tMax( _me->x() -
				    m_xOffset - m_moveXOff, 0 ) * 64 / m_ppt );
	switch( m_action )
	{
		case MovePositionMarker:
			m_pos.setTact( t.getTact() );
			m_pos.setTact64th( t.getTact64th() );
			m_pos.setCurrentFrame( 0 );
			updatePosition();
			break;

		case MoveLoopBegin:
		case MoveLoopEnd:
		{
			const Uint8 i = m_action - MoveLoopBegin;
			if( engine::getMainWindow()->isCtrlPressed() == TRUE )
			{
				// no ctrl-press-hint when having ctrl pressed
				delete m_hint;
				m_hint = NULL;
				m_loopPos[i] = t;
			}
			else
			{
				m_loopPos[i] = t.toNearestTact();
			}
			update();
			break;
		}

		default:
			break;
	}
}




void timeLine::mouseReleaseEvent( QMouseEvent * _me )
{
	delete m_hint;
	m_hint = NULL;
	m_action = NoAction;
}




#include "timeline.moc"


#endif
