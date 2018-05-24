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

#include "ModelView.h"

class VectorGraphModel;
class VectorGraphPoint;




class EXPORT VectorGraph : public QWidget, public ModelView
{
	Q_OBJECT
public:
	VectorGraph( QWidget * _parent, int _width, int _height );
	virtual ~VectorGraph() = default;

	inline void setResolution(int resolution)
	{
		m_resolution = resolution;
	}

	inline VectorGraphModel * model()
	{
		return castModel<VectorGraphModel>();
	}

	inline int getWidth()
	{
		return m_width;
	}

	inline int getHeight()
	{
		return m_height;
	}

protected:
	void paintEvent( QPaintEvent * event ) override;

private:
	QPainter m_canvas;
	int m_resolution;
	int m_width;
	int m_height;
};



class EXPORT VectorGraphModel : public Model
{
	Q_OBJECT
public:
	VectorGraphModel(Model *_parent, bool _default_constructed);
	virtual ~VectorGraphModel() = default;

	inline void setPoints(QVector<VectorGraphPoint> points)
	{
		m_points = points;
	}

	inline int getPointCount()
	{
		return m_points.size();
	}

	VectorGraphPoint * getPoint(int index);

private:
	QVector<VectorGraphPoint> m_points;
};



class VectorGraphPoint
{
public:
	VectorGraphPoint(float x, float y);
	VectorGraphPoint();
	inline float x()
	{
		return m_x;
	}
	inline float y()
	{
		return m_y;
	}
private:
	float m_x;
	float m_y;
};

#endif
