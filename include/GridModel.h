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

#ifndef LMMS_GRID_MODEl_H
#define LMMS_GRID_MODEl_H

#include <set>
#include <memory> // shared_ptr
#include <vector>

#include "Model.h"

namespace lmms
{

static const unsigned int GRID_MAX_STEPS = 100000;

class GridModel : public Model
{
public:
	struct ItemInfo
	{
		float x;
		float y;
	};
	struct Item
	{
		ItemInfo info;
		std::shared_ptr<void> object;
	};

	//! @return index if found inside radius, else -1
	//! (the first index is returned where `xPos` is found)
	int findIndexFromPos(float xPos, float radius);

	void removeItem(size_t index);
	//! @return index of an item with larger or equal x pos
	size_t getNextItem(size_t index) { return index + 1; }

	//! @return new / final index (if x changed)
	size_t setInfo(size_t index, const ItemInfo& info);
	size_t setInfo(size_t index, const ItemInfo& info, unsigned int horizontalSteps, unsigned int verticalSteps);
	const Item& getItem(size_t index);

	//! @return item count
	size_t getCount();
	void resizeGrid(size_t length, size_t height);
	virtual ~GridModel();
protected:
	GridModel(size_t length, size_t height, unsigned int horizontalSteps, unsigned int verticalSteps,
		Model* parent, QString displayName, bool defaultConstructed);

	//! @return index if found, -1 if not
	int findObject(void* object);
	//! @return the index where [index - 1].x < xPos and xPos <= [index].x
	//! so return the first index where xPos <= x
	size_t findIndex(float xPos);

	//! @return index where added
	size_t addItem(Item itemIn);

	//! only call these with fitted values (`fitPos`)
	//! @return new / final index (if x changed)
	size_t setX(size_t index, float newX);
	void setY(size_t index, float newY);

	float fitPos(float position, unsigned int max, unsigned int steps);

private:
	unsigned int m_length;
	unsigned int m_height;
	unsigned int m_horizontalSteps;
	unsigned int m_verticalSteps;

	//! items are sorted by x position ascending
	std::vector<Item> m_items;

	friend class lmms::gui::GridView;
};

template<typename T>
class GridModelTyped : public GridModel
{
public:
	GridModelTyped(size_t length, size_t height, unsigned int horizontalSteps, unsigned int verticalSteps,
		Model* parent, QString displayName = QString(), bool defaultConstructed = false)
		: GridModel{length, height, horizontalSteps, verticalSteps, parent, displayName, defaultConstructed} {}
	~GridModelTyped() = default;

	//! @return index if found, -1 if not
	int findObject(T* object) { return GridModel::findObject(object); }

	//! @return index where added
	size_t addItem(T* object, ItemInfo info)
	{
		return GridModel::addItem(Item{info, std::make_shared<T>(object)});
	}
};

} // namespace lmms

#endif // LMMS_GRID_MODEl_H
