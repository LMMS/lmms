/*
 * VectorGraph.h - vector-based replacement for Graph.h
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

#ifndef VECTORGRAPH_H
#define VECTORGRAPH_H

#include <QWidget>
#include <QPainter>
#include <QVector>
#include <QPair>

#include "ModelView.h"

class vectorGraphModel;



class EXPORT VectorGraph : public QWidget, public ModelView
{
	Q_OBJECT
public:
	VectorGraph( QWidget * _parent, int _width, int _height );
	virtual ~VectorGraph() = default;

protected:
	void paintEvent( QPaintEvent * event ) override;

private:
	QPainter m_canvas;
};



class EXPORT vectorGraphModel : public Model
{
	Q_OBJECT
public:
	vectorGraphModel();
	virtual ~vectorGraphModel() = default;

private:
	QVector<QPair<float, float>> m_points;
	QVector<float> m_tensions;
}

#endif
