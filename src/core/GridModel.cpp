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
#ifdef LMMS_DEBUG
	// asserts:
	if (static_cast<size_t>(mid) >= m_items.size())
	{
		if (m_items.size() > 0)
		{
			assert(xPos > m_items[m_items.size() - 1].info.x);
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
	float newX = fitPos(info.x, m_length, horizontalSteps);
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
void VectorGraphModel::renderAfter(size_t index, std::vector<float>& buffer)
{
	// index of next point
	size_t nextIndex{index};
	size_t attriIndexA{index};
	size_t attriIndexB{index};
	for (size_t i = index + 1; i < getCount(); ++i)
	{
		if (getObject(i).type == VGPoint::Type::attribute)
		{
			if (attribIndexA == index) {attribIndexA = i; }
			else if (attribIndexB == index) { attribIndexB = i; }
		}
		else { nextIndex = i; break; }
	}
	// render between these
	size_t from{static_cast<size_t>(getItem(index).x * buffer.size() / static_cast<float>(getCount()))};
	size_t end{static_cast<size_t>(getItem(nextIndex).x * buffer.size() / static_cast<float>(getCount()))};

	// render edges
	if (index == 0)
	{
		float startY{getItem(index).y};
		for (size_t i = 0; i < from; ++i) { buffer[i] = startY; }
	}
	else if (index + 1 >= getCount())
	{
		float startY{getItem(index).y};
		for (size_t i = from; i < buffer.size(); ++i) { buffer[i] = startY; }
	}
	else if (from < end)
	{
		// attributes
		float attributeX{getItem(attribIndexA).x - getItem(index).x};
		float curNextRatio{(getItem(attribIndexA).x - getItem(index).x) / (getItem(nextIndex).x - getItem(index).x)};
		float attributeY{getItem(attribIndexA).y - (getItem(index).y * (1.0f - curNextRatio) + getItem(nextIndex).y * curNextRatio)};
		switch (getObject(index).type)
		{
			case VGPoint::Type::bezier:
				float startX{getItem(index).x};
				float endX{getItem(nextIndex).x};
				float iRatio{1.0f - curNextRatio};
				// the attribute point is where the bezier should go trough
				// calculating the control point's height at coords (curNextRatio, attributeY)
				float bezierY{(attributeY - startX * curNextRatio * curNextRatio - endX * iRato * iRatio) /
					(2.0f * curNextRatio * iRatio)};
				break;
			case VGPoint::Type::sine:
				// attributeX is at 75% of T, 1.0f / 0.75f = 1.33333f
				float periodSamples{attributeX * 1.33333f * buffer.size() / static_cast<float>(getCount())};
				processLineTypeSine(buffer, from, end,
					attributeY, 1.0f / periodSamples, 0.0f, 0.05f);
				break;
			case VGPoint::Type::peak:
				break;
			case VGPoint::Type::steps:
				break;
			case VGPoint::Type::random:
				break;
		}
	}
}
const std::vector<float>& VectorGraphModel::getBuffer() const
{
	return m_buffer;
}
std::vector<float>& VectorGraphModel::getBufferRef()
{
	return m_buffer;
}
void VectorGraphModel::dataChangedAt(size_t index)
{
}

void VectorGraphModel::processLineTypeBezier(std::vector<float>& samplesOut, size_t startLoc, size_t endLoc,
	float yBefore, float yAfter, float yMid)
{
	for (size_t i = startLoc; i < endLoc; i++)
	{
		//P(t) = P0*t^2 + P1*2*t*(1-t) + P2*(1-t)^2
		// bezier P(t), "t" is a number between 0 and 1
		float t = (i - startLoc) / static_cast<float>(endLoc - startLoc);
		// inverse t
		float iT = (1.0f - t);
		samplesOut[i] = yBefore * t * t + yMid * 2.0f * t * iT + yAfter * iT * iT;
		//samplesOut[i] = std::clamp((*samplesOut)[i] * curveStrength, -1.0f, 1.0f);
	}

	/*
	// draw line
	if (curveStrength < 1.0f)
	{
		for (size_t i = startLoc; i < endLoc; i++)
		{
			(*samplesOut)[i] += (yBefore + (yAfter - yBefore) * (*xArray)[i]) * (1.0f - curveStrength);
		}
	}
	*/
}
void VectorGraphModel::processLineTypeSine(std::vector<float>& samplesOut, size_t startLoc, size_t endLoc,
	float sineAmp, float sineFreq, float sinePhase, float fadeInStartVal)
{
	/*
	float startLocVal = samplesOut[startLoc];
	float endLocVal = samplesOut[endLoc > 0 ? endLoc - 1 : 0];
	int count = static_cast<int>(endLoc) - static_cast<int>(startLoc);
	if (count < 0) { count = 0; }
	float tValB = 0.001f + ((sineFreq + 1.0f) / 2.0f) * 0.999f;
	// calculating how many samples are needed for 1 complete wave
	// we have "count" amount of samples and "tValB * 100.0f" amount of waves
	int end = static_cast<int>(std::floor(count / (tValB * 100.0f)));
	if (count < 0)
	{
		end = 0;
	}
	else if (end > 0)
	{
		end = end > count ? count : end + 1;
	}
	// "allocate" "end" amount of floats
	// for 1 whole sine wave
	// in the universal buffer
	if (static_cast<int>(m_universalSampleBuffer.size()) < end)
	{
		m_universalSampleBuffer.resize(end);
	}
	*/


	for (size_t i = startLoc; i < endLoc; i++)
	{
		float xRatio{(i - startLoc) / static_cast<float>(endLoc - startLoc)};
		samplesOut[i] = sineAmp * std::sin(
			xRatio * 6.28318531f * tValB + sinePhase);
	}

	/*
	// calculate 1 wave of sine
	for (int i = 0; i < end; i++)
	{
		// 628.318531f = 100.0f * 2.0f * pi
		// (1 sine wave is 2pi long and we have 1 * 100 * sineFreq waves)
		// DO NOT CHANGE THIS WITHOUNT UPDATING `VectorGraphHelpView::s_helpText`
		m_universalSampleBuffer[i] = sineAmp * std::sin(
			(*xArray)[startLoc + i] * 628.318531f * tValB + sinePhase * 100.0f);
	}
	// copy the first wave until the end
	for (int i = 0; i < count; i += end)
	{
		int endB = i + end >= count ? count - i : end;
		for (int j = 0; j < endB; j++)
		{
			(*samplesOut)[startLoc + j + i] += m_universalSampleBuffer[j];
		}
	}
	*/

	// fade in
	size_t fadeInEndLoc = static_cast<size_t>(fadeInStartVal * static_cast<float>(endLoc - startLoc));
	for (size_t i = startLoc; i < startLoc + fadeInEndLoc; i++)
	{
		float xRatio{(i - startLoc) / static_cast<float>(endLoc - startLoc)};
		float x{xRatio / fadeInStartVal};
		samplesOut[i] = (*samplesOut)[i] * x + startLocVal * (1.0f - x);
	}
	// fade out
	for (size_t i = endLoc - 1; i > endLoc - fadeInEndLoc; i--)
	{
		float xRatio{(i - startLoc) / static_cast<float>(endLoc - startLoc)};
		float x{xRatio / fadeInStartVal};
		samplesOut[i] = (*samplesOut)[i] * x + startLocVal * (1.0f - x);
	}
}
void VectorGraphModel::processLineTypePeak(std::vector<float>& samplesOut, size_t startLoc, size_t endLoc,
	float peakAmp, float peakX, float peakWidth, float fadeInStartVal)
{
	float startLocVal = (*samplesOut)[startLoc];
	float endLocVal = (*samplesOut)[endLoc > 0 ? endLoc - 1 : 0];
	int count = static_cast<int>(endLoc) - static_cast<int>(startLoc);
	count = count < 0 ? 0 : count;
	for (size_t i = 0; i < static_cast<size_t>(count); i++)
	{
		(*samplesOut)[startLoc + i] += std::pow((peakWidth + 1.0f) * 0.2f + 0.01f,
			std::abs((*xArray)[startLoc + i] - (peakX + 1.0f) * 0.5f) * 10.0f) * peakAmp;
	}

	// fade in
	size_t fadeInEndLoc = static_cast<size_t>(fadeInStartVal * static_cast<float>(count));
	for (size_t i = startLoc; i < startLoc + fadeInEndLoc; i++)
	{
		float x = (*xArray)[i] / fadeInStartVal;
		(*samplesOut)[i] = (*samplesOut)[i] * x + startLocVal * (1.0f - x);
	}
	// fade out
	for (size_t i = endLoc - 1; i > endLoc - fadeInEndLoc; i--)
	{
		float x = (1.0f - (*xArray)[i]) / fadeInStartVal;
		(*samplesOut)[i] = (*samplesOut)[i] * x + endLocVal * (1.0f - x);
	}
}
void VectorGraphModel::processLineTypeSteps(std::vector<float>& samplesOut, size_t startLoc, size_t endLoc,
	std::vector<float>& yArray, float stepCount, float stepCurve, float fadeInStartVal)
{
	float startLocVal = (*samplesOut)[startLoc];
	float endLocVal = (*samplesOut)[endLoc > 0 ? endLoc - 1 : 0];
	int count = static_cast<int>(endLoc) - static_cast<int>(startLoc);
	count = count < 0 ? 0 : count;

	// DO NOT CHANGE THIS WITHOUNT UPDATING `VectorGraphHelpView::s_helpText`
	float stepCountB = (1.0f + stepCount) / 2.0f * 19.0f + 1.0f;
	for (size_t i = 0; i < static_cast<size_t>(count); i++)
	{
		float y = (*yArray)[startLoc + i] + 1.0f;
		float diff = std::round(y * stepCountB) - y * stepCountB;
		float smooth = 1.0f - std::abs(diff) * (1.0f - (stepCurve + 1.0f) / 2.0f) * 2.0f;
		(*samplesOut)[startLoc + i] += diff / stepCountB * smooth;
	}

	// fade in
	size_t fadeInEndLoc = static_cast<size_t>(fadeInStartVal * static_cast<float>(count));
	for (size_t i = startLoc; i < startLoc + fadeInEndLoc; i++)
	{
		float x = (*xArray)[i] / fadeInStartVal;
		(*samplesOut)[i] = (*samplesOut)[i] * x + startLocVal * (1.0f - x);
	}
	// fade out
	for (size_t i = endLoc - 1; i > endLoc - fadeInEndLoc; i--)
	{
		float x = (1.0f - (*xArray)[i]) / fadeInStartVal;
		(*samplesOut)[i] = (*samplesOut)[i] * x + endLocVal * (1.0f - x);
	}
}
void VectorGraphModel::processLineTypeRandom(std::vector<float>& samplesOut, size_t startLoc, size_t endLoc,
	float randomAmp, float randomCount, float randomSeed, float fadeInStartVal)
{
	int count = static_cast<int>(endLoc) - static_cast<int>(startLoc);
	count = count < 0 ? 0 : count;

	// DO NOT CHANGE THIS WITHOUNT UPDATING `VectorGraphHelpView::s_helpText`
	constexpr size_t maxRandomValueCount = 200;
	constexpr size_t maxRandomValueSeed = 20;
	const size_t randomValueCount = static_cast<size_t>(maxRandomValueCount * ((randomCount + 1.0f) * 0.5f));

	if (randomValueCount <= 0) { return; }

	const float seedAsFloat = ((randomSeed + 1.0f) * 0.5f) * static_cast<float>(maxRandomValueSeed - 1);
	size_t seed = static_cast<size_t>(seedAsFloat);
	const float blend = seedAsFloat - static_cast<float>(seed);
	seed = seed * randomValueCount;
	const size_t seedPlusOne = seed + randomValueCount;

	const std::vector<float>& randomNumbers = VectorGraphModel::getRandomValues();
	if (seedPlusOne + randomValueCount + 1 >= randomNumbers->size()) { return; }

	for (size_t i = 0; i < static_cast<size_t>(count); i++)
	{
		const float randomLocationAsFloat = ((*xArray)[startLoc + i] * randomValueCount);
		const size_t randomLocation = static_cast<size_t>(randomLocationAsFloat);
		const float curBlend = randomLocationAsFloat - static_cast<float>(randomLocation);
		const float curInvBlend = 1.0f - curBlend;

		(*samplesOut)[startLoc + i] += (((*randomNumbers)[seed + randomLocation] * (1.0f - curBlend * curBlend) +
			(*randomNumbers)[seed + randomLocation + 1] * (1.0f - curInvBlend * curInvBlend)) * (1.0f - blend) +
			((*randomNumbers)[seedPlusOne + randomLocation] * (1.0f - curBlend * curBlend) +
			(*randomNumbers)[seedPlusOne + randomLocation + 1] * (1.0f - curInvBlend * curInvBlend)) * (blend)) * randomAmp;
	}
}

} // namespace lmms
