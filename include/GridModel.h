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

#include <set>
#include <stddef.h>
#include <vector>

#include "base64.h"
#include "Model.h"

namespace lmms
{

static const unsigned int GRID_MAX_STEPS = 100000;

/* This class handles
	1. grid area
	2. storing and pairing `ItemInfo` to custom data
	3. getting `ItemInfo` and pairing information
	4. position changes and quantization
	5. RT safe allocation
	This class doesn't handle
	1. constructing, storing, interacting or removing custom data in any way
*/

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
		size_t objectIndex; //!< custom data index
	};

	//*** util ***
	//! @return index if found inside radius, else -1
	//! (the first index is returned where `xPos` is found)
	int findIndexFromPos(float xPos, float radius) const;
	//! @return the index where [index - 1].x < xPos and xPos <= [index].x
	//! so return the first index where xPos <= x (this index can be the size)
	size_t findIndex(float xPos) const;

	//*** item management ***
	const Item& getItem(size_t index) const;

	//*** model management ***
	//! @return item count
	size_t getCount() const;
	size_t getCountLimit() const;
	unsigned int getLength() const;
	unsigned int getHeight() const;
	void setSteps(unsigned int horizontalSteps, unsigned int verticalSteps);

	virtual ~GridModel();
protected:
	GridModel(size_t countLimit, unsigned int length, unsigned int height,
		unsigned int horizontalSteps, unsigned int verticalSteps, Model* parent, QString displayName, bool defaultConstructed);
	virtual void dataChangedAt(signed long long index) {}

	//*** item management ***
	//! @return new / final index (if x changed)
	size_t setInfo(size_t index, const ItemInfo& info); // TODO LOCK
	size_t setInfo(size_t index, const ItemInfo& info, unsigned int horizontalSteps, unsigned int verticalSteps); // TODO LOCK
	//! @return index where added, `getCount()` if fails
	size_t addItem(Item itemIn);
	void removeItem(size_t index);
	void clearItems();
	//! @return get and set index pointing to custom data, DOESN'T EMIT
	size_t& getAndSetObjectIndex(size_t index);

	//*** model management ***
	void resizeGridArea(size_t length, size_t height);
	//! @return how many items were deleted
	int resizeGridCountLimit(size_t newLimit);
private:
	//! only call these with fitted values (`fitPos`)
	//! @return new / final index (if x changed)
	size_t setX(size_t index, float newX);
	void setY(size_t index, float newY);
	float fitPos(float position, unsigned int max, unsigned int steps) const;
	//! moves `startIndex` to `finalIndex` by swapping
	//void move(size_t startIndex, size_t finalIndex, std::span<size_t>& lookup); // TODO
	void move(size_t startIndex, size_t finalIndex); // TODO LOCK

	unsigned int m_length;
	unsigned int m_height;
	unsigned int m_horizontalSteps;
	unsigned int m_verticalSteps;

	//! items are sorted by x position ascending
	std::vector<Item> m_items;
	//! limits m_items size
	size_t m_countLimit;

	friend class gui::GridView;
};

/* This class handles
	1. storing custom data T
	2. constructing T `ItemInfo` pair
	3. removing T `ItemInfo` pair
	4. getting T from index in `GridModel`
	5. converting the stored data into a base64 string
*/

template<typename T, typename SaveData>
class LMMS_EXPORT GridModelTyped : public GridModel
{
private:
	struct SaveDataPair
	{
		GridModel::ItemInfo info;
		SaveData data;
	};

	std::vector<T> m_TCustomData;
public:
	GridModelTyped(size_t countLimit, unsigned int length, unsigned int height, unsigned int horizontalSteps, unsigned int verticalSteps,
		Model* parent, QString displayName = QString(), bool defaultConstructed = false)
		: GridModel{countLimit, length, height, horizontalSteps, verticalSteps, parent, displayName, defaultConstructed}
	{
		m_TCustomData.reserve(countLimit);
	}
	~GridModelTyped() = default;

	const T& getObject(size_t index) const { return m_TCustomData[GridModel::getItem(index).objectIndex]; }
	virtual void setObject(size_t index, T object)
	{
		m_TCustomData[GridModel::getItem(index).objectIndex] = object;
		dataChangedAt(index); emit GridModel::dataChanged();
	}

	//! @return index where added, `getCount()` if fails
	size_t addItem(T object, ItemInfo info)
	{
		if (getCount() >= getCountLimit()) { return getCountLimit(); }
		m_TCustomData.push_back(object);
		return GridModel::addItem(Item{info, m_TCustomData.size() - 1});
	}
	void removeItem(size_t index)
	{
		size_t customDataIndex{GridModel::getItem(index).objectIndex};
		// the stored indexes need to be offset after removing (doing it before because of signals)
		for (size_t i = 0; i < getCount(); ++i)
		{
			size_t& storedIndex{GridModel::getAndSetObjectIndex(i)};
			if (storedIndex > customDataIndex) { --storedIndex; }
		}
		// removing the custom data
		for (size_t i = customDataIndex; i < m_TCustomData.size(); ++i)
		{
			m_TCustomData[i] = m_TCustomData[i + 1];
		}
		m_TCustomData.pop_back();
		// removing the Item (object* + coords pair)
		GridModel::removeItem(index);
	}
protected:
	// save mechanism:
	QString dataToBase64(float xOffset, float yOffset, const std::set<size_t>* selection = nullptr)
	{
		std::vector<SaveDataPair> dataArray{};
		if (selection == nullptr || selection->empty())
		{
			dataArray.reserve(getCount());
			for (size_t i = 0; i < getCount(); ++i)
			{
				dataArray.push_back(SaveDataPair{
					GridModel::ItemInfo{getItem(i).info.x + xOffset, getItem(i).info.y + yOffset},
					customDataToSaveData(getObject(i))});
			}
		}
		else
		{
			dataArray.reserve(selection->size());
			for (size_t i : *selection)
			{
				dataArray.push_back(SaveDataPair{
					GridModel::ItemInfo{getItem(i).info.x + xOffset, getItem(i).info.y + yOffset},
					customDataToSaveData(getObject(i))});
			}
		}
		QString output{};
		base64::encode((const char *)(dataArray.data()),
			dataArray.size() * sizeof(SaveDataPair), output);
		return output;
	}
	void addBase64Data(QString base64String, float xOffset, float yOffset)
	{
		int size = 0;
		SaveDataPair* startPtr = nullptr;
		base64::decode<SaveDataPair>(base64String, &startPtr, &size);
		SaveDataPair* ptr{startPtr};
		for (int i = 0; i < size; i += sizeof(SaveDataPair))
		{
			addItem(saveDataToCustomData(ptr->data), GridModel::ItemInfo{ptr->info.x + xOffset, ptr->info.y + yOffset});
			++ptr;
		}
		delete[] startPtr;
	}
	void clearPairs() { GridModel::clearItems(); m_TCustomData.clear(); }
	virtual SaveData customDataToSaveData(const T& data) { return data; }
	virtual T saveDataToCustomData(const SaveData& data) { return data; }
};

} // namespace lmms

#endif // LMMS_GRID_MODEl_H
