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

int GridModel::findObject(void* object) const
{
	for (size_t i{0}; i < m_items.size(); ++i)
	{
		if (m_items[i].object == object)
		{
			return i;
		}
	}
	return -1;
}
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
	int high = m_items.size() - 2;
	int mid = 0;
	// if the target is smaller
	bool isSmaller = false;
    while (low <= high)
	{
        mid = ((high - low) / 2) + low;

        if (m_items[mid].info.x < xPos && xPos <= m_items[mid + 1].info.x)
		{
			return mid;
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

	assert(static_cast<size_t>(mid) + 1 < m_items.size());
	assert(m_items[mid].info.x < xPos);
	assert(m_items[mid + 1].info.x > xPos);
	assert(m_items[m_items.size() - 1].info.x < xPos && static_cast<size_t>(mid + 1) == m_items.size());
  	
  	return mid + 1;
}

size_t GridModel::addItem(GridModel::Item itemIn)
{
	size_t foundIndex{findIndex(itemIn.info.x)};
	m_items.push_back(itemIn);
	if (foundIndex < m_items.size())
	{
		for (size_t i{m_items.size() - 2}; --i > foundIndex;)
		{
			printf("adding: swap: [%ld + 1] = [%ld] found index = %ld\n", i, i, foundIndex);
			m_items[i + 1] = m_items[i];
		}
		m_items[foundIndex] = itemIn;
	}
	emit dataChangedAt(foundIndex);
	emit dataChanged();
	return foundIndex;
}

void GridModel::removeItem(size_t index)
{
	for (size_t i{index}; i + 1 < m_items.size(); ++i)
	{
		m_items[i] = m_items[i + 1];
	}
	emit dataChangedAt(index);
	emit dataChanged();
	m_items.pop_back();
}

void** GridModel::getAndSetObject(size_t index)
{
	assert(index < m_items.size());
	return &m_items[index].object;
}

size_t GridModel::setX(size_t index, float newX)
{
	size_t foundIndex{findIndex(newX)};
	m_items[index].info.x = newX;
	size_t minIndex{0};
	size_t maxIndex{0};
	if (foundIndex < index)
	{
		minIndex = foundIndex;
		maxIndex = index;
	}
	else
	{
		minIndex = index;
		maxIndex = foundIndex;
	}
	for (size_t i{minIndex}; i < maxIndex; ++i)
	{
		Item swap = m_items[i];
		m_items[i] = m_items[i + 1];
		m_items[i + 1] = swap;
	}
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
