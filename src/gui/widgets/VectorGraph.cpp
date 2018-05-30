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

#include <QMouseEvent>

#include "VectorGraph.h"
#include "lmms_math.h"
#include "CaptionMenu.h"

VectorGraph::VectorGraph( QWidget * _parent, int _width, int _height ) :
	QWidget( _parent ),
	ModelView(new VectorGraphModel(NULL, true), this)
{
	resize( _width, _height );

	m_width = _width;
	m_height = _height;

	m_resolution = m_width; // Maybe find a more efficient way to make the ends appear where they should
	m_currentPoint = -1;
	installEventFilter(this);
}

void VectorGraph::paintEvent( QPaintEvent * event )
{
	QPainter m_canvas( this );
	m_canvas.setRenderHint(QPainter::Antialiasing);
	QPen pen = QPen();
	pen.setWidth(1.5);
	pen.setColor(Qt::white);
	m_canvas.setPen(pen);

	QPainterPath path;
	VectorGraphPoint * firstPoint = model()->getPoint(0);
	path.moveTo(firstPoint->x() * m_width, (1 - firstPoint->y()) * m_height);

	for (int i = 0; i < m_resolution; i++)
	{
		auto h = (model()->calculateSample((float) i / m_resolution));
		path.lineTo(((float) i / m_resolution) * m_width,
					(1 - h) * m_height);
	}

	m_canvas.drawPath(path);

	for (int i = 0; i < model()->getPointCount(); i++)
	{
		auto point = model()->getPoint(i);
		m_canvas.drawEllipse(QPoint(point->x() * m_width, (1 - point->y()) * m_height), model()->getPointSize(), model()->getPointSize());
	}

	for (int i = 1; i < model()->getPointCount(); i++)
	{
		auto point = model()->getPoint(i);
		auto previousPoint = model()->getPoint(i - 1);
		float xValueToDrawAt = ((point->x() + previousPoint->x()) / 2) * m_width;
		m_canvas.drawEllipse(QPoint(xValueToDrawAt, (1 - model()->calculateSample(xValueToDrawAt / m_width)) * m_height), model()->getTensionHandleSize(), model()->getTensionHandleSize());
	}
}

void VectorGraph::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::MouseButton::RightButton)
	{
		int pointIndex = model()->getPointIndexFromCoords(event->x(), m_height - event->y(), m_width, m_height);
		if (pointIndex >= 0)
		{
			event->ignore();
			return;
		}

		int leftBoundIndex = model()->getSectionStartIndex((float) event->x() / m_width);
		model()->insertPointAfter(leftBoundIndex,
			VectorGraphPoint(
				(float) event->x() / m_width,
				1 - (float) event->y() / m_height,
				0,
				VectorGraphPoint::SingleCurve
			)
		);
		model()->setCurrentDraggedPoint(leftBoundIndex + 1);
		event->accept();
		update();
	}
	else if (event->button() == Qt::MouseButton::LeftButton)
	{
		int pointIndex = model()->getPointIndexFromCoords(event->x(), m_height - event->y(), m_width, m_height);
		int tensionHandleIndex = model()->getPointIndexFromTensionHandleCoords(event->x(), m_height - event->y(), m_width, m_height);

		if (pointIndex > -1)
		{
			model()->setCurrentDraggedPoint(pointIndex);
		}
		else if (tensionHandleIndex > -1)
		{
			setCursor(Qt::BlankCursor);
			model()->setStoredCursorPos(cursor().pos());
			model()->setCurrentDraggedTensionHandle(tensionHandleIndex);
		}
	}
}

void VectorGraph::mouseMoveEvent(QMouseEvent *event)
{
	if (model()->getCurrentDraggedPoint() != -1)
	{
		model()->tryMove(model()->getCurrentDraggedPoint(), (float) event->x() / m_width, 1 - (float) event->y() / m_height);
		update();
	}

	if (model()->getCurrentDraggedTensionHandle() != -1)
	{
		QCursor c = cursor();
		float delta = c.pos().y() - model()->getStoredCursorPos().y();
		c.setPos(model()->getStoredCursorPos());
		setCursor(c);

		int index = model()->getCurrentDraggedTensionHandle();
		VectorGraphPoint * previousPoint = model()->getPoint(index - 1);
		VectorGraphPoint * pointToEdit = model()->getPoint(index);

		if (previousPoint->y() > pointToEdit->y())
			delta *= -1;

		// Subtracting, moving down vertically makes the y value go up
		float newTension = pointToEdit->tension() - delta / 250; // Make adjustable from somewhere else - this is an important tweak
		if (newTension > 1)
			newTension = 1;
		else if (newTension < -1)
			newTension = -1;
		pointToEdit->setTension(newTension);
		update();
	}
}

void VectorGraph::mouseReleaseEvent(QMouseEvent * event)
{
	if (model()->getCurrentDraggedPoint() != -1)
		model()->resetCurrentDraggedPoint();

	if (model()->getCurrentDraggedTensionHandle() > -1)
	{
		QCursor c = cursor();
		//c.setPos(mapToGlobal(QPoint(15, 15)));
		c.setPos(model()->getStoredCursorPos());
		c.setShape(Qt::ArrowCursor);
		setCursor(c);
		model()->resetCurrentDraggedTensionHandle();
	}
}

bool VectorGraph::eventFilter(QObject *watched, QEvent *event)
{
	if (event->type() == QEvent::ContextMenu)
	{
		if (model()->getCurrentDraggedPoint() >= 0)
			return false;

		QContextMenuEvent * menuEvent = static_cast<QContextMenuEvent*>(event);

		m_currentPoint = model()->getPointIndexFromCoords(menuEvent->x(), m_height - menuEvent->y(), m_width, m_height);

		if (m_currentPoint < 0)
			return false;

		CaptionMenu contextMenu(model()->displayName(), this);
		contextMenu.addAction(QPixmap(), tr("&Delete"), this, SLOT(deletePoint()));
		contextMenu.exec(QCursor::pos());
		return true;
	}
	return false;
}

float VectorGraph::calculateSample(float input)
{
	return model()->calculateSample(input);
}

void VectorGraph::deletePoint()
{
	if (m_currentPoint < 0)
		return;

	model()->deletePoint(m_currentPoint);
	m_currentPoint = -1;
	update();
}




VectorGraphModel::VectorGraphModel(::Model * _parent, bool _default_constructed):
	Model(_parent, tr("VectorGraph"), _default_constructed)
{
	m_points = QVector<VectorGraphPoint>();

	auto firstPoint = VectorGraphPoint(0, 0, 0, VectorGraphPoint::TensionType::SingleCurve);
	firstPoint.permaLockX();
	firstPoint.permaLockY();
	m_points.append(firstPoint);
	auto finalPoint = VectorGraphPoint(1, 1, 0, VectorGraphPoint::TensionType::SingleCurve);
	finalPoint.permaLockX();
	m_points.append(finalPoint);

	m_currentDraggedPoint = -1;
	m_currentDraggedTensionHandle = -1;
}

VectorGraphPoint * VectorGraphModel::getPoint(int index)
{
	return & m_points[index];
}

int VectorGraphModel::getSectionStartIndex(float input)
{
	if (m_points.size() == 0)
	{
		return -1;
	}
	else if (m_points.size() == 1)
	{
		return 0;
	}

	for (int i = 1; i < m_points.size(); i++)
	{
		if (m_points[i].x() > input || floatEqual(m_points[i].x(), input, 0.000001)) // unsure if this is a good epsilon
		{
			return i - 1;
		}
	}

	return -1;
}

float VectorGraphModel::calculateSectionSample(float input, int sectionStartIndex)
{
	if (m_points.size() == 1 && sectionStartIndex == 0)
	{
		return m_points[0].y();
	}

	VectorGraphPoint * point = getPoint(sectionStartIndex + 1);

	// I'm not convinced that the code below provides any sort of speedup.
	// Might be useful for preventing edge cases though.
	// It would if the power function is much less efficient, which I think it might be.
	if (floatEqual(point->tension(), 0, 0.00001)) // I have no idea what epsilon to use, probably doesn't matter in this case though
	{
		return input;
	}

	//return point->dryAmt() * input + (1 - point->dryAmt()) * fastPow(input, point->tensionPower());
	/*if (point->tension() < 0)
		return qPow(input, point->tensionPower());
	else
		return 1 - qPow(1 - input, point->absTensionPower());*/


	// based on a cycloid

	float mult = 0.67502558231353759765625; // yay hard-coded values

	float invInput = 1 - input;

	if (point->tension() < 0)
		return point->dryAmt() * input + (1 - point->dryAmt()) * qPow(mult * (qAcos(1 - input / mult) - qSqrt(input * (2 * mult - input))), point->tensionPower());
	else
		return point->dryAmt() * input + (1 - point->dryAmt()) * (1 - qPow(mult * (qAcos(1 - invInput / mult) - qSqrt(invInput * (2 * mult - invInput))), point->absTensionPower()));
}

float VectorGraphModel::calculateSample(float input)
{
	int startIndex = getSectionStartIndex(input);
	int endIndex = startIndex + 1;

	VectorGraphPoint * startPoint = getPoint(startIndex);
	VectorGraphPoint * endPoint = getPoint(endIndex);

	float sectionNormalizedInput = (input - startPoint->x()) * (1 / (endPoint->x() - startPoint->x()));
	float sectionNormalizedOutput = calculateSectionSample(sectionNormalizedInput, startIndex);
	float output = sectionNormalizedOutput * (endPoint->y() - startPoint->y()) + startPoint->y();
	return output;
}

void VectorGraphModel::insertPointAfter(int index, VectorGraphPoint point)
{
	m_points.insert(index + 1, point);
}

void VectorGraphModel::tryMove(int index, float x, float y)
{
	VectorGraphPoint * currentPoint = getPoint(index);

	if (!currentPoint->isXLocked())
	{
		bool checkRight = true;

		if (index + 1 == m_points.size())
		{
			checkRight = false;
		}

		VectorGraphPoint * leftPoint = getPoint(index - 1);
		VectorGraphPoint * rightPoint;
		if (checkRight)
		{
			rightPoint = getPoint(index + 1);
		}

		if (x < leftPoint->x())
		{
			currentPoint->setX(leftPoint->x());
		}
		else if (checkRight && x > rightPoint->x())
		{
			currentPoint->setX(rightPoint->x());
		}
		else
		{
			currentPoint->setX(x);
		}
	}

	if (!currentPoint->isYLocked())
	{
		if (y > 1)
		{
			currentPoint->setY(1);
		}
		else if (y < 0)
		{
			currentPoint->setY(0);
		}
		else
		{
			currentPoint->setY(y);
		}
	}
}

int VectorGraphModel::getPointIndexFromCoords(int x, int y, int canvasWidth, int canvasHeight)
{
	for (int i = 0; i < m_points.size(); i++)
	{
		VectorGraphPoint * point = getPoint(i);
		if (point->isXLocked() && point->isYLocked())
		{
			continue;
		}
		if (arePointsWithinDistance(x, point->x() * canvasWidth, y, point->y() * canvasHeight, getPointSize() + 2))
		{
			return i;
		}
	}
	return -1;
}

int VectorGraphModel::getPointIndexFromTensionHandleCoords(int x, int y, int canvasWidth, int canvasHeight)
{
	for (int i = 1; i < m_points.size(); i++)
	{
		VectorGraphPoint * startPoint = getPoint(i - 1);
		VectorGraphPoint * endPoint = getPoint(i);
		float tensionHandleCenterX = ((startPoint->x() + endPoint->x()) / 2) * canvasWidth;
		float tensionHandleCenterY = calculateSample(tensionHandleCenterX / canvasWidth) * canvasHeight;
		if (arePointsWithinDistance(x, tensionHandleCenterX, y, tensionHandleCenterY, getTensionHandleSize() + 2))
		{
			return i;
		}
	}
	return -1;
}

void VectorGraphModel::deletePoint(int index)
{
	m_points.removeAt(index);
}





VectorGraphPoint::VectorGraphPoint(float x, float y, float tension, TensionType type)
{
	m_x = x;
	m_y = y;
	setTension(tension);
	m_tensionType = type;
	m_isXLocked = false;
	m_isYLocked = false;
	m_isXPermaLocked = false;
	m_isYPermaLocked = false;
}

VectorGraphPoint::VectorGraphPoint()
{
	m_x = 0;
	m_y = 0;
	setTension(0);
	m_tensionType = TensionType::SingleCurve;
	m_isXLocked = false;
	m_isYLocked = false;
	m_isXPermaLocked = false;
	m_isYPermaLocked = false;
}
