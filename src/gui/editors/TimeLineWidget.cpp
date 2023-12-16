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

#include "TimeLineWidget.h"

#include <cmath>

#include <QDomElement>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>
#include <QToolBar>

#include "embed.h"
#include "GuiApplication.h"
#include "NStateButton.h"
#include "TextFloat.h"

namespace lmms::gui
{

namespace
{
	constexpr int MIN_BAR_LABEL_DISTANCE = 35;
}

TimeLineWidget::TimeLineWidget(const int xoff, const int yoff, const float ppb, Song::PlayPos& pos, Timeline& timeline,
		const TimePos& begin, Song::PlayMode mode, QWidget* parent) :
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
	m_autoScroll( AutoScrollState::Enabled ),
	m_changedPosition( true ),
	m_xOffset( xoff ),
	m_posMarkerX( 0 ),
	m_ppb( ppb ),
	m_snapSize( 1.0 ),
	m_pos( pos ),
	m_timeline{&timeline},
	m_begin( begin ),
	m_mode( mode ),
	m_hint( nullptr ),
	m_action( Action::NoAction ),
	m_moveXOff( 0 )
{
	setAttribute( Qt::WA_OpaquePaintEvent, true );
	move( 0, yoff );

	m_xOffset -= m_posMarkerPixmap.width() / 2;

	setMouseTracking(true);

	auto updateTimer = new QTimer(this);
	connect(updateTimer, &QTimer::timeout, this, &TimeLineWidget::updatePosition);
	updateTimer->start( 1000 / 60 );  // 60 fps
	connect( Engine::getSong(), SIGNAL(timeSignatureChanged(int,int)),
					this, SLOT(update()));
}




TimeLineWidget::~TimeLineWidget()
{
	delete m_hint;
}



void TimeLineWidget::setXOffset(const int x)
{
	m_xOffset = x;
	m_xOffset -= m_posMarkerPixmap.width() / 2;
}




void TimeLineWidget::addToolButtons( QToolBar * _tool_bar )
{
	auto autoScroll = new NStateButton(_tool_bar);
	autoScroll->setGeneralToolTip( tr( "Auto scrolling" ) );
	autoScroll->addState( embed::getIconPixmap( "autoscroll_on" ) );
	autoScroll->addState( embed::getIconPixmap( "autoscroll_off" ) );
	connect( autoScroll, SIGNAL(changedState(int)), this,
					SLOT(toggleAutoScroll(int)));

	auto loopPoints = new NStateButton(_tool_bar);
	loopPoints->setGeneralToolTip( tr( "Loop points" ) );
	loopPoints->addState( embed::getIconPixmap( "loop_points_off" ) );
	loopPoints->addState( embed::getIconPixmap( "loop_points_on" ) );
	connect(loopPoints, &NStateButton::changedState, m_timeline, &Timeline::setLoopEnabled);
	connect(m_timeline, &Timeline::loopEnabledChanged, loopPoints, &NStateButton::changeState);
	connect(m_timeline, &Timeline::loopEnabledChanged, this, static_cast<void (QWidget::*)()>(&QWidget::update));
	loopPoints->changeState(static_cast<int>(m_timeline->loopEnabled()));

	auto behaviourAtStop = new NStateButton(_tool_bar);
	behaviourAtStop->addState( embed::getIconPixmap( "back_to_zero" ),
					tr( "After stopping go back to beginning" )
									);
	behaviourAtStop->addState( embed::getIconPixmap( "back_to_start" ),
					tr( "After stopping go back to "
						"position at which playing was "
						"started" ) );
	behaviourAtStop->addState( embed::getIconPixmap( "keep_stop_position" ),
					tr( "After stopping keep position" ) );
	connect(behaviourAtStop, &NStateButton::changedState, m_timeline,
		[timeline = m_timeline](int value) {
			timeline->setStopBehaviour(static_cast<Timeline::StopBehaviour>(value));
		}
	);
	connect(m_timeline, &Timeline::stopBehaviourChanged, behaviourAtStop,
		[button = behaviourAtStop](Timeline::StopBehaviour value) {
			button->changeState(static_cast<int>(value));
		}
	);
	behaviourAtStop->changeState(static_cast<int>(m_timeline->stopBehaviour()));

	_tool_bar->addWidget( autoScroll );
	_tool_bar->addWidget( loopPoints );
	_tool_bar->addWidget( behaviourAtStop );
}

void TimeLineWidget::updatePosition()
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
	m_autoScroll = static_cast<AutoScrollState>( _n );
}

void TimeLineWidget::paintEvent( QPaintEvent * )
{
	QPainter p( this );

	// Draw background
	p.fillRect( 0, 0, width(), height(), p.background() );

	// Clip so that we only draw everything starting from the offset
	const int leftMargin = m_xOffset + m_posMarkerPixmap.width() / 2;
	p.setClipRect(leftMargin, 0, width() - leftMargin, height() );

	// Draw the loop rectangle
	int const & loopRectMargin = getLoopRectangleVerticalPadding();
	int const loopRectHeight = this->height() - 2 * loopRectMargin;
	int const loopStart = markerX(m_timeline->loopBegin()) + 8;
	int const loopEndR = markerX(m_timeline->loopEnd()) + 9;
	int const loopRectWidth = loopEndR - loopStart;

	bool const loopPointsActive = m_timeline->loopEnabled();

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

	bar_t barNumber = m_begin.getBar();
	int const x = m_xOffset + m_posMarkerPixmap.width() / 2
		- ((static_cast<int>(m_begin * m_ppb) / TimePos::ticksPerBar()) % static_cast<int>(m_ppb));

	// Double the interval between bar numbers until they are far enough appart
	int barLabelInterval = 1;
	while (barLabelInterval * m_ppb < MIN_BAR_LABEL_DISTANCE) { barLabelInterval *= 2; }

	for( int i = 0; x + i * m_ppb < width(); ++i )
	{
		++barNumber;
		if ((barNumber - 1) % barLabelInterval == 0)
		{
			const int cx = x + qRound( i * m_ppb );
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

	// Only draw the position marker if the position line is in view
	if (m_posMarkerX >= m_xOffset && m_posMarkerX < width() - m_posMarkerPixmap.width() / 2)
	{
		// Let the position marker extrude to the left
		p.setClipping(false);
		p.setOpacity(0.6);
		p.drawPixmap(m_posMarkerX, height() - m_posMarkerPixmap.height(), m_posMarkerPixmap);
	}
}




void TimeLineWidget::mousePressEvent( QMouseEvent* event )
{
	if( event->x() < m_xOffset )
	{
		return;
	}
	if( event->button() == Qt::LeftButton  && !(event->modifiers() & Qt::ShiftModifier) )
	{
		m_action = Action::MovePositionMarker;
		if (event->x() - m_xOffset < m_posMarkerPixmap.width())
		{
			m_moveXOff = event->x() - m_xOffset;
		}
		else
		{
			m_moveXOff = m_posMarkerPixmap.width() / 2;
		}
	}
	else if( event->button() == Qt::LeftButton  && (event->modifiers() & Qt::ShiftModifier) )
	{
		m_action = Action::SelectSongClip;
		m_initalXSelect = event->x();
	}
	else if( event->button() == Qt::RightButton )
	{
		m_moveXOff = m_posMarkerPixmap.width() / 2;

		const auto cursorXOffset = std::max(event->x() - m_xOffset - m_moveXOff, 0);
		const TimePos timeAtCursor = m_begin + static_cast<int>(cursorXOffset * TimePos::ticksPerBar() / m_ppb);
		const TimePos loopMid = (m_timeline->loopBegin() + m_timeline->loopEnd()) / 2;

		m_action = timeAtCursor < loopMid ? Action::MoveLoopBegin : Action::MoveLoopEnd;
	}

	if( m_action == Action::MoveLoopBegin || m_action == Action::MoveLoopEnd )
	{
		delete m_hint;
		m_hint = TextFloat::displayMessage( tr( "Hint" ),
					tr( "Press <%1> to disable magnetic loop points." ).arg(UI_CTRL_KEY),
					embed::getIconPixmap( "hint" ), 0 );
	}
	mouseMoveEvent( event );
}




void TimeLineWidget::mouseMoveEvent( QMouseEvent* event )
{
	parentWidget()->update(); // essential for widgets that this timeline had taken their mouse move event from.

	const auto cursorXOffset = std::max(event->x() - m_xOffset - m_moveXOff, 0);
	TimePos timeAtCursor = m_begin + static_cast<int>(cursorXOffset * TimePos::ticksPerBar() / m_ppb);

	switch( m_action )
	{
		case Action::MovePositionMarker:
			m_pos.setTicks(timeAtCursor.getTicks());
			Engine::getSong()->setToTime(timeAtCursor, m_mode);
			if (!( Engine::getSong()->isPlaying()))
			{
				//Song::PlayMode::None is used when nothing is being played.
				Engine::getSong()->setToTime(timeAtCursor, Song::PlayMode::None);
			}
			m_pos.setCurrentFrame( 0 );
			m_pos.setJumped( true );
			updatePosition();
			break;

		case Action::MoveLoopBegin:
		case Action::MoveLoopEnd:
		{
			const auto otherPoint = m_action == Action::MoveLoopBegin
				? m_timeline->loopEnd()
				: m_timeline->loopBegin();
			const bool control = event->modifiers() & Qt::ControlModifier;
			if (control)
			{
				// no ctrl-press-hint when having ctrl pressed
				delete m_hint;
				m_hint = nullptr;
			}
			else
			{
				timeAtCursor = timeAtCursor.quantize(m_snapSize);
			}
			// Catch begin == end
			if (timeAtCursor == otherPoint)
			{
				const int offset = control ? 1 : m_snapSize * TimePos::ticksPerBar();
				if (m_action == Action::MoveLoopBegin) { timeAtCursor -= offset; }
				else { timeAtCursor += offset; }
			}
			// Update m_action so we still move the correct point even if it is
			// dragged past the other.
			m_action = timeAtCursor < otherPoint ? Action::MoveLoopBegin : Action::MoveLoopEnd;
			m_timeline->setLoopPoints(timeAtCursor, otherPoint);
			update();
			break;
		}
	case Action::SelectSongClip:
			emit regionSelectedFromPixels( m_initalXSelect , event->x() );
		break;

		default:
			break;
	}
}




void TimeLineWidget::mouseReleaseEvent( QMouseEvent* event )
{
	delete m_hint;
	m_hint = nullptr;
	if ( m_action == Action::SelectSongClip ) { emit selectionFinished(); }
	m_action = Action::NoAction;
}


} // namespace lmms::gui
