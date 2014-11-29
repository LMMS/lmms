/*
 * timeline.cpp - class timeLine, representing a time-line with position marker
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of LMMS - http://lmms.io
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


#include <QtXml/QDomElement>
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
#include "MainWindow.h"
#include "text_float.h"


#if QT_VERSION < 0x040800
#define MiddleButton MidButton
#endif


QPixmap * timeLine::s_timeLinePixmap = NULL;
QPixmap * timeLine::s_posMarkerPixmap = NULL;
QPixmap * timeLine::s_loopPointBeginPixmap = NULL;
QPixmap * timeLine::s_loopPointEndPixmap = NULL;

timeLine::timeLine( const int _xoff, const int _yoff, const float _ppt,
			song::playPos & _pos, const MidiTime & _begin,
							QWidget * _parent ) :
	QWidget( _parent ),
	m_autoScroll( AutoScrollEnabled ),
	m_loopPoints( LoopPointsDisabled ),
	m_behaviourAtStop( BackToZero ),
	m_changedPosition( true ),
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
	m_loopPos[1] = DefaultTicksPerTact;

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
	if( s_loopPointBeginPixmap == NULL )
	{
		s_loopPointBeginPixmap = new QPixmap( embed::getIconPixmap(
							"loop_point_b" ) );
	}
	if( s_loopPointEndPixmap == NULL )
	{
		s_loopPointEndPixmap = new QPixmap( embed::getIconPixmap(
							"loop_point_e" ) );
	}

	setAttribute( Qt::WA_OpaquePaintEvent, true );
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
	if( engine::songEditor() )
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




void timeLine::updatePosition( const MidiTime & )
{
	const int new_x = markerX( m_pos );

	if( new_x != m_posMarkerX )
	{
		m_posMarkerX = new_x;
		m_changedPosition = true;
		emit positionChanged( m_pos );
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

	QColor bg_color = QApplication::palette().color( QPalette::Active,
							QPalette::Background );
	QLinearGradient g( 0, 0, 0, height() );
	g.setColorAt( 0, bg_color.lighter( 150 ) );
	g.setColorAt( 1, bg_color.darker( 150 ) );
	p.fillRect( 0, 0, width(), height(), g );

	p.setClipRect( m_xOffset, 0, width() - m_xOffset, height() );
	p.setPen( QColor( 0, 0, 0 ) );

	p.setOpacity( loopPointsEnabled() ? 0.9 : 0.2 );
	p.drawPixmap( markerX( loopBegin() )+2, 2, *s_loopPointBeginPixmap );
	p.drawPixmap( markerX( loopEnd() )+2, 2, *s_loopPointEndPixmap );
	p.setOpacity( 1.0 );


	tact_t tact_num = m_begin.getTact();
	int x = m_xOffset + s_posMarkerPixmap->width() / 2 -
			( ( static_cast<int>( m_begin * m_ppt ) / MidiTime::ticksPerTact() ) % static_cast<int>( m_ppt ) );

	p.setPen( QColor( 192, 192, 192 ) );
	for( int i = 0; x + i * m_ppt < width(); ++i )
	{
		const int cx = x + qRound( i * m_ppt );
		p.drawLine( cx, 5, cx, height() - 6 );
		++tact_num;
		if( ( tact_num - 1 ) %
			qMax( 1, qRound( 1.0f / 3.0f *
				MidiTime::ticksPerTact() / m_ppt ) ) == 0 )
		{
			const QString s = QString::number( tact_num );
			p.drawText( cx + qRound( ( m_ppt - p.fontMetrics().
							width( s ) ) / 2 ),
				height() - p.fontMetrics().height() / 2, s );
		}
	}

	p.setOpacity( 0.6 );
	p.drawPixmap( m_posMarkerX, height() - s_posMarkerPixmap->height(),
							*s_posMarkerPixmap );
}




void timeLine::mousePressEvent( QMouseEvent* event )
{
	if( event->x() < m_xOffset )
	{
		return;
	}
	if( event->button() == Qt::LeftButton )
	{
		m_action = MovePositionMarker;
		if( event->x() - m_xOffset < s_posMarkerPixmap->width() )
		{
			m_moveXOff = event->x() - m_xOffset;
		}
		else
		{
			m_moveXOff = s_posMarkerPixmap->width() / 2;
		}
	}
	else if( event->button() == Qt::RightButton || event->button() == Qt::MiddleButton )
	{
        	m_moveXOff = s_posMarkerPixmap->width() / 2;
		const MidiTime t = m_begin + static_cast<int>( event->x() * MidiTime::ticksPerTact() / m_ppt );
		if( m_loopPos[0] > m_loopPos[1]  )
		{
			qSwap( m_loopPos[0], m_loopPos[1] );
		}
		if( ( event->modifiers() & Qt::ShiftModifier ) || event->button() == Qt::MiddleButton )
		{
			m_action = MoveLoopBegin;
		}
		else
		{
			m_action = MoveLoopEnd;
		}
		m_loopPos[( m_action == MoveLoopBegin ) ? 0 : 1] = t;
	}

	if( m_action == MoveLoopBegin )
	{
		delete m_hint;
		m_hint = textFloat::displayMessage( tr( "Hint" ),
					tr( "Press <Ctrl> to disable magnetic loop points." ),
					embed::getIconPixmap( "hint" ), 0 );
	}
	else if( m_action == MoveLoopEnd )
	{
		delete m_hint;
		m_hint = textFloat::displayMessage( tr( "Hint" ),
					tr( "Hold <Shift> to move the begin loop point; Press <Ctrl> to disable magnetic loop points." ),
					embed::getIconPixmap( "hint" ), 0 );
	}

	mouseMoveEvent( event );
}




void timeLine::mouseMoveEvent( QMouseEvent* event )
{
	const MidiTime t = m_begin + static_cast<int>( qMax( event->x() - m_xOffset - m_moveXOff, 0 ) * MidiTime::ticksPerTact() / m_ppt );

	switch( m_action )
	{
		case MovePositionMarker:
			m_pos.setTicks( t.getTicks() );
			engine::getSong()->setMilliSeconds(((((t.getTicks()))*60*1000/48)/engine::getSong()->getTempo()));
			m_pos.setCurrentFrame( 0 );
			updatePosition();
			break;

		case MoveLoopBegin:
		case MoveLoopEnd:
		{
			const int i = m_action - MoveLoopBegin;
			if( event->modifiers() & Qt::ControlModifier )
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
			// Catch begin == end
			if( m_loopPos[0] == m_loopPos[1] )
			{
				// Note, swap 1 and 0 below and the behavior "skips" the other
				// marking instead of pushing it.
				if( m_action == MoveLoopBegin ) 
					m_loopPos[0] -= MidiTime::ticksPerTact();
				else
					m_loopPos[1] += MidiTime::ticksPerTact();
			}
			update();
			break;
		}

		default:
			break;
	}
}




void timeLine::mouseReleaseEvent( QMouseEvent* event )
{
	delete m_hint;
	m_hint = NULL;
	m_action = NoAction;
}




#include "moc_timeline.cxx"


