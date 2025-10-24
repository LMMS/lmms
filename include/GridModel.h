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

namespace lmms
{

class GridModel : public Model
{
public:
	struct ItemInfo
	{
		float x;
		float y;
	}

	//! @return index if found inside radius, else -1
	//! (the first index is returned where `xPos` is found)
	int findIndexFromPos(float xPos, float radius);

	void removeItem(size_t index);
	//! @return index of an item with larger or equal x pos
	size_t getNextItem(size_t index) { return index + 1; }

	//! @return new / final index (if x changed)
	size_t setInfo(size_t index, const ItemInfo& info);
	size_t setInfo(size_t index, const ItemInfo& info, unsigned int horizontalSteps, unsigned int verticalSteps);
	const ItemInfo& getInfo(size_t index);

	void resizeGrid(size_t length, size_t height);
	virtual ~GridModel();
protected:
	GridModel(size_t length, size_t height, unsigned int horizontalSteps, unsigned int verticalSteps);

	//! @return index if found, -1 if not
	int findObject(void* object);
	//! @return index if found, index after xPos if not found
	size_t findIndex(float xPos);

	//! @return index where added
	size_t addItem(Item itemIn);

	//! only call these with fitted values (`fitPos`)
	//! @return new / final index (if x changed)
	size_t setX(size_t index, float newX);
	void setY(size_t index, float newY);

	struct Item
	{
		ItemInfo info;
		std::shared_ptr<void>* object;
	};

	float fitPos(float position, size_t max, unsigned int horizontalSteps);

private:
	size_t m_length;
	size_t m_height;
	unsigned int m_horizontalSteps;
	unsigned int m_verticalSteps;

	//! items are sorted by x position ascending
	std::vector<Item> m_items;
	std::set<size_t> m_selecion;
};

template<typename T>
class GridModelTyped : public GridModel
{
public:
	GridModelTyped(size_t length, size_t height, unsigned int horizontalSteps, unsigned int verticalSteps)
		: GridModel{length, height, horizontalSteps, verticalSteps} {}
	~GridModelTyped() = default;

	//! @return index if found, -1 if not
	int findObject(T* object) { return GridModel::findObject(object); }

	//! @return index where added
	size_t addItem(T* object, ItemInfo info)
	{
		return GridModel::addItem(ItemInfo{info, std::make_shared<T>(object)});
	}
};

} // namespace lmms

#endif // LMMS_GRID_MODEl_H
