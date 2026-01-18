/*
 * GridView.h - a grid display and editor widget
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

#ifndef LMMS_GRID_VIEW_H
#define LMMS_GRID_VIEW_H

#include <set>

#include <QColor>
#include <QPointF>
#include <QWidget>

#include "GridModel.h"
#include "ModelView.h"

/* This class handles
	1. rendering the grid
	2. movement / selection with keyboard
	3. rendering the selection
	4. moving selected items
	This class doesn't handle
	1. adding items in any way
	2. removing items in any way
*/

class QPainter;

namespace lmms::gui
{

class LMMS_EXPORT GridView : public QWidget, public ModelView
{
Q_OBJECT
public:
	GridView(QWidget* parent, GridModel* model, size_t cubeWidth, size_t cubeHeight);
	~GridView() override = default;	

	GridModel* model() { return castModel<GridModel>(); }
	const GridModel* model() const { return castModel<GridModel>(); }
	enum MoveDir
	{
		right, // positive x direction
		left,
		up, // positive y direction
		down
	};

	void moveToWhole(unsigned int x, unsigned int y);
	//! extends `m_selectStart`, `m_selectEnd` to contain `start` and `end`
	//! start's x and y must be SMALLER than end's
	void containSelection(QPointF start, QPointF end);
	void moveToNearest(MoveDir dir);

	//! @return `getCount()` if not found, else index
	size_t getClickedItem(QPointF pos);

	void updateSelection();
	void selectionMoveAction(QPointF offset);
public slots:
	void updateGrid();
protected:
	void paintEvent(QPaintEvent* pe) override;
	void keyPressEvent(QKeyEvent* ke);
	
	void drawGrid(QPainter& painter);
	void drawSelection(QPainter& painter);
	QPointF toModelCoords(QPoint viewPos) const;
	QPointF toModelCoords(QPointF viewPos) const;
	QPoint toViewCoords(QPointF modelPos) const;
	QPoint toViewCoords(float x, float y) const;

	//*** selection logic ***
	//! selects everything between `start` and `end`
	//! uses getBoundingBox's center for bounds checking
	//! @param offset: how before start.x should we start searching
	std::set<size_t> select(QPointF start, QPointF end, float offset);
	QPointF getBoundingBoxCenter(size_t index) const;
	QPointF getBoundingBoxCenter(QPointF start, QPointF end) const;
	//! should return the start coords and the end coords of an object / note / point
	virtual std::pair<QPointF, QPointF> getBoundingBox(size_t index) const = 0;
	//! should return an area where `getSelection()` could run
	virtual std::pair<QPointF, QPointF> getOnClickSearchArea(QPointF clickedPos) const = 0;
	//! use `select()` to apply selection automatically
	//! if your widget doesn't work with points, then you can offset `start` or `end` and use `select()` on that
	virtual std::set<size_t> getSelection(QPointF start, QPointF end) { return select(start, end, 0.0); }
	//! @return model->getCount() if failed else closest index
	size_t getClosest(const std::set<size_t>& selection, QPointF point);
	std::set<size_t> m_selection;

	void modelChanged() override;

	//! if shift is pressed
	bool m_isSelectionPressed;

	//! where the selection will be made
	//! these values are ignored if a selection is active (it m_selection isn't empty)
	QPointF m_selectStartOld;
	QPointF m_selectEndOld;
	QPointF m_selectStart;
	QPointF m_selectEnd;
	QPointF m_cursorPos;

	size_t m_cubeWidth;
	size_t m_cubeHeight;
	//! if false, automatically resizes `m_cubeWidth` and height
	bool m_isSizeStatic;
	//! highlights every nth line on the grid
	size_t m_gridHighlightMod;

	const QColor m_backgroundColor = QColor(13, 16, 19);
	const QColor m_borderColor = QColor(70, 70, 70);
	const QColor m_gridLineColor = QColor(42, 47, 51);
	const QColor m_gridHighlightedLineColor = QColor(42, 101, 72);
	const QColor m_selectionBoxColor = QColor(40, 255, 70);
};

class LMMS_EXPORT VectorGraphView : public GridView
{
public:
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

#endif // LMMS_GRID_VIEW_H
