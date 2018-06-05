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

// Documentation:
// https://github.com/SecondFlight/lmms/wiki/VectorGraph

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

	m_resolution = m_width;
	m_currentPoint = -1;
	installEventFilter(this);
	setMargin(3);
}

void VectorGraph::paintEvent( QPaintEvent * event )
{
	QColor lineColor = QColor(3, 147, 178);
	QColor invisible = QColor(0, 0, 0, 0);
	QColor backgroundColor = QColor(35, 44, 54);
	QColor borderLight = QColor(63, 72, 83);
	QColor borderDark = QColor(32, 40, 50);
	QColor gridColor = QColor(56, 66, 78);

	QPainter m_canvas( this );

	// Background
	m_canvas.setBrush(QBrush(backgroundColor));
	m_canvas.drawRect(QRect(QPoint(0, 1), QPoint(m_width - 1, m_height - 1)));




	// Grid lines
	if (model()->isGridEnabled())
	{
		QPen gridPen = QPen();
		gridPen.setWidth(1);
		gridPen.setColor(gridColor);
		m_canvas.setPen(gridPen);


		int gridAreaWidth = (m_width - 2 * m_margin);
		int gridAreaHeight = (m_height - 2 * m_margin);

		m_canvas.drawLine(m_margin, m_margin, m_margin, m_margin + gridAreaHeight);
		m_canvas.drawLine(m_margin, m_margin + gridAreaHeight, m_margin + gridAreaWidth, m_margin + gridAreaHeight);
		m_canvas.drawLine(m_margin + gridAreaWidth, m_margin + gridAreaHeight, m_margin + gridAreaWidth, m_margin);
		m_canvas.drawLine(m_margin + gridAreaWidth, m_margin, m_margin, m_margin);
		for (int i = 1; i < model()->getNumGridLines(); i++)
		{
			int x = qRound((gridAreaWidth / (float) model()->getNumGridLines()) * i) + m_margin;
			int y = qRound((gridAreaHeight / (float) model()->getNumGridLines()) * i) + m_margin;
			m_canvas.drawLine(x, m_margin, x, m_margin + gridAreaHeight);
			m_canvas.drawLine(m_margin, y, m_margin + gridAreaWidth, y);
		}
	}



	m_canvas.setRenderHint(QPainter::Antialiasing);


	QPen pen = QPen();
	pen.setWidthF(1.2);
	pen.setColor(lineColor);
	m_canvas.setPen(pen);

	m_canvas.setBrush(QBrush(invisible));

	QPainterPath path;
	VectorGraphPoint * firstPoint = model()->getPoint(0);
	path.moveTo(qRound(rawToCoordX(firstPoint->x())), qRound(rawToCoordY(firstPoint->y())));

	int currentSection = -1;

	for (int i = 0; i < m_resolution; i++)
	{
		float x = (float) i / m_resolution;
		int potentialNewSection = model()->getSectionStartIndex(x);
		while (potentialNewSection != currentSection)
		{
			currentSection++;
			path.lineTo(rawToCoordX(x) - 1, rawToCoordY(model()->getPoint(currentSection)->y()));
		}
		auto y = model()->calculateSample(x);
		path.lineTo(rawToCoordX(x), rawToCoordY(y));
	}

	auto lastPoint = model()->getPoint(model()->getPointCount() - 1);

	path.lineTo(rawToCoordX(lastPoint->x()), rawToCoordY(lastPoint->y()));

	m_canvas.drawPath(path);

	m_canvas.setBrush(QBrush(lineColor));

	for (int i = 0; i < model()->getPointCount(); i++)
	{
		auto point = model()->getPoint(i);
		int ps = model()->getPointSize();
		m_canvas.drawEllipse(QPointF(rawToCoordX(point->x()), rawToCoordY(point->y())), ps, ps);
	}

	m_canvas.setBrush(QBrush(invisible));

	for (int i = 1; i < model()->getPointCount(); i++)
	{
		VectorGraphPoint * thisPoint = model()->getPoint(i);
		VectorGraphPoint * prevPoint = model()->getPoint(i - 1);

		auto tensionType = thisPoint->getTensionType();

		if (tensionType == Hold)
			continue;

		int ths = model()->getTensionHandleSize();

		if (tensionType == DoubleCurve ||
			tensionType == Stairs ||
			tensionType == Pulse ||
			tensionType == Wave)
		{
			m_canvas.drawEllipse(QPointF(rawToCoordX((thisPoint->x() + prevPoint->x()) / 2), rawToCoordY((thisPoint->y() + prevPoint->y()) / 2)), ths, ths);
			continue;
		}

		if (model()->floatEqual(thisPoint->x(), prevPoint->x(), 0.00001))
		{
			m_canvas.drawEllipse(QPointF(rawToCoordX(thisPoint->x()), rawToCoordY((thisPoint->y() + prevPoint->y()) / 2)), ths, ths);
			continue;
		}

		float xValueToDrawAt = rawToCoordX(getTensionHandleXVal(i));
		float yValueToDrawAt = rawToCoordY(getTensionHandleYVal(i));
		m_canvas.drawEllipse(QPointF(xValueToDrawAt, yValueToDrawAt), ths, ths);
	}



	m_canvas.setRenderHint(QPainter::Antialiasing, false);

	// Border
	QPen borderPen = QPen();
	borderPen.setWidth(1);
	borderPen.setColor(borderLight);
	m_canvas.setPen(borderPen);

	m_canvas.drawLine(0, 0, 0, m_height - 1);
	m_canvas.drawLine(0, m_height - 1, m_width - 1, m_height - 1);
	m_canvas.drawLine(m_width - 1, m_height - 1, m_width - 1, 0);
	m_canvas.drawLine(m_width - 1, 0, 0, 0);

	borderPen.setColor(borderDark);
	m_canvas.setPen(borderPen);

	m_canvas.drawLine(1, 1, 1, m_height - 2);
	m_canvas.drawLine(1, m_height - 2, m_width - 2, m_height - 2);
	m_canvas.drawLine(m_width - 2, m_height - 2, m_width - 2, 1);
	m_canvas.drawLine(m_width - 2, 1, 1, 1);
}


void VectorGraph::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::MouseButton::RightButton)
	{
		int pointIndex = model()->getPointIndexFromCoords(coordToRawX(event->x()) * m_width, coordToRawY(event->y()) * m_height, m_width, m_height);
		if (pointIndex >= 0)
		{
			event->ignore();
			return;
		}

		int handleIndex = model()->getPointIndexFromTensionHandleCoords(coordToRawX(event->x()) * m_width, coordToRawY(event->y()) * m_height, m_width, m_height);
		if (handleIndex >= 0)
		{
			model()->getPoint(handleIndex)->setTension(0);
			model()->setLastModifiedTension(0);
			update();
			return;
		}

		int leftBoundIndex = model()->getSectionStartIndex((float) event->x() / m_width);
		VectorGraphPoint newPoint = VectorGraphPoint(
										coordToRawX(event->x()),
										coordToRawY(event->y()),
										model()->getLastModifiedTension(),
										model()->getLastModifiedTensionType()
									);
		model()->insertPointAfter(leftBoundIndex, newPoint);
		model()->setCurrentDraggedPoint(leftBoundIndex + 1);
		event->accept(); // maybe unnecessary
		update();
	}
	else if (event->button() == Qt::MouseButton::LeftButton)
	{
		int pointIndex = model()->getPointIndexFromCoords(coordToRawX(event->x()) * m_width, coordToRawY(event->y()) * m_height, m_width, m_height);
		int tensionHandleIndex = model()->getPointIndexFromTensionHandleCoords(coordToRawX(event->x()) * m_width, coordToRawY(event->y()) * m_height, m_width, m_height);

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
	if (event->modifiers() == Qt::KeyboardModifier::AltModifier)
		model()->setIsGridSnapEnabled(true);
	else
		model()->setIsGridSnapEnabled(false);

	if (model()->getCurrentDraggedPoint() != -1)
	{
		model()->tryMove(model()->getCurrentDraggedPoint(), coordToRawX(event->x()), coordToRawY(event->y()));
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
		VectorGraphPoint * point = model()->getPoint(model()->getCurrentDraggedTensionHandle());
		model()->setLastModifiedTension(point->tension());
		model()->setLastModifiedTensionType(point->tensionType());

		QCursor c = cursor();
		QPoint newCursorPoint(
					rawToCoordX(getTensionHandleXVal(model()->getCurrentDraggedTensionHandle())),
					rawToCoordY(getTensionHandleYVal(model()->getCurrentDraggedTensionHandle())));
		c.setPos(mapToGlobal(newCursorPoint));
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

		m_currentPoint = model()->getPointIndexFromCoords(coordToRawX(menuEvent->x()) * m_width, coordToRawY(menuEvent->y()) * m_height, m_width, m_height);

		if (m_currentPoint < 0)
			return false;

		CaptionMenu contextMenu(model()->displayName(), this);
		contextMenu.addAction(QPixmap(), tr("Hold"), this, SLOT(setTensionToHold()));
		contextMenu.addAction(QPixmap(), tr("Single Curve"), this, SLOT(setTensionToSingle()));
		contextMenu.addAction(QPixmap(), tr("Double Curve"), this, SLOT(setTensionToDouble()));
		contextMenu.addAction(QPixmap(), tr("Stairs"), this, SLOT(setTensionToStairs()));
		contextMenu.addAction(QPixmap(), tr("Pulse"), this, SLOT(setTensionToPulse()));
		contextMenu.addAction(QPixmap(), tr("Wave"), this, SLOT(setTensionToWave()));
		if (model()->getPoint(m_currentPoint)->canBeDeleted())
		{
			contextMenu.addSeparator();
			contextMenu.addAction(QPixmap(), tr("&Delete"), this, SLOT(deletePoint()));
		}
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

void VectorGraph::setTensionToHold()
{
	model()->setTensionTypeOnPoint(m_currentPoint, VectorGraph::TensionType::Hold);
	setLastModifiedPoint(m_currentPoint);
	update();
}

void VectorGraph::setTensionToSingle()
{
	model()->setTensionTypeOnPoint(m_currentPoint, VectorGraph::TensionType::SingleCurve);
	setLastModifiedPoint(m_currentPoint);
	update();
}

void VectorGraph::setTensionToDouble()
{
	model()->setTensionTypeOnPoint(m_currentPoint, VectorGraph::TensionType::DoubleCurve);
	setLastModifiedPoint(m_currentPoint);
	update();
}

void VectorGraph::setTensionToStairs()
{
	model()->setTensionTypeOnPoint(m_currentPoint, VectorGraph::TensionType::Stairs);
	setLastModifiedPoint(m_currentPoint);
	update();
}

void VectorGraph::setTensionToPulse()
{
	model()->setTensionTypeOnPoint(m_currentPoint, VectorGraph::TensionType::Pulse);
	setLastModifiedPoint(m_currentPoint);
	update();
}

void VectorGraph::setTensionToWave()
{
	model()->setTensionTypeOnPoint(m_currentPoint, VectorGraph::TensionType::Wave);
	setLastModifiedPoint(m_currentPoint);
	update();
}

float VectorGraph::getTensionHandleYVal(int index)
{
	return model()->calculateSample(getTensionHandleXVal(index));
}

float VectorGraph::getTensionHandleXVal(int index)
{
	auto point = model()->getPoint(index);
	auto previousPoint = model()->getPoint(index - 1);
	return (point->x() + previousPoint->x()) / 2;
}

void VectorGraph::setLastModifiedPoint(int pointIndex)
{
	VectorGraphPoint * point = model()->getPoint(pointIndex);
	model()->setLastModifiedTension(point->tension());
	model()->setLastModifiedTensionType(point->tensionType());
}


VectorGraphModel::VectorGraphModel(::Model * _parent, bool _default_constructed):
	Model(_parent, tr("VectorGraph"), _default_constructed)
{
	m_points = QVector<VectorGraphPoint>();

	auto firstPoint = VectorGraphPoint(0, 0, 0, VectorGraph::TensionType::SingleCurve);
	firstPoint.permaLockX();
	firstPoint.setDeletable(false);
	m_points.append(firstPoint);
	auto finalPoint = VectorGraphPoint(1, 1, 0, VectorGraph::TensionType::SingleCurve);
	finalPoint.permaLockX();
	finalPoint.setDeletable(false);
	m_points.append(finalPoint);

	m_currentDraggedPoint = -1;
	m_currentDraggedTensionHandle = -1;
	m_lastModifiedTension = 0;
	m_lastModifiedTensionType = VectorGraph::TensionType::SingleCurve;
	m_gridEnabled = true;
	m_numGridLines = 12;
	m_gridSnapEnabled = false;
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
	if (point->getTensionType() == VectorGraph::TensionType::Hold)
	{
		return 0;
	}
	else if (point->getTensionType() == VectorGraph::TensionType::SingleCurve)
	{
		return CalculateSingleCurve(input, point);
	}
	else if (point->getTensionType() == VectorGraph::TensionType::DoubleCurve)
	{
		if (input < 0.5)
		{
			return CalculateSingleCurve(input * 2, point) * 0.5;
		}
		else
		{
			VectorGraphPoint * newPoint = new VectorGraphPoint(point);

			//setTension is expensive. This is equivalent to the following, but using precomputed values:
			//point->setTension(point->tension() * -1);
			newPoint->invertTension();

			auto result = CalculateSingleCurve((input - 0.5) * 2, newPoint);
			delete newPoint;
			return result * 0.5 + 0.5;
		}
	}
	else if (point->getTensionType() == VectorGraph::TensionType::Stairs)
	{
		// maybe make this one keep the tension from going below 0
		// also maybe make this one discrete instead of continuous
		float mult = (1 - ((point->tension() + 1)/2)) * .499 + .001;
		int scalar = ((int) (0.5/mult))*2;
		int scaledInput = (int) (input * scalar);
		float output = scaledInput * 1.0 / (scalar);
		return output;
	}
	else if (point->getTensionType() == VectorGraph::TensionType::Pulse)
	{
		return ((int) (input * (int) ((point->tension() + 1.05) * 50))) % 2;
	}
	else if (point->getTensionType() == VectorGraph::TensionType::Wave)
	{
		// triangle case
		if (point->tension() > 0)
		{
			int count = ((int) ((point->tension() + 0.05) * 25)) * 2 + 1;
			float width = 1.0/count;
			int currentCount = input*count;

			float output = input*count - currentCount*width*count;

			if (currentCount % 2 == 1)
				output = 1 - output;
			return output;
		}
		// sine case
		else
		{
			int count = ((int) ((qAbs(point->tension()) + 0.05) * 25)) * 2 + 1;
			return qCos(input * M_PI * count) * -0.5 + 0.5;
		}
	}
	return 0;
}

float VectorGraphModel::CalculateSingleCurve(float input, VectorGraphPoint * point)
{
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
		VectorGraphPoint * rightPoint = getPoint(index); // Fixes a warning, this won't actaully be used.
		if (checkRight)
		{
			rightPoint = getPoint(index + 1);
		}

		float potentialXValue;

		if (x < leftPoint->x())
		{
			potentialXValue = leftPoint->x();
		}
		else if (checkRight && x > rightPoint->x())
		{
			potentialXValue = rightPoint->x();
		}
		else
		{
			potentialXValue = x;
		}

		if (m_gridSnapEnabled)
		{
			int snappedGridLineIndex = qRound(potentialXValue * m_numGridLines);
			float potentialSnappedValue = snappedGridLineIndex / (float)m_numGridLines;
			if (potentialSnappedValue < leftPoint->x())
			{
				currentPoint->setX((snappedGridLineIndex + 1) / (float)m_numGridLines);
			}
			else if (checkRight && potentialSnappedValue > rightPoint->x())
			{
				currentPoint->setX((snappedGridLineIndex - 1) / (float)m_numGridLines);
			}
			else
			{
				currentPoint->setX(potentialSnappedValue);
			}
		}
		else
		{
			currentPoint->setX(potentialXValue);
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
			if (m_gridSnapEnabled)
			{
				currentPoint->setY(qRound(y * m_numGridLines) / (float)m_numGridLines);
			}
			else
			{
				currentPoint->setY(y);
			}
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
		if (endPoint->tensionType() == VectorGraph::TensionType::Hold)
			return -1;

		float tensionHandleCenterX;
		float tensionHandleCenterY;

		if (endPoint->tensionType() == VectorGraph::TensionType::DoubleCurve ||
				 endPoint->tensionType() == VectorGraph::TensionType::Pulse ||
				 endPoint->tensionType() == VectorGraph::TensionType::Stairs ||
				 endPoint->tensionType() == VectorGraph::TensionType::Wave)
		{
			tensionHandleCenterX = ((startPoint->x() + endPoint->x())/2) * canvasWidth;
			tensionHandleCenterY = ((startPoint->y() + endPoint->y())/2) * canvasHeight;
		}
		else
		{
			tensionHandleCenterX = ((startPoint->x() + endPoint->x()) / 2) * canvasWidth;
			tensionHandleCenterY = calculateSample(tensionHandleCenterX / canvasWidth) * canvasHeight;
		}
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

void VectorGraphModel::setTensionTypeOnPoint(int index, VectorGraph::TensionType type)
{
	getPoint(index)->setTensionType(type);
}





VectorGraphPoint::VectorGraphPoint(float x, float y, float tension, VectorGraph::TensionType type)
{
	m_x = x;
	m_y = y;
	setTension(tension);
	m_tensionType = type;
	m_isXLocked = false;
	m_isYLocked = false;
	m_isXPermaLocked = false;
	m_isYPermaLocked = false;
	m_canBeDeleted = true;
}

VectorGraphPoint::VectorGraphPoint(VectorGraphPoint * point)
{
	m_x = point->m_x;
	m_y = point->m_y;
	m_tension = point->m_tension;
	m_tensionPower = point->m_tensionPower;
	m_absTensionPower = point->m_absTensionPower;
	m_invTensionPower = point->m_invTensionPower;
	m_invAbsTensionPower = point->m_invAbsTensionPower;
	m_dryAmt = point->m_dryAmt;
	m_tensionType = point->m_tensionType;
	m_isXLocked = point->m_isXLocked;
	m_isYLocked = point->m_isYLocked;
	m_isXPermaLocked = point->m_isXPermaLocked;
	m_isYPermaLocked = point->m_isYPermaLocked;
	m_canBeDeleted = point->m_canBeDeleted;
}

VectorGraphPoint::VectorGraphPoint()
{
	m_x = 0;
	m_y = 0;
	setTension(0);
	m_tensionType = VectorGraph::TensionType::SingleCurve;
	m_isXLocked = false;
	m_isYLocked = false;
	m_isXPermaLocked = false;
	m_isYPermaLocked = false;
	m_canBeDeleted = true;
}
