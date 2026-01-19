/*
 * VectorGraphModel.h - model for vector based graph
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

#include "VectorGraphModel.h"

#include <cassert>
#include <cmath>

#include <QDomElement>

namespace lmms
{

VectorGraphModel::VectorGraphModel(unsigned int length, unsigned int height, unsigned int horizontalSteps, unsigned int verticalSteps,
	size_t bufferSize, Model* parent, QString displayName, bool defaultConstructed)
	: GridModelTyped{length, height, horizontalSteps, verticalSteps, parent, displayName, defaultConstructed}
	, m_buffer{}
	, m_allChanged{true}
	, m_changedData{}
	, m_setAndBufferMutex{}
{
	m_buffer.resize(bufferSize);
}

void VectorGraphModel::renderAllTo(std::vector<float>& bufferOut)
{
	renderStart(bufferOut);
	for (size_t i = 0; i < getCount(); ++i)
	{
		if (getObject(i).type != VGPoint::Type::attribute)
		{
			renderAfter(i, bufferOut);
		}
	}
}
void VectorGraphModel::renderChangedPoints()
{
	if (m_allChanged)
	{
		renderAllTo(m_buffer);
		m_allChanged = false;
		m_changedData.clear();
		return;
	}
	std::lock_guard<std::mutex> lock(m_setAndBufferMutex);
	renderStart(m_buffer);

	ssize_t lastNotAttributeIndex{-1};
	size_t updatedTo{0};
	for (size_t i : m_changedData)
	{
		if (i >= getCount()) { break; }
		if (i < updatedTo) { continue; }

		// finding lastNotAttributeIndex
		for (size_t j = i; j-- > 0;)
		{
			if (getObject(j).type != VGPoint::Type::attribute && j != i)
			{
				lastNotAttributeIndex = j;
				break;
			}
		}

		// render line segment before i
		if (lastNotAttributeIndex >= static_cast<ssize_t>(updatedTo))
		{
			renderAfter(static_cast<size_t>(lastNotAttributeIndex), m_buffer, &updatedTo);
		}
		// render line segment after i (if i isn't an attribute)
		if (getObject(i).type != VGPoint::Type::attribute)
		{
			renderAfter(i, m_buffer, &updatedTo);
		}
	}

	m_changedData.clear();
}

void VectorGraphModel::renderAfter(size_t index, std::vector<float>& buffer, size_t* updatedTo)
{
	assert(getObject(index).type != VGPoint::Type::attribute);
	// index of next point
	size_t nextIndex{index};
	size_t attribIndexA{index};
	size_t attribIndexB{index};
	for (size_t i = index + 1; i < getCount(); ++i)
	{
		if (getObject(i).type == VGPoint::Type::attribute)
		{
			if (attribIndexA == index) {attribIndexA = i; }
			else if (attribIndexB == index) { attribIndexB = i; }
		}
		else { nextIndex = i; break; }
	}
	if (updatedTo != nullptr) { *updatedTo = nextIndex; }
	// render between these
	size_t from{static_cast<size_t>(getItem(index).info.x * buffer.size() / static_cast<float>(getLength()))};
	size_t end{static_cast<size_t>(getItem(nextIndex).info.x * buffer.size() / static_cast<float>(getLength()))};

	// render edge after endpoint
	if (index == nextIndex)
	{
		float startY{getItem(index).info.y};
		for (size_t i = from; i < buffer.size(); ++i) { buffer[i] = startY; }
	}
	else if (from < end)
	{
		// attributes
		float attributeX{getItem(attribIndexA).info.x - getItem(index).info.x};
		float attribXRatio{(getItem(attribIndexA).info.x - getItem(index).info.x) / (getItem(nextIndex).info.x - getItem(index).info.x)};
		float attributeY{getItem(attribIndexA).info.y - (getItem(index).info.y * (1.0f - attribXRatio) + getItem(nextIndex).info.y * attribXRatio)};

		switch (getObject(index).type)
		{
			case VGPoint::Type::bezier:
			{
				float startY{getItem(index).info.y};
				float endY{getItem(nextIndex).info.y};
				float iRatio{1.0f - attribXRatio};
				if (attribXRatio != 0.0f && iRatio != 0.0f)
				{
					// the attribute point is where the bezier should go trough
					// calculating the control point's height at coords (attribXRatio, attributeY)
					float bezierY{(getItem(attribIndexA).info.y - endY * attribXRatio * attribXRatio - startY * iRatio * iRatio) /
						(2.0f * attribXRatio * iRatio)};
					processLineTypeBezier(buffer, from, end, getItem(index).info.y, getItem(nextIndex).info.y, bezierY);
				}
				else
				{
					processLineTypeLinInterpolate(buffer, from, end, getItem(index).info.y, getItem(nextIndex).info.y, true);
				}
				break;
			}
			case VGPoint::Type::sine:
			{
				if (index == attribIndexA)
				{
					processLineTypeSine(buffer, from, end, 0.5f, 10.0f, 0.0f, 0.05f);
				}
				else
				{
					// attributeX is at 75% of T, 1.0f / 0.75f = 1.33333f
					float periodSamples{attributeX * 1.33333f * buffer.size() / static_cast<float>(getLength())};
					processLineTypeSine(buffer, from, end, -attributeY, (end - from) / periodSamples, 0.0f, 0.05f);
				}
				processLineTypeLinInterpolate(buffer, from, end, getItem(index).info.y, getItem(nextIndex).info.y, false);
				break;
			}
			case VGPoint::Type::peak:
			{
				if (index == attribIndexA)
				{
					processLineTypePeak(buffer, from, end, 0.5f, -0.5f, 10.0f, 0.05f);
				}
				else
				{
					processLineTypePeak(buffer, from, end, attributeY, -attribXRatio, 10.0f, 0.05f);
				}
				processLineTypeLinInterpolate(buffer, from, end, getItem(index).info.y, getItem(nextIndex).info.y, false);
				break;
			}
			case VGPoint::Type::steps:
			{
				processLineTypeLinInterpolate(buffer, from, end, getItem(index).info.y, getItem(nextIndex).info.y, true);
				if (index == attribIndexA)
				{
					processLineTypeSteps(buffer, from, end, getHeight() / 10.0f, 1.0f, getItem(index).info.y, 0.05f);
				}
				else
				{
					size_t attribAt{static_cast<size_t>(getItem(attribIndexA).info.x * buffer.size() / static_cast<float>(getLength()))};
					// mod at this height
					float yAtAttrib{buffer[attribAt] - getItem(index).info.y};
					processLineTypeSteps(buffer, from, end, yAtAttrib, -attributeY / yAtAttrib, getItem(index).info.y, 0.05f);
				}
				break;
			}
			default:
				break;
		}
		// clamp
		const float floatHeight{static_cast<float>(getHeight())};
		for (size_t i = from; i < end; ++i)
		{
			buffer[i] = std::clamp(buffer[i], 0.0f, floatHeight);
		}
	}
}
void VectorGraphModel::renderStart(std::vector<float>& buffer)
{
	// index of next point
	size_t nextIndex{0};
	bool found{false};
	for (size_t i = 0; i < getCount(); ++i)
	{
		if (getObject(i).type != VGPoint::Type::attribute) { found = true; nextIndex = i; break; }
	}
	size_t from{buffer.size()};
	float startY{0.0f};
	if (found)
	{
		from = static_cast<size_t>(getItem(nextIndex).info.x * buffer.size() / static_cast<float>(getLength()));
		startY = getItem(nextIndex).info.y;
	}
	for (size_t i = 0; i < from; ++i) { buffer[i] = startY; }
}

const std::vector<float>& VectorGraphModel::getBuffer()
{
	renderChangedPoints();
	return m_buffer;
}
std::vector<float>& VectorGraphModel::getBufferRef()
{
	renderChangedPoints();
	return m_buffer;
}
void VectorGraphModel::setRenderSize(size_t newSize)
{
	if (newSize != m_buffer.size())
	{
		m_buffer.resize(newSize);
		m_allChanged = true;
	}
}
void VectorGraphModel::dataChangedAt(ssize_t index)
{
	if (index < 0)
	{
		m_allChanged = true;
	}
	else
	{
		m_changedData.insert(index);
	}
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
		samplesOut[i] = yAfter * t * t + yMid * 2.0f * t * iT + yBefore * iT * iT;
	}
}
void VectorGraphModel::processLineTypeSine(std::vector<float>& samplesOut, size_t startLoc, size_t endLoc,
	float sineAmp, float sineFreq, float sinePhase, float fadeInStartVal)
{
	for (size_t i = startLoc; i < endLoc; i++)
	{
		float xRatio{(i - startLoc) / static_cast<float>(endLoc - startLoc)};
		samplesOut[i] = sineAmp * std::sin(
			xRatio * 6.28318531f * sineFreq + sinePhase);
	}
	processLineTypeFade(samplesOut, startLoc, endLoc, fadeInStartVal);
}
void VectorGraphModel::processLineTypePeak(std::vector<float>& samplesOut, size_t startLoc, size_t endLoc,
	float peakAmp, float peakX, float peakWidth, float fadeInStartVal)
{
	for (size_t i = startLoc; i < endLoc; i++)
	{
		float xRatio{(i - startLoc) / static_cast<float>(endLoc - startLoc)};
		samplesOut[i] = std::pow(0.5f, std::abs(xRatio + peakX) * peakWidth) * peakAmp;
	}
	processLineTypeFade(samplesOut, startLoc, endLoc, fadeInStartVal);
}
void VectorGraphModel::processLineTypeSteps(std::vector<float>& samplesIO, size_t startLoc, size_t endLoc,
		float stepHeight, float stepAmp, float yBefore, float fadeInStartVal)
{
	for (size_t i = startLoc; i < endLoc; i++)
	{
		float modY{std::fmod(samplesIO[i] - yBefore, stepHeight)};
		samplesIO[i] = samplesIO[i] - modY * stepAmp;
	}
}
void VectorGraphModel::processLineTypeLinInterpolate(std::vector<float>& samplesOut, size_t startLoc, size_t endLoc,
	float startY, float endY, bool shouldOverride)
{
	if (shouldOverride)
	{
		for (size_t i = startLoc; i < endLoc; i++)
		{
			float xRatio{(i - startLoc) / static_cast<float>(endLoc - startLoc)};
			samplesOut[i] = startY * (1.0f - xRatio) + endY * xRatio;
		}
	}
	else
	{
		// add interpolation if not override
		for (size_t i = startLoc; i < endLoc; i++)
		{
			float xRatio{(i - startLoc) / static_cast<float>(endLoc - startLoc)};
			samplesOut[i] += startY * (1.0f - xRatio) + endY * xRatio;
		}
	}
}
void VectorGraphModel::processLineTypeFade(std::vector<float>& samplesOut, size_t startLoc, size_t endLoc,
	float fadeInStartVal)
{
	// fade in
	size_t fadeInEndLoc = static_cast<size_t>(fadeInStartVal * static_cast<float>(endLoc - startLoc));
	for (size_t i = startLoc; i < startLoc + fadeInEndLoc; ++i)
	{
		float xRatio{(i - startLoc) / (static_cast<float>(endLoc - startLoc) * fadeInStartVal)};
		samplesOut[i] = samplesOut[i] * xRatio;
	}
	// fade out
	for (size_t i = endLoc - fadeInEndLoc - 1; i < endLoc; ++i)
	{
		float xRatio{(endLoc - i - 1) / (static_cast<float>(endLoc - startLoc) * fadeInStartVal)};
		samplesOut[i] = samplesOut[i] * xRatio;
	}
}

void VectorGraphModel::saveSettings(QDomDocument& doc, QDomElement& element, const QString& name)
{
	QDomElement me = doc.createElement(name);
	{
		me.setAttribute("graphPoints", getPointsBase64(0.0f, 0.0f, nullptr));
	}
	element.appendChild(me);
}
void VectorGraphModel::loadSettings(const QDomElement& element, const QString& name)
{
	QDomNode node = element.namedItem(name);

	/* Try this if loading doesn't work
	if(node.isNull() == true)
	{
		for (QDomElement othernode = element.firstChildElement();
			!othernode.isNull();
			othernode = othernode.nextSiblingElement())
		{
			if (othernode.nodeName() == name)
			{
				node = othernode;
				break;
			}
		}
	}
	*/

	if (node.isElement())
	{
		QDomElement nodeElement = node.toElement();
		if (nodeElement.hasAttribute("graphPoints"))
		{
			clear();
			addPointsBase64(nodeElement.attribute("graphPoints"), 0.0f, 0.0f);
		}
	}
}
void VectorGraphModel::saveSettings(QDomDocument& doc, QDomElement& element)
{
	saveSettings(doc, element, "VectorGraphModel");
}
void VectorGraphModel::loadSettings(const QDomElement& element)
{
	loadSettings(element, "VectorGraphModel");
}
QString VectorGraphModel::getPointsBase64(float xOffset, float yOffset, const std::set<size_t>* selection)
{
	std::lock_guard<std::mutex> lock(m_setAndBufferMutex);
	return dataToBase64(xOffset, yOffset, selection);
}
void VectorGraphModel::addPointsBase64(QString base64String, float xOffset, float yOffset)
{
	std::lock_guard<std::mutex> lock(m_setAndBufferMutex);
	addBase64Data(base64String, xOffset, yOffset);
}
void VectorGraphModel::clear()
{
	std::lock_guard<std::mutex> lock(m_setAndBufferMutex);
	clearPairs();
}

} // namespace lmms
