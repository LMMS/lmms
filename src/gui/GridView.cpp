/*
 * GridView.h - a grid display and editor widget
 *
 * Copyright (c) 2025 szeli1
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

#include "GridView.h"

#include <cmath>
#include "stdio.h" // TODO remove
#include <QDebug> // TODO remove

#include <QPainter>

namespace lmms::gui
{

GridView::GridView(QWidget* parent, GridModel* model, size_t cubeWidth, size_t cubeHeight)
	: QWidget{parent}
	, ModelView{model, this}
	, m_selection{}
	, m_isSelectionPressed{false}
	, m_isNearestPressed{false}
	, m_selectStartOld{}
	, m_selectEndOld{}
	, m_selectStart{}
	, m_selectEnd{}
	, m_cursorStart{}
	, m_cursorEnd{}
	, m_cubeWidth{cubeWidth}
	, m_cubeHeight{cubeHeight}
	, m_isSizeStatic{true}
	, m_gridHighlightMod{5}
{
	//QT's resize
	if (model != nullptr)
	{
		resize(model->getLength() * m_cubeWidth + 2, model->getHeight() * m_cubeHeight + 2);
	}
	else
	{
		resize(m_cubeWidth, m_cubeHeight);
	}
}

void GridView::moveToWhole(unsigned int x, unsigned int y)
{
	containSelection(QPointF(x, y), QPoint(x + 1, y + 1));
}
void GridView::containSelection(QPointF start, QPointF end)
{
	m_cursorStart = start;
	m_cursorEnd = end;
	if (m_isSelectionPressed)
	{
		m_selectStart.setX(std::min(start.x(), m_selectStartOld.x()));
		m_selectStart.setY(std::min(start.y(), m_selectStartOld.y()));
		m_selectEnd.setX(std::max(end.x(), m_selectEndOld.x()));
		m_selectEnd.setY(std::max(end.y(), m_selectEndOld.y()));
	}
	else
	{
		m_selectStartOld = start;
		m_selectEndOld = end;

		m_selectStart = m_selectStartOld;
		m_selectEnd = m_selectEndOld;
	}
}
void GridView::moveToNearest(MoveDir dir)
{
	QPointF start{};
	QPointF end{};
	QPointF target{};
	// at first we will get a bounding box where we will search for the closest thing
	switch (dir)
	{
		case right:
			start = QPointF(m_selectEnd.x(), m_selectStart.y());
			end = QPointF(std::ceil(m_selectEnd.x()) + 1.0f, m_selectEnd.y());
			target = QPointF(m_selectEnd.x(), (m_selectStart.y() + m_selectEnd.y()) / 2.0f);
			break;
		case left:
			start = QPointF(std::floor(m_selectStart.x() - 1.0f), m_selectStart.y());
			end = QPointF(m_selectStart.x(), m_selectEnd.y());
			target = QPointF(m_selectStart.x(), (m_selectStart.y() + m_selectEnd.y()) / 2.0f);
			break;
		case up:
			start = QPointF(m_selectStart.x(), std::ceil(m_selectEnd.y()) + 1.0f);
			end = QPointF(m_selectEnd.x(), m_selectStart.y());
			target = QPointF((m_selectStart.x() + m_selectEnd.x()) / 2.0f, m_selectEnd.y());
			break;
		case down:
			start = QPointF(m_selectStart.x(), m_selectEnd.y());
			end = QPointF(m_selectEnd.x(), std::floor(m_selectStart.x()) - 1.0f);
			target = QPointF(m_selectEnd.x(), (m_selectStart.y() + m_selectEnd.y()) / 2.0f);
			target = QPointF((m_selectStart.x() + m_selectEnd.x()) / 2.0f, m_selectStart.y());
			break;
	}
	// this is inefficient for left and right, in those cases our data is sorted so we could break
	// this is done to ensure compatibility with the classes that inherit this:
	// in the case of notes we can't estimate the first X coord
	std::set<size_t> searchBuffer{getSelection(start, end)};
	// this is slightly inefficient because we can compare less bytes of data knowing `dir`
	size_t closestIndex{getClosest(searchBuffer, target)};
	if (closestIndex >= model()->getCount())
	{
		containSelection(start, end);
	}
	else
	{
		auto bb{getBoundingBox(closestIndex)};
		containSelection(bb.first, bb.second);
	}
}

void GridView::paintEvent(QPaintEvent* pe)
{
	QPainter painter(this);
	drawGrid(painter);
}

void GridView::drawGrid(QPainter& painter)
{
	constexpr int borderWidth{1};
	QColor backgroundC{13, 16, 19};
	QColor borderC{70, 70, 70};
	QColor gridC{42, 47, 51};
	QColor highlightedGridC{42, 101, 72};

	// background
	painter.setPen(borderC);
	painter.drawRect(0, 0, width() - 1, height() - 1);
	painter.fillRect(borderWidth, borderWidth, width() - 1 - borderWidth, height() - 1 - borderWidth, backgroundC);

	size_t cubeWidth{(width() - 1 - borderWidth) / model()->getLength()};
	size_t cubeHeight{(height() - 1 - borderWidth) / model()->getHeight()};
	size_t lineCounter{1};
	painter.setPen(gridC);
	// vertical 1. pass
	for (size_t i = 1; i < model()->getLength(); ++i)
	{
		if (lineCounter >= m_gridHighlightMod) { lineCounter = 1; }
		else
		{
			painter.drawLine(i * cubeWidth, borderWidth, i * cubeWidth, height() - 1 - borderWidth);
			++lineCounter;
		}
	}

	// horizontal full pass
	lineCounter = 1;
	for (size_t i = 1; i < model()->getHeight(); ++i)
	{
		if (lineCounter >= m_gridHighlightMod)
		{
			painter.setPen(highlightedGridC);
			painter.drawLine(borderWidth, i * cubeHeight, width() - 1 - borderWidth, i * cubeHeight);
			painter.setPen(gridC);
			lineCounter = 1;
		}
		else
		{
			painter.drawLine(borderWidth, i * cubeHeight, width() - 1 - borderWidth, i * cubeHeight);
			++lineCounter;
		}
	}

	// vertical 2. pass (drawing highlights because horizontal draws over 1. pass)
	painter.setPen(highlightedGridC);
	for (size_t i = m_gridHighlightMod; i < model()->getLength(); i += m_gridHighlightMod)
	{
			painter.drawLine(i * cubeWidth, borderWidth, i * cubeWidth, height() - 1 - borderWidth);
	}
}

QPointF GridView::getBoundingBoxCenter(size_t index) const
{
	auto bb{getBoundingBox(index)};
	return QPointF{(bb.first.x() + bb.second.x()) / 2.0f, (bb.first.y() + bb.second.y()) / 2.0f};
}

QPointF GridView::getBoundingBoxCenter(QPointF start, QPointF end) const
{
	return QPointF{(start.x() + end.x()) / 2.0f, (start.y() + end.y()) / 2.0f};
}

std::set<size_t>&& GridView::select(QPointF start, QPointF end, float offset)
{
	std::set<size_t> output{};
	size_t startIndex = model()->findIndex(start.x() - offset);
	for (size_t i = startIndex; i < model()->getCount(); i = model()->getNextItem(i))
	{
		auto bb{getBoundingBox(i)};
		QPointF curCenter{getBoundingBoxCenter(bb.first, bb.second)};
		// if the current start.x > end.x
		if (bb.first.x() > end.x()) { break; }
		if (curCenter.x() > start.x() && start.y() < curCenter.y() && curCenter.y() > end.y())
		{
			output.insert(i);
		}
	}
	return std::move(output);
}
void GridView::updateSelection()
{
	m_selection = getSelection(m_selectStart, m_selectEnd);
}
size_t GridView::getClosest(const std::set<size_t>& selection, QPointF point)
{
	if (selection.empty()) { return model()->getCount(); }

	size_t closestIndex{*selection.begin()};
	float distance{0.0f};
	{
		QPointF curCenter{getBoundingBoxCenter(closestIndex)};
		distance = std::abs(curCenter.x() - point.x() + curCenter.y() - point.y());
	}
	for (size_t i : selection)
	{
		QPointF curCenter{getBoundingBoxCenter(i)};
		float curDistance{static_cast<float>(std::fabs(curCenter.x() - point.x() + curCenter.y() - point.y()))};
		if (distance > curDistance)
		{
			distance = curDistance;
			closestIndex = i;
		}
	}
	return closestIndex;
}

void GridView::modelChanged()
{
	auto gridModel{castModel<GridModel>()};

	resize(gridModel->getLength() * m_cubeWidth + 2, gridModel->getHeight() * m_cubeHeight + 2);

	QObject::connect(gridModel, &GridModel::dataChanged,
			this, &GridView::updateGrid);

	QObject::connect(gridModel, &GridModel::propertiesChanged,
			this, &GridView::updateGrid);
}

void GridView::updateGrid()
{
	update();
}

VectorGraphView::VectorGraphView(QWidget* parent, size_t cubeWidth, size_t cubeHeight)
	: GridView{parent, new VectorGraphModel{10, 10, GRID_MAX_STEPS, GRID_MAX_STEPS, nullptr, QString(), true}, cubeWidth, cubeHeight}
{}

std::pair<QPointF, QPointF> VectorGraphView::getBoundingBox(size_t index) const
{
	float radius{castModel<VectorGraphModel>()->getObject(index).isBezierHandle ? 0.5 : 0.5};
	GridModel::ItemInfo coords{castModel<VectorGraphModel>()->getItem(index).info};
	return std::make_pair(QPointF{coords.x - radius, coords.y - radius}, QPointF{coords.x + radius, coords.y + radius});
}

std::set<size_t>&& VectorGraphView::getSelection(QPointF start, QPointF end)
{
	return GridView::select(start, end, 0.5);
}

void VectorGraphView::paintEvent(QPaintEvent* pe)
{
	QPainter painter(this);
	drawGrid(painter);
}

} // namespace lmms::gui
