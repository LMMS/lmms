/*
 * PositionLine.cpp
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

#include "PositionLine.h"

#include <QPainter>


namespace lmms::gui
{

PositionLine::PositionLine(QWidget* parent, Song::PlayMode playMode) :
	QWidget(parent),
	m_playMode(playMode),
	m_hasTailGradient(false),
	m_lineColor(0, 0, 0, 0)
{
	resize(8, height());

	setAttribute(Qt::WA_NoSystemBackground, true);
	setAttribute(Qt::WA_TransparentForMouseEvents);
}

void PositionLine::paintEvent(QPaintEvent* pe)
{
	QPainter p(this);
	auto c = QColor(m_lineColor);

	// If width is 1, we don't need a gradient
	if (width() == 1)
	{
		c.setAlpha(153);
		p.fillRect(rect(), c);
	}
	// If width > 1, we need the gradient
	else
	{
		// Create the gradient trail behind the line
		QLinearGradient gradient(rect().bottomLeft(), rect().bottomRight());
		qreal w = (width() - 1.0) / width();

		// If gradient is enabled, we're in focus and we're playing, enable gradient
		if (m_hasTailGradient &&
			Engine::getSong()->isPlaying() &&
			(Engine::getSong()->playMode() == m_playMode))
		{
			c.setAlpha(60);
			gradient.setColorAt(w, c);
		}
		else
		{
			c.setAlpha(0);
			gradient.setColorAt(w, c);
		}

		// Fill in the remaining parts
		c.setAlpha(0);
		gradient.setColorAt(0, c);
		c.setAlpha(153);
		gradient.setColorAt(1, c);

		// Fill line
		p.fillRect(rect(), gradient);
	}
}

// NOTE: the move() implementation fixes a bug where the position line would appear
// in an unexpected location when positioned at the start of the track
void PositionLine::zoomChange(float zoom)
{
	int playHeadPos = x() + width() - 1;

	resize(std::max(8.0 * zoom, 1.0), height());
	move(playHeadPos - width() + 1, y());

	update();
}


} // namespace lmms::gui
