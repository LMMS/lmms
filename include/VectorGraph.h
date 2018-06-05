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

// Documentation:
// https://github.com/SecondFlight/lmms/wiki/VectorGraph

#ifndef VECTORGRAPH_H
#define VECTORGRAPH_H

#include <QWidget>
#include <QPainter>
#include <QVector>
#include <QtMath>

#include "ModelView.h"

class VectorGraphModel;
class VectorGraphPoint;




class EXPORT VectorGraph : public QWidget, public ModelView
{
	Q_OBJECT
public:
	enum TensionType
	{
		Hold,
		SingleCurve,
		DoubleCurve,
		Stairs,
		Pulse,
		Wave
	};
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

	inline int getMargin()
	{
		return m_margin;
	}

	inline float rawToCoordX(float input)
	{
		auto result = input * (m_width - 1) * (((m_width - 1) - 2 * m_margin)/(float) (m_width - 1)) + m_margin;
		return result;
	}

	inline float rawToCoordY(float input)
	{
		auto result = (1 - input) * (m_height - 1) * ((m_height - 1) - 2 * m_margin)/(float) (m_height - 1) + m_margin;
		return result;
	}

	inline float coordToRawX(float input)
	{
		return (input - m_margin) * ((m_width - 1)/(float) ((m_width - 1) - 2 * m_margin)) / (m_width - 1);
	}

	inline float coordToRawY(float input)
	{
		return ((m_height - input) - m_margin) * ((m_height - 1)/(float) ((m_height - 1) - 2 * m_margin)) / (m_height - 1);
	}

	inline void setMargin(int margin)
	{
		// The extra space is necessary for the
		// 2-pixel border, and because the
		// canvas goes from 1 to (m_width - 1) in
		// the X and 1 to (m_height - 1) in the y
		m_margin = margin + 3;
	}

	float calculateSample(float input);
	bool eventFilter(QObject *watched, QEvent *event);

signals:
	void drawn();
protected:
	void paintEvent( QPaintEvent * event ) override;
	void mousePressEvent( QMouseEvent * event );
	void mouseMoveEvent( QMouseEvent * event );
	void mouseReleaseEvent( QMouseEvent * event );

private slots:
	void deletePoint();
	void setTensionToHold();
	void setTensionToSingle();
	void setTensionToDouble();
	void setTensionToStairs();
	void setTensionToPulse();
	void setTensionToWave();

private:
	QPainter m_canvas;
	int m_resolution;
	int m_width;
	int m_height;
	int m_currentPoint;
	int m_margin;
	float getTensionHandleYVal(int index);
	float getTensionHandleXVal(int index);
	void setLastModifiedPoint(int pointIndex);
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

	inline void setCurrentDraggedPoint(int index)
	{
		m_currentDraggedPoint = index;
	}

	inline void resetCurrentDraggedPoint()
	{
		m_currentDraggedPoint = -1;
	}

	inline int getCurrentDraggedPoint()
	{
		return m_currentDraggedPoint;
	}

	inline void setCurrentDraggedTensionHandle(int index)
	{
		m_currentDraggedTensionHandle = index;
	}

	inline void resetCurrentDraggedTensionHandle()
	{
		m_currentDraggedTensionHandle = -1;
	}

	inline int getCurrentDraggedTensionHandle()
	{
		return m_currentDraggedTensionHandle;
	}

	inline QPoint getStoredCursorPos()
	{
		return m_storedCursorPos;
	}

	inline void setStoredCursorPos(QPoint point)
	{
		m_storedCursorPos = point;
	}

	inline void setLastModifiedTension(float tension)
	{
		m_lastModifiedTension = tension;
	}

	inline float getLastModifiedTension()
	{
		return m_lastModifiedTension;
	}

	inline void setLastModifiedTensionType(VectorGraph::TensionType tensionType)
	{
		m_lastModifiedTensionType = tensionType;
	}

	inline VectorGraph::TensionType getLastModifiedTensionType()
	{
		return m_lastModifiedTensionType;
	}

	void setTensionTypeOnPoint(int index, VectorGraph::TensionType type);

	static inline bool floatEqual(float a, float b, float epsilon)
	{
		return qFabs(a - b) < epsilon;
	}

	VectorGraphPoint * getPoint(int index);
	float calculateSample(float input);
	float calculateSectionSample(float input, int sectionStartIndex);
	int getSectionStartIndex(float input);
	void insertPointAfter(int index, VectorGraphPoint point);
	void tryMove(int index, float x, float y);
	int getPointIndexFromCoords(int x, int y, int canvasWidth, int canvasHeight);
	int getPointIndexFromTensionHandleCoords(int x, int y, int canvasWidth, int canvasHeight);
	void deletePoint(int index);


	const inline int getPointSize()
	{
		return 5;
	}
	const inline int getTensionHandleSize()
	{
		return 3;
	}
	inline bool isGridEnabled()
	{
		return m_gridEnabled;
	}
	inline void setIsGridEnabled(bool enabled)
	{
		m_gridEnabled = enabled;
	}
	inline int getNumGridLines()
	{
		return m_numGridLines;
	}
	inline void setNumGridLines(int num)
	{
		m_numGridLines = num;
	}
	inline bool isGridSnapEnabled()
	{
		return m_gridSnapEnabled;
	}
	inline void setIsGridSnapEnabled(bool enabled)
	{
		m_gridSnapEnabled = enabled;
	}

private:
	QVector<VectorGraphPoint> m_points;
	int m_currentDraggedPoint;
	int m_currentDraggedTensionHandle;
	QPoint m_storedCursorPos;
	float m_lastModifiedTension;
	VectorGraph::TensionType m_lastModifiedTensionType;
	bool m_gridEnabled;
	int m_numGridLines;
	bool m_gridSnapEnabled;

	static inline bool arePointsWithinDistance(float x1, float x2, float y1, float y2, float distance)
	{
		//return qPow(x2 - x1, 2) + qPow(y2 - y1, 2) <= qPow(distance, 2);
		qreal aSquared = qPow(x2 - x1, 2);
		qreal bSquared = qPow(y2 - y1, 2);
		qreal cSquared = qPow(distance, 2);
		bool result = aSquared + bSquared <= cSquared;
		return result;
	}

	float CalculateSingleCurve(float input, VectorGraphPoint * point);
};



class VectorGraphPoint
{
public:
	VectorGraphPoint(float x, float y, float tension, VectorGraph::TensionType type);
	VectorGraphPoint(VectorGraphPoint * point);
	VectorGraphPoint();
	inline float x()
	{
		return m_x;
	}
	inline float y()
	{
		return m_y;
	}
	inline void setX(float x)
	{
		m_x = x;
	}
	inline void setY(float y)
	{
		m_y = y;
	}
	inline float tension()
	{
		return m_tension;
	}
	inline VectorGraph::TensionType tensionType()
	{
		return m_tensionType;
	}
	inline float tensionPower()
	{
		return m_tensionPower;
	}
	inline float absTensionPower()
	{
		return m_absTensionPower;
	}
	inline float dryAmt()
	{
		return m_dryAmt;
	}
	inline void setTension(float tension)
	{
		m_tension = tension;

		// Variables for single curve
		m_tensionPower = qPow(20, -1 * tension);
		m_absTensionPower = qPow(20, qAbs(tension));

		m_invTensionPower = qPow(20, tension);
		m_invAbsTensionPower = qPow(20, qAbs(-1 * tension));

		m_dryAmt = qPow(1 - qAbs(tension), 5);
	}
	inline void invertTension()
	{
		auto tensionPowerStore = m_tensionPower;
		auto absTensionPowerStore = m_absTensionPower;
		m_tensionPower = m_invTensionPower;
		m_absTensionPower = m_invAbsTensionPower;
		m_invTensionPower = tensionPowerStore;
		m_invAbsTensionPower = absTensionPowerStore;
		m_tension = -1 * m_tension;
	}
	inline void lockX()
	{
		m_isXLocked = true;
	}
	inline void lockY()
	{
		m_isYLocked = true;
	}
	inline void permaLockX()
	{
		m_isXPermaLocked = true;
	}
	inline void permaLockY()
	{
		m_isYPermaLocked = true;
	}
	inline void unlockX()
	{
		m_isXLocked = false;
	}
	inline void unlockY()
	{
		m_isYLocked = false;
	}
	inline void permaUnlockX()
	{
		m_isXPermaLocked = false;
	}
	inline void permaUnlockY()
	{
		m_isYPermaLocked = false;
	}
	inline bool isXLocked()
	{
		return m_isXLocked || m_isXPermaLocked;
	}
	inline bool isYLocked()
	{
		return m_isYLocked || m_isYPermaLocked;
	}
	inline void setTensionType(VectorGraph::TensionType type)
	{
		m_tensionType = type;
	}
	inline VectorGraph::TensionType getTensionType()
	{
		return m_tensionType;
	}
	inline bool canBeDeleted()
	{
		return m_canBeDeleted;
	}
	inline void setDeletable(bool deletable)
	{
		m_canBeDeleted = deletable;
	}
private:
	float m_x;
	float m_y;
	float m_tension;
	float m_tensionPower;
	float m_absTensionPower;
	float m_invTensionPower;
	float m_invAbsTensionPower;
	float m_dryAmt;
	VectorGraph::TensionType m_tensionType;
	bool m_isXLocked;
	bool m_isYLocked;
	bool m_isXPermaLocked;
	bool m_isYPermaLocked;
	bool m_canBeDeleted;
};

#endif
