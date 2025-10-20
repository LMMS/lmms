/*
 * GridModel.h - implements a grid where objects are placed
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

class GridModel : public Model
{
public:
	GridModel(size_t length, size_t height, bool isXWhole, bool isYWhole);
	~GridModel();

private:
	size_t findItem(void* item);
	size_t findItem(size_t x, size_t y);

	//! @return start and end index of the object inside that column
	std::pair<size_t, size_t> findCol(size_t x);
	std::pair<size_t, size_t> findBetweenRows(size_t minY, size_t maxY);

	//! @return first index found in column, -1 if not found
	ssize_t findColFirst(float xPos);
	size_t findColLast();

	struct Item
	{
		float x;
		float y;
		uint8_t data;
		void* object;
	};

	size_t m_length;
	size_t m_height;

	bool m_isXWhole;
	bool m_isYWhole;

	//! items are sorted by whole x position ascending
	std::list<Item> m_items;
	std::vector<size_t> m_selecion;
};

template<typename T>
class GridModelTyped : public GridModel
{
public:
	
private:
	
};

#endif // LMMS_GRID_VIEW_H
