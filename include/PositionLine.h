/*
 * PositionLine.h - declaration of class PositionLine, a simple widget that
 *                  draws a line, mainly works with TimeLineWidget
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

#ifndef POSITION_LINE_H
#define POSITION_LINE_H

#include <QWidget>

class PositionLine : public QWidget
{
	Q_OBJECT
	Q_PROPERTY(bool tailGradient MEMBER m_hasTailGradient)
	Q_PROPERTY(QColor lineColor MEMBER m_lineColor)
public:
	PositionLine(QWidget* parent);

public slots:
	void zoomChange(double zoom);

private:
	void paintEvent(QPaintEvent* pe) override;

	bool m_hasTailGradient;
	QColor m_lineColor;
};

#endif