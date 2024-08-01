/*
 * VectorGraphModel.cpp - Vector graph model and helper class implementation
 *
 * Copyright (c) 2024 szeli1 </at/gmail/dot/com> TODO
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

#include <algorithm> // sort
#include <array>
#include <cmath> // sine
#include <cstdlib> // rand
#include <vector>
#include <QMutex> // locking when getSamples

#include "AutomatableModel.h"
#include "base64.h"
#include "GuiApplication.h" // getGUI
#include "JournallingObject.h"
#include "MainWindow.h" // getting main window for control dialog
#include "ProjectJournal.h"

//#define VECTORGRAPH_DEBUG_USER_INTERACTION
//#define VECTORGRAPH_DEBUG_PAINT_EVENT

namespace lmms
{

VectorGraphModel::VectorGraphModel(unsigned int arrayMaxLength, Model* parent, bool defaultConstructed) :
	Model(parent, tr("VectorGraphModel"), defaultConstructed)
{
	m_maxLength = arrayMaxLength;
	//m_dataArrays
}

VectorGraphModel::~VectorGraphModel()
{
	m_dataArrays.clear();
}

unsigned int VectorGraphModel::addDataArray()
{
	VectorGraphDataArray tempArray(
		false, false, false, false, false, false, false,
		false, false, true, this, getDataArrayNewId());
	m_dataArrays.push_back(tempArray);
	emit dataChanged();
	return m_dataArrays.size() - 1;
}

void VectorGraphModel::deleteDataArray(unsigned int arrayLocation)
{
	std::vector<int> effectorArrayLocations(m_dataArrays.size());
	for (unsigned int i = arrayLocation; i < m_dataArrays.size() - 1; i++)
	{
		m_dataArrays[i] = m_dataArrays[i + 1];
	}
	m_dataArrays.pop_back();
	// reset effector locations to the correct locations
	for (unsigned int i = 0; i < m_dataArrays.size(); i++)
	{
		effectorArrayLocations[i] = m_dataArrays[i].getEffectorArrayLocation();
		if (m_dataArrays[i].getEffectorArrayLocation() == static_cast<int>(arrayLocation))
		{
			effectorArrayLocations[i] = -1;
		}
		else if (effectorArrayLocations[i] >= static_cast<int>(arrayLocation))
		{
			effectorArrayLocations[i]--;
		}
		// effectorLocations are cleared to avoid the
		// dataArrays detecting loops and then not setting
		m_dataArrays[i].setEffectorArrayLocation(-1, false);
	}
	// setting updated locations
	for (unsigned int i = 0; i < m_dataArrays.size(); i++)
	{
		m_dataArrays[i].setEffectorArrayLocation(effectorArrayLocations[i], false);
	}
	emit dataChanged();
	emit updateGraphView(false);
}

void VectorGraphModel::dataArrayChanged()
{
	emit dataChanged();
	emit updateGraphView(false);
}
void VectorGraphModel::updateGraphModel(bool shouldUseGetLastSamples)
{
	// connects to external update signal
	emit updateGraphView(shouldUseGetLastSamples);
}
void VectorGraphModel::dataArrayClearedEvent(int arrayId)
{
	// TODO needs testing
	int location = getDataArrayLocationFromId(arrayId);
	emit clearedEvent(location);
	emit dataChanged();
	emit updateGraphView(false);
}

void VectorGraphModel::dataArrayStyleChanged()
{
	emit styleChanged();
}
int VectorGraphModel::getDataArrayLocationFromId(int arrayId)
{
	int output = -1;

	if (arrayId < 0) { return output; }

	for (unsigned int i = 0; i < m_dataArrays.size(); i++)
	{
		if (m_dataArrays[i].getId() == arrayId)
		{
			output = i;
			break;
		}
	}
	return output;
}
int VectorGraphModel::getDataArrayNewId()
{
	int maxId = 0;
	for (unsigned int i = 0; i < m_dataArrays.size(); i++)
	{
		if (m_dataArrays[i].getId() > maxId)
		{
			maxId = m_dataArrays[i].getId();
		}
	}
	maxId++;
	return maxId;
}
void VectorGraphModel::saveSettings(QDomDocument& doc, QDomElement& element, const QString& name)
{
#ifdef VECTORGRAPH_DEBUG_PAINT_EVENT
	qDebug("saveSettings start");
#endif

	// getting the models saving name
	QString saveName("VectorGraphModel");
	if (name.size() > 0)
	{
		saveName = name;
	}

	QDomElement me = doc.createElement(saveName);
	me.setAttribute("DataArrayCount", static_cast<unsigned int>(m_dataArrays.size()));
	for (unsigned int i = 0; i < m_dataArrays.size(); i++)
	{
		// getting rid of nullptr FloatMdoels
		// (there should be 0)
		for (unsigned int j = 0; j < m_dataArrays[i].size(); j++)
		{
			if (m_dataArrays[i].getAutomationModel(j) == nullptr)
			{
				m_dataArrays[i].setAutomated(j, false);
			}
		}
		// delete automation that is in the automationModelArray
		// but not used by a point (there should be 0 cases like this)
		m_dataArrays[i].deleteUnusedAutomation();

		// getting the start of the attribute name
		QString readLocation = "a" + QString::number(i) + "-";
		std::vector<FloatModel*>* automationModels = m_dataArrays[i].getAutomationModelArray();
		bool isSaveable = m_dataArrays[i].getIsSaveable();
		// general saving attributes
		me.setAttribute(readLocation + "DataArraySize", isSaveable == true ? static_cast<unsigned int>(m_dataArrays[i].size()) : 0);
		me.setAttribute(readLocation + "AutomationSize", isSaveable == true ? static_cast<unsigned int>(automationModels->size()) : 0);

		if (isSaveable == true && m_dataArrays[i].size() > 0)
		{
			// saving the DataArray
			me.setAttribute(readLocation + "DataArray", m_dataArrays[i].getSavedDataArray());

			// saving the FloatModels
			for (unsigned int j = 0; j < automationModels->size(); j++)
			{
#ifdef VECTORGRAPH_DEBUG_PAINT_EVENT
				qDebug("saveSettings saved automatinModel dataArray (i): %d, model (j): %d", i, j);
#endif
				QString readLocationB = QString::number(j) + "-";
				(*automationModels)[j]->saveSettings(doc, me, readLocation + readLocationB + "AutomationModel");
			}
		}
	}
	element.appendChild(me);
}
void VectorGraphModel::loadSettings(const QDomElement& element, const QString& name)
{
#ifdef VECTORGRAPH_DEBUG_PAINT_EVENT
	qDebug("loadSettings start");
#endif
	QString loadName("VectorGraphModel");
	if (name.size() > 0)
	{
		loadName = name;
	}

	QDomNode node = element.namedItem(loadName);

	if(node.isNull() == true)
	{
		for(QDomElement othernode = element.firstChildElement();
			!othernode.isNull();
			othernode = othernode.nextSiblingElement())
		{
			if((!othernode.hasAttribute("DataArrayCount") &&
				othernode.nodeName() == loadName))
			{
				node = othernode;
				break;
			}
		}
	}

	QDomElement curElement = node.toElement();
	if (curElement.hasAttribute("DataArrayCount") == true)
	{
		unsigned int loadSize = curElement.attribute("DataArrayCount").toInt();
		for (unsigned int i = 0; i < loadSize; i++)
		{
			// getting the start of the attribute name
			QString readLocation = "a" + QString::number(i) + "-";
			if (i < m_dataArrays.size() && curElement.hasAttribute(readLocation + "DataArraySize") == true)
			{
				unsigned int dataArraySize = curElement.attribute(readLocation + "DataArraySize").toInt();
				unsigned int automationSize = curElement.attribute(readLocation + "AutomationSize").toInt();
				// load m_dataArray
				if (dataArraySize > 0)
				{
					m_dataArrays[i].loadDataArray(curElement.attribute(readLocation + "DataArray"), dataArraySize, true);
				}

				// load automationModelDataArray
				std::vector<FloatModel*>* automationModels = m_dataArrays[i].getAutomationModelArray();
				for (unsigned int j = 0; j < automationSize; j++)
				{
					QString readLocationB = QString::number(j) + "-";
					FloatModel* curModel = new FloatModel(0.0f, -1.0f, 1.0f, 0.01f, this, QString(), false);
					curModel->loadSettings(curElement, readLocation + readLocationB + "AutomationModel");
					automationModels->push_back(curModel);
#ifdef VECTORGRAPH_DEBUG_PAINT_EVENT
					qDebug("loadSettings loaded automatinModel: arrayLocation (i): %d, model (j): %d", i, j);
#endif
				}
			}
			else
			{
				break;
			}
		}
	}
}
void VectorGraphModel::lockGetSamplesAccess()
{
	m_getSamplesAccess.lock();
}
void VectorGraphModel::unlockGetSamplesAccess()
{
	m_getSamplesAccess.unlock();
}
void VectorGraphModel::lockBakedSamplesAccess()
{
	m_bakedSamplesAccess.lock();
}
void VectorGraphModel::unlockBakedSamplesAccess()
{
	m_bakedSamplesAccess.unlock();
}
void VectorGraphModel::saveSettings(QDomDocument& doc, QDomElement& element)
{
	saveSettings(doc, element, QString(""));
}
void VectorGraphModel::loadSettings(const QDomElement& element)
{
	loadSettings(element, QString(""));
}
void VectorGraphModel::modelAddJournalCheckPoint()
{
	addJournalCheckPoint();
}

// VectorGraphDataArray ------

VectorGraphDataArray::VectorGraphDataArray()
{
	m_isFixedSize = false;
	m_isFixedY = false;
	m_isFixedX = false;
	m_isFixedEndPoints = false;
	m_isSelectable = false;
	m_isEditableAttrib = false;
	m_isAutomatable = false;
	m_isEffectable = false;
	m_isSaveable = false;
	m_isNonNegative = false;
	
	m_lineColor = QColor(200, 200, 200, 255);
	m_activeColor = QColor(255, 255, 255, 255);
	// fill color is not enabled by default
	// (if alpha = 0)
	m_fillColor = QColor(0, 0, 0, 0);
	m_automatedColor = QColor(0, 0, 0, 0);

	m_effectorLocation = -1;

	// m_dataArray
	m_isDataChanged = false;
	// m_bakedSamples;
	// m_updatingBakedSamples
	// m_needsUpdating;
	// m_automationModelArray;

	m_id = -1;
}

VectorGraphDataArray::VectorGraphDataArray(
	bool isFixedSize, bool isFixedX, bool isFixedY, bool isNonNegative,
	bool isFixedEndPoints, bool isSelectable, bool isEditableAttrib, bool isAutomatable,
	bool isEffectable, bool isSaveable, VectorGraphModel* parent, int arrayId)
{
	m_isFixedSize = isFixedSize;
	m_isFixedY = isFixedX;
	m_isFixedX = isFixedY;
	m_isFixedEndPoints = isFixedEndPoints;
	m_isSelectable = isSelectable;
	m_isEditableAttrib = isEditableAttrib;
	m_isAutomatable = isAutomatable;
	m_isEffectable = isEffectable;
	m_isSaveable = isSaveable;
	m_isNonNegative = isNonNegative;
	
	m_lineColor = QColor(200, 200, 200, 255);
	m_activeColor = QColor(255, 255, 255, 255);
	// fill color is not enabled by default
	// (if alpha = 0)
	m_fillColor = QColor(0, 0, 0, 0);

	m_effectorLocation = -1;
	m_isAnEffector = false;

	// m_dataArray;
	m_isDataChanged = false;
	// m_bakedSamples;
	// m_updatingBakedSamples
	// m_needsUpdating;
	// m_automationModelArray;
	// m_universalSampleBuffer

	m_id = arrayId;
	updateConnections(parent);
}

VectorGraphDataArray::~VectorGraphDataArray()
{
	for (unsigned int i = 0; i < m_automationModelArray.size(); i++)
	{
		if (m_automationModelArray[i] != nullptr)
		{
			delete m_automationModelArray[i];
		}
	}
	m_automationModelArray.clear();
}

void VectorGraphDataArray::updateConnections(VectorGraphModel* parent)
{
	// call VectorGraphModel signals without qt
	m_parent = parent;
	m_id = m_parent->getDataArrayNewId();
	// reseting effectors
	setEffectorArrayLocation(-1, true);
}

void VectorGraphDataArray::setIsFixedSize(bool bValue)
{
	m_isFixedSize = bValue;
	getUpdatingFromPoint(-1);
	dataChanged();
}
void VectorGraphDataArray::setIsFixedX(bool bValue)
{
	m_isFixedX = bValue;
	getUpdatingFromPoint(-1);
	dataChanged();
}
void VectorGraphDataArray::setIsFixedY(bool bValue)
{
	m_isFixedY = bValue;
	getUpdatingFromPoint(-1);
	dataChanged();
}
void VectorGraphDataArray::setIsFixedEndPoints(bool bValue)
{
	m_isFixedEndPoints = bValue;
	formatDataArrayEndPoints();
	getUpdatingFromPoint(-1);
	dataChanged();
}
void VectorGraphDataArray::setIsSelectable(bool bValue)
{
	m_isSelectable = bValue;
	getUpdatingFromPoint(-1);
	dataChanged();
}
void VectorGraphDataArray::setIsEditableAttrib(bool bValue)
{
	m_isEditableAttrib = bValue;
	getUpdatingFromPoint(-1);
	dataChanged();
}
void VectorGraphDataArray::setIsAutomatable(bool bValue)
{
	m_isAutomatable = bValue;
	getUpdatingFromPoint(-1);
	dataChanged();
}
void VectorGraphDataArray::setIsEffectable(bool bValue)
{
	m_isEffectable = bValue;
	if (bValue == false)
	{
		// setEffectorArray will call dataChanged()
		setEffectorArrayLocation(-1, true);
	}
	else
	{
		getUpdatingFromPoint(-1);
		dataChanged();
	}
}
void VectorGraphDataArray::setIsSaveable(bool bValue)
{
	m_isSaveable = bValue;
}
void VectorGraphDataArray::setIsNonNegative(bool bValue)
{
	m_isNonNegative = bValue;
	getUpdatingFromPoint(-1);
	dataChanged();
}
void VectorGraphDataArray::setLineColor(QColor bValue)
{
	m_lineColor = bValue;
	styleChanged();
}
void VectorGraphDataArray::setActiveColor(QColor color)
{
	m_activeColor = color;
	styleChanged();
}
void VectorGraphDataArray::setFillColor(QColor color)
{
	m_fillColor = color;
	styleChanged();
}
void VectorGraphDataArray::setAutomatedColor(QColor color)
{
	m_automatedColor = color;
	styleChanged();
}
bool VectorGraphDataArray::setEffectorArrayLocation(int arrayLocation, bool callDataChanged)
{
	bool found = true;
	if (arrayLocation >= 0)
	{
		// if there is no valid id
		if (m_id < 0)
		{
			m_id = m_parent->getDataArrayNewId();
		}
		int searchArrayLocation = arrayLocation;
		found = false;
		// checking if the effector chain has this dataArray in it
		for (unsigned int i = 0; i < m_parent->getDataArraySize(); i++)
		{
			int arrayId = m_parent->getDataArray(searchArrayLocation)->getId();
			searchArrayLocation = m_parent->getDataArray(searchArrayLocation)->getEffectorArrayLocation();
			if(arrayId == m_id)
			{
				found = true;
				break;
			}
			if (searchArrayLocation == -1)
			{
				break;
			}
		}
		// if the effector chain does not contain this dataArray
		if (found == false)
		{
			m_effectorLocation = arrayLocation;
			m_parent->getDataArray(m_effectorLocation)->setIsAnEffector(true);
			getUpdatingFromPoint(-1);
			if (callDataChanged == true)
			{
				dataChanged();
			}
		}
	}
	else
	{
		if (m_effectorLocation != -1)
		{
			// checking if this VectorGraphDataArray's effector is an effector for an other VectorGraphDataArray
			bool foundB = false;
			for (unsigned int i = 0; i < m_parent->getDataArraySize(); i++)
			{
				if (m_parent->getDataArray(i)->getId() != m_id && m_parent->getDataArray(i)->getEffectorArrayLocation() == m_effectorLocation)
				{
					foundB = true;
					break;
				}
			}
			// setting the correct state for the effector array's m_isAnEffector
			m_parent->getDataArray(m_effectorLocation)->setIsAnEffector(foundB);
			m_effectorLocation = -1;

			getUpdatingFromPoint(-1);
			if (callDataChanged == true)
			{
				dataChanged();
			}
		}
	}
	return !found;
}
void VectorGraphDataArray::setIsAnEffector(bool bValue)
{
	m_isAnEffector = bValue;
	// do not need to update anything after
}

bool VectorGraphDataArray::getIsFixedSize()
{
	return m_isFixedSize;
}
bool VectorGraphDataArray::getIsFixedX()
{
	return m_isFixedX;
}
bool VectorGraphDataArray::getIsFixedY()
{
	return m_isFixedY;
}
bool VectorGraphDataArray::getIsFixedEndPoints()
{
	return m_isFixedEndPoints;
}
bool VectorGraphDataArray::getIsSelectable()
{
	return m_isSelectable;
}
bool VectorGraphDataArray::getIsEditableAttrib()
{
	return m_isEditableAttrib;
}
bool VectorGraphDataArray::getIsAutomatable()
{
	return m_isAutomatable;
}
bool VectorGraphDataArray::getIsEffectable()
{
	return m_isEffectable;
}
bool VectorGraphDataArray::getIsSaveable()
{
	return m_isSaveable;
}
bool VectorGraphDataArray::getIsNonNegative()
{
	return m_isNonNegative;
}
QColor* VectorGraphDataArray::getLineColor()
{
	return &m_lineColor;
}
QColor* VectorGraphDataArray::getActiveColor()
{
	return &m_activeColor;
}
QColor* VectorGraphDataArray::getFillColor()
{
	return &m_fillColor;
}
QColor* VectorGraphDataArray::getAutomatedColor()
{
	return &m_automatedColor;
}
int VectorGraphDataArray::getEffectorArrayLocation()
{
	return m_effectorLocation;
}
bool VectorGraphDataArray::getIsAnEffector()
{
	return m_isAnEffector;
}
int VectorGraphDataArray::getId()
{
	return m_id;
}


// array:

int VectorGraphDataArray::add(float newX)
{
	int location = -1;
	if (m_isFixedSize == true || m_dataArray.size() >= m_parent->getMaxLength()) { return location; }

	bool found = false;
	bool isBefore = false;
	location = getNearestLocation(newX, &found, &isBefore);
	if (found == false)
	{
		int targetLocation = -1;
		bool dataChangedVal = false;
		// if getNearestLocation returned a value
		if (location >= 0)
		{
			targetLocation = location;
			// shift the new data if the closest data x is bigger
			// (done for swaping)
			if (isBefore == true)
			{
				// we are adding one value, so dataArray.size() will be a valid location
				if (targetLocation < m_dataArray.size())
				{
					targetLocation++;
				}
			}
			m_dataArray.push_back(VectorGraphPoint(newX, 0.0f));
			swap(m_dataArray.size() - 1, targetLocation, true);
			dataChangedVal = true;
		}
		else if (m_dataArray.size() <= 0)
		{
			m_dataArray.push_back(VectorGraphPoint(newX, 0.0f));
			targetLocation = 0;
			dataChangedVal = true;
		}
		location = targetLocation;

		if (m_dataArray.size() <= 2)
		{
			formatDataArrayEndPoints();
			dataChangedVal = true;
		}
		if (dataChangedVal == true)
		{
			// addtition breaks the order of the locations
			// in m_needsUpdating, so we update the whole m_dataArray
			getUpdatingFromPoint(-1);
			dataChanged();
		}
	}
	return location;
}

void VectorGraphDataArray::deletePoint(unsigned int pointLocation)
{
	if (m_isFixedSize == true || pointLocation >= m_dataArray.size()) { return; }

	// deleting the points automationModel
	deleteAutomationModel(m_dataArray[pointLocation].m_automationModel, true);
	// swaping the point to the last location
	// in m_dataArray
	swap(pointLocation, m_dataArray.size() - 1, true);
	m_dataArray.pop_back();
	if (pointLocation == 0 || pointLocation == m_dataArray.size())
	{
		formatDataArrayEndPoints();
	}
	// calling clearedEvent
	if (m_dataArray.size() == 0)
	{
		clearedEvent();
	}
	// deletion breaks the order of the locations
	// in m_needsUpdating, so we update the whole m_dataArray
	getUpdatingFromPoint(-1);
	dataChanged();
}

void VectorGraphDataArray::formatArray(std::vector<PointF>* dataArrayOut, bool shouldClamp, bool shouldRescale, bool shouldSort, bool callDataChanged)
{
	if (shouldRescale == true)
	{
		// scale
		float minX = 0.0f;
		float maxX = 1.0f;
		float minY = -1.0f;
		float maxY = 1.0f;
		for (unsigned int i = 0; i < dataArrayOut->size(); i++)
		{
			if ((*dataArrayOut)[i].first < minX)
			{
				minX = (*dataArrayOut)[i].first;
			}
			if ((*dataArrayOut)[i].first > maxX)
			{
				maxX = (*dataArrayOut)[i].first;
			}
			if ((*dataArrayOut)[i].second < minY)
			{
				minY = (*dataArrayOut)[i].second;
			}
			if ((*dataArrayOut)[i].second > maxY)
			{
				maxY = (*dataArrayOut)[i].second;
			}
		}
		maxX = (maxX - minX);
		minX = -minX;
		maxY = (maxY - minY) * 0.5f;
		minY = -minY;
		if (minX != 0.0f || maxX != 1.0f)
		{
			for (unsigned int i = 0; i < dataArrayOut->size(); i++)
			{
				(*dataArrayOut)[i].first = ((*dataArrayOut)[i].first + minX) / maxX;
			}
		}
		if (minY != -1.0f || maxY != 1.0f)
		{
			for (unsigned int i = 0; i < dataArrayOut->size(); i++)
			{
				(*dataArrayOut)[i].second = ((*dataArrayOut)[i].second + minY) / maxY - 1.0f;
			}
		}
	}
	if (shouldClamp == true || shouldRescale == true)
	{
		// clamp
		for (unsigned int i = 0; i < dataArrayOut->size(); i++)
		{
			if ((*dataArrayOut)[i].first < 0.0f)
			{
				(*dataArrayOut)[i].first = 0.0f;
			}
			if ((*dataArrayOut)[i].first > 1.0f)
			{
				(*dataArrayOut)[i].first = 1.0f;
			}
			if ((*dataArrayOut)[i].second < -1.0f)
			{
				(*dataArrayOut)[i].second = -1.0f;
			}
			if ((*dataArrayOut)[i].second > 1.0f)
			{
				(*dataArrayOut)[i].second = 1.0f;
			}
		}
		formatDataArrayEndPoints();
	}

	// sort
	if (shouldSort == true)
	{
		std::sort(dataArrayOut->begin(), dataArrayOut->end(),
			[](PointF a, PointF b)
			{
				return a.first < b.first;
			});
	}

	// delete duplicates
	float lastPos = -1.0f;
	if (dataArrayOut->size() > 0)
	{
		lastPos = (*dataArrayOut)[0].first;
	}
	for (unsigned int i = 1; i < dataArrayOut->size(); i++)
	{
		if ((*dataArrayOut)[i].first == lastPos)
		{
			deletePoint(i);
		}
		else
		{
			lastPos = (*dataArrayOut)[i].first;
		}
	}
	// calling clearedEvent is not needed
	// because all of the values can not be cleared here
	getUpdatingFromPoint(-1);
	if (callDataChanged == true)
	{
		dataChanged();
	}
}

int VectorGraphDataArray::getLocation(float searchX)
{
	bool found = false;
	bool isBefore = false;
	int location = getNearestLocation(searchX, &found, &isBefore);
	if (found == false)
	{
		return -1;
	}
	return location;
}

int VectorGraphDataArray::getNearestLocation(float searchXIn, bool* foundOut, bool* isBeforeOut)
{
	// modified binary search
	if (m_dataArray.size() > 0)
	{
		int start = 0;
		int end = m_dataArray.size() - 1;
		int mid = 0;
		// binary search
		while (start < end)
		{
			mid = start + (end - start) / 2;
			if (m_dataArray[mid].m_x == searchXIn)
			{
				*foundOut = true;
				*isBeforeOut = false;
				return mid;
			}
			else if (m_dataArray[mid].m_x < searchXIn)
			{
				start = mid + 1;
			}
			else
			{
				end = mid - 1;
			}
		}
		int outputDif = 0;
		mid = start + (end - start) / 2;
		if (m_dataArray[mid].m_x > searchXIn && mid > 0)
		{
			mid = mid - 1;
		}
		if (mid + 1 < m_dataArray.size() &&
			std::abs(m_dataArray[mid].m_x - searchXIn) >
			std::abs(m_dataArray[mid + 1].m_x - searchXIn))
		{
			outputDif = 1;
		}
		*foundOut = searchXIn == m_dataArray[mid + outputDif].m_x;
		*isBeforeOut = searchXIn >= m_dataArray[mid + outputDif].m_x;
		return mid + outputDif;
	}
	*foundOut = false;
	*isBeforeOut = false;
	return -1;
}

void VectorGraphDataArray::getSamples(unsigned int targetSizeIn, std::vector<float>* sampleBufferOut)
{
#ifdef VECTORGRAPH_DEBUG_PAINT_EVENT
	qDebug("getSamples start: targetSizeIn: %d", targetSizeIn);
#endif

	// try limiting m_universalSampleBuffer's size
	// to save memory
	if (m_universalSampleBuffer.size() > 10000)
	{
		m_universalSampleBuffer.resize(10000);
	}

	if (sampleBufferOut != nullptr)
	{
		if (sampleBufferOut->size() != targetSizeIn)
		{
			sampleBufferOut->resize(targetSizeIn);
		}
		for (unsigned int i = 0; i < targetSizeIn; i++)
		{
			(*sampleBufferOut)[i] = 0;
		}

		m_parent->lockGetSamplesAccess();
		getSamplesInner(targetSizeIn, nullptr, nullptr, sampleBufferOut);
		m_parent->unlockGetSamplesAccess();
	}
	else
	{
		// this needs to be allocated for getSamplesInner to work
		// because every effector VectorGraphDataArray uses this to return its m_bakedValues directly
		std::vector<float> updatingSampleArray(targetSizeIn);

		m_parent->lockGetSamplesAccess();
		getSamplesInner(targetSizeIn, nullptr, nullptr, &updatingSampleArray);
		m_parent->unlockGetSamplesAccess();
	}
#ifdef VECTORGRAPH_DEBUG_PAINT_EVENT
	qDebug("getSamples end");
#endif
}
void VectorGraphDataArray::getLastSamples(std::vector<float>* sampleBufferOut)
{
	m_parent->lockBakedSamplesAccess();
	*sampleBufferOut = m_bakedSamples;
	m_parent->unlockBakedSamplesAccess();
}
std::vector<int> VectorGraphDataArray::getEffectorArrayLocations()
{
	// getting the effector array chain
	std::vector<int> output;
	int currentLocation = m_effectorLocation;
	for (unsigned int i = 0; i < m_parent->getDataArraySize(); i++)
	{
		if (currentLocation == -1)
		{
			break;
		}
		else
		{
			output.push_back(m_effectorLocation);
			currentLocation = m_parent->getDataArray(currentLocation)->getEffectorArrayLocation();
		}
	}
	return output;
}

void VectorGraphDataArray::setDataArray(std::vector<PointF>* inputDataArray,
	bool shouldCurve, bool shouldClear, bool shouldClamp, bool shouldRescale, bool shouldSort, bool callDataChanged)
{
#ifdef VECTORGRAPH_DEBUG_PAINT_EVENT
	qDebug("setDataArray start");
#endif
	if (shouldClear == true)
	{
		m_dataArray.clear();
	}
	m_dataArray.resize(inputDataArray->size());
	if (m_dataArray.size() == 0)
	{
		clearedEvent();
	}
	if (shouldClamp == true || shouldRescale == true || shouldSort == true)
	{
		formatArray(inputDataArray, shouldClamp, shouldRescale, shouldSort, false);
	}
	bool noneBefore = true;
	bool isNegativeBefore = false;
	for (unsigned int i = 0; i < m_dataArray.size(); i++)
	{
		m_dataArray[i].m_x = (*inputDataArray)[i].first;
		m_dataArray[i].m_y = (*inputDataArray)[i].second;
		// calculating curves
		if (shouldCurve == true && i > 0)
		{
			// TODO take in to account x coords
			float diff = m_dataArray[i - 1].m_y - m_dataArray[i].m_y;
			// if before is bigger than after
			bool isNegative = diff >= 0;
			float direction = 0;
			if (noneBefore == true)
			{
				direction = isNegative == true ? 1.0f : -1.0f;
			}
			else
			{
				direction = isNegative == isNegativeBefore ? 1.0f : isNegative == true ? -1.0f : 1.0f;
			}

			diff = diff * diff * 10.0f * direction;
			diff = std::clamp(diff, -0.7f, 0.7f);
			m_dataArray[i - 1].m_c = diff;

			noneBefore = diff < 0.1f && diff > -0.1f;
			isNegativeBefore = isNegative;
		}
	}
	// the whole m_dataArray needs to be updated
	getUpdatingFromPoint(-1);
	if (callDataChanged == true)
	{
		dataChanged();
	}
}
void VectorGraphDataArray::setDataArray(std::vector<float>* inputDataArray,
	bool shouldCurve, bool shouldClear, bool shouldClamp, bool shouldRescale, bool callDataChanged)
{
	std::vector<PointF> convertedDataArray(inputDataArray->size());
	float stepSize = 1.0f / static_cast<float>(convertedDataArray.size());
	for (unsigned int i = 0; i < inputDataArray->size(); i++)
	{
		convertedDataArray[i].first = i * stepSize;
		convertedDataArray[i].second = (*inputDataArray)[i];
	}
	setDataArray(&convertedDataArray, shouldCurve, shouldClear, shouldClamp, shouldRescale, false, callDataChanged);
}
void VectorGraphDataArray::setDataArray(float* inputDataArray, unsigned int size,
	bool shouldCurve, bool shouldClear, bool shouldClamp, bool shouldRescale, bool callDataChanged)
{
	std::vector<PointF> convertedDataArray(size);
	float stepSize = 1.0f / static_cast<float>(size);
	for (unsigned int i = 0; i < size; i++)
	{
		convertedDataArray[i].first = i * stepSize;
		convertedDataArray[i].second = inputDataArray[i];
	}
	setDataArray(&convertedDataArray, shouldCurve, shouldClear, shouldClamp, shouldRescale, false, callDataChanged);
}

unsigned int VectorGraphDataArray::setX(unsigned int pointLocation, float newX)
{
	int location = pointLocation;
	if (m_isFixedX == false && newX <= 1.0f)
	{
		bool found = false;
		bool isBefore = false;
		location = getNearestLocation(newX, &found, &isBefore);
		// if an other point was not found exactly at newX
		// and if dataArray end points are changeable
		if (found == false && ((m_isFixedEndPoints == true &&
			pointLocation < m_dataArray.size() - 1 && pointLocation > 0) ||
			m_isFixedEndPoints == false))
		{
			int targetLocation = location;
			// bool dataChangedVal = false;
			// if getNearestLocation returned a value
			if (location >= 0)
			{
				if (location < pointLocation && isBefore == true)
				{
					if (targetLocation + 1 < m_dataArray.size())
					{
						targetLocation++;
					}
				}
				else if (location > pointLocation && isBefore == false)
				{
					if (targetLocation > 0)
					{
						targetLocation--;
					}
				}
				m_dataArray[pointLocation].m_x = newX;
				swap(pointLocation, targetLocation, true);
				location = targetLocation;

				getUpdatingFromPoint(-1);
				// changes in the position can change lines before
				// so the point before this is updated
				if (location > 0)
				{
					getUpdatingFromPoint(location - 1);
				}
				dataChanged();
			}
			else
			{
				location = pointLocation;
			}
		}
		else
		{
			location = pointLocation;
		}
	}
	return location;
}

void VectorGraphDataArray::setY(unsigned int pointLocation, float newY)
{
	if (m_isFixedY == true) { return; }

	m_dataArray[pointLocation].m_y = newY;
	getUpdatingFromPoint(pointLocation);
	// changes in the position can change lines before
	// so the point before this is updated
	if (pointLocation > 0)
	{
		getUpdatingFromPoint(pointLocation - 1);
	}
	if (m_isFixedEndPoints == true &&
		(pointLocation <= 0 || pointLocation >= m_dataArray.size() - 1))
	{
		formatDataArrayEndPoints();
		getUpdatingFromPoint(0);
		getUpdatingFromPoint(m_dataArray.size() - 1);
	}
	dataChanged();
}

void VectorGraphDataArray::setC(unsigned int pointLocation, float newC)
{
	if (m_isEditableAttrib == false) { return; }

	m_dataArray[pointLocation].m_c = newC;
	getUpdatingFromPoint(pointLocation);
	dataChanged();
}
void VectorGraphDataArray::setValA(unsigned int pointLocation, float fValue)
{
	if (m_isEditableAttrib == false) { return; }

	m_dataArray[pointLocation].m_valA = fValue;
	getUpdatingFromPoint(pointLocation);
	dataChanged();
}
void VectorGraphDataArray::setValB(unsigned int pointLocation, float fValue)
{
	if (m_isEditableAttrib == false) { return; }

	m_dataArray[pointLocation].m_valB = fValue;
	getUpdatingFromPoint(pointLocation);
	dataChanged();
}
void VectorGraphDataArray::setType(unsigned int pointLocation, unsigned int newType)
{
	if (m_isEditableAttrib == false) { return; }

	// set the type without changing the automated attribute location
	m_dataArray[pointLocation].m_type = newType;
	getUpdatingFromPoint(pointLocation);
	dataChanged();
}
void VectorGraphDataArray::setAutomatedAttrib(unsigned int pointLocation, unsigned int attribLocation)
{
	if (m_isAutomatable == false || m_isEditableAttrib == false) { return; }

	// clamp only 4 attributes can be automated (y, c, valA, valB)
	attribLocation = attribLocation > 3 ? 0 : attribLocation;
	// set automated location correctly (effected_location = automatedEffectedLocation % 4)
	m_dataArray[pointLocation].m_automatedEffectedAttribLocations = attribLocation * 4 + getEffectedAttribLocation(pointLocation);

	getUpdatingFromPoint(pointLocation);
	// the line before this can get added later
	// in getUpdatingFromAutomation
	// so the point before this is not updated here

	dataChanged();
}
void VectorGraphDataArray::setEffectedAttrib(unsigned int pointLocation, unsigned int attribLocation)
{
	if (m_isEffectable == false || m_isEditableAttrib == false) { return; }

	// clamp only 4 attributes can be automated (y, c, valA, valB)
	attribLocation = attribLocation > 3 ? 0 : attribLocation;
	// set effected location correctly
	m_dataArray[pointLocation].m_automatedEffectedAttribLocations = attribLocation + getAutomatedAttribLocation(pointLocation) * 4;

	getUpdatingFromPoint(pointLocation);
	// if the current point can effect the line before it
	// update the point before it
	if (getEffectPoints(pointLocation) == false && pointLocation > 0)
	{
		getUpdatingFromPoint(pointLocation - 1);
	}
	dataChanged();
}
unsigned int VectorGraphDataArray::getAutomatedAttribLocation(unsigned int pointLocation)
{
	return m_dataArray[pointLocation].m_automatedEffectedAttribLocations / 4;
}
unsigned int VectorGraphDataArray::getEffectedAttribLocation(unsigned int pointLocation)
{
	return m_dataArray[pointLocation].m_automatedEffectedAttribLocations % 4;
}
bool VectorGraphDataArray::getEffectPoints(unsigned int pointLocation)
{
	// be careful with changing this
	return (m_dataArray[pointLocation].m_effectPoints == true || getEffectedAttribLocation(pointLocation) > 0);
}
bool VectorGraphDataArray::getEffectLines(unsigned int pointLocation)
{
	// be careful with changing this
	// m_effectLines ought to be false if lines (each sample) can not be changed
	return (m_dataArray[pointLocation].m_effectLines == true && getEffectedAttribLocation(pointLocation) == 0);
}
void VectorGraphDataArray::setEffectPoints(unsigned int pointLocation, bool bValue)
{
	if (m_isEffectable == false || m_isEditableAttrib == false) { return; }

	if (m_dataArray[pointLocation].m_effectPoints != bValue)
	{
		// getEffectPoints does not return m_effectePoints
		bool dataChangedValue = getEffectPoints(pointLocation);
		m_dataArray[pointLocation].m_effectPoints = bValue;
		// this change does effect the main output if this
		// data array is an effector of an other so dataChanged() 
		// and getUpdatingFromPoint is called
		if (dataChangedValue != getEffectPoints(pointLocation))
		{
			getUpdatingFromPoint(pointLocation);
			// if the current point can effect the line before it
			// update the point before it
			if (getEffectedAttribLocation(pointLocation) <= 0 && pointLocation > 0)
			{
				getUpdatingFromPoint(pointLocation - 1);
			}
		}
		dataChanged();
	}
}
void VectorGraphDataArray::setEffectLines(unsigned int pointLocation, bool bValue)
{
	if (m_isEffectable == false || m_isEditableAttrib == false) { return; }

	if (m_dataArray[pointLocation].m_effectLines != bValue)
	{
		// getEffectLines does not return m_effectLines
		bool dataChangedValue = getEffectLines(pointLocation);
		m_dataArray[pointLocation].m_effectLines = bValue;
		// this change does effect the main output if this
		// data array is an effector of an other so dataChanged() 
		// and getUpdatingFromPoint is called
		if (dataChangedValue != getEffectLines(pointLocation))
		{
			getUpdatingFromPoint(pointLocation);
		}
		dataChanged();
	}
}
unsigned int VectorGraphDataArray::getEffect(unsigned int pointLocation, unsigned int effectSlot)
{
	switch (effectSlot)
	{
		case 0:
			return m_dataArray[pointLocation].m_effectTypeA;
		case 1:
			return m_dataArray[pointLocation].m_effectTypeB;
		case 2:
			return m_dataArray[pointLocation].m_effectTypeC;
	}
	return 0;
}
void VectorGraphDataArray::setEffect(unsigned int pointLocation, unsigned int effectSlot, unsigned int effectType)
{
	if (m_isEffectable == false || m_isEditableAttrib == false) { return; }

	switch (effectSlot)
	{
		case 0:
			m_dataArray[pointLocation].m_effectTypeA = effectType;
			break;
		case 1:
			m_dataArray[pointLocation].m_effectTypeB = effectType;
			break;
		case 2:
			m_dataArray[pointLocation].m_effectTypeC = effectType;
			break;
	}
	getUpdatingFromPoint(pointLocation);
	// if the current point can effect the line before it
	// update the point before it
	if (getEffectPoints(pointLocation) == true && pointLocation > 0)
	{
		getUpdatingFromPoint(pointLocation - 1);
	}
	dataChanged();
}
bool VectorGraphDataArray::getIsAutomationValueChanged(unsigned int pointLocation)
{
	if (getAutomationModel(pointLocation) != nullptr &&
		m_dataArray[pointLocation].m_bufferedAutomationValue != getAutomationModel(pointLocation)->value())
	{
		m_dataArray[pointLocation].m_bufferedAutomationValue = getAutomationModel(pointLocation)->value();
		return true;
	}
	return false;
}
void VectorGraphDataArray::setAutomated(unsigned int pointLocation, bool bValue)
{
#ifdef VECTORGRAPH_DEBUG_USER_INTERACTION
	qDebug("setAutomated start");
#endif
	if (m_isAutomatable == false) { return; }

	if (bValue == true)
	{
		// if it is not already automated
		if (m_dataArray[pointLocation].m_automationModel == -1)
		{
#ifdef VECTORGRAPH_DEBUG_USER_INTERACTION
			qDebug("setAutomated: make new floatModel, location: %d", pointLocation);
#endif
			m_automationModelArray.push_back(new FloatModel(0.0f, -1.0f, 1.0f, 0.01f, m_parent, QString(), false));
			m_dataArray[pointLocation].m_automationModel = m_automationModelArray.size() - 1;
			getUpdatingFromPoint(pointLocation);
			dataChanged();
		}
	}
	else
	{
#ifdef VECTORGRAPH_DEBUG_USER_INTERACTION
		qDebug("setAutomated: delete floatModel, location: %d, size: %d", pointLocation, static_cast<int>(m_automationModelArray.size()));
#endif
		// dataChanged() is called in this function
		// this function check if the current point has an automationModel
		deleteAutomationModel(m_dataArray[pointLocation].m_automationModel, true);
	}
}
FloatModel* VectorGraphDataArray::getAutomationModel(unsigned int pointLocation)
{
	if (m_dataArray[pointLocation].m_automationModel != -1)
	{
		return m_automationModelArray[m_dataArray[pointLocation].m_automationModel];
	}
	return nullptr;
}

// private:
std::vector<FloatModel*>* VectorGraphDataArray::getAutomationModelArray()
{
	return &m_automationModelArray;
}
void VectorGraphDataArray::deleteUnusedAutomation()
{
	bool dataChangedVal = false;
	std::vector<int> usedAutomation;
	for (auto i : m_dataArray)
	{
		if (i.m_automationModel != -1)
		{
			usedAutomation.push_back(i.m_automationModel);
		}
	}
	for	(unsigned int i = 0; i < m_automationModelArray.size(); i++)
	{
		bool found = false;
		for	(unsigned int j = 0; j < usedAutomation.size(); j++)
		{
			if (i == usedAutomation[j])
			{
				found = true;
				break;
			}
		}
		if (found == false)
		{
			dataChangedVal = true;
			deleteAutomationModel(i, false);
		}
	}

	// getUpdatingFromPoint() is called in deleteAutomationModel()
	if (dataChangedVal == true)
	{
		dataChanged();
	}
}
QString VectorGraphDataArray::getSavedDataArray()
{
		QString output;
		base64::encode((const char *)(m_dataArray.data()),
			m_dataArray.size() * sizeof(VectorGraphPoint), output);
		return output;
}
void VectorGraphDataArray::loadDataArray(QString data, unsigned int arraySize, bool callDataChanged)
{
#ifdef VECTORGRAPH_DEBUG_PAINT_EVENT
	qDebug("loadDatatArray start: arraySize: %d", arraySize);
#endif
	int size = 0;
	char* dst = 0;
	base64::decode(data, &dst, &size);

	if (size == arraySize * sizeof(VectorGraphPoint))
	{
		m_dataArray.resize(arraySize);

		VectorGraphPoint* points = (VectorGraphPoint*)dst;
		for (unsigned int i = 0; i < arraySize; i++)
		{
			m_dataArray[i] = points[i];
		}
	}

	delete[] dst;
	getUpdatingFromPoint(-1);
	if (callDataChanged == true)
	{
		dataChanged();
	}
}

void VectorGraphDataArray::deleteAutomationModel(int modelLocation, bool callDataChanged)
{
	if (modelLocation < 0 || modelLocation >= m_automationModelArray.size()) { return; }

	FloatModel* curModel = m_automationModelArray[modelLocation];

	// copy the last FloatModel* to the current location
	m_automationModelArray[modelLocation] =
		m_automationModelArray[m_automationModelArray.size() - 1];

	m_automationModelArray.pop_back();

	// replace all m_auttomationModel-s in the current copyed location with -1
	// replace all last m_automationModel-s to the currently copyed location
	// there should be only 2 points changed but because of safety
	// all of them are checked
	for (unsigned int i = 0; i < m_dataArray.size(); i++)
	{
		if (m_dataArray[i].m_automationModel == modelLocation)
		{
			m_dataArray[i].m_automationModel = -1;
			getUpdatingFromPoint(i);
			if (i > 0)
			{
				getUpdatingFromPoint(i - 1);
			}
		}
		if (m_dataArray[i].m_automationModel == m_automationModelArray.size())
		{
			m_dataArray[i].m_automationModel = modelLocation;
		}
	}
	if (curModel != nullptr)
	{
		delete curModel;
		curModel = nullptr;
	}

	if (callDataChanged == true)
	{
		dataChanged();
	}
}

void VectorGraphDataArray::swap(unsigned int pointLocationA, unsigned int pointLocationB, bool shouldShiftBetween)
{
	if (pointLocationA == pointLocationB) { return; }

	if (shouldShiftBetween == true)
	{
#ifdef VECTORGRAPH_DEBUG_USER_INTERACTION
		qDebug("swap:    -------");
		qDebug("first point location: %d, second point locaiton: %d", pointLocationA, pointLocationB);
		for (unsigned int i = 0; i < m_dataArray.size(); i++)
		{
			qDebug("   - i: %d  -  x: %f", i, m_dataArray[i].m_x);
		}
#endif
		
		if (pointLocationA < pointLocationB)
		{
			VectorGraphPoint swap = m_dataArray[pointLocationA];
			for (unsigned int i = pointLocationA; i < pointLocationB; i++)
			{
				m_dataArray[i] = m_dataArray[i + 1];
			}
			m_dataArray[pointLocationB] = swap;
		}
		else
		{
			VectorGraphPoint swap = m_dataArray[pointLocationA];
			for (unsigned int i = pointLocationA; i > pointLocationB; i--)
			{
				m_dataArray[i] = m_dataArray[i - 1];
			}
			m_dataArray[pointLocationB] = swap;
		}
		
#ifdef VECTORGRAPH_DEBUG_USER_INTERACTION
		qDebug(" --------- ");
		for (unsigned int i = 0; i < m_dataArray.size(); i++)
		{
			qDebug("   - i: %d  -  x: %f", i, m_dataArray[i].m_x);
		}
#endif
	}
	else
	{
		// normal swap
		VectorGraphPoint swap = m_dataArray[pointLocationA];
		m_dataArray[pointLocationA] = m_dataArray[pointLocationB];
		m_dataArray[pointLocationB] = swap;
	}
	getUpdatingFromPoint(pointLocationB - 1 > 0 ? pointLocationB - 1 : 0);
	getUpdatingFromPoint(pointLocationA - 1 > 0 ? pointLocationA - 1 : 0);
	getUpdatingFromPoint(pointLocationA);
	getUpdatingFromPoint(pointLocationB);
	dataChanged();
}

float VectorGraphDataArray::processEffect(unsigned int pointLocation, float attribValue,
	unsigned int attribLocation, float effectValue)
{
	// calculating an effect on attribValue
	float output = attribValue;
	// effects
	if (getEffectedAttribLocation(pointLocation) == attribLocation)
	{
		output = processSingleEffect(pointLocation, 0, output, effectValue);
		output = processSingleEffect(pointLocation, 1, output, effectValue);
		output = processSingleEffect(pointLocation, 2, output, effectValue);

		// clamp
		output = std::clamp(output, -1.0f, 1.0f);
	}
	return output;
}

float VectorGraphDataArray::processSingleEffect(unsigned int pointLocation, unsigned int effectSlot,
	float attribValue, float effectValue)
{
	// calculating an effect on attribValue
	float output = attribValue;

	// none
	if (getEffect(pointLocation, effectSlot) == 0) { return output; }

	// effects
	if (getEffect(pointLocation, effectSlot) == 1)
	{
		// add
		output += effectValue;
	}
	else if (getEffect(pointLocation, effectSlot) == 2)
	{
		// subtract
		output -= effectValue;
	}
	else if (getEffect(pointLocation, effectSlot) == 3)
	{
		// multiply
		output = output * 5.0f * effectValue;
	}
	else if (getEffect(pointLocation, effectSlot) == 4 && effectValue != 0.0f)
	{
		// divide
		output = output / 5.0f / effectValue;
	}
	else if (getEffect(pointLocation, effectSlot) == 5 && output > 0.0f)
	{
		// power
		output = std::pow(output, effectValue * 5.0f);
		output = std::clamp(output, -1.0f, 1.0f);
	}
	else if (getEffect(pointLocation, effectSlot) == 6 && output > 0.0f && effectValue > 0.0f)
	{
		// log
		output = std::log(output) / std::log(effectValue);
		output = std::clamp(output, -1.0f, 1.0f);
	}
	else if (getEffect(pointLocation, effectSlot) == 7)
	{
		// sine
		output = output + std::sin(effectValue * 100.0f);
	}
	else if (getEffect(pointLocation, effectSlot) == 8)
	{
		// clamp lower
		output = std::max(effectValue, output);
	}
	else if (getEffect(pointLocation, effectSlot) == 9)
	{
		// clamp upper
		output = std::min(effectValue, output);
	}
	return output;
}

float VectorGraphDataArray::processAutomation(unsigned int pointLocation, float attribValue, unsigned int attribLocation)
{
	// adding the automation value to attribValue
	float output = 0.0f;
	// if automated
	FloatModel* automationModel = getAutomationModel(pointLocation);
	if (automationModel != nullptr)
	{
		if (getAutomatedAttribLocation(pointLocation) == attribLocation)
		{
			output += automationModel->value();
		}
	}
	output += attribValue;
	
	output = std::clamp(output, -1.0f, 1.0f);
	return output;
}

void VectorGraphDataArray::processBezier(std::vector<float>* samplesOut, std::vector<float>* xArray, unsigned int startLoc, unsigned int endLoc,
	float yBefore, float yAfter, float targetYBefore, float targetYAfter, float curveStrength)
{
	// note: targetYBefore and targetYAfter can be bigger than 1
	curveStrength = std::abs(curveStrength);
	// draw bezier
	if (curveStrength != 0.0f)
	{
		for (unsigned int i = startLoc; i < endLoc; i++)
		{
			// bezier B(t), "t" is a number between 0 and 1
			float t = (*xArray)[i];
			// inverse t
			float iT = (1.0f - t);
			(*samplesOut)[i] = (std::pow(iT, 3.0f) * yBefore) +
				(3.0f * iT * iT * t * targetYBefore) +
				(3.0f * iT * t * t * targetYAfter) +
				(std::pow(t, 3.0f) * yAfter);
			(*samplesOut)[i] = std::clamp((*samplesOut)[i] * curveStrength, -1.0f, 1.0f);
		}
	}
	else
	{
		// if no bezier is drawn, reset the line's contents to 0
		// (so a drawn line can be added later)
		for (unsigned int i = startLoc; i < endLoc; i++)
		{
			(*samplesOut)[i] = 0.0f;
		}
	}

	// draw line
	if (curveStrength < 1.0f)
	{
		for (unsigned int i = startLoc; i < endLoc; i++)
		{
			(*samplesOut)[i] += (yBefore + (yAfter - yBefore) * (*xArray)[i]) * (1.0f - curveStrength);
		}
	}
}

void VectorGraphDataArray::processLineTypeArraySine(std::vector<float>* samplesOut, std::vector<float>* xArray, unsigned int startLoc, unsigned int endLoc,
	float sineAmp, float sineFreq, float fadeInStartVal)
{
	processLineTypeArraySineB(samplesOut, xArray, startLoc, endLoc,
		sineAmp, sineFreq, 0.0f, fadeInStartVal);
}
void VectorGraphDataArray::processLineTypeArraySineB(std::vector<float>* samplesOut, std::vector<float>* xArray, unsigned int startLoc, unsigned int endLoc,
	float sineAmp, float sineFreq, float sinePhase, float fadeInStartVal)
{
	float startLocVal = (*samplesOut)[startLoc];
	float endLocVal = (*samplesOut)[endLoc > 0 ? endLoc - 1 : 0];
	int count = static_cast<int>(endLoc) - static_cast<int>(startLoc);
	if (count < 0)
	{
		count = 0;
	}
	float tValB = 0.001f + ((sineFreq + 1.0f) / 2.0f) * 0.999f;
	// calculating how many samples are needed to 1 complete wave
	// we have "count" amount of samples and "tValB * 100.0f" amount of waves
	int end = static_cast<int>(std::floor(count / (tValB * 100.0f)));
	if (count <= 0)
	{
		end = 0;
	}
	else
	{
		end = end > count ? count : end + 1;
	}
	// "allocate" "end" amount of floats
	// for 1 whole sine wave
	// in the universal buffer
	if (m_universalSampleBuffer.size() < end)
	{
		m_universalSampleBuffer.resize(end);
	}

	// calculate 1 wave of sine
	for (unsigned int i = 0; i < end; i++)
	{
		// 628.318531f = 100.0f * 2.0f * pi
		// (1 sine wave is 2pi long and we have 1 * 100 * sineFreq waves)
		m_universalSampleBuffer[i] = sineAmp * std::sin(
			(*xArray)[startLoc + i] * 628.318531f * tValB + sinePhase * 100.0f);
	}
	// copy the first wave until the end
	for (int i = 0; i < count; i += end)
	{
		int endB = i + end >= count ? count - i : end;
		for (unsigned int j = 0; j < endB; j++)
		{
			(*samplesOut)[startLoc + j + i] += m_universalSampleBuffer[j];
		}
	}

	// fade in
	unsigned int fadeInEndLoc = static_cast<unsigned int>(fadeInStartVal * static_cast<float>(count));
	for (unsigned int i = startLoc; i < startLoc + fadeInEndLoc; i++)
	{
		float x = (*xArray)[i] / fadeInStartVal;
		(*samplesOut)[i] = (*samplesOut)[i] * x + startLocVal * (1.0f - x);
	}
	// fade out
	for (unsigned int i = endLoc - 1; i > endLoc - fadeInEndLoc; i--)
	{
		float x = (1.0f - (*xArray)[i]) / fadeInStartVal;
		(*samplesOut)[i] = (*samplesOut)[i] * x + endLocVal * (1.0f - x);
	}
}
void VectorGraphDataArray::processLineTypeArrayPeak(std::vector<float>* samplesOut, std::vector<float>* xArray, unsigned int startLoc, unsigned int endLoc,
	float peakAmp, float peakX, float peakWidth, float fadeInStartVal)
{
	float startLocVal = (*samplesOut)[startLoc];
	float endLocVal = (*samplesOut)[endLoc > 0 ? endLoc - 1 : 0];
	int count = static_cast<int>(endLoc) - static_cast<int>(startLoc);
	if (count < 0)
	{
		count = 0;
	}
	for (unsigned int i = 0; i < count; i++)
	{
		(*samplesOut)[startLoc + i] += std::pow((peakWidth + 1.0f) * 0.2f + 0.01f,
			std::abs((*xArray)[startLoc + i] - (peakX + 1.0f) * 0.5f) * 10.0f) * peakAmp;
	}

	// fade in
	unsigned int fadeInEndLoc = static_cast<unsigned int>(fadeInStartVal * static_cast<float>(count));
	for (unsigned int i = startLoc; i < startLoc + fadeInEndLoc; i++)
	{
		float x = (*xArray)[i] / fadeInStartVal;
		(*samplesOut)[i] = (*samplesOut)[i] * x + startLocVal * (1.0f - x);
	}
	// fade out
	for (unsigned int i = endLoc - 1; i > endLoc - fadeInEndLoc; i--)
	{
		float x = (1.0f - (*xArray)[i]) / fadeInStartVal;
		(*samplesOut)[i] = (*samplesOut)[i] * x + endLocVal * (1.0f - x);
	}
}
void VectorGraphDataArray::processLineTypeArraySteps(std::vector<float>* samplesOut, std::vector<float>* xArray, unsigned int startLoc, unsigned int endLoc,
	std::vector<float>* yArray, float stepCount, float stepCurve, float fadeInStartVal)
{
	float startLocVal = (*samplesOut)[startLoc];
	float endLocVal = (*samplesOut)[endLoc > 0 ? endLoc - 1 : 0];
	int count = static_cast<int>(endLoc) - static_cast<int>(startLoc);
	if (count < 0)
	{
		count = 0;
	}

	float stepCountB = (1.0f + stepCount) / 2.0f * 19.0f + 1.0f;
	for (unsigned int i = 0; i < count; i++)
	{
		float y = (*yArray)[startLoc + i] + 1.0f;
		float diff = std::round(y * stepCountB) - y * stepCountB;
		float smooth = 1.0f - std::abs(diff) * (1.0f - (stepCurve + 1.0f) / 2.0f) * 2.0f;
		(*samplesOut)[startLoc + i] += diff / stepCountB * smooth;
	}

	// fade in
	unsigned int fadeInEndLoc = static_cast<unsigned int>(fadeInStartVal * static_cast<float>(count));
	for (unsigned int i = startLoc; i < startLoc + fadeInEndLoc; i++)
	{
		float x = (*xArray)[i] / fadeInStartVal;
		(*samplesOut)[i] = (*samplesOut)[i] * x + startLocVal * (1.0f - x);
	}
	// fade out
	for (unsigned int i = endLoc - 1; i > endLoc - fadeInEndLoc; i--)
	{
		float x = (1.0f - (*xArray)[i]) / fadeInStartVal;
		(*samplesOut)[i] = (*samplesOut)[i] * x + endLocVal * (1.0f - x);
	}
}
void VectorGraphDataArray::processLineTypeArrayRandom(std::vector<float>* samplesOut, std::vector<float>* xArray, unsigned int startLoc, unsigned int endLoc,
	float randomAmp, float randomCount, float randomSeed, float fadeInStartVal)
{
	int count = static_cast<int>(endLoc) - static_cast<int>(startLoc);
	if (count < 0)
	{
		count = 0;
	}

	unsigned int randomValuesSize = static_cast<unsigned int>(50.0f * (randomCount + 1.0f)) * 2;

	if (randomValuesSize <= 0) { return; }

	// "allocate" "randomValuesSize" amount of floats
	// for generating random values
	// in the universal buffer
	if (m_universalSampleBuffer.size() < randomValuesSize)
	{
		m_universalSampleBuffer.resize(randomValuesSize);
	}

	float blend = 10.0f + randomSeed * 10.0f;
	int randomSeedB = static_cast<int>(blend);
	blend = blend - randomSeedB;

	std::srand(randomSeedB);

	// getting the random values
	// generating 2 seeds and blending in between them
	for (unsigned int i = 0; i < randomValuesSize / 2; i++)
	{
		m_universalSampleBuffer[i] = std::fmod((static_cast<float>(rand()) / 10000.0f), 2.0f) - 1.0f;
	}
	std::srand(randomSeedB + 1);
	for (unsigned int i = randomValuesSize / 2; i < randomValuesSize; i++)
	{
		m_universalSampleBuffer[i] = std::fmod((static_cast<float>(rand()) / 10000.0f), 2.0f) - 1.0f;
	}

	// blending
	// real size
	float size = static_cast<float>(randomValuesSize / 2);
	for (unsigned int i = 0; i < count; i++)
	{
		float randomValueX = (*xArray)[startLoc + i] * size;
		float randomValueLocation = std::floor(randomValueX);
		(*samplesOut)[startLoc + i] += -((randomValueX - randomValueLocation) - 1.0f) * (randomValueX - randomValueLocation) * 4.0f *
			(m_universalSampleBuffer[static_cast<int>(randomValueLocation)] * (1.0f - blend)  + m_universalSampleBuffer[static_cast<int>(randomValueLocation + size)] * blend) * randomAmp;
	}
}

void VectorGraphDataArray::getUpdatingFromEffector(std::vector<unsigned int>* effectorUpdatingPoints)
{
	/*
	 * here we decide m_needsUpdating points from an effector array's updated points (effectorUpdatingPoints is the location of these points)
	 * firstly we get changed points from the effector graph (effectorUpdatingPoints)
	 * we get a segment consisting of changed effector points that come after each other
	 * this will be useful because we can update this graph's points between this segment (segment start = i, segment end = updatingEnd)
	 * secondly we find a (this graph's) point before the segment start and after the segment end
	 * so we get locationBefore and locationAfter, everything between them will be added to m_needsUpdating
	 * thirdly we finalyze locationBefore and locationAfter, clamp them and start adding the points between them to m_needsUpdating
	 * if the (this graph's) point is not effected, we avoid adding it to m_needsUpdating
	*/
	VectorGraphDataArray* effector = m_parent->getDataArray(m_effectorLocation);
	for (unsigned int i = 0; i < effectorUpdatingPoints->size(); i++)
	{
		// since effectorUpdatingPoints is a sorted list, we can get the end
		// location and update everithing between them
		// starting effector location is i, end effector location is updatingEnd
		unsigned int updatingEnd = i;
		for (unsigned int j = i + 1; j < effectorUpdatingPoints->size(); j++)
		{
			// we can not skip gaps because
			// every effectorUpdatingPoints point effects their line only
			// (the line that starts with the point)
			if ((*effectorUpdatingPoints)[updatingEnd] + 1 >=
					(*effectorUpdatingPoints)[j])
			{
				updatingEnd = j;
#ifdef VECTORGRAPH_DEBUG_PAINT_EVENT
				qDebug("getUpdatingFromEffector: new updatingEnd: %d, start (i): %d", updatingEnd, i);
#endif
			}
			else
			{
#ifdef VECTORGRAPH_DEBUG_PAINT_EVENT
				qDebug("getUpdatingFromEffector: updatingEnd: %d brake: %d < %d [j = %d]", updatingEnd,
					((*effectorUpdatingPoints)[updatingEnd] + 1), (*effectorUpdatingPoints)[j], j);
#endif
				break;
			}
		}
		// getting the point that comes after updatingEnd
		// this is done because updatingEnd was changed so the line directly after updatingEnd point was changed
		// so every (current graph's) point before updatingEnd + 1 needs to be changed
		int updatingEndSlide = 0;
		if (updatingEnd + 1 < effector->size())
		{
#ifdef VECTORGRAPH_DEBUG_PAINT_EVENT
			qDebug("getUpdatingFromEffector: updatingEndSlide = 1");
#endif
			updatingEndSlide = 1;
		}

		// translating the effector data array locations to m_dataArray locations
		bool found = false;
		bool isBefore = false;
		// this can return -1
		int locationBefore = getNearestLocation(effector->getX((*effectorUpdatingPoints)[i]), &found, &isBefore);
#ifdef VECTORGRAPH_DEBUG_PAINT_EVENT
		qDebug("getUpdatingFromEffector: getNearestLocation before: %d, i: %d", locationBefore, i);
#endif
		if (isBefore == true && locationBefore > 0 && getEffectPoints(locationBefore) == true)
		{
#ifdef VECTORGRAPH_DEBUG_PAINT_EVENT
			qDebug("getUpdatingFromEffector: locationBefore = %d - 1", locationBefore);
#endif
			// the line (or point) before might be effected if the current nearest point
			// is effected in some way
			// so subtract 1
			// remember points control the line after (connected to) them
			// but in this case changes in the points position can effect the line before it
			locationBefore--;
			// now (here) locationBefore is Always before (*effectorUpdatingPoints)[i]
		}
		// clamp
		locationBefore = locationBefore < 0 ? 0 :
			m_dataArray.size() - 1 < locationBefore ? m_dataArray.size() - 1 : locationBefore;

		isBefore = false;
		int locationAfter = getNearestLocation(effector->getX((*effectorUpdatingPoints)[updatingEnd] + updatingEndSlide), &found, &isBefore);
#ifdef VECTORGRAPH_DEBUG_PAINT_EVENT
		qDebug("getUpdatingFromEffector: getNearestLocation after: %d, updatingEnd: %d (+ %d), effector x: %f, dataArray x: %f", locationAfter, updatingEnd, updatingEndSlide,
			effector->getX((*effectorUpdatingPoints)[updatingEnd] + updatingEndSlide), m_dataArray[locationAfter].m_x);
#endif
		if (isBefore == false)
		{
#ifdef VECTORGRAPH_DEBUG_PAINT_EVENT
			qDebug("getUpdatingFromEffector: locationAfter = %d - 1", locationAfter);
#endif
			// if the nearest point is after ([updatingEnd] + upadtingEndSlide) (where updating ends)
			locationAfter--;
		}
		// updating everything before if i -> 0
		if ((*effectorUpdatingPoints)[i] == 0)
		{
#ifdef VECTORGRAPH_DEBUG_PAINT_EVENT
			qDebug("getUpdatingFromEffector updating everything before");
#endif
			locationBefore = 0;
		}
		// if updatingEnd is the last point in effecor, then
		// update everithing after
		if ((*effectorUpdatingPoints)[updatingEnd] + updatingEndSlide + 1 >= effector->size())
		{
#ifdef VECTORGRAPH_DEBUG_PAINT_EVENT
			qDebug("getUpdatingFromEffector updating everything after");
#endif
			locationAfter = m_dataArray.size() - 1;
		}
		// clamp
		locationAfter = locationAfter < 0 ? 0 :
			m_dataArray.size() - 1 < locationAfter ? m_dataArray.size() - 1 : locationAfter;

#ifdef VECTORGRAPH_DEBUG_PAINT_EVENT
		qDebug("getUpdatingFromEffector: final start: %d, final end: %d, i: %d", locationBefore, locationAfter, i);
#endif

		// if the last point was updated (ture in case of j = 0)
		bool lastUpdated = true;
		// adding the values between locationBefore, locationAfter
		// (including locationBefore and locationAfter) to m_needsUpdating
		for (unsigned int j = locationBefore; j <= locationAfter; j++)
		{
			// update only if effected
			if (isEffectedPoint(j) == true && (getEffectPoints(j) == true || getEffectLines(j) == true))
			{
#ifdef VECTORGRAPH_DEBUG_PAINT_EVENT
				qDebug("getUpdatingFromEffector: i: %d, updating: %d", i, j);
#endif
				m_needsUpdating.push_back(j);
				if (lastUpdated == false && getEffectPoints(j) == true)
				{
					m_needsUpdating.push_back(j - 1);
				}
				lastUpdated = true;
			}
			else
			{
				lastUpdated = false;
			}
		}
		if (i < updatingEnd)
		{
			i = updatingEnd;
		}
	}
}
void VectorGraphDataArray::getUpdatingFromPoint(int pointLocation)
{
	// changes in position need to cause updates before the changed point
	// changes in m_dataArray.size() needs to cause getUpdatingFromPoint(-1)
	if (m_isDataChanged == false && pointLocation >= 0)
	{
		m_needsUpdating.push_back(pointLocation);
		if (m_needsUpdating.size() > m_dataArray.size() * 3)
		{
			m_isDataChanged = true;
		}
	}
	else if (pointLocation < 0)
	{
		m_isDataChanged = true;
	}
}
void VectorGraphDataArray::getUpdatingFromAutomation()
{
	// adding points with changed automation values
	for (unsigned int i = 0; i < m_dataArray.size(); i++)
	{
		if (getIsAutomationValueChanged(i) == true)
		{
#ifdef VECTORGRAPH_DEBUG_PAINT_EVENT
			qDebug("getUpdatingFromAutomation: point location: %d, attrib location: %d", i, getAutomatedAttribLocation(i));
#endif
			m_needsUpdating.push_back(i);
			// if the automatable value effects the y (so the position)
			// the point before this is updated too
			if (i > 0 && getAutomatedAttribLocation(i) == 0)
			{
				m_needsUpdating.push_back(i - 1);
			}
		}
	}
}
void VectorGraphDataArray::getUpdatingOriginals()
{
	// selecting only original values and sorting

#ifdef VECTORGRAPH_DEBUG_PAINT_EVENT
	for (unsigned int i = 0; i < m_needsUpdating.size(); i++)
	{
		qDebug("getUpatingOriginals before: m_needsUpdating[%d] -> %d  (point)", i, m_needsUpdating[i]);
	}
#endif

	// sorting the array
	// this is done becaues functions that use m_needsUpdating
	// are optimized for a sorted array
	std::sort(m_needsUpdating.begin(), m_needsUpdating.end(),
		[](unsigned int a, unsigned int b)
		{
			return a < b;
		});

	// removing duplicates
	int sizeDiff = 0;
	for (int i = 1; i < m_needsUpdating.size(); i++)
	{
		if (m_needsUpdating[i - 1 + sizeDiff] == m_needsUpdating[i + sizeDiff])
		{
			for (unsigned int j = i + sizeDiff; j < m_needsUpdating.size() - 1 + sizeDiff; j++)
			{
				m_needsUpdating[j] = m_needsUpdating[j + 1];
			}
			sizeDiff--;
		}
	}
	m_needsUpdating.resize(m_needsUpdating.size() + sizeDiff);

	// removing invalid locations
	// because sometimes deleted locations can be in m_needsUpdating
	for (unsigned int i = 0; i < m_needsUpdating.size(); i++)
	{
		if (m_needsUpdating[i] >= m_dataArray.size())
		{
			m_needsUpdating.resize(i);
			break;
		}
	}
	
#ifdef VECTORGRAPH_DEBUG_PAINT_EVENT
	for (unsigned int i = 0; i < m_needsUpdating.size(); i++)
	{
		qDebug("getUpatingOriginals final: m_needsUpdating[%d] -> %d  (point)", i, m_needsUpdating[i]);
	}
#endif
}
void VectorGraphDataArray::getSamplesInner(unsigned int targetSizeIn, bool* isChangedOut,
		std::vector<unsigned int>* updatingValuesOut, std::vector<float>* sampleBufferOut)
{
	bool effectorIsChanged = false;
	//std::shared_ptr<std::vector<unsigned int>> effectorUpdatingValues = std::make_shared<std::vector<unsigned int>>();
	std::vector<unsigned int> effectorUpdatingValues;
	// sampleBufferOut will serve as the effector's sampleBufferOut until the new m_bakedSamples gets made
	bool isEffected = m_effectorLocation >= 0;
	if (isEffected == true)
	{
		m_parent->getDataArray(m_effectorLocation)->getSamplesInner(targetSizeIn, &effectorIsChanged, &effectorUpdatingValues, sampleBufferOut);
	}
#ifdef VECTORGRAPH_DEBUG_PAINT_EVENT
	qDebug("getSamplesInner: id: %d", m_id);
#endif

	m_isDataChanged = m_isDataChanged || targetSizeIn != m_updatingBakedSamples.size();

	// deciding if the whole dataArray should be updated
	// if the whole effectorDataArray was updated
	int effectedCount = 0;
	if (effectorIsChanged == true)
	{
		for (unsigned int i = 0; i < m_dataArray.size(); i++)
		{
			effectedCount += isEffectedPoint(i) == true? 1 : 0;
		}
		if (effectedCount > m_dataArray.size() / 2)
		{
			m_isDataChanged = m_isDataChanged || effectorIsChanged;
		}
	}

	// updating m_needsUpdating
	if (m_isDataChanged == false && targetSizeIn == m_updatingBakedSamples.size())
	{
#ifdef VECTORGRAPH_DEBUG_PAINT_EVENT
		qDebug("getSamplesInner: get updating");
#endif
		if (isEffected == true && effectorUpdatingValues.size() > 0 &&
			(effectorIsChanged == false || effectedCount > 0))
		{
			// effectorUpdatingValues needs to be sorted
			// before use (in this case it is already sorted)
			getUpdatingFromEffector(&effectorUpdatingValues);
		}
		getUpdatingFromAutomation();
		// sort and select only original
		// values
		getUpdatingOriginals();
	}
	else
	{
#ifdef VECTORGRAPH_DEBUG_PAINT_EVENT
		qDebug("getSamplesInner: update all points");
#endif
		m_parent->lockBakedSamplesAccess();
		if (targetSizeIn != m_bakedSamples.size())
		{
			m_bakedSamples.resize(targetSizeIn);
		}
		m_parent->unlockBakedSamplesAccess();
		if (targetSizeIn != m_updatingBakedSamples.size())
		{
			m_updatingBakedSamples.resize(targetSizeIn);
		}
		m_needsUpdating.resize(m_dataArray.size());
		for (unsigned int i = 0; i < m_needsUpdating.size(); i++)
		{
			m_needsUpdating[i] = i;
		}
	}

	float stepSize = 1.0f / static_cast<float>(targetSizeIn);
	// calculating point data and lines
	if (m_needsUpdating.size() > 0 && m_updatingBakedSamples.size() > 0)
	{
		// calculating the relative X locations (in lines) of the output samples
		// sampleXLocations[sample_location] is equal to 0.0f if it is at the start of a line
		// and it is equal to 1.0f if it is at the end of a line
		std::vector<float> sampleXLocations(targetSizeIn);
		for (unsigned int i = 0; i < m_dataArray.size(); i++)
		{
			unsigned int start = static_cast<unsigned int>
				(std::ceil(m_dataArray[i].m_x / stepSize));
			if (i + 1 < m_dataArray.size())
			{
				unsigned int end = static_cast<unsigned int>
					(std::ceil(m_dataArray[i + 1].m_x / stepSize));
				for (unsigned int j = start; j < end; j++)
				{
					sampleXLocations[j] = (stepSize * static_cast<float>(j) - m_dataArray[i].m_x) / (m_dataArray[i + 1].m_x - m_dataArray[i].m_x);
				}
			}
		}

		// getting effectorDataArray pointer
		VectorGraphDataArray* effector = nullptr;
		if (m_effectorLocation >= 0 && m_parent->getDataArray(m_effectorLocation)->size() > 0)
		{
			effector = m_parent->getDataArray(m_effectorLocation);
		}

#ifdef VECTORGRAPH_DEBUG_PAINT_EVENT
		qDebug("getSamplesInner: updatingsize: %d", static_cast<int>(m_needsUpdating.size()));
#endif

		// calculate final lines
		for (unsigned int i = 0; i < m_needsUpdating.size(); i++)
		{
			// sampleBufferOut contains the effector m_bakedValues here
			getSamplesUpdateLines(effector, sampleBufferOut, &sampleXLocations, i, stepSize);
		}

		m_parent->lockBakedSamplesAccess();
		m_bakedSamples = m_updatingBakedSamples;
		m_parent->unlockBakedSamplesAccess();
	}

	// setting outputs
	if (isChangedOut != nullptr)
	{
		*isChangedOut = m_isDataChanged;
	}
	if (m_needsUpdating.size() > 0)
	{
		if (updatingValuesOut != nullptr)
		{
			*updatingValuesOut = m_needsUpdating;
		}

		// clearing the updated values
		m_needsUpdating.clear();
	}
	m_parent->lockBakedSamplesAccess();
	*sampleBufferOut = m_bakedSamples;
	m_parent->unlockBakedSamplesAccess();

	m_isDataChanged = false;
#ifdef VECTORGRAPH_DEBUG_PAINT_EVENT
	qDebug("getSamplesInner end");
#endif
}
void VectorGraphDataArray::getSamplesUpdateLines(VectorGraphDataArray* effector, std::vector<float>* effectorSamples,
	std::vector<float>* sampleXLocations, unsigned int pointLocation, float stepSize)
{
	unsigned int effectYLocation = static_cast<unsigned int>
		(std::ceil(m_dataArray[m_needsUpdating[pointLocation]].m_x / stepSize));
#ifdef VECTORGRAPH_DEBUG_PAINT_EVENT
	qDebug("getSamplesUpdatinLines: m_needsUpdating[%d]: %d   (point)\neffectYLocation: %d", pointLocation, m_needsUpdating[pointLocation], effectYLocation);
#endif
	// current effector output Y near m_needsUpdating[pointLocation] point
	float curEffectY = (*effectorSamples)[effectYLocation];
	float nextEffectY = (*effectorSamples)[effectYLocation];

	// getting the final automatable / effectable point values
	float curY = processAutomation(m_needsUpdating[pointLocation], m_dataArray[m_needsUpdating[pointLocation]].m_y, 0);
	float curC = processAutomation(m_needsUpdating[pointLocation], m_dataArray[m_needsUpdating[pointLocation]].m_c, 1);
	float curValA = processAutomation(m_needsUpdating[pointLocation], m_dataArray[m_needsUpdating[pointLocation]].m_valA, 2);
	float curValB = processAutomation(m_needsUpdating[pointLocation], m_dataArray[m_needsUpdating[pointLocation]].m_valB, 3);
	if (effector != nullptr && getEffectPoints(m_needsUpdating[pointLocation]) == true && getEffectLines(m_needsUpdating[pointLocation]) == false)
	{
		curY = processEffect(m_needsUpdating[pointLocation], curY, 0, curEffectY);
		curC = processEffect(m_needsUpdating[pointLocation], curC, 1, curEffectY);
		curValA = processEffect(m_needsUpdating[pointLocation], curValA, 2, curEffectY);
		curValB = processEffect(m_needsUpdating[pointLocation], curValB, 3, curEffectY);
	}
	// from where to update line
	int start = effectYLocation;
	int end = start;

	float nextY = curY;

	if (m_needsUpdating[pointLocation] + 1 < m_dataArray.size())
	{
		effectYLocation = static_cast<unsigned int>
			(std::ceil(m_dataArray[m_needsUpdating[pointLocation] + 1].m_x / stepSize));
		// where to end updating line (+1)
		end = effectYLocation;
		nextY = processAutomation(m_needsUpdating[pointLocation] + 1, m_dataArray[m_needsUpdating[pointLocation] + 1].m_y, 0);

		bool isCurEffected = isEffectedPoint(m_needsUpdating[pointLocation]);
		// if the next point (y location) can be effected
		// and the current point's line is uneffected
		if (effector != nullptr && getEffectPoints(m_needsUpdating[pointLocation] + 1) == true &&
			(getEffectLines(m_needsUpdating[pointLocation]) == false || isCurEffected == false))
		{
			nextEffectY = (*effectorSamples)[effectYLocation];
			nextY = processEffect(m_needsUpdating[pointLocation] + 1, nextY, 0, nextEffectY);
		}
	}
	// calculating line ends
	if (m_needsUpdating[pointLocation] + 1 >= m_dataArray.size())
	{
		// if this point is at the last location in m_dataArray
		for (int j = end; j < m_updatingBakedSamples.size(); j++)
		{
			m_updatingBakedSamples[j] = curY;
		}
	}
	if (m_needsUpdating[pointLocation] == 0)
	{
		// if this point is at the 0 location in m_dataArray
		for (int j = 0; j < start; j++)
		{
			m_updatingBakedSamples[j] = curY;
		}
	}

	float fadeInStart = 0.05f;
	unsigned int type = m_dataArray[m_needsUpdating[pointLocation]].m_type;
#ifdef VECTORGRAPH_DEBUG_PAINT_EVENT
	qDebug("getSamplesUpdatinLines: point: [%d] start: %d, end: %d, line type: %d,      ---     attribs: y: %f, next y: %f, curve %f, valA %f, valB %f",
		pointLocation, start, end, type, curY, nextY, curC, curValA, curValB);
#endif

	// calculate final updated line
	if (type == 0)
	{
		// calculate curve
		float curveStrength = std::abs(curC);
		float curveTarget = curY + (nextY - curY) * (curC + 1.0f) / 2.0f;
		if (curveStrength > 0.0f)
		{
			curveStrength = std::clamp(std::sqrt(curveStrength), -1.0f, 1.0f);
		}
		processBezier(&m_updatingBakedSamples, sampleXLocations, start, end,
			curY, nextY, curveTarget, curveTarget, curveStrength);
		// no line type
	}
	else if (type == 1)
	{
		// don't calculate curve
		// line type
		processBezier(&m_updatingBakedSamples, sampleXLocations, start, end,
			curY, nextY, curY + curValA * 2.0f, nextY + curValB * 2.0f, (curC + 1.0f) / 2.0f);
	}
	else if (type == 2)
	{
		// curve
		float curveStrength = std::abs(curC);
		float curveTarget = curY + (nextY - curY) * (curC + 1.0f) / 2.0f;
		if (curveStrength > 0.0f)
		{
			curveStrength = std::clamp(std::sqrt(curveStrength), -1.0f, 1.0f);
		}
		processBezier(&m_updatingBakedSamples, sampleXLocations, start, end,
			curY, nextY, curveTarget, curveTarget, curveStrength);
		// line type
		processLineTypeArraySine(&m_updatingBakedSamples, sampleXLocations, start, end, curValA, curValB, fadeInStart);
	}
	else if (type == 3)
	{
		// curve
		processBezier(&m_updatingBakedSamples, sampleXLocations, start, end,
			curY, nextY, curY, nextY, 0.0f);
		// line type
		processLineTypeArraySineB(&m_updatingBakedSamples, sampleXLocations, start, end, curValA, curValB, curC, fadeInStart);
	}
	else if (type == 4)
	{
		// curve
		processBezier(&m_updatingBakedSamples, sampleXLocations, start, end,
			curY, nextY, curY, nextY, 0.0f);
		// line type
		processLineTypeArrayPeak(&m_updatingBakedSamples, sampleXLocations, start, end, curValA, curValB, curC, fadeInStart);
	}
	else if (type == 5)
	{
		// curve
		float curveStrength = std::abs(curC);
		float curveTarget = curY + (nextY - curY) * (curC + 1.0f) / 2.0f;
		if (curveStrength > 0.0f)
		{
			curveStrength = std::clamp(std::sqrt(curveStrength), -1.0f, 1.0f);
		}
		processBezier(&m_updatingBakedSamples, sampleXLocations, start, end,
			curY, nextY, curveTarget, curveTarget, curveStrength);
		// line type
		processLineTypeArraySteps(&m_updatingBakedSamples, sampleXLocations, start, end, &m_updatingBakedSamples, curValA, curValB, fadeInStart);
	}
	else if (type == 6)
	{
		// curve
		processBezier(&m_updatingBakedSamples, sampleXLocations, start, end,
			curY, nextY, curY, nextY, 0.0f);
		// line type
		processLineTypeArrayRandom(&m_updatingBakedSamples, sampleXLocations, start, end, curValA, curValB, curC, fadeInStart);
	}
	if (effector != nullptr && getEffectLines(m_needsUpdating[pointLocation]) == true)
	{
		int startB = m_needsUpdating[pointLocation] == 0 ? 0 : start;
		int endB = m_needsUpdating[pointLocation] >= m_dataArray.size() - 1 ? m_updatingBakedSamples.size() : end;
		// process line effect
		// if it is enabled
		for (int j = startB; j < endB; j++)
		{
			m_updatingBakedSamples[j] = processEffect(m_needsUpdating[pointLocation], m_updatingBakedSamples[j], 0, (*effectorSamples)[j]);
		}
	}
	// clamp
	for (int j = start; j < end; j++)
	{
		if (m_updatingBakedSamples[j] > 1.0f)
		{
			m_updatingBakedSamples[j] = 1.0f;
		}
		else if (m_updatingBakedSamples[j] < -1.0f)
		{
			m_updatingBakedSamples[j] = -1.0f;
		}
	}
	if (m_isNonNegative == true)
	{
		int startB = m_needsUpdating[pointLocation] == 0 ? 0 : start;
		int endB = m_needsUpdating[pointLocation] >= m_dataArray.size() - 1 ? m_updatingBakedSamples.size() : end;
		for (int j = startB; j < endB; j++)
		{
			m_updatingBakedSamples[j] = m_updatingBakedSamples[j] / 2.0f + 0.5f;
		}
	}
}

bool VectorGraphDataArray::isEffectedPoint(unsigned int pointLocation)
{
	// return true when 1 or more effects are active
	return (getEffect(pointLocation, 0) + getEffect(pointLocation, 1) + getEffect(pointLocation, 2)) != 0;
}
void VectorGraphDataArray::formatDataArrayEndPoints()
{
	if (m_isFixedEndPoints == true && m_dataArray.size() > 0)
	{
		m_dataArray[m_dataArray.size() - 1].m_x = 1;
		m_dataArray[m_dataArray.size() - 1].m_y = 1.0f;
		m_dataArray[0].m_x = 0;
		m_dataArray[0].m_y = -1.0f;
		// dataChanged is called in functions using this function
		// so this function does not call it
	}
}

void VectorGraphDataArray::dataChanged()
{
	m_parent->dataArrayChanged();
}
void VectorGraphDataArray::clearedEvent()
{
	m_parent->dataArrayClearedEvent(m_id);
}
void VectorGraphDataArray::styleChanged()
{
	m_parent->dataArrayStyleChanged();
}

} // namespace lmms
