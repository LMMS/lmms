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

#ifndef LMMS_GRID_VIEW_H
#define LMMS_GRID_VIEW_H

#include <QPointF>

namespace lmms::gui
{

class GridView : public QWidget, public ModelView
{
public:
	GridView(QWidget* parent);
	~GridView() override = default;	

	GridModel* model()
	{
		return castModel<GridModel>();
	}
	enum MoveDir
	{
		right,
		left,
		up,
		down
	};

	void moveToWhole(unsigned int x, unsigned int y);
	void moveToNearest(MoveDir dir);

	//! selects everything between `start` and `end`
	//! uses getBoundingBox's center for bounds checking
	void select(QPointF start, QPointF end);

protected:
	void paintEvent(QPaintEvent* pe) override;
	void dropEvent(QDropEvent* de) override;
	void dragEnterEvent(QDragEnterEvent* dee) override;
	void mousePressEvent(QMouseEvent* me) override;
	void mouseMoveEvent(QMouseEvent* me) override;
	void mouseReleaseEvent(QMouseEvent* me) override;
	
	void drawGrid();

	//! should return the start coords and the end coords of an object / note / point
	virtual std::pair<QPointF, QPointF> getBoundingBox(size_t index) = 0;
	//! use `select()` to apply selection automatically
	virtual void updateSelection(QPointF start, QPointF, end);
	std::set<size_t> m_selecion;

	void modelChanged() override;

	//! if shift is pressed
	bool m_isSelectionPressed;
	//! if control is sperssed
	bool m_isNearestPressed;

	QPointF m_selectStartOld;
	QPointF m_selectEndOld;
	QPointF m_selectStart;
	QPointF m_selectEnd;

	unsigned int m_cubeWidth;
	unsigned int m_cubeHeight;
};

} // namespace lmms::gui

#endif // LMMS_GRID_VIEW_H
