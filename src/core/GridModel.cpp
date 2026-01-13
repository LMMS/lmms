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

#include <cassert>
#include <cmath>
#include "stdio.h" // TODO remove

namespace lmms
{

GridModel::GridModel(unsigned int length, unsigned int height, unsigned int horizontalSteps, unsigned int verticalSteps,
	Model* parent, QString displayName, bool defaultConstructed)
	: Model(parent, displayName, defaultConstructed)
	, m_length{length}
	, m_height{height}
	, m_horizontalSteps{horizontalSteps}
	, m_verticalSteps{verticalSteps}
{}

GridModel::~GridModel()
{}

int GridModel::findIndexFromPos(float xPos, float radius) const
{
	size_t output = findIndex(xPos);
	if (output < m_items.size() && std::abs(m_items[output].info.x - xPos) < radius)
	{
		return output;
	}
	if (0 < output && std::abs(m_items[output - 1].info.x - xPos) < radius)
	{
		return output - 1;
	}
	return -1;
}

size_t GridModel::findIndex(float xPos) const
{
	// binary search
	int low = 0;
	int high = m_items.size();
	int mid = 0;
	// if the target is smaller
	bool isSmaller = false;
    while (low <= high)
	{
        mid = ((high - low) / 2) + low;

        // DO NOT CHANGE
        if (static_cast<size_t>(mid) >= m_items.size()
			|| (xPos <= m_items[mid].info.x && (mid == 0 || m_items[mid - 1].info.x < xPos)))
		{
			break;
		}
		isSmaller = xPos <= m_items[mid].info.x;
        if (isSmaller)
		{
            high = mid - 1;
		}
        else
		{
            low = mid + 1;
		}
    }
#ifdef NDEBUG
	// asserts:
	printf("findIndex: pos: %f, mid: %d, low: %d, high: %d\n", xPos, mid, low, high);
	if (mid >= m_items.size())
	{
		if (m_items.size() > 0)
		{
			assert(xPos > m_items[m_items.size() - 1].info.x);
		}
	}
	else
	{
		printf("assert(m_items[%d].info.x = %f >= xPos %f)\n", mid, m_items[mid].info.x, xPos);
		assert(m_items[mid].info.x >= xPos);
		if (mid > 0)
		{
			assert(m_items[mid - 1].info.x < xPos);
		}
	}
#endif
  	return mid;
}

size_t GridModel::addItem(GridModel::Item itemIn)
{
	//printf("addItem at %f %ld\n", itemIn.info.x, itemIn.objectIndex);
	/*
	for (size_t i = 0; i < m_items.size(); ++i)
	{
		printf("item[%ld] = %f\n", i, m_items[i].info.x);
	}
	*/

	size_t foundIndex{findIndex(itemIn.info.x)};
	m_items.push_back(itemIn);
	if (foundIndex + 1 < m_items.size())
	{
		move(m_items.size() - 1, foundIndex);
	}
	emit dataChangedAt(foundIndex);
	emit dataChanged();
	return foundIndex;
}

void GridModel::removeItem(size_t index)
{
	//printf("removeItem: %d\n", index);
	move(index, m_items.size() - 1);
	m_items.pop_back();
	emit dataChangedAt(index);
	emit dataChanged();
}

void GridModel::move(size_t startIndex, size_t finalIndex)
{
	auto itemData{m_items[startIndex]};
	if (startIndex > finalIndex)
	{
		for (size_t i = startIndex; i > finalIndex; --i)
		{
			m_items[i] = m_items[i - 1];
		}
	}
	else
	{
		for (size_t i = startIndex; i < finalIndex; ++i)
		{
			m_items[i] = m_items[i + 1];
		}
	}
	m_items[finalIndex] = itemData;
}

size_t& GridModel::getAndSetObjectIndex(size_t index)
{
	assert(index < m_items.size());
	return m_items[index].objectIndex;
}

size_t GridModel::setX(size_t index, float newX)
{
	size_t foundIndex{findIndex(newX)};
	m_items[index].info.x = newX;
	if (foundIndex > index)
	{
		foundIndex -= 1;
	}
	move(index, foundIndex);
	emit dataChangedAt(index);
	emit dataChangedAt(foundIndex);
	emit dataChanged();
	return foundIndex;
}
void GridModel::setY(size_t index, float newY)
{
	m_items[index].info.y = newY;
	emit dataChangedAt(index);
	emit dataChanged();
}
size_t GridModel::setInfo(size_t index, const GridModel::ItemInfo& info)
{
	return setInfo(index, info, m_horizontalSteps, m_verticalSteps);
}
size_t GridModel::setInfo(size_t index, const GridModel::ItemInfo& info, unsigned int horizontalSteps, unsigned int verticalSteps)
{
	float newY = fitPos(info.y, m_height, verticalSteps);
	if (newY != m_items[index].info.y) { setY(index, newY); }

	size_t finalIndex{index};
	float newX = fitPos(info.y, m_length, horizontalSteps);
	if (newX != m_items[index].info.x) { finalIndex = setX(index, newX); }
	return finalIndex;
}
const GridModel::Item& GridModel::getItem(size_t index) const
{
	assert(index < m_items.size());
	return m_items[index];
}
float GridModel::fitPos(float position, unsigned int max, unsigned int steps) const
{
	if (steps >= GRID_MAX_STEPS)
	{
		return std::clamp(position, 0.0f, static_cast<float>(max));
	}
	return std::clamp(std::round(position * steps) / steps, 0.0f, static_cast<float>(max));
}

size_t GridModel::getCount() const
{
	return m_items.size();
}
unsigned int GridModel::getLength() const
{
	return m_length;
}
unsigned int GridModel::getHeight() const
{
	return m_height;
}

void GridModel::resizeGrid(size_t length, size_t height)
{
	bool shouldClamp = length < m_length || height < m_height;
	m_length = length;
	m_height = height;
	
	if (shouldClamp)
	{
		// innefficiently clamping
		for (size_t i = 0; i < m_items.size(); ++i)
		{
			setInfo(i, m_items[i].info);
		}
	}
	emit propertiesChanged();
}

void GridModel::setSteps(unsigned int horizontalSteps, unsigned int verticalSteps)
{
	m_horizontalSteps = horizontalSteps;
	m_verticalSteps = verticalSteps;
	emit propertiesChanged();
}

VectorGraphModel::VectorGraphModel(unsigned int length, unsigned int height, unsigned int horizontalSteps, unsigned int verticalSteps,
	Model* parent, QString displayName, bool defaultConstructed)
	: GridModelTyped{length, height, horizontalSteps, verticalSteps, parent, displayName, defaultConstructed}
{
}

void VectorGraphModel::setPoint(size_t index, float x, float y, bool isBezierHandle)
{
}

void VectorGraphModel::renderPoints(size_t resolution, size_t start, size_t end)
{
}
void VectorGraphModel::renderAfter(size_t index)
{
}
const std::vector<float>& VectorGraphModel::getBuffer() const
{
	return m_buffer;
}
std::vector<float>& VectorGraphModel::getBufferRef()
{
	return m_buffer;
}

} // namespace lmms
