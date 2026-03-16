/*
 * GridModel.h - implements a grid where objects are placed
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

#include "GridModel.h"

#include <cassert>
#include <cmath>

#include "Engine.h"
#include "AudioEngine.h"

namespace lmms
{

GridModel::GridModel(unsigned int countLimit, unsigned int length, unsigned int height,
	unsigned int horizontalSteps, unsigned int verticalSteps, Model* parent, QString displayName, bool defaultConstructed)
	: Model(parent, displayName, defaultConstructed)
	, m_length{length}
	, m_height{height}
	, m_horizontalSteps{horizontalSteps}
	, m_verticalSteps{verticalSteps}
	, m_countLimit{countLimit}
	, m_viewerSize{0}
{
	m_items.resize(countLimit);
}

GridModel::~GridModel()
{}

int GridModel::findIndexFromPos(float xPos, float radius) const
{
	size_t output = findIndex(xPos);
	if (output < getCount() && std::abs(m_items[output].info.x - xPos) < radius)
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
	int high = getCount();
	int mid = 0;
	// if the target is smaller
	bool isSmaller = false;
    while (low <= high)
	{
        mid = ((high - low) / 2) + low;

        // DO NOT CHANGE
        if (static_cast<size_t>(mid) >= getCount()
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
#ifdef LMMS_DEBUG
	// asserts:
	if (static_cast<size_t>(mid) >= getCount())
	{
		if (getCount() > 0)
		{
			assert(xPos > m_items[getCount() - 1].info.x);
		}
	}
	else
	{
		assert(m_items[mid].info.x >= xPos);
		if (mid > 0)
		{
			assert(m_items[mid - 1].info.x < xPos);
		}
	}
#endif
  	return mid;
}

size_t GridModel::addItemG(GridModel::Item itemIn)
{
	if (getCount() >= m_countLimit) { return m_countLimit; }

	size_t foundIndex{findIndex(itemIn.info.x)};
	m_items[getCount()] = itemIn;
	++m_viewerSize;
	if (foundIndex + 1 < getCount())
	{
		move(getCount() - 1, foundIndex);
	}
	dataChangedAt(foundIndex);
	emit dataChanged();
	return foundIndex;
}

void GridModel::removeItemG(size_t relIndex)
{
	if (getCount() <= 0) { return; }
	move(relIndex, getCount() - 1);
	--m_viewerSize;
	dataChangedAt(relIndex);
	emit dataChanged();
}
void GridModel::clearItemsG()
{
	m_viewerSize.store(0, std::memory_order_release);
}

void GridModel::setStaticIndex(size_t relIndex, unsigned int newStaticIndex)
{
	m_items[relIndex].staticIndex.store(newStaticIndex, std::memory_order_relaxed);
	m_items[newStaticIndex].lookupIndex.store(relIndex, std::memory_order_release);
}
size_t GridModel::sToRIndex(GridModel::StaticIndex staticIndex) const
{
	return m_items[staticIndex.index].lookupIndex.load(std::memory_order_acquire);
}
GridModel::StaticIndex GridModel::rToSIndex(size_t relIndex) const
{
	return StaticIndex(m_items[relIndex].staticIndex.load(std::memory_order_acquire));
}

void GridModel::move(size_t startIndex, size_t finalIndex)
{
	auto itemData{m_items[startIndex]};
	if (startIndex > finalIndex)
	{
		for (size_t i = startIndex; i > finalIndex; --i)
		{
			m_items[m_items[i - 1].staticIndex.load(std::memory_order_acquire)].lookupIndex.store(i, std::memory_order_release);
			m_items[i] << m_items[i - 1];
		}
	}
	else
	{
		for (size_t i = startIndex; i < finalIndex; ++i)
		{
			m_items[m_items[i + 1].staticIndex.load(std::memory_order_acquire)].lookupIndex.store(i, std::memory_order_release);
			m_items[i] << m_items[i + 1];
		}
	}
	m_items[itemData.staticIndex.load(std::memory_order_acquire)].lookupIndex.store(finalIndex, std::memory_order_release);
	m_items[finalIndex] << itemData;
}

size_t GridModel::setX(size_t relIndex, float newX)
{
	size_t foundIndex{findIndex(newX)};
	m_items[relIndex].info.x = newX;
	if (foundIndex > relIndex)
	{
		foundIndex -= 1;
	}
	move(relIndex, foundIndex);
	dataChangedAt(relIndex);
	dataChangedAt(foundIndex);
	emit dataChanged();
	return foundIndex;
}
void GridModel::setY(size_t relIndex, float newY)
{
	m_items[relIndex].info.y = newY;
	dataChangedAt(relIndex);
	emit dataChanged();
}
size_t GridModel::setInfo(size_t relIndex, const GridModel::ItemInfo& info)
{
	return setInfo(relIndex, info, m_horizontalSteps, m_verticalSteps);
}
size_t GridModel::setInfo(size_t relIndex, const GridModel::ItemInfo& info, unsigned int horizontalSteps, unsigned int verticalSteps)
{
	float newY = fitPos(info.y, m_height, verticalSteps);
	if (newY != m_items[relIndex].info.y) { setY(relIndex, newY); }

	size_t finalIndex{relIndex};
	float newX = fitPos(info.x, m_length, horizontalSteps);
	if (newX != m_items[relIndex].info.x) { finalIndex = setX(relIndex, newX); }
	return finalIndex;
}
const GridModel::Item& GridModel::getItem(size_t relIndex) const
{
	assert(relIndex < getCount());
	return m_items[relIndex];
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
	return m_viewerSize.load(std::memory_order_acquire);
}
unsigned int GridModel::getCountLimit() const
{
	return m_countLimit;
}
unsigned int GridModel::getLength() const
{
	return m_length;
}
unsigned int GridModel::getHeight() const
{
	return m_height;
}

void GridModel::resizeGridArea(size_t length, size_t height)
{
	bool shouldClamp = length < m_length || height < m_height;
	m_length = length;
	m_height = height;
	
	if (shouldClamp)
	{
		// inefficiently clamping
		for (size_t i = 0; i < getCount(); ++i)
		{
			setInfo(i, m_items[i].info);
		}
	}
	emit propertiesChanged();
}
int GridModel::resizeGridCountLimitG(size_t newLimit)
{
	Engine::audioEngine()->requestChangeInModel();

	size_t curSize{getCount()};
	int deleteCount = std::max(0, (int)curSize - (int)newLimit);
	m_countLimit = newLimit;
	m_viewerSize.store(std::min(curSize, m_countLimit), std::memory_order_release);
	m_items.resize(newLimit);

	Engine::audioEngine()->doneChangeInModel();
	return deleteCount;
}

void GridModel::setSteps(unsigned int horizontalSteps, unsigned int verticalSteps)
{
	m_horizontalSteps = horizontalSteps;
	m_verticalSteps = verticalSteps;
	emit propertiesChanged();
}

} // namespace lmms
