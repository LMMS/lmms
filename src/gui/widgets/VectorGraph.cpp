/*
 * VectorGraph.cpp - vector-based replacement for Graph.cpp
 *
 * Copyright (c) 2018 Joshua Wade
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

#include "VectorGraph.h"

VectorGraph::VectorGraph( QWidget * _parent, int _width, int _height ) :
	QWidget( _parent ),
	ModelView(new VectorGraphModel(NULL, true), this)
{
	resize( _width, _height );

	m_width = _width;
	m_height = _height;

	QVector<VectorGraphPoint> points = QVector<VectorGraphPoint>();

	points.append(VectorGraphPoint(0, 0));
	points.append(VectorGraphPoint(0.2, 0.5));
	points.append(VectorGraphPoint(0.7, 0.3));
	points.append(VectorGraphPoint(0.8, 0.9));
	points.append(VectorGraphPoint(1, 1));

	model()->setPoints(points);
}

void VectorGraph::paintEvent( QPaintEvent * event )
{
	QPainter m_canvas( this );
	m_canvas.setRenderHint(QPainter::Antialiasing);
	QPen pen = QPen();
	pen.setWidth(1.5);
	pen.setColor(Qt::white);
	m_canvas.setPen(pen);

	for (int i = 0; i < model()->getPointCount(); i++)
	{
		auto point = model()->getPoint(i);
		m_canvas.drawEllipse(QPoint(point->x() * m_width, (1 - point->y()) * m_height), 5, 5);
	}
}




VectorGraphModel::VectorGraphModel(::Model * _parent, bool _default_constructed):
	Model(_parent, tr("VectorGraph"), _default_constructed)
{
	m_points = QVector<VectorGraphPoint>();
}

VectorGraphPoint * VectorGraphModel::getPoint(int index)
{
	return & m_points[index];
}




VectorGraphPoint::VectorGraphPoint(float x, float y)
{
	m_x = x;
	m_y = y;
}

VectorGraphPoint::VectorGraphPoint()
{
	m_x = 0;
	m_y = 0;
}
