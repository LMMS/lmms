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
#include <vector>

#include "base64.h"
#include "Model.h"

namespace lmms
{

static const unsigned int GRID_MAX_STEPS = 100000;

/* This class handles
	1. grid's size
	2. storing and pairing `ItemInfo` to custom data
	3. getting `ItemInfo` custom data pairs
	4. saving `ItemInfo` with paired custom data
	5. changes in `ItemInfo` (position changes)
	This class doesn't handle
	1. constructing, storing, interacting or removing custom data in any way
*/

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

	//*** item management ***
	//! @return new / final index (if x changed)
	size_t setInfo(size_t index, const ItemInfo& info);
	size_t setInfo(size_t index, const ItemInfo& info, unsigned int horizontalSteps, unsigned int verticalSteps);
	const Item& getItem(size_t index) const;
	//! @return the index where [index - 1].x < xPos and xPos <= [index].x
	//! so return the first index where xPos <= x (this index can be the size)
	size_t findIndex(float xPos) const;

	//*** model management ***
	//! @return item count
	size_t getCount() const;
	unsigned int getLength() const;
	unsigned int getHeight() const;
	void resizeGrid(size_t length, size_t height);
	void setSteps(unsigned int horizontalSteps, unsigned int verticalSteps);

	virtual ~GridModel();
protected:
	GridModel(unsigned int length, unsigned int height, unsigned int horizontalSteps, unsigned int verticalSteps,
		Model* parent, QString displayName, bool defaultConstructed);
	virtual void dataChangedAt(ssize_t index) {}

	//! @return index where added
	size_t addItem(Item itemIn);
	void removeItem(size_t index);
	void clearItems();
	//! DOESN'T EMIT, get and set index pointing to custom data
	size_t& getAndSetObjectIndex(size_t index);
private:
	//! only call these with fitted values (`fitPos`)
	//! @return new / final index (if x changed)
	size_t setX(size_t index, float newX);
	void setY(size_t index, float newY);
	float fitPos(float position, unsigned int max, unsigned int steps) const;
	//! moves `startIndex` to `finalIndex` by swapping
	void move(size_t startIndex, size_t finalIndex);

	unsigned int m_length;
	unsigned int m_height;
	unsigned int m_horizontalSteps;
	unsigned int m_verticalSteps;

	//! items are sorted by x position ascending
	std::vector<Item> m_items;
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
	struct TrueDataPair
	{
		GridModel::ItemInfo info;
		SaveData data;
	};

	std::vector<T> m_TCustomData;
public:
	GridModelTyped(unsigned int length, unsigned int height, unsigned int horizontalSteps, unsigned int verticalSteps,
		Model* parent, QString displayName = QString(), bool defaultConstructed = false)
		: GridModel{length, height, horizontalSteps, verticalSteps, parent, displayName, defaultConstructed} {}
	~GridModelTyped() = default;

	const T& getObject(size_t index) const { return m_TCustomData[GridModel::getItem(index).objectIndex]; }
	virtual void setObject(size_t index, T object)
	{
		m_TCustomData[GridModel::getItem(index).objectIndex] = object;
		dataChangedAt(index); emit GridModel::dataChanged();
	}

	//! @return index where added
	size_t addItem(T object, ItemInfo info)
	{
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
	QString dataToBase64(const std::set<size_t>* selection = nullptr)
	{
		std::vector<TrueDataPair> dataArray{};
		if (selection == nullptr || selection->empty())
		{
			dataArray.reserve(getCount());
			for (size_t i = 0; i < getCount(); ++i)
			{
				dataArray.push_back(
					TrueDataPair{getItem(i).info, customDataToSaveData(getObject(i))});
			}
		}
		else
		{
			dataArray.reserve(selection->size());
			for (size_t i : *selection)
			{
				dataArray.push_back(
					TrueDataPair{getItem(i).info, customDataToSaveData(getObject(i))});
			}
		}
		QString output{};
		base64::encode((const char *)(dataArray.data()),
			dataArray.size() * sizeof(TrueDataPair), output);
		return output;
	}
	void addBase64Data(QString base64String)
	{
		int size = 0;
		TrueDataPair* startPtr = nullptr;
		base64::decode<TrueDataPair>(base64String, &startPtr, &size);
		TrueDataPair* ptr{startPtr};
		for (int i = 0; i < size; i += sizeof(TrueDataPair))
		{
			addItem(saveDataToCustomData(ptr->data), ptr->info);
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
