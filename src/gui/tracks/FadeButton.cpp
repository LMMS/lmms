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

#include "embed.h"
#include "FadeButton.h"


namespace lmms::gui
{

const float FadeDuration = 300;


FadeButton::FadeButton(const QColor & _normal_color,
		const QColor & _activated_color,
		const QColor & holdColor,
		QWidget * _parent) :
	QAbstractButton( _parent ),
	m_stateTimer(),
	m_releaseTimer(),
	m_normalColor( _normal_color ),
	m_activatedColor( _activated_color ),
	m_holdColor( holdColor )
{
	setCursor(Qt::PointingHandCursor);
	setFocusPolicy(Qt::NoFocus);
	activeNotes = 0;
}



void FadeButton::setActiveColor(const QColor & activated_color)
{
	m_activatedColor = activated_color;
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
	QColor col = m_normalColor;

	if(m_stateTimer.isValid() && m_stateTimer.elapsed() < FadeDuration)
	{
		// The first part of the fade, when a note is triggered.
		col = fadeToColor(m_activatedColor, m_holdColor, m_stateTimer, FadeDuration);
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
		col = fadeToColor(m_holdColor, m_normalColor, m_releaseTimer, FadeDuration);
		QTimer::singleShot(20, this, SLOT(update()));
	}
	else
	{
		// No fade, no notes. Set to default color.
		col = m_normalColor;
	}

	QPainter p(this);
	p.fillRect(rect(), col);

	int w = rect().right();
	int h = rect().bottom();
	p.setPen(m_normalColor.darker(130));
	p.drawLine(w, 1, w, h);
	p.drawLine(1, h, w, h);
	p.setPen(m_normalColor.lighter(130));
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


} // namespace lmms::gui
