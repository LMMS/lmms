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


#include <QGuiApplication>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>
#include <QToolBar>

#include "ConfigManager.h"
#include "embed.h"
#include "KeyboardShortcuts.h"
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
	QWidget{parent},
	m_xOffset{xoff},
	m_ppb{ppb},
	m_pos{pos},
	m_timeline{&timeline},
	m_begin{begin},
	m_mode{mode}
{
	move( 0, yoff );

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
}

void TimeLineWidget::addToolButtons( QToolBar * _tool_bar )
{
	auto autoScroll = new NStateButton(_tool_bar);
	autoScroll->setGeneralToolTip(tr("Auto scrolling"));
	autoScroll->addState(embed::getIconPixmap("autoscroll_stepped_on"), tr("Stepped auto scrolling"));
	autoScroll->addState(embed::getIconPixmap("autoscroll_continuous_on"), tr("Continuous auto scrolling"));
	autoScroll->addState(embed::getIconPixmap("autoscroll_off"), tr("Auto scrolling disabled"));
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
	emit positionChanged(m_pos);
	update();
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
	p.setClipRect(m_xOffset, 0, width() - m_xOffset, height());

	// Variables for the loop rectangle
	int const loopRectMargin = getLoopRectangleVerticalPadding();
	int const loopRectHeight = this->height() - 2 * loopRectMargin;
	int const loopStart = markerX(m_timeline->loopBegin());
	int const loopEndR = markerX(m_timeline->loopEnd());
	int const loopRectWidth = loopEndR - loopStart;

	bool const loopPointsActive = m_timeline->loopEnabled();

	// Draw the main loop rectangle (inner fill only)
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
	int const x = m_xOffset - ((static_cast<int>(m_begin * m_ppb) / TimePos::ticksPerBar()) % static_cast<int>(m_ppb));

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

	// Draw the loop rectangle's outer outline
	p.setPen( loopPointsActive ? getActiveLoopColor() : getInactiveLoopColor() );
	p.setBrush( Qt::NoBrush );
	p.drawRect( outerRectangle );

	// Draw the loop rectangle's inner outline
	QRect innerRectangle = outerRectangle.adjusted( 1, 1, -1, -1 );
	p.setPen( loopPointsActive ? getActiveLoopInnerColor() : getInactiveLoopInnerColor() );
	p.setBrush( Qt::NoBrush );
	p.drawRect( innerRectangle );
	
	// Draw loop handles if necessary
	const auto handleMode = ConfigManager::inst()->value("app", "loopmarkermode") == "handles";
	if (handleMode && underMouse() && QGuiApplication::keyboardModifiers().testFlag(Qt::ShiftModifier))
	{
		const auto handleWidth = std::min(m_loopHandleWidth, loopRectWidth / 2 - 1);
		const auto leftHandle = QRectF(loopStart - .5, loopRectMargin - .5, handleWidth, loopRectHeight);
		const auto rightHandle = QRectF(loopEndR - handleWidth - .5, loopRectMargin - .5, handleWidth, loopRectHeight);
		const auto color = loopPointsActive ? m_activeLoopHandleColor : m_inactiveLoopHandleColor;

		p.fillRect(leftHandle, color);
		p.fillRect(rightHandle, color);
	}

	// Only draw the position marker if the position line is in view
	if (markerX(m_pos) >= m_xOffset && markerX(m_pos) < width() - m_posMarkerPixmap.width() / 2)
	{
		// Let the position marker extrude to the left
		p.setClipping(false);
		p.setOpacity(0.6);
		p.drawPixmap(markerX(m_pos) - (m_posMarkerPixmap.width() / 2),
			height() - m_posMarkerPixmap.height(), m_posMarkerPixmap);
	}
}

auto TimeLineWidget::getClickedTime(const int xPosition) const -> TimePos
{
	// How far into the timeline we clicked, measuring pixels from the leftmost part of the editor
	const auto pixelDelta = std::max(xPosition - m_xOffset, 0);
	return m_begin + static_cast<int>(pixelDelta * TimePos::ticksPerBar() / m_ppb);
}

auto TimeLineWidget::getLoopAction(QMouseEvent* event) const -> TimeLineWidget::Action
{
	const auto mode = ConfigManager::inst()->value("app", "loopmarkermode");
	const auto xPos = event->x();
	const auto button = event->button();

	if (mode == "handles")
	{
		// Loop start and end pos, or closest edge of screen if loop extends off it
		const auto leftMost = std::max(markerX(m_timeline->loopBegin()), m_xOffset);
		const auto rightMost = std::min(markerX(m_timeline->loopEnd()), width());
		// Distance from click to handle, positive aimed towards center of loop
		const auto deltaLeft = xPos - leftMost;
		const auto deltaRight = rightMost - xPos;

		if (deltaLeft < 0 || deltaRight < 0) { return Action::NoAction; } // Clicked outside loop
		else if (deltaLeft <= m_loopHandleWidth && deltaLeft < deltaRight) { return Action::MoveLoopBegin; }
		else if (deltaRight <= m_loopHandleWidth) { return Action::MoveLoopEnd; }
		else { return Action::MoveLoop; }
	}
	else if (mode == "closest")
	{
		const TimePos loopMid = (m_timeline->loopBegin() + m_timeline->loopEnd()) / 2;
		return getClickedTime(xPos) < loopMid ? Action::MoveLoopBegin : Action::MoveLoopEnd;
	}
	else // Default to dual-button mode
	{
		if (button == Qt::LeftButton) { return Action::MoveLoopBegin; }
		else if (button == Qt::RightButton) { return Action::MoveLoopEnd; }
		return Action::NoAction;
	}
}

auto TimeLineWidget::actionCursor(Action action) const -> QCursor
{
	switch (action) {
		case Action::MoveLoop: return Qt::SizeHorCursor;
		case Action::MoveLoopBegin: return m_cursorSelectLeft;
		case Action::MoveLoopEnd: return m_cursorSelectRight;
		// Fall back to normal cursor if no action or action cursor not specified
		default: return Qt::ArrowCursor;
	}
}

void TimeLineWidget::mousePressEvent(QMouseEvent* event)
{
	if (event->x() < m_xOffset) { return; }

	const auto shift = event->modifiers() & Qt::ShiftModifier;
	const auto ctrl = event->modifiers() & Qt::ControlModifier;

	if (shift) // loop marker manipulation
	{
		m_action = getLoopAction(event);
		setCursor(actionCursor(m_action));

		if (m_action == Action::MoveLoop)
		{
			m_dragStartPos = getClickedTime(event->x());
			m_oldLoopPos = {m_timeline->loopBegin(), m_timeline->loopEnd()};
		}
	}
	else if (event->button() == Qt::LeftButton && ctrl) // selection
	{
		m_action = Action::SelectSongClip;
		m_initalXSelect = event->x();
	}
	else if (event->button() == Qt::LeftButton && !ctrl) // move playhead
	{
		m_action = Action::MovePositionMarker;
	}

	if (m_action == Action::MoveLoopBegin || m_action == Action::MoveLoopEnd)
	{
		delete m_hint;
		m_hint = TextFloat::displayMessage(tr("Hint"),
			tr("Press <%1> to disable magnetic loop points.").arg(UI_CTRL_KEY),
			embed::getIconPixmap("hint"), 0);
	}

	setContextMenuPolicy(m_action == Action::NoAction ? Qt::DefaultContextMenu : Qt::PreventContextMenu);

	mouseMoveEvent(event);
}

void TimeLineWidget::mouseMoveEvent( QMouseEvent* event )
{
	parentWidget()->update(); // essential for widgets that this timeline had taken their mouse move event from.

	auto timeAtCursor = getClickedTime(event->x());
	const auto control = event->modifiers() & Qt::ControlModifier;

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
		case Action::MoveLoop:
		{
			const TimePos dragDelta = timeAtCursor - m_dragStartPos;
			auto loopPos = m_oldLoopPos;
			for (auto& point : loopPos)
			{
				point += dragDelta;
				if (!control) { point = point.quantize(m_snapSize); }
			}
			m_timeline->setLoopPoints(loopPos[0], loopPos[1]);
			break;
		}
		case Action::SelectSongClip:
			emit regionSelectedFromPixels( m_initalXSelect , event->x() );
			break;

		default:
			break;
	}

	if (event->buttons() == Qt::NoButton)
	{
		setCursor(QGuiApplication::keyboardModifiers().testFlag(Qt::ShiftModifier)
			? actionCursor(getLoopAction(event))
			: Qt::ArrowCursor
		);
	}
}

void TimeLineWidget::mouseReleaseEvent( QMouseEvent* event )
{
	delete m_hint;
	m_hint = nullptr;
	if ( m_action == Action::SelectSongClip ) { emit selectionFinished(); }
	m_action = Action::NoAction;
}

void TimeLineWidget::contextMenuEvent(QContextMenuEvent* event)
{
	if (event->x() < m_xOffset) { return; }

	auto menu = QMenu{};

	menu.addAction(tr("Set loop begin here"), [this, event] {
		auto begin = getClickedTime(event->x());
		const auto end = m_timeline->loopEnd();
		if (!QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier)) { begin = begin.quantize(m_snapSize); }
		if (begin == end) { m_timeline->setLoopEnd(end + m_snapSize * TimePos::ticksPerBar()); }
		m_timeline->setLoopBegin(begin);
		update();
	});
	menu.addAction(tr("Set loop end here"), [this, event] {
		const auto begin = m_timeline->loopBegin();
		auto end = getClickedTime(event->x());
		if (!QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier)) { end = end.quantize(m_snapSize); }
		if (begin == end) { m_timeline->setLoopBegin(begin - m_snapSize * TimePos::ticksPerBar()); }
		m_timeline->setLoopEnd(end);
		update();
	});

	menu.addSeparator();

	const auto loopMenu = menu.addMenu(tr("Loop edit mode (hold shift)"));
	const auto loopMode = ConfigManager::inst()->value("app", "loopmarkermode", "dual");
	const auto addLoopModeAction = [loopMenu, &loopMode](QString text, QString mode) {
		const auto action = loopMenu->addAction(text, [mode] {
			ConfigManager::inst()->setValue("app", "loopmarkermode", mode);
		});
		action->setCheckable(true);
		if (loopMode == mode) { action->setChecked(true); }
	};
	addLoopModeAction(tr("Dual-button"), "dual");
	addLoopModeAction(tr("Grab closest"), "closest");
	addLoopModeAction(tr("Handles"), "handles");

	menu.exec(event->globalPos());
}

} // namespace lmms::gui
