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

#include "stdio.h" // TODO remove

namespace lmms::gui
{

GridView::GridView(size_t length, size_t height, unsigned int horizontalSteps, unsigned int verticalSteps,
	Model* parent, QString displayName, bool defaultConstructed)
	: Model(parent, displayName, defaultConstructed)
	, m_length{length}
	, m_height{height}
	, m_horizontalSteps{horizontalSteps}
	, m_verticalSteps{verticalSteps}
{}

GridView::~GridView()
{}


void moveToWhole(unsigned int x, unsigned int y)
{
	if (m_isSelectionPressed)
	{
		m_selectStart.setX(std::min(static_cast<float>(x), m_selectStartOld.x()));
		m_selectStart.setY(std::min(static_cast<float>(y), m_selectStartOld.y()));
		m_selectEnd.setX(std::max(static_cast<float>(x), m_selectEndOld.x()));
		m_selectEnd.setY(std::max(static_cast<float>(y), m_selectEndOld.y()));
	}
	else
	{
		m_selectStartOld.setX(x);
		m_selectStartOld.setY(y);
		m_selectEndOld.setX(x + 1.0);
		m_selectEndOld.setY(y + 1.0);

		m_selectStart = m_selectStartOld;
		m_selectEnd = m_selectEndOld;
	}
}
void moveToNearest(MoveDir dir)
{
}

void paintEvent(QPaintEvent* pe) override
{
}

void select(QPointF start, QPointF end)
{
	size_t startIndex = model()->findIndex(start.x());
	for (size_t i = startIndex; i < model()->getCount(); i = model()->getNextItem())
	{
		QPointF curStart{};
		QPointF curEnd{}
		{curStart, curEnd} = getBoundingBox(i);
		QPointF curCenter{(curStart.x() + curEnd.x()) / 2.0f, (curStart.y() + curEnd.y()) / 2.0f};
		if (curCenter.x() > end.x()) { break; }
		if (curCenter.x() > start.x() && start.y() < curCenter.y() && curCenter.y() > end.y())
		{
			m_selecion.insert(i);
		}
	}
}


} // namespace lmms::gui
