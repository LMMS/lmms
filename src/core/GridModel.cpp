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

#include "GridModel.h"


GridModel::GridModel(size_t length, size_t height, bool isXWhole, bool isYWhole)
	: m_length{length}
	, m_height{height}
	, m_isXWhole{isXWhole}
	, m_isYWhole{isYWhole}
{}

GridModel::~GridModel()
{}

size_t GridModel::findItem(void* item);
size_t GridModel::findItem(size_t x, size_t y);

//! @return start and end index of the object inside that column
std::pair<size_t, size_t> GridModel::findCol(size_t x);
std::pair<size_t, size_t> GridModel::findBetweenRows(size_t minY, size_t maxY);

ssize_t GridModel::findColFirst(float xPos)
{
	float wholeX{static_cast<float>(static_cast<int>(xPos))};
  	
	// binary search
	int low = 0;
	int high = m_items.size() - 1;
	int mid = 0;
	// if the target is smaller
	bool isSmaller = false;
    while (low <= high)
	{
        mid = ((high - low) / 2) + low;

        if (xPos == m_items[mid].x)
		{
			return mid;
		}
		isSmaller = xPos < m_items[mid].x;
        if (isSmaller)
		{
            high = mid - 1;
		}
        else
		{
            low = mid + 1;
		}
    }
	
	// goal: we would like to find indexes larger or eaqual to the first col index
	/* last cases:
	 * 1. low = 1; mid = 2; high = 3; smaller -> high = 2 - 1 = 1 == low	RETURN
	 * 2. low = 1; mid = 2; high = 3; larger ->  low  = 2 + 1 = 3 == high	RETURN
	 * 3. low = 1; mid = 2; high = 4; smaller -> high = 2 - 1 = 1 == low	RETURN
	 * 4. low = 1; mid = 2; high = 4; larger ->  low  = 2 + 1 = 3 < 4 high
	 * 4. low = 3; mid = 3; high = 4; smaller -> high = 4 - 1 = 3 == low	RETURN
	 * 4. low = 3; mid = 3; high = 4; larger ->  low  = 3 + 1 = 4 == high	RETURN
	 * 1. low = 0; mid = 1; high = 3; smaller -> high = 1 - 1 = 0 == low	RETURN
	 * 2. low = 0; mid = 1; high = 3; larger ->  low  = 1 + 1 = 2 < 3 high
	 * 1. low = 2; mid = 2; high = 3; smaller -> high = 2 - 1 = 1 < 2 low	RETURN
	 * 2. low = 2; mid = 2; high = 3; larger ->  low  = 2 + 1 = 3 == high	RETURN
	 *
	*/

	// goal: return the index where it is or the index where it should be placed (index before it should be placed)
	
  	
  	return -(isSmaller ? mid : high) - 1;
}
size_t findColLast()
{
}
