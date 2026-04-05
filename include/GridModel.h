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

#include <atomic>
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
	2. storing `ItemInfo`
	3. managing static lookup indexes for the stored items 
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
		float x = 0.0f;
		float y = 0.0f;
	};
	struct Item
	{
		ItemInfo info;
		std::atomic<unsigned int> staticIndex; //!< converts `m_items` index to static index
		std::atomic<unsigned int> lookupIndex; //!< converts static index to `m_items` relative index

		Item(ItemInfo infoIn, unsigned int staticIndexIn, unsigned int lookupIndexIn)
			: info{infoIn}
			, staticIndex{staticIndexIn}
			, lookupIndex{lookupIndexIn}
		{}
		Item()
			: info{0.0f, 0.0f}
			, staticIndex{0}
			, lookupIndex{0}
		{}
		Item(const Item& rhs)
			: info{rhs.info}
			, staticIndex{rhs.staticIndex.load(std::memory_order_relaxed)}
			, lookupIndex{rhs.lookupIndex.load()}
		{}
		void operator=(const Item& rhs)
		{
			info = rhs.info;
			staticIndex.store(rhs.staticIndex.load(std::memory_order_relaxed), std::memory_order_relaxed);
			lookupIndex.store(rhs.lookupIndex.load(std::memory_order_acquire), std::memory_order_release);
		}
		void operator<<(const Item& rhs) //! used to assign while keeping `lookupIndex` intact
		{
			info = rhs.info;
			staticIndex.store(rhs.staticIndex.load(std::memory_order_acquire), std::memory_order_release);
			// lookupIndex shouldn't be changed, it is at a static index!
		}
	};
	struct StaticIndex
	{
		StaticIndex() : index{0} {}
		StaticIndex(const StaticIndex& stIndex) : index{stIndex.index} {}
		StaticIndex& operator=(StaticIndex rhs) { index = rhs.index; return *this; }
		StaticIndex operator+(size_t rhs) { return StaticIndex{index + rhs}; }
		StaticIndex operator-(size_t rhs) { return StaticIndex{index - rhs}; }
		bool operator>=(const StaticIndex& rhs) const { return index >= rhs.index; }
		bool operator>(const StaticIndex& rhs) const { return index > rhs.index; }
		bool operator<=(const StaticIndex& rhs) const { return index <= rhs.index; }
		bool operator<(const StaticIndex& rhs) const { return index < rhs.index; }
		bool operator==(const StaticIndex& rhs) const { return index == rhs.index; }
		bool operator>=(size_t rhs) const { return index >= rhs; }
		bool operator>(size_t rhs) const { return index > rhs; }
		bool operator<=(size_t rhs) const { return index <= rhs; }
		bool operator<(size_t rhs) const { return index < rhs; }
		operator size_t() = delete;
		size_t getRaw() const { return index; }
		void setRaw(size_t indexIn) { index = indexIn; }
	private:
		StaticIndex(size_t indexIn) : index{indexIn} {}
		size_t index;
		friend class GridModel;
	};

	//*** util ***
	//! @return relative index if found inside radius, else -1
	//! (the first index is returned where `xPos` is found)
	int findIndexFromPos(float xPos, float radius) const;
	//! @return the index where [index - 1].x < xPos and xPos <= [index].x
	//! so return the first index where xPos <= x (this index can be the size)
	size_t findIndex(float xPos) const;

	//*** item management ***
	const Item& getItem(size_t relIndex) const;
	//! @return new / final relative index (if x changed)
	size_t setInfo(size_t relIndex, const ItemInfo& info);
	size_t setInfo(size_t relIndex, const ItemInfo& info, unsigned int horizontalSteps, unsigned int verticalSteps);
	size_t sToRIndex(StaticIndex statIndex) const; //!< converts static indexes to relative index
	StaticIndex rToSIndex(size_t relIndex) const; //!< converts relative indexes to static index

	//*** model management ***
	//! @return item count
	size_t getCount() const;
	unsigned int getCountLimit() const;
	unsigned int getLength() const;
	unsigned int getHeight() const;
	void setSteps(unsigned int horizontalSteps, unsigned int verticalSteps);
	void resizeGridArea(size_t length, size_t height);

	virtual ~GridModel();
protected:
	GridModel(unsigned int countLimit, unsigned int length, unsigned int height,
		unsigned int horizontalSteps, unsigned int verticalSteps, Model* parent, QString displayName, bool defaultConstructed);
	virtual void dataChangedAt(signed long long index) {}

	//*** item management ***
	//! @return relative index where added, `getCount()` if fails
	size_t addItemG(Item itemIn);
	void removeItemG(size_t relIndex);
	void clearItemsG();
	//! @return get and set relative index pointing to custom data, DOESN'T EMIT
	void setStaticIndex(size_t relIndex, unsigned int newStaticIndex);

	//! @return how many items were deleted
	int resizeGridCountLimitG(size_t newLimit);
private:
	//! only call these with fitted values (`fitPos`)
	//! @return new / final relIndex (if x changed)
	size_t setX(size_t relIndex, float newX);
	void setY(size_t relIndex, float newY);
	float fitPos(float position, unsigned int max, unsigned int steps) const;
	//! moves `startIndex` to `finalIndex` by swapping
	void move(size_t startRelIndex, size_t finalRelIndex);

	unsigned int m_length;
	unsigned int m_height;
	unsigned int m_horizontalSteps;
	unsigned int m_verticalSteps;

	//! items are sorted by x position ascending
	std::vector<Item> m_items;
	//! what users of this class see as the size
	std::atomic<unsigned int> m_viewerSize;
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
	GridModelTyped(unsigned int countLimit, unsigned int length, unsigned int height, unsigned int horizontalSteps, unsigned int verticalSteps,
		Model* parent, QString displayName = QString(), bool defaultConstructed = false)
		: GridModel{countLimit, length, height, horizontalSteps, verticalSteps, parent, displayName, defaultConstructed}
	{
		m_TCustomData.resize(countLimit);
	}
	~GridModelTyped() = default;

	const T& getObject(size_t relIndex) const
	{
		return m_TCustomData[GridModel::getItem(relIndex).staticIndex.load(std::memory_order_acquire)];
	}
	virtual void setObject(size_t relIndex, T object)
	{
		m_TCustomData[GridModel::getItem(relIndex).staticIndex.load(std::memory_order_acquire)] = object;
		dataChangedAt(relIndex); emit GridModel::dataChanged();
	}

	//! @return relative index where added, `getCount()` if fails
	size_t addItem(T object, ItemInfo info)
	{
		size_t curCount = getCount();
		if (curCount >= getCountLimit()) { return getCountLimit(); }
		m_TCustomData[curCount] = object;
		size_t returnIndex = GridModel::addItemG(Item{info, (unsigned int)(curCount), (unsigned int)(curCount)});
		return returnIndex;
	}
	//! @return the new relative index of the last elem
	size_t removeItem(size_t relIndex)
	{
		size_t curCount = getCount();
		size_t removeThisIndex{GridModel::getItem(relIndex).staticIndex.load(std::memory_order_acquire)};
		size_t swapIndex{GridModel::getItem(getCount() - 1).lookupIndex.load(std::memory_order_relaxed)};
		//! set the last element's static index to `removeThisIndex`, then copy the last element to `removeThisIndex`
		GridModel::setStaticIndex(swapIndex, removeThisIndex);
		m_TCustomData[removeThisIndex] = m_TCustomData[curCount - 1];
		GridModel::setStaticIndex(relIndex, curCount - 1);
		// removing the Item (object* + coords pair)
		GridModel::removeItemG(relIndex);
		return swapIndex;
	}
	//! @return how many items were deleted, NOTE: LOCKS
	int resizeGridCountLimit(size_t newLimit)
	{
		if (newLimit > getCountLimit()) { m_TCustomData.resize(newLimit); }
		int deleteCount = resizeGridCountLimit(newLimit);
		if (newLimit <= getCountLimit()) { m_TCustomData.resize(newLimit); }
		return deleteCount;
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
	void clearPairs() { clearItemsG(); }
	virtual SaveData customDataToSaveData(const T& data) { return data; }
	virtual T saveDataToCustomData(const SaveData& data) { return data; }
};

} // namespace lmms

#endif // LMMS_GRID_MODEl_H
