/*
 * TimeLineWidget.cpp - class timeLine, representing a time-line with position marker
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


#include <QDomElement>
#include <QTimer>
#include <QApplication>
#include <QLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QToolBar>


#include "TimeLineWidget.h"
#include "embed.h"
#include "NStateButton.h"
#include "GuiApplication.h"
#include "TextFloat.h"
#include "SongEditor.h"


#if QT_VERSION < 0x040800
#define MiddleButton MidButton
#endif


QPixmap * TimeLineWidget::s_posMarkerPixmap = NULL;

TimeLineWidget::TimeLineWidget( const int xoff, const int yoff, const float ppt,
			Song::PlayPos & pos, const MidiTime & begin,
							QWidget * parent ) :
	QWidget( parent ),
	m_inactiveLoopColor( 52, 63, 53, 64 ),
	m_inactiveLoopBrush( QColor( 255, 255, 255, 32 ) ),
	m_inactiveLoopInnerColor( 255, 255, 255, 32 ),
	m_activeLoopColor( 52, 63, 53, 255 ),
	m_activeLoopBrush( QColor( 55, 141, 89 ) ),
	m_activeLoopInnerColor( 74, 155, 100, 255 ),
	m_loopRectangleVerticalPadding( 1 ),
	m_barLineColor( 192, 192, 192 ),
	m_barNumberColor( m_barLineColor.darker( 120 ) ),
	m_autoScroll( AutoScrollEnabled ),
	m_loopPoints( LoopPointsDisabled ),
	m_behaviourAtStop( BackToZero ),
	m_changedPosition( true ),
	m_xOffset( xoff ),
	m_posMarkerX( 0 ),
	m_ppt( ppt ),
	m_pos( pos ),
	m_begin( begin ),
	m_savedPos( -1 ),
	m_hint( NULL ),
	m_action( NoAction ),
	m_moveXOff( 0 )
{
	m_loopPos[0] = 0;
	m_loopPos[1] = DefaultTicksPerTact;

	if( s_posMarkerPixmap == NULL )
	{
		s_posMarkerPixmap = new QPixmap( embed::getIconPixmap(
							"playpos_marker" ) );
	}

	setAttribute( Qt::WA_OpaquePaintEvent, true );
	move( 0, yoff );

	m_xOffset -= s_posMarkerPixmap->width() / 2;

	m_pos.m_timeLine = this;

	QTimer * updateTimer = new QTimer( this );
	connect( updateTimer, SIGNAL( timeout() ),
					this, SLOT( updatePosition() ) );
	updateTimer->start( 50 );
	connect( Engine::getSong(), SIGNAL( timeSignatureChanged( int,int ) ),
					this, SLOT( update() ) );
}




TimeLineWidget::~TimeLineWidget()
{
	if( gui->songEditor() )
	{
		m_pos.m_timeLine = NULL;
	}
	delete m_hint;
}




void TimeLineWidget::addToolButtons( QToolBar * _tool_bar )
{
	NStateButton * autoScroll = new NStateButton( _tool_bar );
	autoScroll->setGeneralToolTip( tr( "Enable/disable auto-scrolling" ) );
	autoScroll->addState( embed::getIconPixmap( "autoscroll_on" ) );
	autoScroll->addState( embed::getIconPixmap( "autoscroll_off" ) );
	connect( autoScroll, SIGNAL( changedState( int ) ), this,
					SLOT( toggleAutoScroll( int ) ) );

	NStateButton * loopPoints = new NStateButton( _tool_bar );
	loopPoints->setGeneralToolTip( tr( "Enable/disable loop-points" ) );
	loopPoints->addState( embed::getIconPixmap( "loop_points_off" ) );
	loopPoints->addState( embed::getIconPixmap( "loop_points_on" ) );
	connect( loopPoints, SIGNAL( changedState( int ) ), this,
					SLOT( toggleLoopPoints( int ) ) );
	connect( this, SIGNAL( loopPointStateLoaded( int ) ), loopPoints,
					SLOT( changeState( int ) ) );

	NStateButton * behaviourAtStop = new NStateButton( _tool_bar );
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

	_tool_bar->addWidget( autoScroll );
	_tool_bar->addWidget( loopPoints );
	_tool_bar->addWidget( behaviourAtStop );
}




void TimeLineWidget::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setAttribute( "lp0pos", (int) loopBegin() );
	_this.setAttribute( "lp1pos", (int) loopEnd() );
	_this.setAttribute( "lpstate", m_loopPoints );
}




void TimeLineWidget::loadSettings( const QDomElement & _this )
{
	m_loopPos[0] = _this.attribute( "lp0pos" ).toInt();
	m_loopPos[1] = _this.attribute( "lp1pos" ).toInt();
	m_loopPoints = static_cast<LoopPointStates>(
					_this.attribute( "lpstate" ).toInt() );
	update();
	emit loopPointStateLoaded( m_loopPoints );
}




void TimeLineWidget::updatePosition( const MidiTime & )
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




void TimeLineWidget::toggleAutoScroll( int _n )
{
	m_autoScroll = static_cast<AutoScrollStates>( _n );
}




void TimeLineWidget::toggleLoopPoints( int _n )
{
	m_loopPoints = static_cast<LoopPointStates>( _n );
	update();
}




void TimeLineWidget::toggleBehaviourAtStop( int _n )
{
	m_behaviourAtStop = static_cast<BehaviourAtStopStates>( _n );
}




void TimeLineWidget::paintEvent( QPaintEvent * )
{
	QPainter p( this );

	// Draw background
	p.fillRect( 0, 0, width(), height(), p.background() );

	// Clip so that we only draw everything starting from the offset
	p.setClipRect( m_xOffset, 0, width() - m_xOffset, height() );

	// Draw the loop rectangle
	int const & loopRectMargin = getLoopRectangleVerticalPadding();
	int const loopRectHeight = this->height() - 2 * loopRectMargin;
	int const loopStart = markerX( loopBegin() ) + 8;
	int const loopEndR = markerX( loopEnd() ) + 9;
	int const loopRectWidth = loopEndR - loopStart;

	bool const loopPointsActive = loopPointsEnabled();

	// Draw the main rectangle (inner fill only)
	QRect outerRectangle( loopStart, loopRectMargin, loopRectWidth - 1, loopRectHeight - 1 );
	p.fillRect( outerRectangle, loopPointsActive ? getActiveLoopBrush() : getInactiveLoopBrush());

	// Draw the bar lines and numbers
	// Activate hinting on the font
	QFont font = p.font();
	font.setHintingPreference( QFont::PreferFullHinting );
	p.setFont(font);
	int const fontAscent = p.fontMetrics().ascent();
	int const fontHeight = p.fontMetrics().height();

	QColor const & barLineColor = getBarLineColor();
	QColor const & barNumberColor = getBarNumberColor();

	tact_t barNumber = m_begin.getTact();
	int const x = m_xOffset + s_posMarkerPixmap->width() / 2 -
			( ( static_cast<int>( m_begin * m_ppt ) / MidiTime::ticksPerTact() ) % static_cast<int>( m_ppt ) );

	for( int i = 0; x + i * m_ppt < width(); ++i )
	{
		++barNumber;
		if( ( barNumber - 1 ) %
			qMax( 1, qRound( 1.0f / 3.0f *
				MidiTime::ticksPerTact() / m_ppt ) ) == 0 )
		{
			const int cx = x + qRound( i * m_ppt );
			p.setPen( barLineColor );
			p.drawLine( cx, 5, cx, height() - 6 );

			const QString s = QString::number( barNumber );
			p.setPen( barNumberColor );
			p.drawText( cx + 5, ((height() - fontHeight) / 2) + fontAscent, s );
		}
	}

	// Draw the main rectangle (outer border)
	p.setPen( loopPointsActive ? getActiveLoopColor() : getInactiveLoopColor() );
	p.setBrush( Qt::NoBrush );
	p.drawRect( outerRectangle );

	// Draw the inner border outline (no fill)
	QRect innerRectangle = outerRectangle.adjusted( 1, 1, -1, -1 );
	p.setPen( loopPointsActive ? getActiveLoopInnerColor() : getInactiveLoopInnerColor() );
	p.setBrush( Qt::NoBrush );
	p.drawRect( innerRectangle );

	// Draw the position marker
	p.setOpacity( 0.6 );
	p.drawPixmap( m_posMarkerX, height() - s_posMarkerPixmap->height(), *s_posMarkerPixmap );
}




void TimeLineWidget::mousePressEvent( QMouseEvent* event )
{
	if( event->x() < m_xOffset )
	{
		return;
	}
	if( event->button() == Qt::LeftButton  && !(event->modifiers() & Qt::ShiftModifier) )
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
	else if( event->button() == Qt::LeftButton  && (event->modifiers() & Qt::ShiftModifier) )
	{
		m_action = SelectSongTCO;
		m_initalXSelect = event->x();
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
		m_hint = TextFloat::displayMessage( tr( "Hint" ),
					tr( "Press <%1> to disable magnetic loop points." ).arg(
						#ifdef LMMS_BUILD_APPLE
						"⌘"),
						#else
						"Ctrl"),
						#endif
					embed::getIconPixmap( "hint" ), 0 );
	}
	else if( m_action == MoveLoopEnd )
	{
		delete m_hint;
		m_hint = TextFloat::displayMessage( tr( "Hint" ),
					tr( "Hold <Shift> to move the begin loop point; Press <%1> to disable magnetic loop points." ).arg(
						#ifdef LMMS_BUILD_APPLE
						"⌘"),
						#else
						"Ctrl"),
						#endif
					embed::getIconPixmap( "hint" ), 0 );
	}

	mouseMoveEvent( event );
}




void TimeLineWidget::mouseMoveEvent( QMouseEvent* event )
{
	const MidiTime t = m_begin + static_cast<int>( qMax( event->x() - m_xOffset - m_moveXOff, 0 ) * MidiTime::ticksPerTact() / m_ppt );

	switch( m_action )
	{
		case MovePositionMarker:
			m_pos.setTicks( t.getTicks() );
			Engine::getSong()->setMilliSeconds( ( t.getTicks() *
					( 60 * 1000 / 48 ) ) /
						Engine::getSong()->getTempo() );
			m_pos.setCurrentFrame( 0 );
			updatePosition();
			positionMarkerMoved();
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
	case SelectSongTCO:
			emit regionSelectedFromPixels( m_initalXSelect , event->x() );
		break;

		default:
			break;
	}
}




void TimeLineWidget::mouseReleaseEvent( QMouseEvent* event )
{
	delete m_hint;
	m_hint = NULL;
	if ( m_action == SelectSongTCO ) { emit selectionFinished(); }
	m_action = NoAction;
}
