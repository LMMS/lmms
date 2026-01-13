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

#ifndef LMMS_GRID_MODEl_H
#define LMMS_GRID_MODEl_H

#include <memory> // shared_ptr
#include <vector>

#include "stdio.h" // TODO remove

#include "Model.h"

namespace lmms
{

static const unsigned int GRID_MAX_STEPS = 100000;

namespace gui
{
class GridView;
}

class LMMS_EXPORT GridModel : public Model
{
Q_OBJECT
public:
	struct ItemInfo
	{
		float x;
		float y;
	};
	struct Item
	{
		ItemInfo info;
		size_t objectIndex;
	};

	//*** util ***
	//! @return index if found inside radius, else -1
	//! (the first index is returned where `xPos` is found)
	int findIndexFromPos(float xPos, float radius) const;

	//*** item management ***
	//! @return index of an item with larger or equal x pos
	size_t getNextItem(size_t index) const { return index + 1; }

	//! @return new / final index (if x changed)
	size_t setInfo(size_t index, const ItemInfo& info);
	size_t setInfo(size_t index, const ItemInfo& info, unsigned int horizontalSteps, unsigned int verticalSteps);
	const Item& getItem(size_t index) const;

	//*** model management ***
	//! @return item count
	size_t getCount() const;
	unsigned int getLength() const;
	unsigned int getHeight() const;
	void resizeGrid(size_t length, size_t height);
	void setSteps(unsigned int horizontalSteps, unsigned int verticalSteps);

	virtual ~GridModel();
signals:
	void dataChangedAt(size_t index);
protected:
	GridModel(unsigned int length, unsigned int height, unsigned int horizontalSteps, unsigned int verticalSteps,
		Model* parent, QString displayName, bool defaultConstructed);

	//! @return index if found, -1 if not
	int findObject(void* object) const;
	//! @return the index where [index - 1].x < xPos and xPos <= [index].x
	//! so return the first index where xPos <= x (this index can be the size)
	size_t findIndex(float xPos) const;

	//! @return index where added
	size_t addItem(Item itemIn);
	void removeItem(size_t index);
	//! DOESN'T EMIT
	size_t& getAndSetObjectIndex(size_t index);
private:
	//! only call these with fitted values (`fitPos`)
	//! @return new / final index (if x changed)
	size_t setX(size_t index, float newX);
	void setY(size_t index, float newY);
	float fitPos(float position, unsigned int max, unsigned int steps) const;
	void move(size_t startIndex, size_t finalIndex);

	unsigned int m_length;
	unsigned int m_height;
	unsigned int m_horizontalSteps;
	unsigned int m_verticalSteps;

	//! items are sorted by x position ascending
	std::vector<Item> m_items;

	friend class gui::GridView;
};

template<typename T>
class LMMS_EXPORT GridModelTyped : public GridModel
{
private:
	std::vector<T> m_TInstances;
public:
	GridModelTyped(unsigned int length, unsigned int height, unsigned int horizontalSteps, unsigned int verticalSteps,
		Model* parent, QString displayName = QString(), bool defaultConstructed = false)
		: GridModel{length, height, horizontalSteps, verticalSteps, parent, displayName, defaultConstructed} {}
	~GridModelTyped() = default;


	const T& getObject(size_t index) const { return m_TInstances[GridModel::getItem(index).objectIndex]; }

	//! @return index where added
	size_t addItem(T object, ItemInfo info)
	{
		m_TInstances.push_back(object);
		return GridModel::addItem(Item{info, m_TInstances.size() - 1});
	}
	void removeItem(size_t index)
	{
		size_t instanceIndex{GridModel::getItem(index).objectIndex};
		// the stored indexes need to be offset after removing (doing it before because of signals)
		for (size_t i = 0; i < getCount(); ++i)
		{
			size_t& storedIndex{GridModel::getAndSetObjectIndex(i)};
			if (storedIndex > instanceIndex) { --storedIndex; }
		}
		// removing the instance
		for (size_t i = instanceIndex; i < m_TInstances.size(); ++i)
		{
			m_TInstances[i] = m_TInstances[i + 1];
		}
		m_TInstances.pop_back();
		// removing the Item (object* + coords pair)
		GridModel::removeItem(index);
	}
};

struct VGPoint
{
	float bezierL;
	float bezierR;
	bool isBezierHandle;
};

class LMMS_EXPORT VectorGraphModel : public GridModelTyped<VGPoint>
{
Q_OBJECT
public:
	VectorGraphModel(unsigned int length, unsigned int height, unsigned int horizontalSteps, unsigned int verticalSteps,
		Model* parent, QString displayName = QString(), bool defaultConstructed = false);

	void setPoint(size_t index, float x, float y, bool isBezierHandle);

	void renderPoints(size_t resolution, size_t start, size_t end);
	void renderAfter(size_t index);
	const std::vector<float>& getBuffer() const;
	std::vector<float>& getBufferRef();

private:
	std::vector<float> m_buffer;
};

} // namespace lmms

#endif // LMMS_GRID_MODEl_H
