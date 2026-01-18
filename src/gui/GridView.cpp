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

#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>

namespace lmms::gui
{

GridView::GridView(QWidget* parent, GridModel* model, size_t cubeWidth, size_t cubeHeight)
	: QWidget{parent}
	, ModelView{model, this}
	, m_selection{}
	, m_isSelectionPressed{false}
	, m_selectStartOld{}
	, m_selectEndOld{}
	, m_selectStart{}
	, m_selectEnd{}
	, m_cursorPos{}
	, m_cubeWidth{cubeWidth}
	, m_cubeHeight{cubeHeight}
	, m_isSizeStatic{true}
	, m_gridHighlightMod{5}
{
	// if the widget is clicked, gain keyboard focus
	setFocusPolicy(Qt::ClickFocus);
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
	m_cursorPos = getBoundingBoxCenter(start, end);
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
	// the selection changed, so reset the current selection
	m_selection.clear();
}
void GridView::moveToNearest(MoveDir dir)
{
	QPointF start{};
	QPointF end{};
	// at first we will get a bounding box where we will search for the closest thing
	switch (dir)
	{
		case right: // positive dir
			start = QPointF(m_cursorPos.x(), std::floor(m_cursorPos.y()));
			end = QPointF(m_cursorPos.x() + 1.0f, std::floor(m_cursorPos.y()) + 1.0f);
			break;
		case left:
			start = QPointF(m_cursorPos.x() - 1.0f, std::floor(m_cursorPos.y()));
			end = QPointF(m_cursorPos.x(), std::floor(m_cursorPos.y()) + 1.0f);
			break;
		case up: // positive dir
			start = QPointF(std::floor(m_cursorPos.x()), m_cursorPos.y());
			end = QPointF(std::floor(m_cursorPos.x()) + 1.0f, m_cursorPos.y() + 1.0f);
			break;
		case down:
			start = QPointF(std::floor(m_cursorPos.x()), m_cursorPos.y() - 1.0f);
			end = QPointF(std::floor(m_cursorPos.x()) + 1.0f, m_cursorPos.y());
			break;
	}
	assert(start.x() < end.x());
	assert(start.y() < end.y());
	// this is inefficient for left and right, in those cases our data is sorted so we could break
	// this is done to ensure compatibility with the classes that inherit this:
	// in the case of notes we can't estimate the first X coord
	std::set<size_t> searchBuffer{getSelection(start, end)};
	// this is slightly inefficient because we can compare less bytes of data knowing `dir`
	size_t closestIndex{getClosest(searchBuffer, m_cursorPos)};
	if (closestIndex >= model()->getCount())
	{
		unsigned int moveX{static_cast<unsigned int>(m_cursorPos.x())};
		unsigned int moveY{static_cast<unsigned int>(m_cursorPos.y())};
		switch (dir)
		{
			case right:
				moveX += moveX + 1 < model()->getLength() ? 1 : 0;
				break;
			case left:
				moveX -= moveX > 0 ? 1 : 0;
				break;
			case up:
				moveY += moveY + 1 < model()->getHeight() ? 1 : 0;
				break;
			case down:
				moveY -= moveY > 0 ? 1 : 0;
				break;
		}
		moveToWhole(moveX, moveY);
	}
	else
	{
		auto bb{getBoundingBox(closestIndex)};
		containSelection(bb.first, bb.second);
	}
}
size_t GridView::getClickedItem(QPointF pos)
{
	auto searchBox{getOnClickSearchArea(pos)};
	std::set<size_t> searchBuffer{getSelection(searchBox.first, searchBox.second)};
	return getClosest(searchBuffer, pos);
}

void GridView::paintEvent(QPaintEvent* pe)
{
	QPainter painter(this);
	drawGrid(painter);
	if (m_selection.empty())
	{
		drawSelection(painter);
	}
}
void GridView::keyPressEvent(QKeyEvent* ke)
{
	m_isSelectionPressed = ke->modifiers() & Qt::ShiftModifier;
	switch (ke->key())
	{
		case Qt::Key_Up:
			moveToNearest(GridView::MoveDir::up);
			ke->accept();
			break;
		case Qt::Key_Down:
			moveToNearest(GridView::MoveDir::down);
			ke->accept();
			break;
		case Qt::Key_Left:
			moveToNearest(GridView::MoveDir::left);
			ke->accept();
			break;
		case Qt::Key_Right:
			moveToNearest(GridView::MoveDir::right);
			ke->accept();
			break;
		default:
			ke->ignore();
			break;
	}
	update();
}

void GridView::drawGrid(QPainter& painter)
{
	constexpr int borderWidth{1};
	// background
	painter.setPen(m_borderColor);
	painter.drawRect(0, 0, width() - 1, height() - 1);
	painter.fillRect(borderWidth, borderWidth, width() - 1 - borderWidth, height() - 1 - borderWidth, m_backgroundColor);

	size_t cubeWidth{(width() - 1 - borderWidth) / model()->getLength()};
	size_t cubeHeight{(height() - 1 - borderWidth) / model()->getHeight()};
	size_t lineCounter{1};
	painter.setPen(m_gridLineColor);
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
			painter.setPen(m_gridHighlightedLineColor);
			painter.drawLine(borderWidth, i * cubeHeight, width() - 1 - borderWidth, i * cubeHeight);
			painter.setPen(m_gridLineColor);
			lineCounter = 1;
		}
		else
		{
			painter.drawLine(borderWidth, i * cubeHeight, width() - 1 - borderWidth, i * cubeHeight);
			++lineCounter;
		}
	}

	// vertical 2. pass (drawing highlights because horizontal draws over 1. pass)
	painter.setPen(m_gridHighlightedLineColor);
	for (size_t i = m_gridHighlightMod; i < model()->getLength(); i += m_gridHighlightMod)
	{
			painter.drawLine(i * cubeWidth, borderWidth, i * cubeWidth, height() - 1 - borderWidth);
	}
}
void GridView::drawSelection(QPainter& painter)
{
	QColor selectionBorderC{40, 255, 70};

	if (m_selectStart != m_selectEnd)
	{
		QPoint selectStart{toViewCoords(m_selectStart)};
		QPoint selectEnd{toViewCoords(m_selectEnd)};
		QPoint cursorPos{toViewCoords(m_cursorPos)};

		painter.setPen(selectionBorderC);
		painter.drawRect(selectStart.x(), selectStart.y(), selectEnd.x() - selectStart.x(), selectEnd.y() - selectStart.y());
		painter.drawLine(cursorPos.x() - 1, cursorPos.y(), cursorPos.x() + 1, cursorPos.y());
		painter.drawLine(cursorPos.x(), cursorPos.y() - 1, cursorPos.x(), cursorPos.y() + 1);
	}
}
QPointF GridView::toModelCoords(QPoint viewPos) const
{
	constexpr int borderWidth{1};
	return QPointF(static_cast<float>(viewPos.x() - borderWidth) / m_cubeWidth, -static_cast<float>(viewPos.y() - borderWidth) / m_cubeHeight + model()->getHeight());
}
QPointF GridView::toModelCoords(QPointF viewPos) const
{
	constexpr int borderWidth{1};
	return QPointF(static_cast<float>(viewPos.x() - borderWidth) / m_cubeWidth, -static_cast<float>(viewPos.y() - borderWidth) / m_cubeHeight + model()->getHeight());
}
QPoint GridView::toViewCoords(QPointF modelPos) const
{
	return toViewCoords(modelPos.x(), modelPos.y());
}
QPoint GridView::toViewCoords(float x, float y) const
{
	constexpr int borderWidth{1};
	return QPoint(static_cast<int>(x * m_cubeWidth) + borderWidth, static_cast<int>(-(y - model()->getHeight()) * m_cubeHeight) + borderWidth);
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

std::set<size_t> GridView::select(QPointF start, QPointF end, float offset)
{
	std::set<size_t> output{};
	size_t startIndex = model()->findIndex(start.x() - offset);
	for (size_t i = startIndex; i < model()->getCount(); ++i)
	{
		auto bb{getBoundingBox(i)};
		QPointF curCenter{getBoundingBoxCenter(bb.first, bb.second)};
		// if curStart.x > end.x (assumption: bb.first.x is smaller than center.x)
		if (end.x() <= bb.first.x()) { break; }
		if (start.x() < curCenter.x() && curCenter.x() < end.x() && start.y() < curCenter.y() && curCenter.y() < end.y())
		{
			output.insert(i);
		}
	}
	return output;
}
void GridView::updateSelection()
{
	if (m_selectStart != m_selectEnd)
	{
		if (m_selection.empty()) { m_selection = getSelection(m_selectStart, m_selectEnd); }
	}
	else
	{
		m_selection.clear();
	}
}
void GridView::selectionMoveAction(QPointF offset)
{
	updateSelection();
	if (m_selection.empty()) { return; }
	if (offset.x() > 0)
	{
		for (auto it = m_selection.rbegin(); it != m_selection.rend(); ++it)
		{
			GridModel::ItemInfo curInfo{model()->getItem(*it).info};
			size_t newIndex{model()->setInfo(*it, GridModel::ItemInfo{
				curInfo.x + static_cast<float>(offset.x()),
				curInfo.y + static_cast<float>(offset.y())})};
			if (newIndex != *it) { m_selection.erase(*it); m_selection.insert(newIndex); }
		}
	}
	else
	{
		for (auto it = m_selection.begin(); it != m_selection.end(); ++it)
		{
			GridModel::ItemInfo curInfo{model()->getItem(*it).info};
			size_t newIndex{model()->setInfo(*it, GridModel::ItemInfo{
				curInfo.x + static_cast<float>(offset.x()),
				curInfo.y + static_cast<float>(offset.y())})};
			if (newIndex != *it) { m_selection.erase(*it); m_selection.insert(newIndex); }
		}
	}
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
	: GridView{parent, new VectorGraphModel{10, 10, GRID_MAX_STEPS, GRID_MAX_STEPS, 200, nullptr, QString(), true}, cubeWidth, cubeHeight}
	, m_mouseAction{}
{}

std::pair<QPointF, QPointF> VectorGraphView::getBoundingBox(size_t index) const
{
	float radius{castModel<VectorGraphModel>()->getObject(index).type == VGPoint::Type::attribute ? 0.3f : 0.5f};
	GridModel::ItemInfo coords{castModel<VectorGraphModel>()->getItem(index).info};
	return std::make_pair(QPointF{coords.x - radius, coords.y - radius}, QPointF{coords.x + radius, coords.y + radius});
}
std::pair<QPointF, QPointF> VectorGraphView::getOnClickSearchArea(QPointF clickedPos) const
{
	float radius{0.5f};
	return std::make_pair(QPointF{clickedPos.x() - radius, clickedPos.y() - radius}, QPointF{clickedPos.x() + radius, clickedPos.y() + radius});
}

std::set<size_t> VectorGraphView::getSelection(QPointF start, QPointF end)
{
	return GridView::select(start, end, 0.0f);
}

void VectorGraphView::paintEvent(QPaintEvent* pe)
{
	QPainter painter(this);
	drawGrid(painter);


	QPen lightPen{m_pointColor};
	lightPen.setWidth(2);
	QPen darkPen{m_backgroundColor};
	darkPen.setWidth(3);

	painter.setPen(lightPen);
	painter.setBrush(QBrush(m_pointColor, Qt::NoBrush));

	painter.setRenderHints(QPainter::Antialiasing, true);
	auto graphModel{castModel<VectorGraphModel>()};
	for (size_t i = 0; i < model()->getCount(); ++i)
	{
		auto curXY{toViewCoords(QPointF{model()->getItem(i).info.x, model()->getItem(i).info.y})};
		if (graphModel->getObject(i).type == VGPoint::Type::attribute)
		{
			painter.setPen(darkPen);
			painter.setBrush(QBrush(m_pointColor, Qt::SolidPattern));
			painter.drawEllipse(curXY, 4, 4);
			painter.setPen(lightPen);
			painter.setBrush(QBrush(m_pointColor, Qt::NoBrush));
		}
		else
		{
			painter.drawEllipse(curXY, 5, 5);
		}
	}

	const std::vector<float>& buffer{graphModel->getBuffer()};
	if (buffer.size() > 0)
	{
		painter.setPen(m_lineColor);
		QPoint oldPos{toViewCoords(0.0f, buffer[0])};
		for (size_t i = 1; i < buffer.size(); ++i)
		{
			QPoint curPos{toViewCoords(static_cast<float>(i * graphModel->getLength()) / buffer.size(), buffer[i])};
			if (curPos.x() != oldPos.x())
			{
				painter.drawLine(oldPos.x(), oldPos.y(), curPos.x(), curPos.y());
				oldPos = curPos;
			}
		}
	}
	painter.setRenderHints(QPainter::Antialiasing, false);

	if (m_selection.empty())
	{
		drawSelection(painter);
	}
}

void VectorGraphView::mousePressEvent(QMouseEvent* me)
{
	const auto mousePos{me->pos()};
	QPointF modelPos(toModelCoords(mousePos));

	auto graphModel{castModel<VectorGraphModel>()};

	m_isSelectionPressed = false;
	if (me->button() == Qt::LeftButton)
	{
		if (me->modifiers() & Qt::ControlModifier)
		{
			// select
			m_mouseAction = VectorGraphView::MouseAction::selectAction;
			GridView::containSelection(modelPos, modelPos);
			m_isSelectionPressed = true;
		}
		else
		{
			m_mouseAction = VectorGraphView::MouseAction::placeAction;
			// select clicked point
			size_t foundIndex(GridView::getClickedItem(modelPos));
			if (foundIndex >= model()->getCount())
			{
				// place if not found
				foundIndex = graphModel->addItem(VGPoint{VGPoint::Type::bezier},
					GridModel::ItemInfo(modelPos.x(), modelPos.y()));
				auto bb{getBoundingBox(foundIndex)};
				GridView::containSelection(bb.first, bb.second);
			}
			else
			{
				// selection before drag
				if (modelPos.x() > m_selectStart.x() && modelPos.y() > m_selectStart.y()
					&& modelPos.x() < m_selectEnd.x() && modelPos.y() < m_selectEnd.y())
				{
					m_cursorPos = getBoundingBoxCenter(foundIndex);
				}
				else
				{
					auto bb{getBoundingBox(foundIndex)};
					GridView::containSelection(bb.first, bb.second);
				}
			}
			update(); // display selection
		}
		me->accept();
	}
	else if (me->button() == Qt::RightButton)
	{
		// delete
		m_mouseAction = VectorGraphView::MouseAction::removeAction;
		size_t foundIndex(GridView::getClickedItem(modelPos));
		if (foundIndex < model()->getCount())
		{
			graphModel->removeItem(foundIndex);
		}
		me->accept();
	}
}
void VectorGraphView::keyPressEvent(QKeyEvent* ke)
{
	m_isSelectionPressed = ke->modifiers() & Qt::ShiftModifier;
	if (ke->modifiers() & Qt::ControlModifier)
	{
		float moveDistX{model()->getLength() / 50.0f};
		float moveDistY{model()->getHeight() / 50.0f};
		switch (ke->key())
		{
			case Qt::Key_Up:
				selectionMoveAction(QPointF{0.0f, moveDistY});
				ke->accept();
				break;
			case Qt::Key_Down:
				selectionMoveAction(QPointF{0.0f, -moveDistY});
				ke->accept();
				break;
			case Qt::Key_Left:
				selectionMoveAction(QPointF{-moveDistX, 0.0f});
				ke->accept();
				break;
			case Qt::Key_Right:
				selectionMoveAction(QPointF{moveDistX, 0.0f});
				ke->accept();
				break;
			case Qt::Key_Delete:
				selectionDeleteAction();
				ke->accept();
				break;
			case Qt::Key_C:
				ke->accept();
				break;
			case Qt::Key_V:
				ke->accept();
				break;
			case Qt::Key_X:
				ke->accept();
				break;
			case Qt::Key_A:
				GridView::containSelection(QPointF{0.0f, 0.0f},
					QPointF(model()->getLength(), model()->getHeight()));
				update();
				ke->accept();
				break;
			case Qt::Key_T:
				selectionSwitchTypeAction(true);
				ke->accept();
				break;
		}
	}
	else
	{
		switch (ke->key())
		{
			case Qt::Key_Delete:
				selectionDeleteAction();
				ke->accept();
				break;
			default:
				GridView::keyPressEvent(ke);
				break;
		}
	}
}
void VectorGraphView::mouseMoveEvent(QMouseEvent* me)
{
	const auto mousePos{me->pos()};
	QPointF modelPos(toModelCoords(mousePos));

	auto graphModel{castModel<VectorGraphModel>()};

	switch (m_mouseAction)
	{
		case VectorGraphView::MouseAction::selectAction:
			m_isSelectionPressed = true;
			// moving the cursor
			GridView::containSelection(modelPos, modelPos);
			update();
			me->accept();
			break;
		case VectorGraphView::MouseAction::placeAction:
			// preparing cursorPos for move action
			m_cursorPos = modelPos;
			m_mouseAction = VectorGraphView::MouseAction::moveAction;
			break;
		case VectorGraphView::MouseAction::moveAction:
			selectionMoveAction(modelPos - m_cursorPos);
			// moving the cursor
			m_cursorPos = modelPos;
			me->accept();
			break;
		case VectorGraphView::MouseAction::removeAction:
			// this is expensive, but it is used to ensure
			// this part of the code works when `getOnClickSearchArea` changes
			size_t foundIndex(GridView::getClickedItem(modelPos));
			if (foundIndex < model()->getCount())
			{
				graphModel->removeItem(foundIndex);
			}
			me->accept();
			break;
	}
}
void VectorGraphView::wheelEvent(QWheelEvent* we)
{
	const auto mousePos{we->position()};
	QPointF modelPos(toModelCoords(mousePos));

	bool dir{we->angleDelta().y() > 0};
	if (m_selection.empty() == false)
	{
		size_t foundIndex(GridView::getClickedItem(modelPos));
		if (foundIndex < model()->getCount())
		{
			we->accept();
			m_selection.insert(foundIndex);
			selectionSwitchTypeAction(we->inverted() ? !dir : dir);
		}
		we->ignore();
	}
	else
	{
		we->accept();
		selectionSwitchTypeAction(we->inverted() ? !dir : dir);
	}
	we->ignore();
}
void VectorGraphView::selectionDeleteAction()
{
	auto graphModel{castModel<VectorGraphModel>()};

	GridView::updateSelection();
	for (auto it = m_selection.rbegin(); it != m_selection.rend(); it = m_selection.rbegin())
	{
		graphModel->removeItem(*it);
		m_selection.erase(*it);
	}
}
void VectorGraphView::selectionCopyAction()
{
}
void VectorGraphView::selectionPasteAction()
{
}
void VectorGraphView::selectionSwitchTypeAction(bool direction)
{
	auto graphModel{castModel<VectorGraphModel>()};

	GridView::updateSelection();
	for (auto it = m_selection.begin(); it != m_selection.end(); ++it)
	{
		VGPoint point{graphModel->getObject(*it)};
		int nextType{direction ? point.type + 1 : point.type - 1};
		if (nextType < 0) { nextType = VGPoint::Type::steps; }
		if (nextType == VGPoint::Type::count) { nextType = VGPoint::Type::bezier; }
		graphModel->setObject(*it, VGPoint{static_cast<VGPoint::Type>(nextType)});
	}
}

} // namespace lmms::gui
