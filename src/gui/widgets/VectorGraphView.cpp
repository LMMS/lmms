/*
 * VectorGraphView.h - view for vector based graph
 *
 * Copyright (c) 2025 - 2026 szeli1
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

#include "VectorGraphView.h"

#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>

namespace lmms::gui
{

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
