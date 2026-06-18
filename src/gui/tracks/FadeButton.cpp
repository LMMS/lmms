/*
 * FadeButton.cpp - implementation of fade-button
 *
 * Copyright (c) 2005-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QTimer>
#include <QPainter>
#include <QApplication>

#include "FadeButton.h"


namespace lmms::gui
{

const float FadeDuration = 300;

FadeButton::FadeButton(const QColor& inactiveColor, const QColor& normalColor, const QColor& corruptedColor,
	const QColor& mutedColor, const QColor& holdColor, QWidget* _parent)
	: QAbstractButton(_parent)
	, m_stateTimer()
	, m_releaseTimer()
	, m_inactiveColor(inactiveColor)
	, m_normalColor(normalColor)
	, m_coruptedColor(corruptedColor)
	, m_mutedColor(mutedColor)
	, m_holdColor(holdColor)
	, activeNotes(0)
{
	setCursor(Qt::PointingHandCursor);
	setFocusPolicy(Qt::NoFocus);
}

FadeButton::FadeButton(QWidget* parent)
	: FadeButton(QApplication::palette().color(QPalette::Active, QPalette::Window),
		  QApplication::palette().color(QPalette::Active, QPalette::BrightText),
		  QColor::fromHsv(0, 255, QApplication::palette().color(QPalette::Active, QPalette::BrightText).value()),
		  QApplication::palette().color(QPalette::Active, QPalette::Highlight),
		  QApplication::palette().color(QPalette::Active, QPalette::BrightText).darker())
{
}

auto FadeButton::state() const -> State
{
	return m_state;
}

auto FadeButton::muted() const -> bool
{
	return m_muted;
}

void FadeButton::setState(State state)
{
	m_state = state;
}

void FadeButton::setMuted(bool mute)
{
	m_muted = mute;
}

void FadeButton::activate()
{
	m_stateTimer.restart();
	activeNotes++;
	update();
}




void FadeButton::activateOnce()
{
	if (activeNotes == 0) { activate(); }
}




void FadeButton::noteEnd()
{
	if (activeNotes <= 0)
	{
		qWarning("noteEnd() triggered without a corresponding activate()!");
		activeNotes = 0;
	}
	else
	{
		activeNotes--;
	}

	if (activeNotes == 0)
	{
		m_releaseTimer.restart();
	}

	update();
}




void FadeButton::paintEvent(QPaintEvent * _pe)
{
	QColor col = m_inactiveColor;

	if(m_stateTimer.isValid() && m_stateTimer.elapsed() < FadeDuration)
	{
		// The first part of the fade, when a note is triggered.
		col = fadeToColor(activeColor(), m_holdColor, m_stateTimer, FadeDuration);
		QTimer::singleShot(20, this, SLOT(update()));
	}
	else if (m_stateTimer.isValid()
		&& m_stateTimer.elapsed() >= FadeDuration
		&& activeNotes > 0)
	{
		// The fade is done, but at least one note is still held.
		col = m_holdColor;
	}
	else if (m_releaseTimer.isValid() && m_releaseTimer.elapsed() < FadeDuration)
	{
		// Last note just ended. Fade to default color.
		col = fadeToColor(m_holdColor, m_inactiveColor, m_releaseTimer, FadeDuration);
		QTimer::singleShot(20, this, SLOT(update()));
	}
	else
	{
		// No fade, no notes. Set to default color.
		col = m_inactiveColor;
	}

	QPainter p(this);
	p.fillRect(rect(), col);

	int w = rect().right();
	int h = rect().bottom();
	p.setPen(m_inactiveColor.darker(130));
	p.drawLine(w, 1, w, h);
	p.drawLine(1, h, w, h);
	p.setPen(m_inactiveColor.lighter(130));
	p.drawLine(0, 0, 0, h-1);
	p.drawLine(0, 0, w, 0);
}


QColor FadeButton::fadeToColor(QColor startCol, QColor endCol, QElapsedTimer timer, float duration)
{
	QColor col;

	const float state = 1 - timer.elapsed() / duration;
	const int r = (int)(endCol.red() * (1.0f - state)
		+ startCol.red() * state);
	const int g = (int)(endCol.green() * (1.0f - state)
		+ startCol.green() * state);
	const int b = (int)(endCol.blue() * (1.0f - state)
		+ startCol.blue() * state);
	col.setRgb(r, g, b);

	return col;
}

QColor FadeButton::activeColor() const
{
	if (m_muted) { return m_mutedColor; }

	switch (m_state)
	{
	case State::Normal:
		return m_normalColor;
	case State::Corrupted:
		return m_coruptedColor;
	default:
		qWarning("Invalid active color");
		return QColor{};
	}
}


} // namespace lmms::gui
