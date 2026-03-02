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

#ifndef LMMS_VECTOR_GRAPH_VIEW_H
#define LMMS_VECTOR_GRAPH_VIEW_H

#include <QColor>
#include <QPointF>

#include "GridView.h"
#include "VectorGraphModel.h"

class QPainter;

namespace lmms::gui
{

class LMMS_EXPORT VectorGraphView : public GridView
{
public:
	VectorGraphModel* model() { return castModel<VectorGraphModel>(); }
	const VectorGraphModel* model() const { return castModel<VectorGraphModel>(); }

	VectorGraphView(QWidget* parent, size_t cubeWidth, size_t cubeHeight);
protected:
	void paintEvent(QPaintEvent* pe) override;
	void mousePressEvent(QMouseEvent* me) override;
	void mouseMoveEvent(QMouseEvent* me) override;
	void keyPressEvent(QKeyEvent* ke) override;
	void wheelEvent(QWheelEvent* we) override;
	std::pair<QPointF, QPointF> getBoundingBox(size_t index) const override;
	std::pair<QPointF, QPointF> getOnClickSearchArea(QPointF clickedPos) const override;
	std::set<size_t> getSelection(QPointF start, QPointF end) override;

	//! returns getCount() if not found, else index
	size_t selectOnClick(QPointF pos);

	void selectionDeleteAction();
	void selectionCopyAction();
	void selectionPasteAction();
	void selectionSwitchTypeAction(bool direction);
private:
	enum MouseAction
	{
		selectAction,
		placeAction,
		moveAction,
		removeAction
	};

	MouseAction m_mouseAction;
	const QColor m_pointColor = QColor(22, 150, 54);
	const QColor m_lineColor = QColor(11, 213, 86);
};

} // namespace lmms::gui

#endif // LMMS_VECTOR_GRAPH_VIEW_H
