/*
 * VectorGraph.cpp - Vector graph widget, model, helper class implementation
 *
 * Copyright (c) 2024 Szeli1 </at/gmail/dot/com> TODO
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

#include <vector>
#include <cmath> // sine
#include <algorithm> // sort
#include <cstdlib> // rand
#include <cstdint> // unintptr_t
#include <mutex> // locking when getValues
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QInputDialog> // showInputDialog()
#include <QMenu> // context menu


#include "VectorGraph.h"
#include "StringPairDrag.h"
#include "CaptionMenu.h" // context menu
#include "embed.h" // context menu
#include "MainWindow.h" // getting main window for context menu
#include "GuiApplication.h" // getGUI
#include "AutomatableModel.h"
#include "ControllerConnectionDialog.h"
#include "ControllerConnection.h"
#include "ProjectJournal.h"
#include "JournallingObject.h"
#include "base64.h"


namespace lmms
{

namespace gui
{

VectorGraphView::VectorGraphView(QWidget * parentIn, int widthIn, int heightIn,
		unsigned int pointSizeIn, unsigned int maxLengthIn, bool shouldApplyDefaultVectorGraphColorsIn) :
		QWidget(parentIn),
		ModelView(new VectorGraphModel(maxLengthIn, nullptr, false), this)
{
	resize(widthIn, heightIn);

	m_mousePress = false;
	m_addition = false;

	m_pointSize = pointSizeIn;
	m_fontSize = 12;
	m_isSimplified = false;
	//m_background;
	m_useGetLastValues = false;

	m_selectedLocation = 0;
	m_selectedArray = 0;
	m_isSelected = false;
	m_isCurveSelected = false;
	m_isLastSelectedArray = false;

	m_graphHeight = height();
	m_controlHeight = 30;
	m_controlDisplayCount = 4;
	m_controlDisplayPage = 0;
	m_isEditingActive = false;
	m_controlText = {
		tr("x coordinate"), tr("y coordinate"), tr("curve"), tr("1. attribute value"),
		tr("2. attribute value"), tr("switch graph line type"), tr("switch graph automated value"),
		tr("switch graph effected value"), tr("can only effect graph points"), tr("\"add\" effect"), tr("\"subtract\" effect"),
		tr("\"multiply\" effect"), tr("\"divide\" effect"), tr("\"power\" effect"), tr("\"log\" effect"),
		tr("\"sine\" effect"), tr("\"clamp lower\" effect"), tr("\"clamp upper\" effect")
	};
	m_controlLineEffectText = {
		tr("none"),
		tr("sine"),
		tr("phase changable sine"),
		tr("peak"),
		tr("steps"),
		tr("random")
	};
	m_controlIsFloat = {
		true, true, true, true,
		true, false, false,
		false, false, false, false,
		false, false, false, false,
		false, false, false
	};

	m_lastTrackPoint.first = -1;
	m_lastTrackPoint.second = 0;
	m_lastScndTrackPoint.first = 0;
	m_lastScndTrackPoint.second = 0;

	setCursor(Qt::CrossCursor);

	modelChanged();
	if (shouldApplyDefaultVectorGraphColorsIn == true)
	{
		applyDefaultColors();
	}
}
VectorGraphView::~VectorGraphView()
{
	qDebug("VectorGraphView dstc");
	m_controlText.clear();
	m_controlIsFloat.clear();
	m_controlLineEffectText.clear();
	qDebug("VectorGraphView dstc end");
}

void VectorGraphView::setLineColor(QColor colorIn, unsigned int dataArrayLocationIn)
{
	if (model()->getDataArraySize() > dataArrayLocationIn)
	{
		model()->getDataArray(dataArrayLocationIn)->setLineColor(colorIn);
		updateGraph();
	}
}
void VectorGraphView::setActiveColor(QColor colorIn, unsigned int dataArrayLocationIn)
{
	if (model()->getDataArraySize() > dataArrayLocationIn)
	{
		model()->getDataArray(dataArrayLocationIn)->setActiveColor(colorIn);
		updateGraph();
	}
}
void VectorGraphView::setFillColor(QColor colorIn, unsigned int dataArrayLocationIn)
{
	if (model()->getDataArraySize() > dataArrayLocationIn)
	{
		model()->getDataArray(dataArrayLocationIn)->setFillColor(colorIn);
		updateGraph();
	}
}
void VectorGraphView::setAutomatedColor(QColor colorIn, unsigned int dataArrayLocationIn)
{
	if (model()->getDataArraySize() > dataArrayLocationIn)
	{
		model()->getDataArray(dataArrayLocationIn)->setAutomatedColor(colorIn);
		updateGraph();
	}
}
void VectorGraphView::applyDefaultColors()
{
	unsigned int size = model()->getDataArraySize();
	if (size > 0)
	{
		qDebug("applyDefaultColors lineColor: %d, %d, %d, %d", m_vectorGraphDefaultLineColor.red(), m_vectorGraphDefaultLineColor.green(), m_vectorGraphDefaultLineColor.blue(), m_vectorGraphDefaultLineColor.alpha());
		setLineColor(m_vectorGraphDefaultLineColor, 0);
		setActiveColor(m_vectorGraphDefaultActiveColor, 0);
		setFillColor(m_vectorGraphDefaultFillColor, 0);
		setAutomatedColor(m_vectorGraphDefaultAutomatedColor, 0);
		if (size > 1)
		{
			setLineColor(m_vectorGraphSecondaryLineColor, 1);
			setActiveColor(m_vectorGraphSecondaryActiveColor, 1);
			setFillColor(m_vectorGraphSecondaryFillColor, 1);
			setAutomatedColor(m_vectorGraphDefaultAutomatedColor, 1);
		}
	}
}

void VectorGraphView::setIsSimplified(bool isSimplifiedIn)
{
	m_isSimplified = isSimplifiedIn;
}

std::pair<float, float> VectorGraphView::getSelectedData()
{
	std::pair<float, float> output(-1.0f, 0.00);
	if (m_isSelected == true)
	{
		output.first = model()->getDataArray(m_selectedArray)->getX(m_selectedLocation);
		output.second = model()->getDataArray(m_selectedArray)->getY(m_selectedLocation);
	}
	return output;
}
int VectorGraphView::getLastSelectedArray()
{
	if (m_isLastSelectedArray == true)
	{
		return m_selectedArray;
	}
	return -1;
}
void VectorGraphView::setSelectedData(std::pair<float, float> dataIn)
{
	if (m_isSelected == true)
	{
		qDebug("setSelectedData");
		model()->getDataArray(m_selectedArray)->setY(m_selectedLocation, dataIn.second);
		qDebug("set value done");
		m_selectedLocation = model()->getDataArray(m_selectedArray)->setX(m_selectedLocation, dataIn.first);
		qDebug("set position done");
	}
}
void VectorGraphView::setBackground(const QPixmap backgroundIn)
{
	m_background = backgroundIn;
}
void VectorGraphView::useGetLastValues()
{
	m_useGetLastValues = true;
}

void VectorGraphView::mousePressEvent(QMouseEvent* me)
{
	qDebug("\n\nmousePressStart ---------");
	// get position
	int x = me->x();
	int y = me->y();
	m_addition = false;
	m_mousePress = false;

	if(me->button() == Qt::LeftButton && me->modifiers() & Qt::ControlModifier && m_isSelected == true)
	{
			qDebug("mousePress automation started");
		// connect to automation
		model()->getDataArray(m_selectedArray)->setAutomated(m_selectedLocation, true);
		FloatModel* curFloatModel = model()->getDataArray(m_selectedArray)->getAutomationModel(m_selectedLocation);
		// check if setAutomated is failed (like when isAutomatableEffecable is not enabled)
		if (curFloatModel != nullptr)
		{
			qDebug("mousePress automation sent");
			new gui::StringPairDrag("automatable_model", QString::number(curFloatModel->id()), QPixmap(), widget());
			me->accept();
		}
	}
	else
	{
		if (me->button() == Qt::LeftButton)
		{
			// add
			m_addition = true;
			m_mousePress = true;
		}
		else if (me->button() == Qt::RightButton)
		{
			// delete
			m_addition = false;
			m_mousePress = true;
		}
		if (isGraphPressed(x, m_graphHeight - y) == true)
		{
			// try selecting the clicked point (if it is near)
			selectData(x, m_graphHeight - y);
			// avoid triggering editing window while deleting
			// points are only deleted when
			// m_isSelected == true -> m_isEditingActive = true
			// so m_isEditingActive is set to false
			if (m_isSelected == true && m_addition == false)
			{
				m_isEditingActive = false;
			}
			if (m_isSelected == true)
			{
				setCursor(Qt::ArrowCursor);
			}
			else
			{
				setCursor(Qt::CrossCursor);
			}
		}
	}
}

void VectorGraphView::mouseMoveEvent(QMouseEvent* me)
{
	// get position
	// the x coord is clamped because
	// m_lastTrackPoint.first < 0 is used
	int x = me->x() >= 0 ? me->x() : 0;
	int y = me->y();

	bool startMoving = false;
	if (m_lastTrackPoint.first < 0)
	{
		m_lastTrackPoint.first = m_lastScndTrackPoint.first = x;
		m_lastTrackPoint.second = m_lastScndTrackPoint.second = m_graphHeight - y;
		m_mousePress = true;
	}

	if (m_mousePress == true)
	{
		// the mouse needs to move a bigger distance
		// before it is registered as dragging (-> m_mousePress = false)
		float curDistance = getDistance(x, m_graphHeight - y,
			m_lastTrackPoint.first, m_lastTrackPoint.second);
		if (curDistance > m_pointSize)
		{
			m_mousePress = false;
			startMoving = true;
		}
	}

	// if the mouse was moved a lot
	if (m_mousePress == false)
	{
		if (isGraphPressed(x, m_lastScndTrackPoint.second) == true)
		{
			if (m_isSelected == true && m_addition == true)
			{
				if (m_isCurveSelected == false)
				{
					// dragging point
					std::pair<float, float> convertedCoords = mapMousePos(x, m_graphHeight - y);
					convertedCoords.first = std::clamp(convertedCoords.first, 0.0f, 1.0f);
					convertedCoords.second = std::clamp(convertedCoords.second, -1.0f, 1.0f);
					setSelectedData(convertedCoords);
				}
				else if (model()->getDataArray(m_selectedArray)->getIsEditableAttrib() == true)
				{
					// dragging curve
					std::pair<float, float> convertedCoords = mapMousePos(x - m_lastTrackPoint.first, m_graphHeight - y + m_lastTrackPoint.second);
					float curveValue = convertedCoords.second + convertedCoords.first * 0.1f;
					curveValue = std::clamp(curveValue, -1.0f, 1.0f);
					model()->getDataArray(m_selectedArray)->setC(m_selectedLocation, curveValue);
				}
			}
			else if (m_addition == false)
			{
				// deleting points
				float curDistance = getDistance(x, m_graphHeight - y,
					m_lastTrackPoint.first, m_lastTrackPoint.second);
				if (curDistance > m_pointSize)
				{
					m_lastTrackPoint.first = x;
					m_lastTrackPoint.second = m_graphHeight - y;
					m_isSelected = false;
					selectData(x, m_graphHeight - y);
					if (m_isSelected == true)
					{
						model()->getDataArray(m_selectedArray)->del(m_selectedLocation);
						m_isSelected = false;
						m_isEditingActive = false;
					}
				}
			}
			else
			{
				// adding points
				if (startMoving == true && m_isLastSelectedArray == true)
				{
					// trying to add to the last selected array
					addPoint(m_selectedArray, x, m_graphHeight - y);
				}
				float curDistance = getDistance(x, m_graphHeight - y,
					m_lastTrackPoint.first, m_lastTrackPoint.second);
				if (curDistance > m_pointSize)
				{
					// calculating angle
					// getting the angle between (lastScndTrackPoint and lastTrackPoint)
					// and (lastTrackPoint and x and y)
					float curAngle = static_cast<float>(
						(m_lastTrackPoint.second - m_lastScndTrackPoint.second) *
						(m_graphHeight - y - m_lastTrackPoint.second) + 
						(m_lastTrackPoint.first - m_lastScndTrackPoint.first) *
						(x - m_lastTrackPoint.first));
					curAngle = std::acos(curAngle / curDistance /
						getDistance(m_lastScndTrackPoint.first, m_lastScndTrackPoint.second,
						m_lastTrackPoint.first, m_lastTrackPoint.second));
					// if the angle difference is bigger than 0.3 rad
					if (std::abs(curAngle) * curDistance * curDistance / static_cast<float>(m_pointSize * m_pointSize) > 0.3f)
					{
						m_lastScndTrackPoint.first = m_lastTrackPoint.first;
						m_lastScndTrackPoint.second = m_lastTrackPoint.second;
						
						if (m_isLastSelectedArray == true)
						{
							// trying to add to the last selected array
							addPoint(m_selectedArray, x, m_graphHeight - y);
						}
					}
					m_lastTrackPoint.first = x;
					m_lastTrackPoint.second = m_graphHeight - y;
				}
				// else m_mousePress does not change
			}
		}
		else if (isControlWindowPressed(m_lastScndTrackPoint.second) == true)
		{
			processControlWindowPressed(m_lastTrackPoint.first, m_graphHeight - m_lastScndTrackPoint.second, true, startMoving, x, m_graphHeight - y);
		}
	}
}

void VectorGraphView::mouseReleaseEvent(QMouseEvent* me)
{
	// get position
	int x = me->x();
	int y = me->y();
	// if did not drag and graph is pressed
	if (m_mousePress == true && isGraphPressed(x, m_graphHeight - y) == true)
	{
	qDebug("mouseMove graphPressed: %d", m_lastTrackPoint.first);
		// add/delete point
		if (m_isSelected == false && m_addition == true)
		{
			// if selection failed and addition
			// get the first editable daraArray and add value
			qDebug("release size: %ld", model()->getDataArraySize());
			bool success = false;
			if (m_isLastSelectedArray == true)
			{
				// trying to add to the last selected array
				success = addPoint(m_selectedArray, x, m_graphHeight - y);
			}
			if (success == false)
			{
				// trying to add to all the selected arrays
				for(unsigned int i = 0; i < model()->getDataArraySize(); i++)
				{
					success = addPoint(i, x, m_graphHeight - y);
					if (success == true)
					{
						m_selectedArray = i;
						m_isLastSelectedArray = true;
						break;
					}
				}
			}
		}
		else if (m_isSelected == true && m_addition == false)
		{
			// if selection was successful -> deletion
			model()->getDataArray(m_selectedArray)->del(m_selectedLocation);
			m_isSelected = false;
			m_isEditingActive = false;
		}
	}
	else if (m_mousePress == true && isControlWindowPressed(m_graphHeight - y) == true)
	{
		qDebug("Mouse Release event try running processControlWindowPressed");
		processControlWindowPressed(x, m_graphHeight - y, false, false, 0, 0);
	}
	else if (isGraphPressed(x, m_graphHeight - y) == false)
	{
qDebug("mouseRelease 8, select new array, m_selectedArray: %d", m_selectedArray);
		// if the "switch graph" button was pressed in editing mode
		unsigned int oldSelectedArray = m_selectedArray;
		m_selectedLocation = 0;
		m_selectedArray = 0;
		m_isLastSelectedArray = false;
		m_isSelected = false;
		m_isEditingActive = false;
		m_isCurveSelected = false;

		// looping throught the data arrays to get a new
		// selected data array
		for (unsigned int i = 0; i < model()->getDataArraySize(); i++)
		{
			//qDebug("mouseRelease select new array: [%d], m_selectedArray: %d, old: %d", i, m_selectedArray, oldSelectedArray);
			if (model()->getDataArray(i)->getIsSelectable() == true)
			{
				// if this data array is the first one that is selectable
				if (m_isLastSelectedArray == false)
				{
					m_selectedArray = i;
					m_isLastSelectedArray = true;
				}
				// if this data array's location is bigger than the old location
				// if this is false then m_selectedArray will equal to the first selectable array
				if (i > oldSelectedArray)
				{
					m_selectedArray = i;
					m_isLastSelectedArray = true;
					break;
				}
			}
		}
		//qDebug("mouseRelease select new array final:", m_selectedArray);
	}
	m_mousePress = false;
	m_addition = false;
	// reset trackpoint
	m_lastTrackPoint.first = -1;
	updateGraph(false);
	qDebug("mouseReleaseEnd");
}

void VectorGraphView::mouseDoubleClickEvent(QMouseEvent * me)
{
	// get position
	int x = me->x();
	int y = me->y();

	// if a data/sample is selected then show input dialog to change the data
	qDebug("mouseDoubleClickEvent");
	if (isGraphPressed(x, m_graphHeight - y) == true)
	{
		if (m_isSelected == true && me->button() == Qt::LeftButton)
		{
			// display dialog
			std::pair<float, float> curData = showCoordInputDialog();
			// change data
			setSelectedData(curData);
		}
	}
	else if (isControlWindowPressed(m_graphHeight - y) == true)
	{
		m_mousePress = true;
		int pressLocation = getPressedControlInput(x, m_graphHeight - y, m_controlDisplayCount + 1);
		if (pressLocation >= 0 && pressLocation != m_controlDisplayCount)
		{
			unsigned int location = m_controlDisplayCount * m_controlDisplayPage + pressLocation;
			if (location < m_controlText.size() && m_controlIsFloat[location] == true)
			{
				// unused bool
				bool isTrue = false;
				// set m_lastScndTrackPoint.first to the current input value
				float curValue = getInputAttribValue(location, &isTrue);
				setInputAttribValue(location, showInputDialog(curValue), isTrue);
			}
		}
	}
}

void VectorGraphView::paintEvent(QPaintEvent* pe)
{
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing, true);

	qDebug("paintEvent");
	m_graphHeight = m_isEditingActive == true ? height() - m_controlHeight : height();

	// paint background
	if (m_background.isNull() == false)
	{
		p.drawPixmap(0, 0, m_background);
	}

	// paint outline
	p.setPen(QPen(QColor(127, 127, 127, 255), 1));
	p.drawLine(0, 0, width() - 1, 0);
	p.drawLine(width() - 1, 0, width() - 1, height() - 1);
	p.drawLine(0, height() - 1, width() - 1, height() - 1);
	p.drawLine(0, 0, 0, height() - 1);

	std::vector<int> alreadyUpdatedDataArrays;

	for (unsigned int i = 0; i < model()->getDataArraySize(); i++)
	{
		paintGraph(&p, i, &alreadyUpdatedDataArrays);
	}

	paintEditing(&p);

	m_useGetLastValues = false;
	qDebug("paint event end");
	emit drawn();
}

void VectorGraphView::paintGraph(QPainter* pIn, unsigned int locationIn, std::vector<int>* alreadyUpdatedDataArraysIn)
{
	VectorGraphDataArray* dataArray = model()->getDataArray(locationIn);
	unsigned int length = dataArray->size();
	if (length > 0)
	{
		pIn->setPen(QPen(*dataArray->getLineColor(), 2));
		pIn->setBrush(QBrush(*dataArray->getLineColor(), Qt::NoBrush));

		std::pair<int, int> posA(0, 0);
		std::pair<int, int> posB(0, 0);
		std::pair<int, int> startPos(mapDataPos(0.0f, dataArray->getY(0), false));
		// draw line
		if (m_isSimplified == false)
		{
			QPainterPath pt;
			posA = startPos;
			pt.moveTo(startPos.first + 1, m_graphHeight - startPos.second);

			std::vector<float> dataArrayValues;
			if (m_useGetLastValues == true)
			{
				dataArrayValues = dataArray->getLastValues();
			}
			else
			{
				// getting most updated dataArray values
				// if this dataArray has not been updated by
				// an other dataArray while paintEvent is happening
				bool found = false;
				for (unsigned int j = 0; j < alreadyUpdatedDataArraysIn->size(); j++)
				{
					if (locationIn == alreadyUpdatedDataArraysIn->operator[](j))
					{
						found = true;
						break;
					}
				}
				if (found == false)
				{
					dataArrayValues = dataArray->getValues(width());
					std::vector<int> updatedArrays = dataArray->getEffectorArrayLocations();
					for (unsigned int j = 0; j < updatedArrays.size(); j++)
					{
						alreadyUpdatedDataArraysIn->push_back(updatedArrays[j]);
					}
				}
				else
				{
					dataArrayValues = dataArray->getLastValues();
				}
			}

			qDebug("paint dataArrayValues size: %ld", dataArrayValues.size());
			for (unsigned int j = 0; j < dataArrayValues.size(); j++)
			{
				// if nonNegative then only the dataArray output (getDataValues)
				// is bigger than 0 so it matters only here
				posB = mapDataPos(0, dataArrayValues[j], dataArray->getIsNonNegative());
				posB.first = static_cast<int>((j * width()) / static_cast<float>(dataArrayValues.size()));
	
				if (posA.first != posB.first)
				{
					pt.lineTo(posB.first, m_graphHeight - posB.second);
					// pt replaces drawing with path
					//pIn->drawLine(posA.first, m_graphHeight - posA.second, posB.first, m_graphHeight - posB.second);
				}
				posA = posB;
			}

			// final draw line, fill
			if (dataArray->getFillColor()->alpha() > 0)
			{
				// getting the line for
				// drawing later
				QPainterPath ptline(pt);
				pt.lineTo(width() - 1, posB.second);
				pt.lineTo(width() - 1, m_graphHeight - 1);
				pt.lineTo(startPos.first + 1, m_graphHeight - 1);
				pt.lineTo(startPos.first + 1, startPos.second);
				// draw fill
				pIn->fillPath(pt, QBrush(*dataArray->getFillColor()));
				// draw line
				pIn->drawPath(ptline);
			}
			else
			{
				pIn->drawPath(pt);
			}
		}

		// draw points
		if (dataArray->getIsSelectable() == true || m_isSimplified == true)
		{
			posA = startPos;

			QColor automatedFillColor(getFillColorFromBaseColor(*dataArray->getAutomatedColor()));
			bool drawPoints = dataArray->getIsSelectable() && width() / length > m_pointSize * 2;
			bool resetColor = false;
			for (unsigned int j = 0; j < length; j++)
			{
				posB = mapDataPos(dataArray->getX(j), dataArray->getY(j), false);
				// draw point
				if (drawPoints == true)
				{
					// set point color
					if (dataArray->getAutomationModel(j) != nullptr)
					{
						// if automated
						pIn->setPen(QPen(*dataArray->getAutomatedColor(), 2));
						pIn->setBrush(QBrush(automatedFillColor, Qt::SolidPattern));
						resetColor = true;
					}
					else if (m_isSelected == true && m_selectedArray == locationIn && m_selectedLocation == j)
					{
						// if selected
						pIn->setBrush(QBrush(*dataArray->getFillColor(), Qt::SolidPattern));
						resetColor = true;
					}

					pIn->drawEllipse(posB.first - m_pointSize, m_graphHeight - posB.second - m_pointSize, m_pointSize * 2, m_pointSize * 2);

					// reset point color
					if (resetColor == true)
					{
						pIn->setPen(QPen(*dataArray->getLineColor(), 2));
						pIn->setBrush(Qt::NoBrush);
						resetColor = false;
					}

					if (j > 0)
					{
						if (dataArray->getIsEditableAttrib() == true)
						{
							std::pair<int, int> posC = mapDataCurvePos(posA.first, posA.second, posB.first, posB.second, dataArray->getC(j - 1));
							pIn->drawRect(posC.first - m_pointSize / 4 - m_pointSize / 2,
								m_graphHeight - posC.second - m_pointSize / 4 - m_pointSize / 2, m_pointSize, m_pointSize);
						}
					}
				}

				// draw simplified line
				if (m_isSimplified == true)
				{
					pIn->drawLine(posA.first, m_graphHeight - posA.second, posB.first, m_graphHeight - posB.second);
				}
				posA = posB;
			}
		}
		// draw last simplified line
		if (m_isSimplified == true)
		{
			pIn->drawLine(posB.first, m_graphHeight - posB.second, width(), m_graphHeight - posB.second);
		}
	}
}
void VectorGraphView::paintEditing(QPainter* pIn)
{
	pIn->setFont(QFont("Arial", m_fontSize));
	if (m_isEditingActive == true)
	{
		VectorGraphDataArray* dataArray = model()->getDataArray(m_selectedArray);
		QColor textColor = getTextColorFromBaseColor(*dataArray->getLineColor());
		// background of float values
		QColor backColor(getFillColorFromBaseColor(*dataArray->getFillColor()));
		QColor foreColor = *dataArray->getLineColor();
		if (dataArray->getFillColor()->alpha() > 0)
		{
			foreColor = *dataArray->getFillColor();
		}

		int controlTextCount = m_controlText.size();
		if (dataArray->getIsEditableAttrib() == false)
		{
			// x, y
			controlTextCount = 2;
		}
		else if (dataArray->getIsAutomatableEffectable() == false)
		{
			// x, y, curve, valA, valB, switch type
			controlTextCount = 6;
		}

		int segmentLength = width() / (m_controlDisplayCount + 1);
		// draw inputs
		pIn->setPen(QPen(textColor, 1));
		for (unsigned int i = 0; i < m_controlDisplayCount; i++)
		{
			int controlLocation = m_controlDisplayCount * m_controlDisplayPage + i;
			if (controlLocation < controlTextCount)
			{
				if (m_controlIsFloat[controlLocation] == true)
				{
					QColor curBackColor = backColor;
					QColor curForeColor = foreColor;
					// unused bool
					bool isTrue = false;
					float inputValue = getInputAttribValue(controlLocation, &isTrue);
					if (dataArray->getAutomationModel(m_selectedLocation) != nullptr  && static_cast<int>(getInputAttribValue(6, &isTrue)) == controlLocation - 1)
					{
						curForeColor = *dataArray->getAutomatedColor();
						curBackColor = getFillColorFromBaseColor(curForeColor);
					}
					else if (dataArray->getIsAutomatableEffectable() == true && static_cast<int>(getInputAttribValue(7, &isTrue)) == controlLocation - 1)
					{
						curForeColor = *dataArray->getActiveColor();
						curBackColor = getFillColorFromBaseColor(curForeColor);
					}
					pIn->fillRect(i * segmentLength, m_graphHeight, segmentLength, m_controlHeight, curBackColor);
					pIn->fillRect(i * segmentLength, m_graphHeight, mapControlInputX(inputValue, segmentLength), m_controlHeight, curForeColor);
					pIn->drawText(i * segmentLength, m_graphHeight + (m_controlHeight - m_fontSize) / 2 + m_fontSize,
						getTextFromDisplayLength(m_controlText[controlLocation], segmentLength));
				}
				else
				{
					QColor curForeColor = *dataArray->getFillColor();
					bool isTrue = false;
					getInputAttribValue(controlLocation, &isTrue);
					if (isTrue == true)
					{
						curForeColor = *dataArray->getActiveColor();
					}
					pIn->fillRect(i * segmentLength, m_graphHeight, segmentLength, m_controlHeight, curForeColor);
					pIn->drawText(i * segmentLength, m_graphHeight + (m_controlHeight - m_fontSize) / 2 + m_fontSize,
						getTextFromDisplayLength(m_controlText[controlLocation], segmentLength));
				}
			}
		}

		// draw "next page" button
		pIn->fillRect(m_controlDisplayCount * segmentLength, m_graphHeight, segmentLength, m_controlHeight, *dataArray->getFillColor());
		pIn->setPen(textColor);
		pIn->drawText(m_controlDisplayCount * segmentLength, m_graphHeight + (m_controlHeight - m_fontSize) / 2 + m_fontSize, ">>");
		// draw selected array display outline
		pIn->setPen(*dataArray->getLineColor());
		pIn->drawRect(0, 0, m_controlHeight, m_controlHeight);
		// draw outline
		pIn->drawLine(0, m_graphHeight, width(), m_graphHeight);
		for (unsigned int i = 1; i < m_controlDisplayCount + 1; i++)
		{
			if (m_controlDisplayCount * m_controlDisplayPage + i < controlTextCount || i >= m_controlDisplayCount)
			{
				pIn->drawLine(i * segmentLength, m_graphHeight, i * segmentLength, height());
			}
		}
	}
	if (m_isLastSelectedArray == true)
	{
		// draw selected array number
		pIn->setPen(QPen(QColor(127, 127, 127, 255), 2));
		pIn->drawText(2, (m_controlHeight - m_fontSize) / 2 + m_fontSize, QString::number(m_selectedArray));
	}
}

void VectorGraphView::modelChanged()
{
	auto gModel = model();
	QObject::connect(gModel, SIGNAL(updateGraphView(bool)),
			this, SLOT(updateGraph(bool)));
	QObject::connect(gModel, SIGNAL(styleChanged()),
			this, SLOT(updateGraph()));
}

void VectorGraphView::updateGraph()
{
	update();
}
void VectorGraphView::updateGraph(bool shouldUseGetLastValuesIn)
{
	m_useGetLastValues = shouldUseGetLastValuesIn;
	update();
}
void VectorGraphView::execConnectionDialog()
{
	if (m_isSelected == true)
	{
		model()->getDataArray(m_selectedArray)->setAutomated(m_selectedLocation, true);
		FloatModel* curAutomationModel = model()->getDataArray(m_selectedArray)->getAutomationModel(m_selectedLocation);
		gui::ControllerConnectionDialog dialog(getGUI()->mainWindow(), curAutomationModel);

		if (dialog.exec() == 1)
		{
			// Actually chose something
			if (curAutomationModel != nullptr && dialog.chosenController() != nullptr)
			{
				// Update
				if (curAutomationModel->controllerConnection() != nullptr)
				{
					curAutomationModel->controllerConnection()->setController(dialog.chosenController());
				}
				else
				{
					// New
					auto cc = new ControllerConnection(dialog.chosenController());
					curAutomationModel->setControllerConnection(cc);
				}
			}
			else
			{
				// no controller, so delete existing connection
				removeController();
			}
		}
		else
		{
			// did not return 1 -> delete the created floatModel
			removeController();
		}
	}
}
void VectorGraphView::removeAutomation()
{
	if (m_isSelected == true)
	{
		// deleting the floatmodel will delete the connecitons
		model()->getDataArray(m_selectedArray)->setAutomated(m_selectedLocation, false);
	}
}
void VectorGraphView::removeController()
{
	removeAutomation();
}

std::pair<float, float> VectorGraphView::mapMousePos(int xIn, int yIn)
{
	// mapping the position to 0 - 1, -1 - 1 using qWidget width and height
	return std::pair<float, float>(
		static_cast<float>(xIn / (float)width()),
		static_cast<float>(yIn) * 2.0f / static_cast<float>(m_graphHeight) - 1.0f);
}
std::pair<int, int> VectorGraphView::mapDataPos(float xIn, float yIn, bool isNonNegativeIn)
{
	// mapping the point/sample positon to mouse/view position
	if (isNonNegativeIn == true)
	{
		return std::pair<int, int>(
			static_cast<int>(xIn * width()),
			static_cast<int>(yIn * m_graphHeight));
	}
	else
	{
		return std::pair<int, int>(
			static_cast<int>(xIn * width()),
			static_cast<int>((yIn + 1.0f) * static_cast<float>(m_graphHeight) / 2.0f));
	}
}
std::pair<float, float> VectorGraphView::mapDataCurvePos(float xAIn, float yAIn, float xBIn, float yBIn, float curveIn)
{
	return std::pair<float, float>(
		(xAIn + xBIn) / 2.0f,
		yAIn + (curveIn / 2.0f + 0.5f) * (yBIn - yAIn));
}
std::pair<int, int> VectorGraphView::mapDataCurvePos(int xAIn, int yAIn, int xBIn, int yBIn, float curveIn)
{
	return std::pair<int, int>(
		(xAIn + xBIn) / 2,
		yAIn + static_cast<int>((curveIn / 2.0f + 0.5f) * (yBIn - yAIn)));
}
int VectorGraphView::mapControlInputX(float inputValueIn, unsigned int displayLengthIn)
{
	return (inputValueIn / 2.0f + 0.5f) * displayLengthIn;
}

float VectorGraphView::getDistance(int xAIn, int yAIn, int xBIn, int yBIn)
{
	return std::sqrt(static_cast<float>((xAIn - xBIn) * (xAIn - xBIn) + (yAIn - yBIn) * (yAIn - yBIn)));
}
float VectorGraphView::getDistance(float xAIn, float yAIn, float xBIn, float yBIn)
{
	return std::sqrt((xAIn - xBIn) * (xAIn - xBIn) + (yAIn - yBIn) * (yAIn - yBIn));
}

bool VectorGraphView::addPoint(unsigned int locationIn, int mouseXIn, int mouseYIn)
{
	// mouseYIn is calculated like this:
	// m_graphHeight - y
	bool output = false;
	std::pair<float, float> curMouseCoords = mapMousePos(mouseXIn, mouseYIn);
	curMouseCoords.first = std::clamp(curMouseCoords.first, 0.0f, 1.0f);
	curMouseCoords.second = std::clamp(curMouseCoords.second, -1.0f, 1.0f);
	int location = model()->getDataArray(locationIn)->add(curMouseCoords.first);
	// if adding was successful
	if (location >= 0)
	{
		output = true;
		qDebug("addPoint point added: array: %d location: %d   %f,%d", locationIn, location, curMouseCoords.second, m_graphHeight);
		model()->getDataArray(locationIn)->setY(location, curMouseCoords.second);
	}
	qDebug("addPoint LocationIn: %d", locationIn);
	return output;
}

bool VectorGraphView::isGraphPressed(int mouseXIn, int mouseYIn)
{
	bool output = true;
	// mouseYIn is calculated like this:
	// m_graphHeight - y
	if (m_isEditingActive == true && m_graphHeight - mouseYIn < m_controlHeight && mouseXIn < m_controlHeight)
	{
		// if switch selected data array was pressed
		//qDebug("isGraphPressed switch selected dataArray");
		output = false;
	}
	else if (isControlWindowPressed(mouseYIn) == true)
	{
		// if the control window was pressed
		output = false;
	}
	return output;
}
bool VectorGraphView::isControlWindowPressed(int mouseYIn)
{
	bool output = false;
	// mouseYIn is calculated like this:
	// m_graphHeight - y
	if (m_isEditingActive == true && mouseYIn <= 0)
	{
		//qDebug("isGraphPressed control window was pressed");
		output = true;
	}
	return output;
}
void VectorGraphView::processControlWindowPressed(int mouseXIn, int mouseYIn, bool isDraggingIn, bool startMovingIn, int curXIn, int curYIn)
{
	// mouseYIn is calculated like this:
	// m_graphHeight - y
	setCursor(Qt::ArrowCursor);

	qDebug("mouseMove 7: %d", m_lastTrackPoint.first);
	if (m_isEditingActive == true)
	{
		int pressLocation = getPressedControlInput(mouseXIn, m_graphHeight - mouseYIn, m_controlDisplayCount + 1);
		int location = m_controlDisplayCount * m_controlDisplayPage + pressLocation;
		if (isDraggingIn == false && pressLocation == m_controlDisplayCount)
		{
			// if the last button was pressed

			// how many inputs are there
			int controlTextCount = m_controlText.size();
			if (m_isSelected == true)
			{
				if (model()->getDataArray(m_selectedArray)->getIsEditableAttrib() == false)
				{
					// x, y
					controlTextCount = 2;
				}
				else if (model()->getDataArray(m_selectedArray)->getIsAutomatableEffectable() == false)
				{
					// x, y, curve, valA, valB, switch type
					controlTextCount = 6;
				}
			}

			m_controlDisplayPage++;
			if (m_controlDisplayCount * m_controlDisplayPage >= controlTextCount)
			{
				m_controlDisplayPage = 0;
			}
			qDebug("mouseRelease controlPage: %d", m_controlDisplayPage);
		}
		else if (pressLocation >= 0 && location < m_controlText.size())
		{
			// pressLocation should always be bigger than -1
			// if the control window was pressed

			if (m_addition == false)
			{
				// if the right mouse button was pressed
				// get context menu input text
				QString controlDisplayText = m_controlText[location];
				if (location == 5)
				{
					bool isTrue = false;
					int typeVal = static_cast<int>(getInputAttribValue(location, &isTrue));
					if (typeVal < m_controlLineEffectText.size())
					{
						controlDisplayText = controlDisplayText + QString(" (") + m_controlLineEffectText[typeVal] + QString(")");
					}
				}
				// show context menu
				CaptionMenu contextMenu(model()->displayName() + QString(" - ") + controlDisplayText);
				addDefaultActions(&contextMenu, QString("(") + m_controlText[location] + QString(")"));
				contextMenu.exec(QCursor::pos());
			}
			else if (isDraggingIn == false && m_controlIsFloat[location] == false)
			{
				// if the input type is a bool

				bool curBoolValue = true;
				// if location is not at "set type" or "set automation location" or "set effect location"
				// (else curBoolValue = true -> setInputAttribValue will add 1 to the attribute)
				if (location < 5 || location > 7)
				{
					getInputAttribValue(location, &curBoolValue);
					curBoolValue = !curBoolValue;
				}
				setInputAttribValue(location, 0.0f, curBoolValue);
			}
			else if (isDraggingIn == true && m_controlIsFloat[location] == true)
			{
				// if the input type is a float

				// if the user just started to move the mouse it is pressed
				if (startMovingIn == true)
				{
					// unused bool
					bool isTrue = false;
					// set m_lastScndTrackPoint.first to the current input value
					m_lastScndTrackPoint.first = mapControlInputX(getInputAttribValue(location, &isTrue), m_graphHeight);

					m_lastTrackPoint.first = curXIn;
					m_lastTrackPoint.second = curYIn;
	qDebug("get last value: %d, lasttrack: %d, x: %d, y: %d, x2: %d, y2: %d, location: %d", m_lastScndTrackPoint.first, m_lastScndTrackPoint.second, curXIn, (curYIn), m_lastTrackPoint.first, m_lastTrackPoint.second, pressLocation);
				}
				std::pair<float, float> convertedCoords = mapMousePos(0,
					m_lastScndTrackPoint.first + static_cast<int>(curYIn - m_lastTrackPoint.second) / 2);
				qDebug("dragging ... %d, %f", (m_lastScndTrackPoint.first + static_cast<int>(curYIn - m_lastTrackPoint.second) / 2), convertedCoords.second);
				setInputAttribValue(location, convertedCoords.second, false);
			}
		}
	}
}
int VectorGraphView::getPressedControlInput(int mouseXIn, int mouseYIn, unsigned int inputCountIn)
{
	int output = -1;
	if (m_isEditingActive == true && mouseYIn > m_graphHeight)
	{
		output = mouseXIn * inputCountIn / width();
	}
	if (output > inputCountIn)
	{
		output = inputCountIn;
qDebug("getPressedControlInput x location ERRROR: %d", mouseXIn);
	}
	return output;
}
float VectorGraphView::getInputAttribValue(unsigned int controlArrayLocationIn, bool* valueOut)
{
	float output = 0.0f;
	if (m_isSelected == true)
	{
		switch (controlArrayLocationIn)
		{
			case 0:
				*valueOut = false;
				output = model()->getDataArray(m_selectedArray)->getX(m_selectedLocation);
				break;
			case 1:
				*valueOut = false;
				output = model()->getDataArray(m_selectedArray)->getY(m_selectedLocation);
				break;
			case 2:
				*valueOut = false;
				output = model()->getDataArray(m_selectedArray)->getC(m_selectedLocation);
				break;
			case 3:
				*valueOut = false;
				output = model()->getDataArray(m_selectedArray)->getValA(m_selectedLocation);
				break;
			case 4:
				*valueOut = false;
				output = model()->getDataArray(m_selectedArray)->getValB(m_selectedLocation);
				break;
			case 5:
				// type
				*valueOut = false;
				output = model()->getDataArray(m_selectedArray)->getType(m_selectedLocation);
				break;
			case 6:
				// automation location
				*valueOut = false;
				output = model()->getDataArray(m_selectedArray)->getAutomatedAttribLocation(m_selectedLocation);
				break;
			case 7:
				// effect location
				*valueOut = false;
				output = model()->getDataArray(m_selectedArray)->getEffectedAttribLocation(m_selectedLocation);
				break;
			case 8:
				*valueOut = model()->getDataArray(m_selectedArray)->getEffectOnlyPoints(m_selectedLocation);
				break;
			case 9:
				*valueOut = model()->getDataArray(m_selectedArray)->getEffect(m_selectedLocation, 0);
				break;
			case 10:
				*valueOut = model()->getDataArray(m_selectedArray)->getEffect(m_selectedLocation, 1);
				break;
			case 11:
				*valueOut = model()->getDataArray(m_selectedArray)->getEffect(m_selectedLocation, 2);
				break;
			case 12:
				*valueOut = model()->getDataArray(m_selectedArray)->getEffect(m_selectedLocation, 3);
				break;
			case 13:
				*valueOut = model()->getDataArray(m_selectedArray)->getEffect(m_selectedLocation, 4);
				break;
			case 14:
				*valueOut = model()->getDataArray(m_selectedArray)->getEffect(m_selectedLocation, 5);
				break;
			case 15:
				*valueOut = model()->getDataArray(m_selectedArray)->getEffect(m_selectedLocation, 6);
				break;
			case 16:
				*valueOut = model()->getDataArray(m_selectedArray)->getEffect(m_selectedLocation, 7);
				break;
			case 17:
				*valueOut = model()->getDataArray(m_selectedArray)->getEffect(m_selectedLocation, 8);
				break;
		}
	}
	return output;
}
void VectorGraphView::setInputAttribValue(unsigned int controlArrayLocationIn, float floatValueIn, bool boolValueIn)
{
	qDebug("setInputAttribValue started");
	if (m_isSelected == true)
	{
		float clampedValue = std::clamp(floatValueIn, -1.0f, 1.0f);
		unsigned int clampedValueB = 0;
		switch (controlArrayLocationIn)
		{
			case 0:
				m_selectedLocation = model()->getDataArray(m_selectedArray)->setX(m_selectedLocation, std::max(clampedValue, 0.0f));
				break;
			case 1:
				model()->getDataArray(m_selectedArray)->setY(m_selectedLocation, clampedValue);
				break;
			case 2:
				model()->getDataArray(m_selectedArray)->setC(m_selectedLocation, clampedValue);
				break;
			case 3:
				model()->getDataArray(m_selectedArray)->setValA(m_selectedLocation, clampedValue);
				break;
			case 4:
				model()->getDataArray(m_selectedArray)->setValB(m_selectedLocation, clampedValue);
				break;
			case 5:
				// type
				clampedValueB = 0;
				if (boolValueIn == true)
				{
					clampedValueB = model()->getDataArray(m_selectedArray)->getType(m_selectedLocation) + 1;
					if (clampedValueB > 5)
					{
						clampedValueB = 0;
					}
				}
				else
				{
					clampedValueB = static_cast<unsigned int>(std::clamp(floatValueIn, 0.0f, 5.0f));
				}
				model()->getDataArray(m_selectedArray)->setType(m_selectedLocation, clampedValueB);
				break;
			case 6:
				// automation location
				clampedValueB = 0;
				if (boolValueIn == true)
				{
					clampedValueB = model()->getDataArray(m_selectedArray)->getAutomatedAttribLocation(m_selectedLocation) + 1;
					if (clampedValueB > 4)
					{
						clampedValueB = 0;
					}
				}
				else
				{
					clampedValueB = static_cast<unsigned int>(std::clamp(floatValueIn, 0.0f, 4.0f));
				}
				model()->getDataArray(m_selectedArray)->setAutomatedAttrib(m_selectedLocation, clampedValueB);
				break;
			case 7:
				// effect location
				clampedValueB = 0;
				if (boolValueIn == true)
				{
					clampedValueB = model()->getDataArray(m_selectedArray)->getEffectedAttribLocation(m_selectedLocation) + 1;
					if (clampedValueB > 4)
					{
						clampedValueB = 0;
					}
				}
				else
				{
					clampedValueB = static_cast<unsigned int>(std::clamp(floatValueIn, 0.0f, 4.0f));
				}
				model()->getDataArray(m_selectedArray)->setEffectedAttrib(m_selectedLocation, clampedValueB);
				break;
			case 8:
				model()->getDataArray(m_selectedArray)->setEffectOnlyPoints(m_selectedLocation, boolValueIn);
				break;
			case 9:
				model()->getDataArray(m_selectedArray)->setEffect(m_selectedLocation, 0, boolValueIn);
				break;
			case 10:
				model()->getDataArray(m_selectedArray)->setEffect(m_selectedLocation, 1, boolValueIn);
				break;
			case 11:
				model()->getDataArray(m_selectedArray)->setEffect(m_selectedLocation, 2, boolValueIn);
				break;
			case 12:
				model()->getDataArray(m_selectedArray)->setEffect(m_selectedLocation, 3, boolValueIn);
				break;
			case 13:
				model()->getDataArray(m_selectedArray)->setEffect(m_selectedLocation, 4, boolValueIn);
				break;
			case 14:
				model()->getDataArray(m_selectedArray)->setEffect(m_selectedLocation, 5, boolValueIn);
				break;
			case 15:
				model()->getDataArray(m_selectedArray)->setEffect(m_selectedLocation, 6, boolValueIn);
				break;
			case 16:
				model()->getDataArray(m_selectedArray)->setEffect(m_selectedLocation, 7, boolValueIn);
				break;
			case 17:
				model()->getDataArray(m_selectedArray)->setEffect(m_selectedLocation, 8, boolValueIn);
				break;
		}
	}
	qDebug("setInputAttribValue finished");
}
QColor VectorGraphView::getTextColorFromBaseColor(QColor baseColorIn)
{
	QColor output(255, 255, 255, 255);
	int colorSum = baseColorIn.red() + baseColorIn.green() + baseColorIn.blue();
	// > 127 * 3
	if (colorSum > 382)
	{
		output = QColor(0, 0, 0, 255);
	}
	return output;
}
QColor VectorGraphView::getFillColorFromBaseColor(QColor baseColorIn)
{
	QColor output;
	int colorSum = baseColorIn.red() + baseColorIn.green() + baseColorIn.blue();
	int brighten = 0;
	int alpha = baseColorIn.alpha();
	if (alpha == 0)
	{
		alpha = 255;
	}
	if (std::abs(colorSum - 382) > 191)
	{
		brighten = 45;
	}
	// > 127 * 3
	if (colorSum > 382)
	{
		// (red * 0.6f + avg * 0.4f / 3.0f) * 0.7
		//qDebug("getFillColorFromBaseColor bigger, %d", brighten);
		output = QColor(static_cast<int>(static_cast<float>(baseColorIn.red()) * 0.42f + colorSum * 0.09f) - brighten,
			static_cast<int>(static_cast<float>(baseColorIn.green()) * 0.42f + colorSum * 0.09f) - brighten,
			static_cast<int>(static_cast<float>(baseColorIn.blue()) * 0.42f + colorSum * 0.09f) - brighten, 255);
	}
	else
	{
		// (red * 0.6f + avg * 0.4f / 3.0f) * 1.3
		output = QColor(static_cast<int>(static_cast<float>(baseColorIn.red()) * 0.78f + colorSum * 0.17f) + brighten,
			static_cast<int>(static_cast<float>(baseColorIn.green()) * 0.78f + colorSum * 0.17f) + brighten,
			static_cast<int>(static_cast<float>(baseColorIn.blue()) * 0.78f + colorSum * 0.17f) + brighten, 255);
	}
	return output;
}
QString VectorGraphView::getTextFromDisplayLength(QString textIn, unsigned int displayLengthIn)
{
	// estimating text length
	int charLength = static_cast<int>(m_fontSize * 0.65f);
	QString output = "";
	int targetSize = displayLengthIn / charLength < textIn.size() ? displayLengthIn / charLength : textIn.size();
	if (targetSize != textIn.size())
	{
		for (unsigned int i = 0; i < targetSize; i++)
		{
			if (i + 2 < targetSize)
			{
				output = output + textIn[i];
			}
			else
			{
				output = output + QString(".");
			}
		}
	}
	else
	{
		output = textIn;
	}
	return output;
}
void VectorGraphView::addDefaultActions(QMenu* menu, QString controlDisplayTextIn)
{
	// context menu settings
	menu->addAction(embed::getIconPixmap("reload"),
		controlDisplayTextIn + tr(" remove automation") ,
		this, SLOT(removeAutomation()));
	menu->addSeparator();

	model()->getDataArray(m_selectedArray)->setAutomated(m_selectedLocation, true);
	FloatModel* curAutomationModel = model()->getDataArray(m_selectedArray)->getAutomationModel(m_selectedLocation);
	QString controllerTxt;

	menu->addAction(embed::getIconPixmap("controller"),
		tr("Connect to controller..."),
		this, SLOT(execConnectionDialog()));
	if(curAutomationModel != nullptr && curAutomationModel->controllerConnection() != nullptr)
	{
		
		Controller* cont = curAutomationModel->controllerConnection()->getController();
		if(cont)
		{
			controllerTxt = AutomatableModel::tr( "Connected to %1" ).arg( cont->name() );
		}
		else
		{
			controllerTxt = AutomatableModel::tr( "Connected to controller" );
		}


		QMenu* contMenu = menu->addMenu(embed::getIconPixmap("controller"), controllerTxt);

		contMenu->addAction(embed::getIconPixmap("cancel"),
			tr("Remove connection"),
			this, SLOT(removeConnection()));
	}
}

std::pair<float, float> VectorGraphView::showCoordInputDialog()
{
	std::pair<float, float> curData(0.0f, 0.0f);
	if (m_isSelected == true)
	{
		curData = getSelectedData();

		// show position input dialog
		bool ok;
		double changedX = QInputDialog::getDouble(this, tr("Set value"),
			tr("Please enter a new value between 0 and 100"),
			static_cast<double>(curData.first * 100.0f),
			0.0, 100.0, 2, &ok);
		if (ok == true)
		{
			curData.first = static_cast<float>(changedX) / 100.0f;
		}

		double changedY = QInputDialog::getDouble(this, tr("Set value"),
			tr("Please enter a new value between -100 and 100"),
			static_cast<double>(curData.second * 100.0f),
			-100.0, 100.0, 2, &ok);
		if (ok == true)
		{
			curData.second = static_cast<float>(changedY) / 100.0f;
		}
	}
	return curData;
}
float VectorGraphView::showInputDialog(float curInputValueIn)
{
	float output = 0.0f;

	bool ok;
	double changedPos = QInputDialog::getDouble(this, tr("Set value"),
		tr("Please enter a new value between -100 and 100"),
		static_cast<double>(curInputValueIn * 100.0f),
		-100.0, 100.0, 2, &ok);
	if (ok == true)
	{
		output = static_cast<float>(changedPos) / 100.0f;
	}

	return output;
}

void VectorGraphView::selectData(int mouseXIn, int mouseYIn)
{
	qDebug("selectData");

	m_isSelected = false;

	// trying to select the last selected array
	if (m_isLastSelectedArray == true)
	{
		VectorGraphDataArray* dataArray = model()->getDataArray(m_selectedArray);
		int location = searchForData(mouseXIn, mouseYIn, static_cast<float>(m_pointSize) / width(), dataArray, false);
		if (location > -1)
		{
			m_selectedLocation = location;
			// m_selectedArray - do not set
			m_isSelected = true;
			m_isCurveSelected = false;
			m_isEditingActive = true;
		}
	}

	if (m_isSelected == false)
	{
		m_selectedLocation = 0;
		// m_selectedArray can not be set to 0 in case of
		// m_isLastSelectedArray is active
		// m_selectedArray = 0; - do not reset
		m_isSelected = false;
		m_isCurveSelected = false;
		m_isEditingActive = false;
		//m_isLastSelectedArray = false;

		for (unsigned int i = 0; i < model()->getDataArraySize(); i++)
		{
			VectorGraphDataArray* dataArray = model()->getDataArray(i);
			if (dataArray->getIsSelectable() == true)
			{
				int location = searchForData(mouseXIn, mouseYIn, static_cast<float>(m_pointSize) / width(), dataArray, false);
				if (location > -1)
				{
		//qDebug("selected data!");
					m_selectedLocation = location;
					m_selectedArray = i;
		//qDebug("selected data location: %d, %d", location, i);
					m_isSelected = true;
					m_isCurveSelected = false;
					m_isLastSelectedArray = true;
					m_isEditingActive = true;
					break;
				}
			}
		}
		if (m_isSelected == false)
		{
			for (unsigned int i = 0; i < model()->getDataArraySize(); i++)
			{
				VectorGraphDataArray* dataArray = model()->getDataArray(i);
				if (dataArray->getIsSelectable() == true)
				{
					int location = searchForData(mouseXIn, mouseYIn, static_cast<float>(m_pointSize) / width(), dataArray, true);
					if (location > -1)
					{
			//qDebug("selected data curve!");
						m_selectedLocation = location;
						m_selectedArray = i;
			//qDebug("selected data curve location: %d, %d", location, i);
						m_isSelected = true;
						m_isCurveSelected = true;
						m_isLastSelectedArray = true;
						m_isEditingActive = true;
						break;
					}
				}
			}
		}
	}
	qDebug("selectDataEnd, Arr: %d, location: %d", m_selectedArray, m_selectedLocation);
}

int VectorGraphView::searchForData(int mouseXIn, int mouseYIn, float maxDistanceIn, VectorGraphDataArray* arrayIn, bool curvedIn)
{
	int output = -1;
	float maxDistance = maxDistanceIn * 2.0f;
	qDebug("searchData");

	std::pair<float, float> transformedMouse = mapMousePos(mouseXIn, mouseYIn);

	// unused bool
	bool found = false;
	bool isBefore = false;
	// get the nearest data to the mouse pos (x) in an optimalized way
	int location = arrayIn->getNearestLocation(transformedMouse.first, &found, &isBefore);
	//qDebug("selected location: %d", location);

	// if getNearestLocation was successful
	if (location >= 0)
	{
		float dataX = arrayIn->getX(location);
		float dataY = arrayIn->getY(location);
		// this is set to one when curveIn == true
		// and isBefore == false
		int curvedBefore = 0;
		// if curved then get the closest curved coord
		if (curvedIn == true && arrayIn->size() > 1)
		{
			if (isBefore == false && 1 < location)
			{
				curvedBefore = 1;
			}
			if (location - curvedBefore < arrayIn->size()  - 1)
			{
				std::pair<float, float> curvedDataCoords = mapDataCurvePos(
					arrayIn->getX(location - curvedBefore), arrayIn->getY(location - curvedBefore),
					arrayIn->getX(location - curvedBefore + 1), arrayIn->getY(location - curvedBefore + 1),
					arrayIn->getC(location - curvedBefore));
				dataX = curvedDataCoords.first;
				dataY = curvedDataCoords.second;
			}
		}
		// check distance against x coord
		//qDebug("selected x distance: %f", std::abs(dataX - transformedMouse.first));
		if (std::abs(dataX - transformedMouse.first) <= maxDistance)
		{
			// calculate real distance (x and y)
			float curDistance = getDistance(transformedMouse.first * 2.0f, transformedMouse.second,
				dataX * 2.0f, dataY);

			//qDebug("selected full distance: %f  (%d, %d)", curDistance, location, (location - curvedBefore));
			if (curDistance <= maxDistance)
			{
				//qDebug("search successful");
				output = location - curvedBefore;
			}
			else
			{
				// sometimes the mouse x and the nearest point x
				// coordinates are close but the y coords are not
				// calculating and testing all near by point distances
				int searchStart = 0;
				int searchEnd = arrayIn->size() - 1;
				// from where we need to search the data
				for (int i = location - curvedBefore - 1; i > 0; i--)
				{
					if (std::abs(arrayIn->getX(i) - transformedMouse.first) > maxDistance)
					{
						// if it is curved, then subtract 1
						// add 1 to i because [i] > maxDistanceIn
						searchStart = i + 1 - (i > 0 && curvedIn == true ? 1 : 0);
						break;
					}
				}
				//qDebug("search V2AAA temp, start: %d, end: %d", searchStart, searchEnd);
				// getting where the search needs to end
				for (int i = location - curvedBefore + 1; i < arrayIn->size(); i++)
				{
					if (std::abs(arrayIn->getX(i) - transformedMouse.first) > maxDistance)
					{
						searchEnd = i - 1 - (i > 0 && curvedIn == true ? 1 : 0);
						break;
					}
				}
							//qDebug("search V2, start: %d, end: %d", searchStart, searchEnd);
				// calculating real distances from the point coords
				for (int i = searchStart; i <= searchEnd; i++)
				{
					if (i != location)
					{
						dataX = arrayIn->getX(i);
						dataY = arrayIn->getY(i);
						if (curvedIn == true && arrayIn->size() > 1)
						{
							if (arrayIn->size() - 1 > i)
							{
								std::pair<float, float> curvedDataCoords = mapDataCurvePos(
									arrayIn->getX(i), arrayIn->getY(i), arrayIn->getX(i + 1), arrayIn->getY(i + 1),
									arrayIn->getC(i));
								dataX = curvedDataCoords.first;
								dataY = curvedDataCoords.second;
							}
						}
						curDistance = getDistance(transformedMouse.first * 2.0f, transformedMouse.second,
							dataX * 2.0f, dataY);
			//qDebug("search v2 full distance %d: %f  / %f     y: %f, my: %f    y:%f  size:%ld", i, curDistance, maxDistance, dataY, transformedMouse.second, arrayIn->getY(i), arrayIn->size());
						if (curDistance <= maxDistance)
						{
							//qDebug("search successful V2");
							output = i;
							break;
						}
					}
				}
			}
		}
	}
	qDebug("searchDataEnd");
	return output;
}

} // namespace gui

VectorGraphModel::VectorGraphModel(unsigned int maxLengthIn, Model* parentIn, bool defaultConstructedIn) :
	Model(parentIn, tr("VectorGraphModel"), defaultConstructedIn)
{
	m_maxLength = maxLengthIn;
	m_dataArrays = {};
}

VectorGraphModel::~VectorGraphModel()
{
	qDebug("VectorGraphModel dstc");
	m_dataArrays.clear();

	qDebug("VectorGraphModel dstc end");
}

unsigned int VectorGraphModel::addArray()
{
	VectorGraphDataArray tempArray(
		false, false, false, false, false, false, false,
		false, true, this, getDataArrayNewId());
	m_dataArrays.push_back(tempArray);
	emit dataChanged();
	return m_dataArrays.size() - 1;
}

void VectorGraphModel::delArray(unsigned int locationIn)
{
	qDebug("delArray");
	std::vector<int> effectorArrayLocations(m_dataArrays.size());
	for (unsigned int i = locationIn; i < m_dataArrays.size() - 1; i++)
	{
		//qDebug("copyed [%d] to [%d]", i + 1, i);
		m_dataArrays[i] = m_dataArrays[i + 1];
	}
	m_dataArrays.pop_back();
	// reset effector locations to the correct locations
	for (unsigned int i = 0; i < m_dataArrays.size(); i++)
	{
		effectorArrayLocations[i] = m_dataArrays[i].getEffectorArrayLocation();
		if (m_dataArrays[i].getEffectorArrayLocation() == static_cast<int>(locationIn))
		{
			effectorArrayLocations[i] = -1;
		}
		else if (effectorArrayLocations[i] >= static_cast<int>(locationIn))
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
		//qDebug("delArray end: set effector location: [%d], %d", i, effectorArrayLocations[i]);
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
void VectorGraphModel::updateGraphModel(bool shouldUseGetLastValuesIn)
{
	// connects to external update signal
	emit updateGraphView(shouldUseGetLastValuesIn);
}
void VectorGraphModel::dataArrayClearedEvent(int idIn)
{
	// TODO needs testing
	int location = getDataArrayLocationFromId(idIn);
	emit clearedEvent(location);
	emit dataChanged();
	emit updateGraphView(false);
}

void VectorGraphModel::dataArrayStyleChanged()
{
	emit styleChanged();
}
int VectorGraphModel::getDataArrayLocationFromId(int idIn)
{
	int output = -1;
	if (idIn >= 0)
	{
		for (unsigned int i = 0; i < m_dataArrays.size(); i++)
		{
			if (m_dataArrays[i].getId() == idIn)
			{
				output = i;
				break;
			}
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
	qDebug("saveSettings");

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
		m_dataArrays[i].delUnusedAutomation();

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
				qDebug("saveSettings saved automatinModel %d, %d", i, j);
				QString readLocationB = QString::number(j) + "-";
				automationModels->operator[](j)->saveSettings(doc, me, readLocation + readLocationB + "AutomationModel");
			}
		}
	}
	element.appendChild(me);
}
void VectorGraphModel::loadSettings(const QDomElement& element, const QString& name)
{
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
	qDebug("loadSettings");
	if (curElement.hasAttribute("DataArrayCount") == true)
	{
		qDebug("loadSettings 2");
		unsigned int loadSize = curElement.attribute("DataArrayCount").toInt();
		for (unsigned int i = 0; i < loadSize; i++)
		{
			qDebug("loadSettings 3");
			// getting the start of the attribute name
			QString readLocation = "a" + QString::number(i) + "-";
			if (i < m_dataArrays.size() && curElement.hasAttribute(readLocation + "DataArraySize") == true)
			{
				unsigned int dataArraySize = curElement.attribute(readLocation + "DataArraySize").toInt();
				unsigned int automationSize = curElement.attribute(readLocation + "AutomationSize").toInt();
				// load m_dataArray
				if (dataArraySize > 0)
				{
					qDebug("loadSettings 4");
					m_dataArrays[i].loadDataArray(curElement.attribute(readLocation + "DataArray"), dataArraySize);
				}

				// load automationModelDataArray
				std::vector<FloatModel*>* automationModels = m_dataArrays[i].getAutomationModelArray();
				for (unsigned int j = 0; j < automationSize; j++)
				{
					qDebug("loadSettings 5");
					QString readLocationB = QString::number(j) + "-";
					FloatModel* curModel = new FloatModel(0.0f, -1.0f, 1.0f, 0.01f, this, QString(), false);
					curModel->loadSettings(curElement, readLocation + readLocationB + "AutomationModel");
					automationModels->push_back(curModel);
					qDebug("loaded automatinModel %d, %d, size: %ld", i, j, automationModels->size());
				}
			}
			else
			{
				break;
			}
		}
	}
}
void VectorGraphModel::lockGetValuesAccess()
{
	m_getValuesAccess.lock();
}
void VectorGraphModel::unlockGetValuesAccess()
{
	m_getValuesAccess.unlock();
}
void VectorGraphModel::saveSettings(QDomDocument& doc, QDomElement& element)
{
	saveSettings(doc, element, QString(""));
}
void VectorGraphModel::loadSettings(const QDomElement& element)
{
	loadSettings(element, QString(""));
}
/*
int VectorGraphModel::readLoc(unsigned int startIn, QString dataIn)
{
	int output = -1;
	for (unsigned int i = startIn; i < dataIn.size(); i++)
	{
		if (dataIn[i] == QString("-"))
		{
			output = dataIn.left(i - 1).toInt();
			break;
		}
	}
	return output;
}
*/

// VectorGraphDataArray ------

VectorGraphDataArray::VectorGraphDataArray()
{
	m_isFixedSize = false;
	m_isFixedY = false;
	m_isFixedX = false;
	m_isFixedEndPoints = false;
	m_isSelectable = false;
	m_isEditableAttrib = false;
	m_isAutomatableEffectable = false;
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
	// m_bakedValues;
	// m_needsUpdating;
	// m_automationModelArray;

	m_id = -1;
}

VectorGraphDataArray::VectorGraphDataArray(
	bool isFixedSizeIn, bool isFixedXIn, bool isFixedYIn, bool isNonNegativeIn,
	bool isFixedEndPointsIn, bool isSelectableIn, bool isEditableAttribIn, bool isAutomatableEffectableIn,
	bool isSaveableIn, VectorGraphModel* parentIn, int idIn)
{
	m_isFixedSize = isFixedSizeIn;
	m_isFixedY = isFixedXIn;
	m_isFixedX = isFixedYIn;
	m_isFixedEndPoints = isFixedEndPointsIn;
	m_isSelectable = isSelectableIn;
	m_isEditableAttrib = isEditableAttribIn;
	m_isAutomatableEffectable = isAutomatableEffectableIn;
	m_isSaveable = isSaveableIn;
	m_isNonNegative = isNonNegativeIn;
	
	m_lineColor = QColor(200, 200, 200, 255);
	m_activeColor = QColor(255, 255, 255, 255);
	// fill color is not enabled by default
	// (if alpha = 0)
	m_fillColor = QColor(0, 0, 0, 0);

	m_effectorLocation = -1;

	// m_dataArray;
	m_isDataChanged = false;
	// m_bakedValues;
	// m_needsUpdating;
	// m_automationModelArray;

	m_id = idIn;
	updateConnections(parentIn);
}

VectorGraphDataArray::~VectorGraphDataArray()
{
	qDebug("VectorGraphDataArray dstc");
	m_dataArray.clear();
	m_bakedValues.clear();
	m_needsUpdating.clear();

	for (unsigned int i = 0; i < m_automationModelArray.size(); i++)
	{
		if (m_automationModelArray[i] != nullptr)
		{
			delete m_automationModelArray[i];
		}
	}
	m_automationModelArray.clear();

	qDebug("VectorGraphDataArray dstc end");
}

void VectorGraphDataArray::updateConnections(VectorGraphModel* parentIn)
{
	// call VectorGraphModel signals without qt
	m_parent = parentIn;
	m_id = m_parent->getDataArrayNewId();
	// reseting effectors
	setEffectorArrayLocation(-1, true);
}

void VectorGraphDataArray::setIsFixedSize(bool valueIn)
{
	m_isFixedSize = valueIn;
	getUpdatingFromPoint(-1);
	dataChanged();
}
void VectorGraphDataArray::setIsFixedX(bool valueIn)
{
	m_isFixedX = valueIn;
	getUpdatingFromPoint(-1);
	dataChanged();
}
void VectorGraphDataArray::setIsFixedY(bool valueIn)
{
	m_isFixedY = valueIn;
	getUpdatingFromPoint(-1);
	dataChanged();
}
void VectorGraphDataArray::setIsFixedEndPoints(bool valueIn)
{
	m_isFixedEndPoints = valueIn;
	formatDataArrayEndPoints();
	getUpdatingFromPoint(-1);
	dataChanged();
}
void VectorGraphDataArray::setIsSelectable(bool valueIn)
{
	m_isSelectable = valueIn;
	getUpdatingFromPoint(-1);
	dataChanged();
}
void VectorGraphDataArray::setIsEditableAttrib(bool valueIn)
{
	m_isEditableAttrib = valueIn;
	getUpdatingFromPoint(-1);
	dataChanged();
}
void VectorGraphDataArray::setIsAutomatableEffectable(bool valueIn)
{
	m_isAutomatableEffectable = valueIn;
	if (valueIn == false)
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
void VectorGraphDataArray::setIsSaveable(bool valueIn)
{
	m_isSaveable = valueIn;
}
void VectorGraphDataArray::setIsNonNegative(bool valueIn)
{
	m_isNonNegative = valueIn;
	getUpdatingFromPoint(-1);
	dataChanged();
}
void VectorGraphDataArray::setLineColor(QColor colorIn)
{
	m_lineColor = colorIn;
	styleChanged();
}
void VectorGraphDataArray::setActiveColor(QColor colorIn)
{
	m_activeColor = colorIn;
	styleChanged();
}
void VectorGraphDataArray::setFillColor(QColor colorIn)
{
	m_fillColor = colorIn;
	styleChanged();
}
void VectorGraphDataArray::setAutomatedColor(QColor colorIn)
{
	m_automatedColor = colorIn;
	styleChanged();
}
bool VectorGraphDataArray::setEffectorArrayLocation(int locationIn, bool callDataChangedIn)
{
	qDebug("setEffectorArrayLocation start");
	bool found = true;
	if (locationIn >= 0)
	{
		// if there is no valid id
		if (m_id < 0)
		{
			m_id = m_parent->getDataArrayNewId();
		}
		qDebug("setEffectorArrayLocation cur_id %d", m_id);
		int arrayLocation = locationIn;
		found = false;
		// checking if the effector chain has this dataArray in it
		for (unsigned int i = 0; i < m_parent->getDataArraySize(); i++)
		{
			int arrayId = m_parent->getDataArray(arrayLocation)->getId();
			arrayLocation = m_parent->getDataArray(arrayLocation)->getEffectorArrayLocation();
			if(arrayId == m_id)
			{
				found = true;
				break;
			}
			if (arrayLocation == -1)
			{
				break;
			}
		}
		// if the effector chain does not contain this dataArray
		if (found == false)
		{
			m_effectorLocation = locationIn;
			getUpdatingFromPoint(-1);
			if (callDataChangedIn == true)
			{
				dataChanged();
			}
		}
	}
	else
	{
		if (m_effectorLocation != -1)
		{
			m_effectorLocation = -1;
			getUpdatingFromPoint(-1);
			if (callDataChangedIn == true)
			{
				dataChanged();
			}
		}
	}
	return !found;
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
bool VectorGraphDataArray::getIsAutomatableEffectable()
{
	return m_isAutomatableEffectable;
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
int VectorGraphDataArray::getId()
{
	return m_id;
}


// array:

int VectorGraphDataArray::add(float xIn)
{
	int location = -1;
	if (m_isFixedSize == false && m_dataArray.size() < m_parent->getMaxLength())
	{
	qDebug("add 1. success");
		bool found = false;
		bool isBefore = false;
		location = getNearestLocation(xIn, &found, &isBefore);
		if (found == false)
		{
	qDebug("add 2. success, nearest: %d", location);
			int targetLocation = -1;
			bool dataChangedVal = false;
			// if getNearestLocation returned a value
			if (location >= 0)
			{
	qDebug("add 3. success, nearest: %d", location);
				targetLocation = location;
				// slide the new data if the closest data x is bigger
				// (done for swaping)
				if (isBefore == true)
				{
					// we are adding one value, so dataArray.size() will be a valid location
					if (targetLocation < m_dataArray.size())
					{
						targetLocation++;
					}
				}
				m_dataArray.push_back(VectorGraphPoint(xIn, 0.0f));
	qDebug("add 4. success, target: %d", targetLocation);
				swap(m_dataArray.size() - 1, targetLocation, true);
				dataChangedVal = true;
			}
			else if (m_dataArray.size() <= 0)
			{
	qDebug("add 5. success");
				m_dataArray.push_back(VectorGraphPoint(xIn, 0.0f));
				targetLocation = 0;
				dataChangedVal = true;
			}
	qDebug("add size: %ld", m_dataArray.size());
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
	}
	return location;
}

void VectorGraphDataArray::del(unsigned int locationIn)
{
	if (m_isFixedSize == false && locationIn < m_dataArray.size())
	{
		// deleting the points automationModel
		delAutomationModel(m_dataArray[locationIn].m_automationModel, true);
		// swaping the point to the last location
		// in m_dataArray
		swap(locationIn, m_dataArray.size() - 1, true);
		m_dataArray.pop_back();
		if (locationIn == 0 || locationIn == m_dataArray.size())
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
}

void VectorGraphDataArray::formatArray(std::vector<std::pair<float, float>>* dataArrayIn, bool clampIn, bool rescaleIn, bool sortIn, bool callDataChangedIn)
{
	if (rescaleIn == true)
	{
		// scale
		float minX = 0.0f;
		float maxX = 1.0f;
		float minY = -1.0f;
		float maxY = 1.0f;
		for (unsigned int i = 0; i < dataArrayIn->size(); i++)
		{
			if (dataArrayIn->operator[](i).first < minX)
			{
				minX = dataArrayIn->operator[](i).first;
			}
			if (dataArrayIn->operator[](i).first > maxX)
			{
				maxX = dataArrayIn->operator[](i).first;
			}
			if (dataArrayIn->operator[](i).second < minY)
			{
				minY = dataArrayIn->operator[](i).second;
			}
			if (dataArrayIn->operator[](i).second > maxY)
			{
				maxY = dataArrayIn->operator[](i).second;
			}
		}
		//qDebug("formatArray 1: minx: %f maxx: %f miny: %f maxy: %f", minX, maxX, minY, maxY);
		maxX = (maxX - minX);
		minX = -minX;
		maxY = (maxY - minY) * 0.5f;
		minY = -minY;
		//qDebug("formatArray 2: minx: %f maxx: %f miny: %f maxy: %f", minX, maxX, minY, maxY);
		if (minX != 0.0f || maxX != 1.0f)
		{
			for (unsigned int i = 0; i < dataArrayIn->size(); i++)
			{
				dataArrayIn->operator[](i).first = (dataArrayIn->operator[](i).first + minX) / maxX;
			}
		}
		if (minY != -1.0f || maxY != 1.0f)
		{
			for (unsigned int i = 0; i < dataArrayIn->size(); i++)
			{
				dataArrayIn->operator[](i).second = (dataArrayIn->operator[](i).second + minY) / maxY - 1.0f;
			}
		}
	}
	if (clampIn == true || rescaleIn == true)
	{
		// clamp
		for (unsigned int i = 0; i < dataArrayIn->size(); i++)
		{
			if (dataArrayIn->operator[](i).first < 0.0f)
			{
				dataArrayIn->operator[](i).first = 0.0f;
			}
			if (dataArrayIn->operator[](i).first > 1.0f)
			{
				dataArrayIn->operator[](i).first = 1.0f;
			}
			if (dataArrayIn->operator[](i).second < -1.0f)
			{
				dataArrayIn->operator[](i).second = -1.0f;
			}
			if (dataArrayIn->operator[](i).second > 1.0f)
			{
				dataArrayIn->operator[](i).second = 1.0f;
			}
		}
		formatDataArrayEndPoints();
	}

	// sort
	if (sortIn == true)
	{
		std::sort(dataArrayIn->begin(), dataArrayIn->end(),
			[](std::pair<float, float> a, std::pair<float, float> b)
			{
				return a.first < b.first;
			});
	}

	// delete duplicates
	float lastPos = -1.0f;
	if (dataArrayIn->size() > 0)
	{
		lastPos = dataArrayIn->operator[](0).first;
	}
	for (unsigned int i = 1; i < dataArrayIn->size(); i++)
	{
		if (dataArrayIn->operator[](i).first == lastPos)
		{
			del(i);
		}
		else
		{
			lastPos = dataArrayIn->operator[](i).first;
		}
	}
	// calling clearedEvent is not needed
	// because all of the values can not be cleared here
	getUpdatingFromPoint(-1);
	if (callDataChangedIn == true)
	{
		dataChanged();
	}
}

int VectorGraphDataArray::getLocation(float xIn)
{
	bool found = false;
	bool isBefore = false;
	int location = getNearestLocation(xIn, &found, &isBefore);
	if (found == false)
	{
		return -1;
	}
	return location;
}

int VectorGraphDataArray::getNearestLocation(float xIn, bool* foundOut, bool* isBeforeOut)
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
	//qDebug("getNearestLocation, mid: %d, start: %d, end: %d", mid, start, end);
	//qDebug("getNearestLocation, val: %f, pos: %f", m_dataArray[mid].m_x, xIn);
			if (m_dataArray[mid].m_x == xIn)
			{
				*foundOut = true;
				*isBeforeOut = false;
				return mid;
			}
			else if (m_dataArray[mid].m_x < xIn)
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
		if (m_dataArray[mid].m_x > xIn && mid > 0)
		{
			mid = mid - 1;
		}
		if (mid + 1 < m_dataArray.size() &&
			std::abs(m_dataArray[mid].m_x - xIn) >
			std::abs(m_dataArray[mid + 1].m_x - xIn))
		{
			outputDif = 1;
			//*isBeforeOut = false;
		}
	//qDebug("getNearestLocation, outputDif: %d", outputDif);
		*foundOut = false;
		*isBeforeOut = xIn >= m_dataArray[mid + outputDif].m_x;
		return mid + outputDif;
	}
	//qDebug("getNearestLocation, xIn: %f", xIn);
	*foundOut = false;
	*isBeforeOut = false;
	return -1;
}

std::vector<float> VectorGraphDataArray::getValues(unsigned int countIn)
{
	qDebug("getValuesA1");
	m_parent->lockGetValuesAccess();
	std::vector<float> output = getValues(countIn, nullptr, nullptr);
	m_parent->unlockGetValuesAccess();
	qDebug("getValuesA2, size: %ld", output.size());
	qDebug("getValuesA3 finished");
	return output;
}
std::vector<float> VectorGraphDataArray::getLastValues()
{
	return m_bakedValues;
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

void VectorGraphDataArray::setDataArray(std::vector<std::pair<float, float>>* dataArrayIn,
	bool isCurvedIn, bool clearIn, bool clampIn, bool rescaleIn, bool sortIn, bool callDataChangedIn)
{
	qDebug("setDataArray size: %ld", dataArrayIn->size());
	if (clearIn == true)
	{
		m_dataArray.clear();
	}
	m_dataArray.resize(dataArrayIn->size());
	if (m_dataArray.size() == 0)
	{
		clearedEvent();
	}
	if (clampIn == true || rescaleIn == true || sortIn == true)
	{
		formatArray(dataArrayIn, clampIn, rescaleIn, sortIn, false);
	}
	bool noneBefore = true;
	bool isNegativeBefore = false;
	for (unsigned int i = 0; i < m_dataArray.size(); i++)
	{
		//qDebug("setDataArray 1, x: %f, y: %f", dataArrayIn->operator[](i).first, dataArrayIn->operator[](i).second);
		m_dataArray[i].m_x = dataArrayIn->operator[](i).first;
		m_dataArray[i].m_y = dataArrayIn->operator[](i).second;
		// calculating curves
		if (isCurvedIn == true && i > 0)
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
			//qDebug("setDataArray curve: %f", m_dataArray[i].m_c);
		}
	}
	// the whole m_dataArray needs to be updated
	getUpdatingFromPoint(-1);
	if (callDataChangedIn == true)
	{
		dataChanged();
	}
}
void VectorGraphDataArray::setDataArray(std::vector<float>* dataArrayIn,
	bool isCurvedIn, bool clearIn, bool clampIn, bool rescaleIn, bool callDataChangedIn)
{
	std::vector<std::pair<float, float>> convertedDataArray(dataArrayIn->size());
	float stepSize = 1.0f / static_cast<float>(convertedDataArray.size());
	for (unsigned int i = 0; i < dataArrayIn->size(); i++)
	{
		convertedDataArray[i].first = i * stepSize;
		convertedDataArray[i].second = dataArrayIn->operator[](i);
	}
	setDataArray(&convertedDataArray, isCurvedIn, clearIn, clampIn, rescaleIn, false, callDataChangedIn);
}

unsigned int VectorGraphDataArray::setX(unsigned int locationIn, float xIn)
{
	int location = locationIn;
	if (m_isFixedX == false && xIn <= 1.0f)
	{
		bool found = false;
		bool isBefore = false;
		location = getNearestLocation(xIn, &found, &isBefore);
		// if an other point was not found exactly at xIn
		// and if dataArray end points are changeable
		if (found == false && ((m_isFixedEndPoints == true &&
			locationIn < m_dataArray.size() - 1 && locationIn > 0) ||
			m_isFixedEndPoints == false))
		{
			int targetLocation = location;
			// bool dataChangedVal = false;
			// if getNearestLocation returned a value
			if (location >= 0)
			{
	qDebug("set 3. success, location: %d", targetLocation);
				if (location < locationIn && isBefore == true)
				{
					if (targetLocation + 1 < m_dataArray.size())
					{
						targetLocation++;
					}
				}
				else if (location > locationIn && isBefore == false)
				{
					if (targetLocation > 0)
					{
						targetLocation--;
					}
				}
	qDebug("set 4. success, target: %d", targetLocation);
				m_dataArray[locationIn].m_x = xIn;
				swap(locationIn, targetLocation, true);
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
				location = locationIn;
			}
		}
		else
		{
			location = locationIn;
		}
	}
	return location;
}

void VectorGraphDataArray::setY(unsigned int locationIn, float yIn)
{
	if (m_isFixedY == false)
	{
		m_dataArray[locationIn].m_y = yIn;
		getUpdatingFromPoint(locationIn);
		// changes in the position can change lines before
		// so the point before this is updated
		if (locationIn > 0)
		{
			getUpdatingFromPoint(locationIn - 1);
		}
		if (m_isFixedEndPoints == true &&
			(locationIn <= 0 || locationIn >= m_dataArray.size() - 1))
		{
			formatDataArrayEndPoints();
			getUpdatingFromPoint(0);
			getUpdatingFromPoint(m_dataArray.size() - 1);
		}
		dataChanged();
	}
}

void VectorGraphDataArray::setC(unsigned int locationIn, float cIn)
{
	if (m_isEditableAttrib == true)
	{
		m_dataArray[locationIn].m_c = cIn;
		getUpdatingFromPoint(locationIn);
		dataChanged();
	}
}
void VectorGraphDataArray::setValA(unsigned int locationIn, float valueIn)
{
	if (m_isEditableAttrib == true)
	{
		m_dataArray[locationIn].m_valA = valueIn;
		getUpdatingFromPoint(locationIn);
		dataChanged();
	}
}
void VectorGraphDataArray::setValB(unsigned int locationIn, float valueIn)
{
	if (m_isEditableAttrib == true)
	{
		m_dataArray[locationIn].m_valB = valueIn;
		getUpdatingFromPoint(locationIn);
		dataChanged();
	}
}
void VectorGraphDataArray::setType(unsigned int locationIn, unsigned int typeIn)
{
	if (m_isEditableAttrib == true)
	{
		// set the type without changing the automated attribute location
		m_dataArray[locationIn].m_type = typeIn;
		getUpdatingFromPoint(locationIn);
		dataChanged();
	}
}
void VectorGraphDataArray::setAutomatedAttrib(unsigned int locationIn, unsigned int attribLocationIn)
{
	if (m_isAutomatableEffectable == true && m_isEditableAttrib == true)
	{
		// clamp only 4 attributes can be automated (y, c, valA, valB)
		attribLocationIn = attribLocationIn > 3 ? 0 : attribLocationIn;
		// set automated location correctly (effected_location = automatedEffectedLocation % 4)
		m_dataArray[locationIn].m_automatedEffectedAttribLocations = attribLocationIn * 4 + getEffectedAttribLocation(locationIn);

		getUpdatingFromPoint(locationIn);
		// the line before this can get added later
		// in getUpdatingFromAutomation
		// so the point before this is not updated here

		dataChanged();
	}
}
void VectorGraphDataArray::setEffectedAttrib(unsigned int locationIn, unsigned int attribLocationIn)
{
	if (m_isAutomatableEffectable == true && m_isEditableAttrib == true)
	{
		// clamp only 4 attributes can be automated (y, c, valA, valB)
		attribLocationIn = attribLocationIn > 3 ? 0 : attribLocationIn;
		// set effected location correctly
		m_dataArray[locationIn].m_automatedEffectedAttribLocations = attribLocationIn + getAutomatedAttribLocation(locationIn);

		getUpdatingFromPoint(locationIn);
		// if the current point can effect line before it
		// update the point before it
		if (getEffectOnlyPoints(locationIn) == false && locationIn > 0)
		{
			getUpdatingFromPoint(locationIn - 1);
		}
		dataChanged();
	}
}
unsigned int VectorGraphDataArray::getAutomatedAttribLocation(unsigned int locationIn)
{
	return m_dataArray[locationIn].m_automatedEffectedAttribLocations / 4;
}
unsigned int VectorGraphDataArray::getEffectedAttribLocation(unsigned int locationIn)
{
	return m_dataArray[locationIn].m_automatedEffectedAttribLocations % 4;
}
bool VectorGraphDataArray::getEffectOnlyPoints(unsigned int locationIn)
{
	return (m_dataArray[locationIn].m_effectOnlyPoints == true || getEffectedAttribLocation(locationIn) > 0);
}
void VectorGraphDataArray::setEffectOnlyPoints(unsigned int locationIn, bool boolIn)
{
	if (m_isAutomatableEffectable == true && m_isEditableAttrib == true)
	{
		if (m_dataArray[locationIn].m_effectOnlyPoints != boolIn)
		{
			// getEffectOnlyPoints does not return m_effecteOnlyPoints
			bool dataChangedValue = getEffectOnlyPoints(locationIn);
			m_dataArray[locationIn].m_effectOnlyPoints = boolIn;
			// this change does effect the main output if this
			// data array is an effector of an other so dataChanged() 
			// and getUpdatingFromPoint is called
			if (dataChangedValue != getEffectOnlyPoints(locationIn))
			{
				getUpdatingFromPoint(locationIn);
				// if the current point can effect line before it
				// update the point before it
				if (getEffectedAttribLocation(locationIn) <= 0&& locationIn > 0)
				{
					getUpdatingFromPoint(locationIn - 1);
				}
			}
			dataChanged();
		}
	}
}
bool VectorGraphDataArray::getEffect(unsigned int locationIn, unsigned int effectNumberIn)
{
	switch (effectNumberIn)
	{
		case 0:
			return m_dataArray[locationIn].m_effectAdd;
			break;
		case 1:
			return m_dataArray[locationIn].m_effectSubtract;
			break;
		case 2:
			return m_dataArray[locationIn].m_effectMultiply;
			break;
		case 3:
			return m_dataArray[locationIn].m_effectDivide;
			break;
		case 4:
			return m_dataArray[locationIn].m_effectPower;
			break;
		case 5:
			return m_dataArray[locationIn].m_effectLog;
			break;
		case 6:
			return m_dataArray[locationIn].m_effectSine;
			break;
		case 7:
			return m_dataArray[locationIn].m_effectClampLower;
			break;
		case 8:
			return m_dataArray[locationIn].m_effectClampUpper;
			break;
	}
	return false;
}
void VectorGraphDataArray::setEffect(unsigned int locationIn, unsigned int effectNumberIn, bool boolIn)
{
	if (m_isAutomatableEffectable == true && m_isEditableAttrib == true)
	{
		switch (effectNumberIn)
		{
			case 0:
				m_dataArray[locationIn].m_effectAdd = boolIn;
				break;
			case 1:
				m_dataArray[locationIn].m_effectSubtract = boolIn;
				break;
			case 2:
				m_dataArray[locationIn].m_effectMultiply = boolIn;
				break;
			case 3:
				m_dataArray[locationIn].m_effectDivide = boolIn;
				break;
			case 4:
				m_dataArray[locationIn].m_effectPower = boolIn;
				break;
			case 5:
				m_dataArray[locationIn].m_effectLog = boolIn;
				break;
			case 6:
				m_dataArray[locationIn].m_effectSine = boolIn;
				break;
			case 7:
				m_dataArray[locationIn].m_effectClampLower = boolIn;
				break;
			case 8:
				m_dataArray[locationIn].m_effectClampUpper = boolIn;
				break;
		}
		getUpdatingFromPoint(locationIn);
		// if the current point can effect line before it
		// update the point before it
		if (getEffectOnlyPoints(locationIn) == false && locationIn > 0)
		{
			getUpdatingFromPoint(locationIn - 1);
		}
		dataChanged();
	}
}
bool VectorGraphDataArray::getIsAutomationValueChanged(unsigned int locationIn)
{
	if (getAutomationModel(locationIn) != nullptr &&
		m_dataArray[locationIn].m_bufferedAutomationValue != getAutomationModel(locationIn)->value())
	{
		m_dataArray[locationIn].m_bufferedAutomationValue = getAutomationModel(locationIn)->value();
		return true;
	}
	return false;
}
void VectorGraphDataArray::setAutomated(unsigned int locationIn, bool isAutomatedIn)
{
	qDebug("setAutomated start");
	if (m_isAutomatableEffectable == true)
	{
		if (isAutomatedIn == true)
		{
			qDebug("setAutomated make");
			// if it is not already automated
			if (m_dataArray[locationIn].m_automationModel == -1)
			{
				m_automationModelArray.push_back(new FloatModel(0.0f, -1.0f, 1.0f, 0.01f, m_parent, QString(), false));
				m_dataArray[locationIn].m_automationModel = m_automationModelArray.size() - 1;
				getUpdatingFromPoint(locationIn);
				dataChanged();
			}
		}
		else
		{
			qDebug("setAutomated delete");

			// dataChanged() is called in this function
			// this function check if the current point has an automationModel
			delAutomationModel(m_dataArray[locationIn].m_automationModel, true);
		}
	}
	qDebug("setAutomated end");
}
FloatModel* VectorGraphDataArray::getAutomationModel(unsigned int locationIn)
{
	if (m_dataArray[locationIn].m_automationModel != -1)
	{
		return m_automationModelArray[m_dataArray[locationIn].m_automationModel];
	}
	return nullptr;
}

// protected:
std::vector<FloatModel*>* VectorGraphDataArray::getAutomationModelArray()
{
	return &m_automationModelArray;
}
void VectorGraphDataArray::delUnusedAutomation()
{
	bool dataChangedVal = false;
	std::vector<int> usedAutomation;
	for (unsigned int i = 0; i < m_dataArray.size(); i++)
	{
		if (m_dataArray[i].m_automationModel != -1)
		{
			usedAutomation.push_back(m_dataArray[i].m_automationModel);
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
			delAutomationModel(i, false);
		}
	}

	// getUpdatingFromPoint() is called in delAutomationModel()
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
void VectorGraphDataArray::loadDataArray(QString dataIn, unsigned int sizeIn)
{
	qDebug("loadDatatArray start");
	int size = 0;
	char* dst = 0;
	base64::decode(dataIn, &dst, &size);

	if (size == sizeIn * sizeof(VectorGraphPoint))
	{
		m_dataArray.resize(sizeIn);

		VectorGraphPoint* points = (VectorGraphPoint*)dst;
		for (unsigned int i = 0; i < sizeIn; i++)
		{
			m_dataArray[i] = points[i];
		}
	}

	delete[] dst;
	qDebug("loadDatatArray end");
}

// private:
void VectorGraphDataArray::delAutomationModel(unsigned int modelLocationIn, bool callDataChangedIn)
{
	if (modelLocationIn != -1)
	{
		FloatModel* curModel = m_automationModelArray[modelLocationIn];

		// copy the last FloatModel* to the current location
		m_automationModelArray[modelLocationIn] =
			m_automationModelArray[m_automationModelArray.size() - 1];

		m_automationModelArray.pop_back();

		// replace all m_auttomationModel-s in the current copyed location with -1
		// replace all last m_automationModel-s to the currently copyed location
		// there should be only 2 points changed but because of safety
		// all of them are checked
		for (unsigned int i = 0; i < m_dataArray.size(); i++)
		{
			if (m_dataArray[i].m_automationModel == modelLocationIn)
			{
				m_dataArray[i].m_automationModel = -1;
				getUpdatingFromPoint(i);
			}
			if (m_dataArray[i].m_automationModel == m_automationModelArray.size())
			{
				m_dataArray[i].m_automationModel = modelLocationIn;
			}
		}
		if (curModel != nullptr)
		{
			delete curModel;
			curModel = nullptr;
		}

		if (callDataChangedIn == true)
		{
			dataChanged();
		}
	}
}

void VectorGraphDataArray::swap(unsigned int locationAIn, unsigned int locationBIn, bool slide)
{
	if (locationAIn != locationBIn)
	{
		if (slide == true)
		{
			qDebug("swap:    -------");
			qDebug("first.: %d, second.: %d", locationAIn, locationBIn);
			
			/*
			for (unsigned int i = 0; i < m_dataArray.size(); i++)
			{
				qDebug("   - i: %d  -  x: %f", i, m_dataArray[i].m_x);
			}
			*/
			
			if (locationAIn < locationBIn)
			{
				VectorGraphPoint swap = m_dataArray[locationAIn];
				for (unsigned int i = locationAIn; i < locationBIn; i++)
				{
					m_dataArray[i] = m_dataArray[i + 1];
				}
				m_dataArray[locationBIn] = swap;
			}
			else
			{
				VectorGraphPoint swap = m_dataArray[locationAIn];
				for (unsigned int i = locationAIn; i > locationBIn; i--)
				{
					m_dataArray[i] = m_dataArray[i - 1];
				}
				m_dataArray[locationBIn] = swap;
			}
			
			/*
			qDebug(" --------- ");
			for (unsigned int i = 0; i < m_dataArray.size(); i++)
			{
				qDebug("   - i: %d  -  x: %f", i, m_dataArray[i].m_x);
			}
			*/
		}
		else
		{
			// normal swap
			VectorGraphPoint swap = m_dataArray[locationAIn];
			m_dataArray[locationAIn] = m_dataArray[locationBIn];
			m_dataArray[locationBIn] = swap;
		}
		getUpdatingFromPoint(locationBIn - 1 > 0 ? locationBIn - 1 : 0);
		getUpdatingFromPoint(locationAIn - 1 > 0 ? locationAIn - 1 : 0);
		getUpdatingFromPoint(locationAIn);
		getUpdatingFromPoint(locationBIn);
		dataChanged();
	}
}
float VectorGraphDataArray::processCurve(float valueBeforeIn, float valueAfterIn, float curveIn, float xIn)
{
	// calculating line curve
	float absCurveIn = std::abs(curveIn);
	float pow = curveIn < 0.0f ? 1.0f - xIn : xIn;
	pow = std::pow(pow, 1.0f - absCurveIn) - pow;

	float output = valueBeforeIn + (valueAfterIn - valueBeforeIn) * xIn;
	output = curveIn > 0.0f ? output + pow * (valueAfterIn - valueBeforeIn) : output - pow * (valueAfterIn - valueBeforeIn);
	// clamp
	if (valueBeforeIn > valueAfterIn)
	{
		output = std::clamp(output, valueAfterIn, valueBeforeIn);
	}
	else
	{
		output = std::clamp(output, valueBeforeIn, valueAfterIn);
	}
	return output;
}

float VectorGraphDataArray::processEffect(unsigned int locationIn, float attribValueIn,
	unsigned int attribLocationIn, float effectValueIn)
{
	// calculating an effect on attribValueIn
	float output = attribValueIn;
	unsigned int attribLocation = getEffectedAttribLocation(locationIn);
	// effects
	if (attribLocationIn == attribLocation)
	{
		if (getEffect(locationIn, 6) == true)
		{
			// sine
			output = output + std::sin(effectValueIn * 100.0f);
		}
		if (getEffect(locationIn, 4) == true && output > 0.0f)
		{
			// power
			output = std::pow(output, effectValueIn);
		}
		else if (getEffect(locationIn, 5) == true && output > 0.0f && effectValueIn != 0.0f)
		{
			// log
			output = std::log(output) / std::log(effectValueIn);
		}

		if (getEffect(locationIn, 2) == true)
		{
			// multiply
			output = output * 5.0f * effectValueIn;
		}
		else if (getEffect(locationIn, 3) == true && effectValueIn != 0.0f)
		{
			// divide
			output = output / 5.0f / effectValueIn;
		}

		if (getEffect(locationIn, 0) == true)
		{
			// add
			output += effectValueIn;
		}
		else if (getEffect(locationIn, 1) == true)
		{
			// subtract
			output -= effectValueIn;
		}

		if (getEffect(locationIn, 7) == true)
		{
			// clamp lower
			output = std::max(effectValueIn, output);
		}
		else if (getEffect(locationIn, 8) == true)
		{
			// clamp upper
			output = std::min(effectValueIn, output);
		}

		// clamp
		output = std::clamp(output, -1.0f, 1.0f);
	}
	return output;
}
float VectorGraphDataArray::processAutomation(float attribValueIn, unsigned int locationIn, unsigned int attribLocationIn)
{
	// adding the automation value to attribValueIn
	float output = 0.0f;
	// if automated
	FloatModel* automationModel = getAutomationModel(locationIn);
	if (automationModel != nullptr)
	{
		unsigned int attribLocation = getAutomatedAttribLocation(locationIn);
		if (attribLocation == attribLocationIn)
		{
			output += automationModel->value();
		}
	}
	output += attribValueIn;
	
	output = std::clamp(output, -1.0f, 1.0f);
	return output;
}

// line type effects:
/*
float VectorGraphDataArray::processLineTypeSine(float xIn, float valAIn, float valBIn, float fadeInStartIn)
{
	return processLineTypeSineB(xIn, valAIn, valBIn, 0.0f, fadeInStartIn);
}
*/
// valA: amp, valB: freq, fadeInStartIn: from what xIn value should the line type fade out
std::vector<float> VectorGraphDataArray::processLineTypeArraySine(std::vector<float>* xIn, unsigned int startIn, unsigned int endIn,
	float valAIn, float valBIn, float fadeInStartIn)
{
	return VectorGraphDataArray::processLineTypeArraySineB(xIn, startIn, endIn,
		valAIn, valBIn, 0.0f, fadeInStartIn);
}
/*
float VectorGraphDataArray::processLineTypeSineB(float xIn, float valAIn, float valBIn, float curveIn, float fadeInStartIn)
{
	// sine
	// 628.318530718f = 100.0f * 2.0f * pi
	float output = valAIn * std::sin(xIn * 628.318530718f * valBIn + curveIn * 100.0f);
	
	// fade in
	if (xIn < fadeInStartIn)
	{
		output = output * xIn / fadeInStartIn;
	}
	// fade out
	if (xIn > 1.0f - fadeInStartIn)
	{
		output = output * (1.0f - xIn) / fadeInStartIn;
	}
	return output;
}
*/
// valA: amp, valB: freq, curveIn: phase
std::vector<float> VectorGraphDataArray::processLineTypeArraySineB(std::vector<float>* xIn, unsigned int startIn, unsigned int endIn,
	float valAIn, float valBIn, float curveIn, float fadeInStartIn)
{
	int count = static_cast<int>(endIn) - static_cast<int>(startIn);
	if (count < 0)
	{
		count = 0;
	}
	float valB = 0.001f + ((valBIn + 1.0f) / 2.0f) * 0.999f;
	std::vector<float> output(count);
	// calculating how many samples are needed to 1 complete wave
	// we have "count" amount of samples and "valB * 100.0f" amount of waves
	int end = static_cast<int>(std::floor(count / (valB * 100.0f)));
	//qDebug("sineB_1, %f, %d", (count / (valB * 100.0f)), end);
	if (count <= 0)
	{
		end = 0;
	}
	else
	{
		end = end > count ? count : end + 1;
	}

	// calculate 1 wave of sine
	for (unsigned int i = 0; i < end; i++)
	{
		// 628.318531f = 100.0f * 2.0f * pi
		// (1 sine wave is 2pi long and we have 1 * 100 * valBIn waves)
		output[i] = valAIn * std::sin(
			xIn->operator[](startIn + i) * 628.318531f * valB + curveIn * 100.0f);
	}
	//qDebug("sineB_2");
	// copy the first wave until the end
	for (unsigned int i = end; i < count; i++)
	{
		//qDebug("sineB_2.5: i: %d, %d, %d", (i - end), end, i);
		output[i] =	output[i - end];
	}
	//qDebug("sineB_3");
	// fade in
	for (unsigned int i = 0; i < count; i++)
	{
		float x = xIn->operator[](startIn + i);
		if (x > fadeInStartIn)
		{
			break;
		}
		output[i] = output[i] * x / fadeInStartIn;
	}
	//qDebug("sineB_4");
	// fade out
	for (unsigned int i = count - 1; i >= 0; i--)
	{
		float x = 1.0f - xIn->operator[](startIn + i);
		if (x > fadeInStartIn)
		{
			break;
		}
		output[i] = output[i] * x / fadeInStartIn;
	}
	//qDebug("sineB_5");
	return output;
}
/*
float VectorGraphDataArray::processLineTypePeak(float xIn, float valAIn, float valBIn, float curveIn, float fadeInStartIn)
{
	// peak
	float output = std::pow((curveIn + 1.0f) * 0.2f + 0.01f, std::abs(xIn - (valBIn + 1.0f) * 0.5f) * 10.0f) * valAIn;

	// fade in
	if (xIn < fadeInStartIn)
	{
		output = output * xIn / fadeInStartIn;
	}
	// fade out
	if (xIn > 1.0f - fadeInStartIn)
	{
		output = output * (1.0f - xIn) / fadeInStartIn;
	}
	return output;
}
*/

// valA: amp, valB: x coord, curve: width
std::vector<float> VectorGraphDataArray::processLineTypeArrayPeak(std::vector<float>* xIn, unsigned int startIn, unsigned int endIn,
	float valAIn, float valBIn, float curveIn, float fadeInStartIn)
{
	int count = static_cast<int>(endIn) - static_cast<int>(startIn);
	if (count < 0)
	{
		count = 0;
	}
	std::vector<float> output(count);
	for (unsigned int i = 0; i < count; i++)
	{
		output[i] = std::pow((curveIn + 1.0f) * 0.2f + 0.01f,
			std::abs(xIn->operator[](startIn + i) - (valBIn + 1.0f) * 0.5f) * 10.0f) * valAIn;
	}
	// fade in
	for (unsigned int i = 0; i < count; i++)
	{
		float x = xIn->operator[](startIn + i);
		if (x > fadeInStartIn)
		{
			break;
		}
		output[i] = output[i] * x / fadeInStartIn;
	}
	// fade out
	for (unsigned int i = count - 1; i >= 0; i--)
	{
		float x = 1.0f - xIn->operator[](startIn + i);
		if (x > fadeInStartIn)
		{
			break;
		}
		output[i] = output[i] * x / fadeInStartIn;
	}
	return output;
}
/*
float VectorGraphDataArray::processLineTypeSteps(float xIn, float yIn, float valAIn, float valBIn, float fadeInStartIn)
{
}
*/
// y: calculate steps from, valA: y count, valB: curve
std::vector<float> VectorGraphDataArray::processLineTypeArraySteps(std::vector<float>* xIn, unsigned int startIn, unsigned int endIn,
	std::vector<float>* yIn, float valAIn, float valBIn, float fadeInStartIn)
{
	int count = static_cast<int>(endIn) - static_cast<int>(startIn);
	if (count < 0)
	{
		count = 0;
	}
	std::vector<float> output(count);

	float stepCount = (1.0f + valAIn) / 2.0f * 19.0f + 1.0f;
	//qDebug("stepsA - stepCount = %f", stepCount);
	for (unsigned int i = 0; i < count; i++)
	{
		float y = yIn->operator[](startIn + i) + 1.0f;
		float diff = std::round(y * stepCount) - y * stepCount;
		float smooth = 1.0f - std::abs(diff) * (1.0f - (valBIn + 1.0f) / 2.0f) * 2.0f;
		output[i] = diff / stepCount * smooth;
	}

	// fade in
	for (unsigned int i = 0; i < count; i++)
	{
		float x = xIn->operator[](startIn + i);
		if (x > fadeInStartIn)
		{
			break;
		}
		output[i] = output[i] * x / fadeInStartIn;
	}
	// fade out
	for (unsigned int i = count - 1; i >= 0; i--)
	{
		float x = 1.0f - xIn->operator[](startIn + i);
		if (x > fadeInStartIn)
		{
			break;
		}
		output[i] = output[i] * x / fadeInStartIn;
	}
	return output;
}
/*
float VectorGraphDataArray::processLineTypeRandom(float xIn, float valAIn, float valBIn, float curveIn, float fadeInStartIn)
{
}
*/
// valA: amp, valB: random number count, curveIn: seed
std::vector<float> VectorGraphDataArray::processLineTypeArrayRandom(std::vector<float>* xIn, unsigned int startIn, unsigned int endIn,
		float valAIn, float valBIn, float curveIn, float fadeInStartIn)
{
	int count = static_cast<int>(endIn) - static_cast<int>(startIn);
	if (count < 0)
	{
		count = 0;
	}
	std::vector<float> output(count);
	std::vector<float> randomValues(static_cast<int>(50.0f * (valBIn + 1.0f)) * 2);

	float blend = 10.0f + curveIn * 10.0f;
	int randomSeed = static_cast<int>(blend);
	blend = blend - randomSeed;
	std::srand(randomSeed);

	// getting the random values
	// generating 2 seeds and blending in between them
	for (unsigned int i = 0; i < randomValues.size() / 2; i++)
	{
		randomValues[i] = std::fmod((static_cast<float>(rand()) / 10000.0f), 2.0f) - 1.0f;
	}
	std::srand(randomSeed + 1);
	for (unsigned int i = randomValues.size() / 2; i < randomValues.size(); i++)
	{
		randomValues[i] = std::fmod((static_cast<float>(rand()) / 10000.0f), 2.0f) - 1.0f;
	}

	// blending
	if (randomValues.size() > 0)
	{
		// real size
		float size = static_cast<float>(randomValues.size() / 2);
		for (unsigned int i = 0; i < count; i++)
		{
			float randomValueX = xIn->operator[](startIn + i) * size;
			float randomValueLocation = std::floor(randomValueX);
			output[i] = -((randomValueX - randomValueLocation) - 1.0f) * (randomValueX - randomValueLocation) * 4.0f *
				(randomValues[static_cast<int>(randomValueLocation)] * (1.0f - blend)  + randomValues[static_cast<int>(randomValueLocation + size)] * blend) *
				valAIn;
		}
	}

	return output;
}

void VectorGraphDataArray::getUpdatingFromEffector(std::vector<unsigned int>* updatingValuesIn)
{
	VectorGraphDataArray* effector = m_parent->getDataArray(m_effectorLocation);
	for (unsigned int i = 0; i < updatingValuesIn->size(); i++)
	{
		// since updatingValuesIn is a sorted list, we can get the end
		// location and update everithing between them
		// starting effector location is i, end effector location is updatingEnd
		unsigned int updatingEnd = i;
		for (unsigned int j = i + 1; j < updatingValuesIn->size(); j++)
		{
			// we can not skip gaps because
			// every updatingValuesIn point effects their line only
			// (the line that starts with the point)
			if (updatingValuesIn->operator[](updatingEnd) + 1 >=
					updatingValuesIn->operator[](j))
			{
				updatingEnd = j;
				qDebug("getUpdatingFromEffector new updatingEnd: %d, i: %d", updatingEnd, i);
			}
			else
			{
				qDebug("getUpdatingFromEffector updatingEnd %d brake: %d < %d [j = %d]", updatingEnd, (updatingValuesIn->operator[](updatingEnd) + 1), updatingValuesIn->operator[](j), j);
				break;
			}
		}
		// getting the point that comes after updatingEnd
		int updatingEndSlide = 0;
		if (updatingEnd + 1 < effector->size())
		{
			qDebug("getUpdatingFromEffector updatingEndSlide = 1");
			updatingEndSlide = 1;
		}

		// translating the effector data array locations to m_dataArray locations
		bool found = false;
		bool isBefore = false;
		int locationBefore = getNearestLocation(effector->getX(updatingValuesIn->operator[](i)), &found, &isBefore);
		qDebug("getUpdatingFromEffector getNearestLocation before: %d, i: %d", locationBefore, i);
		if (isBefore == false && locationBefore >= 0 && getEffectOnlyPoints(locationBefore) == true)
		{
			qDebug("getUpdatingFromEffector locationBefore = %d - 1", locationBefore);
			// lines before could be effected eaven if the current nearest point
			// can only be effected (its line can not be effected)
			// so subtract 1
			// remember points control the line after (connected to) them
			// but in this case changes in the points position can effect the line before it
			locationBefore--;
		}
		// clamp
		locationBefore = locationBefore < 0 ? 0 :
			m_dataArray.size() - 1 < locationBefore ? m_dataArray.size() - 1 : locationBefore;
		isBefore = false;
		int locationAfter = getNearestLocation(effector->getX(updatingValuesIn->operator[](updatingEnd) + updatingEndSlide), &found, &isBefore);
		qDebug("getUpdatingFromEffector getNearestLocation after: %d, updatingEnd: %d (+ %d), ex: %f, dx: %f", locationAfter, updatingEnd, updatingEndSlide, effector->getX(updatingValuesIn->operator[](updatingEnd) + updatingEndSlide), m_dataArray[locationAfter].m_x);
		if (isBefore == false)
		{
			qDebug("getUpdatingFromEffector locationAfter = %d - 1", locationAfter);
			// if the nearest point is after ([updatingEnd] + upadtingEndSlide) (where updating ends)
			locationAfter--;
		}
		// updating everything before if i -> 0
		if (updatingValuesIn->operator[](i) == 0)
		{
			qDebug("getUpdatingFromEffector updating everything before");
			locationBefore = 0;
		}
		// if updatingEnd is the last point in effecor, then
		// update everithing after
		if (updatingValuesIn->operator[](updatingEnd) + updatingEndSlide + 1 >= effector->size())
		{
			qDebug("getUpdatingFromEffector updating everything after");
			locationAfter = m_dataArray.size() - 1;
		}
		// clamp
		locationAfter = locationAfter < 0 ? 0 :
			m_dataArray.size() - 1 < locationAfter ? m_dataArray.size() - 1 : locationAfter;

		qDebug("getUpdatingFromEffector start: %d, end: %d", locationBefore, locationAfter);
		// adding the values between locationBefore, locationAfter
		for (unsigned int j = locationBefore; j <= locationAfter; j++)
		{
			if (isEffectedPoint(j) == true)
			{
				qDebug("getUpdatingFromEffector: [%d] -> %d", i, j);
				m_needsUpdating.push_back(j);
			}
		}
		if (i < updatingEnd)
		{
			i = updatingEnd;
		}
	}
}
void VectorGraphDataArray::getUpdatingFromPoint(int locationIn)
{
	// changes in position need to cause updates before the changed point
	// changes in m_dataArray.size() needs to cause getUpdatingFromPoint(-1)
	if (m_isDataChanged == false && locationIn >= 0)
	{
		m_needsUpdating.push_back(locationIn);
		if (m_needsUpdating.size() > m_dataArray.size() * 3)
		{
			m_isDataChanged = true;
		}
	}
	else if (locationIn < 0)
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
			qDebug("getUpdatingFromAutomation: %d, attrib location: %d", i, getAutomatedAttribLocation(i));
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
	// selecting only original values
	// TODO this might be faster if the sort happens before
	std::vector<unsigned int> originalValues;
	if (m_needsUpdating.size() > 0)
	{
		originalValues.push_back(m_needsUpdating[0]);
	}

	// Debug testing
	for (unsigned int i = 0; i < m_needsUpdating.size(); i++)
	{
		qDebug("getUpatingOriginals: [%d] -> %d", i, m_needsUpdating[i]);
	}

	for (unsigned int i = 1; i < m_needsUpdating.size(); i++)
	{
		bool found = false;
		for (unsigned int j = 0; j < originalValues.size(); j++)
		{
			if (m_needsUpdating[i] == originalValues[j])
			{
				found = true;
				break;
			}
		}
		if (found == false)
		{
			originalValues.push_back(m_needsUpdating[i]);
		}
	}

	// sorting the array
	// this is done to optimize the functions that use
	// m_needsUpdating in getValues()
	std::sort(originalValues.begin(), originalValues.end(),
		[](unsigned int a, unsigned int b)
		{
			return a < b;
		});

	// removing invalid locations
	// because sometimes deleted locations can be in originalValues
	for (unsigned int i = 0; i < originalValues.size(); i++)
	{
		if (originalValues[i] >= m_dataArray.size())
		{
			originalValues.resize(i);
			break;
		}
	}
	

	m_needsUpdating = originalValues;
	/*
	for (unsigned int i = 0; i < m_needsUpdating.size(); i++)
	{
		qDebug("getUpatingOriginals final: [%d] -> %d", i, m_needsUpdating[i]);
	}
	*/
}
std::vector<float> VectorGraphDataArray::getValues(unsigned int countIn, bool* isChangedOut, std::vector<unsigned int>* updatingValuesOut)
{
	bool effectorIsChanged = false;
	//std::shared_ptr<std::vector<unsigned int>> effectorUpdatingValues = std::make_shared<std::vector<unsigned int>>();
	std::vector<unsigned int> effectorUpdatingValues;
	std::vector<float> effectorOutput;
	std::vector<float> outputXLocations(countIn);
	bool isEffected = m_effectorLocation >= 0;
	if (isEffected == true)
	{
		effectorOutput = m_parent->getDataArray(m_effectorLocation)->getValues(countIn, &effectorIsChanged, &effectorUpdatingValues);
	}
	else
	{
		effectorOutput.resize(countIn);
	}
	qDebug("getValuesB1, size: %ld    - id: %d", outputXLocations.size(), m_id);

	m_isDataChanged = m_isDataChanged || countIn != m_bakedValues.size();

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
	if (m_isDataChanged == false && countIn == m_bakedValues.size())
	{
		if (isEffected == true && effectorUpdatingValues.size() > 0 &&
			(effectorIsChanged == false || effectedCount > 0))
		{
			// effectorUpdatingValues needs to be sorted
			// before use (in this case it is already sorted)
			getUpdatingFromEffector(&effectorUpdatingValues);
		}
	qDebug("getValuesB2");
		getUpdatingFromAutomation();
		// sort and select only original
		// values
		getUpdatingOriginals();
	qDebug("getValuesB3");
	}
	else
	{
		if (countIn != m_bakedValues.size())
		{
			m_bakedValues.resize(countIn);
		}
		m_needsUpdating.resize(m_dataArray.size());
		for (unsigned int i = 0; i < m_needsUpdating.size(); i++)
		{
			m_needsUpdating[i] = i;
		}
		qDebug("getValuesB4, needsUpdating size: %ld", m_needsUpdating.size());
	}

	float stepSize = 1.0f / static_cast<float>(countIn);
	// calculating point data and lines
	if (m_needsUpdating.size() > 0 && m_bakedValues.size() > 0)
	{
		// calculating relative X locations (in lines) of the output values
		// for later use in the line calculations
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
					outputXLocations[j] = (stepSize * static_cast<float>(j) - m_dataArray[i].m_x) / (m_dataArray[i + 1].m_x - m_dataArray[i].m_x);
					//qDebug("getValuesB outputXLocations: [%d] [%d] %f", i, j, outputXLocations[j]);
				}
			}
		}
		// getting effectorDataArray pointer
		VectorGraphDataArray* effector = nullptr;
		if (m_effectorLocation >= 0 && m_parent->getDataArray(m_effectorLocation)->size() > 0)
		{
			effector = m_parent->getDataArray(m_effectorLocation);
		}

		// m_dataArray[i] location in effecor m_dataArray, next location in effecor m_dataArray,
		//std::vector<std::pair<unsigned int, unsigned int>> effectorData;
		//getValuesLocations(effector, &effectorData);
		/*
		for (unsigned int j = 0; j < effectorData.size(); j++)
		{
			qDebug("getValuesB6.4, [%d] %d, %d", j, effectorData[j].first, effectorData[j].second);
		}
		*/
		qDebug("getValuesB6, updatingsize: %ld", m_needsUpdating.size());

		// calculate final lines
		for (unsigned int i = 0; i < m_needsUpdating.size(); i++)
		{
			getValuesUpdateLines(effector, &effectorOutput, &outputXLocations, i, stepSize);
		}
		//effectorData.clear();
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

	m_isDataChanged = false;
	qDebug("getValuesB9");
	return m_bakedValues;
}
// unused function, might be useful later
/*
void VectorGraphDataArray::getValuesLocations(VectorGraphDataArray* effectorIn, std::vector<std::pair<unsigned int, unsigned int>>* effectorDataOut)
{
	if (effectorIn != nullptr)
	{
		effectorDataOut->resize(m_needsUpdating.size());
		for (unsigned int i = 0; i < m_needsUpdating.size(); i++)
		{
			bool found = false;
			bool isBefore = false;
			int curLocation = effectorIn->getNearestLocation(m_dataArray[m_needsUpdating[i]].m_x, &found, &isBefore);
			if (curLocation >= 0)
			{
				curLocation = isBefore == false ? (curLocation > 0 ? curLocation - 1 : curLocation) : curLocation;
				// location
				effectorDataOut->operator[](i).first = curLocation;
qDebug("getValuesC5.1, [%d] set to: %d", i, curLocation);
				// next location
				effectorDataOut->operator[](i).second = curLocation;
				// if the location of this is the next location for before this
				if (i > 0 && m_needsUpdating[i - 1] == m_needsUpdating[i] - 1)
				{
qDebug("getValuesC5.2, [%d - 1] changed to: %d", i, curLocation);
					effectorDataOut->operator[](i- 1).second = curLocation;
				}
			}
		}
qDebug("getValuesC5");
		// getting the missing next location values
		for (unsigned int i = 0; i < m_needsUpdating.size(); i++)
		{
			// if there is a gap in m_needsUpdating
			// (next needsUpdating point is not the point after this one in m_dataArray)
			if (i + 1 < m_needsUpdating.size() && m_needsUpdating[i] + 1 < m_dataArray.size() &&
				m_needsUpdating[i + 1] != m_needsUpdating[i] + 1)
			{
				bool found = false;
				bool isBefore = false;
				int curLocation = effectorIn->getNearestLocation(m_dataArray[m_needsUpdating[i] + 1].m_x, &found, &isBefore);
				if (curLocation >= 0)
				{
					curLocation = isBefore == false ? (curLocation > 0 ? curLocation - 1 : curLocation) : curLocation;
					// next location
qDebug("getValuesC5.3, [%d] set after to: %d", i, curLocation);
					effectorDataOut->operator[](i).second = curLocation;
				}
			}
		}
		// getting the last updated point's next location value
		if (m_needsUpdating[m_needsUpdating.size() - 1] + 1 >= m_dataArray.size())
		{
			effectorDataOut->operator[](m_needsUpdating.size() - 1).second = effectorIn->size() - 1;
		}
	}
}
*/
void VectorGraphDataArray::getValuesUpdateLines(VectorGraphDataArray* effectorIn, std::vector<float>* effectorOutputIn,
	std::vector<float>* outputXLocationsIn, unsigned int iIn, float stepSizeIn)
{
	qDebug("getValuesD6.1  m_needsUpdating[%d]: %d", iIn, m_needsUpdating[iIn]);
	unsigned int effectYLocation = static_cast<unsigned int>
		(std::ceil(m_dataArray[m_needsUpdating[iIn]].m_x / stepSizeIn));
	qDebug("getValuesD6.2  effectYlocation: %d, %ld", effectYLocation, effectorOutputIn->size());
	// current effector output Y near m_needsUpdating[iIn] point
	float curEffectY = effectorOutputIn->operator[](effectYLocation);
	float nextEffectY = effectorOutputIn->operator[](effectYLocation);

	// getting the final automatable / effectable point values
	float curY = processAutomation(m_dataArray[m_needsUpdating[iIn]].m_y, m_needsUpdating[iIn], 0);
	float curC = processAutomation(m_dataArray[m_needsUpdating[iIn]].m_c, m_needsUpdating[iIn], 1);
	float curValA = processAutomation(m_dataArray[m_needsUpdating[iIn]].m_valA, m_needsUpdating[iIn], 2);
	float curValB = processAutomation(m_dataArray[m_needsUpdating[iIn]].m_valB, m_needsUpdating[iIn], 3);
	if (effectorIn != nullptr && getEffectOnlyPoints(m_needsUpdating[iIn]) == true)
	{
		curY = processEffect(m_needsUpdating[iIn], curY, 0, curEffectY);
		curC = processEffect(m_needsUpdating[iIn], curC, 1, curEffectY);
		curValA = processEffect(m_needsUpdating[iIn], curValA, 2, curEffectY);
		curValB = processEffect(m_needsUpdating[iIn], curValB, 3, curEffectY);
	}
	// from where to update line
	int start = effectYLocation;
	int end = start;

	float nextY = curY;

	if (m_needsUpdating[iIn] + 1 < m_dataArray.size())
	{
		effectYLocation = static_cast<unsigned int>
			(std::ceil(m_dataArray[m_needsUpdating[iIn] + 1].m_x / stepSizeIn));
		// where updating line ends (+1)
		end = effectYLocation;
		nextY = processAutomation(m_dataArray[m_needsUpdating[iIn] + 1].m_y, m_needsUpdating[iIn] + 1, 0);

		// if the current point can only be effected (and not its line)
		// and the next point can only be effected
		// this is done to avoid adding effectorOutputIn to the line and to the next point (line's end point) at the same time
		if (effectorIn != nullptr && getEffectOnlyPoints(m_needsUpdating[iIn] + 1) == true &&
			((getEffectOnlyPoints(m_needsUpdating[iIn]) == true && isEffectedPoint(m_needsUpdating[iIn]) == true) ||
			isEffectedPoint(m_needsUpdating[iIn]) == false))
		{
			nextEffectY = effectorOutputIn->operator[](effectYLocation);
			nextY = processEffect(m_needsUpdating[iIn] + 1, nextY, 0, nextEffectY);
		}
	}
	// calculating line ends
	if (m_needsUpdating[iIn] + 1 >= m_dataArray.size())
	{
		// if this point is at the last location in m_dataArray
		for (int j = end; j < m_bakedValues.size(); j++)
		{
			m_bakedValues[j] = curY;
		}
	}
	if (m_needsUpdating[iIn] == 0)
	{
		// if this point is at the 0 location in m_dataArray
		for (int j = 0; j < start; j++)
		{
			m_bakedValues[j] = curY;
		}
	}

	float fadeInStart = 0.05f;
	unsigned int type = m_dataArray[m_needsUpdating[iIn]].m_type;
qDebug("getValuesD8 [%d] start: %d, end: %d, type: %d,      ---       %f, %f, %f, AB: %f, %f", iIn, start, end, type, curY, nextY, curC, curValA, curValB);

	// calculate final updated line
	if (type == 0)
	{
		// calculate curve
		for (int j = start; j < end; j++)
		{
			m_bakedValues[j] = processCurve(curY, nextY, curC, outputXLocationsIn->operator[](j));
		}
		// no line type
	}
	else if (type == 1)
	{
		// curve
		for (int j = start; j < end; j++)
		{
			m_bakedValues[j] = processCurve(curY, nextY, curC, outputXLocationsIn->operator[](j));
		}
		// line type
		std::vector<float> lineTypeOutput = processLineTypeArraySine(outputXLocationsIn, start, end, curValA, curValB, fadeInStart);
		for (int j = start; j < end; j++)
		{
			m_bakedValues[j] = m_bakedValues[j] + lineTypeOutput[j - start];
		}
	}
	else if (type == 2)
	{
		// curve
		for (int j = start; j < end; j++)
		{
			m_bakedValues[j] = processCurve(curY, nextY, 0.0f, outputXLocationsIn->operator[](j));
		}
		// line type
		std::vector<float> lineTypeOutput = processLineTypeArraySineB(outputXLocationsIn, start, end, curValA, curValB, curC, fadeInStart);
		for (int j = start; j < end; j++)
		{
			m_bakedValues[j] = m_bakedValues[j] + lineTypeOutput[j - start];
		}
	}
	else if (type == 3)
	{
		// curve
		for (int j = start; j < end; j++)
		{
			m_bakedValues[j] = processCurve(curY, nextY, 0.0f, outputXLocationsIn->operator[](j));
		}
		// line type
		std::vector<float> lineTypeOutput = processLineTypeArrayPeak(outputXLocationsIn, start, end, curValA, curValB, curC, fadeInStart);
		for (int j = start; j < end; j++)
		{
			m_bakedValues[j] = m_bakedValues[j] + lineTypeOutput[j - start];
		}
	}
	else if (type == 4)
	{
		// curve
		for (int j = start; j < end; j++)
		{
			m_bakedValues[j] = processCurve(curY, nextY, curC, outputXLocationsIn->operator[](j));
		}
		// line type
		std::vector<float> lineTypeOutput = processLineTypeArraySteps(outputXLocationsIn, start, end, &m_bakedValues, curValA, curValB, fadeInStart);
		for (int j = start; j < end; j++)
		{
			m_bakedValues[j] = m_bakedValues[j] + lineTypeOutput[j - start];
		}
	}
	else if (type == 5)
	{
		// curve
		for (int j = start; j < end; j++)
		{
			m_bakedValues[j] = processCurve(curY, nextY, 0.0f, outputXLocationsIn->operator[](j));
		}
		// line type
		std::vector<float> lineTypeOutput = processLineTypeArrayRandom(outputXLocationsIn, start, end, curValA, curValB, curC, fadeInStart);
		for (int j = start; j < end; j++)
		{
			m_bakedValues[j] = m_bakedValues[j] + lineTypeOutput[j - start];
		}
	}
	if (effectorIn != nullptr && getEffectOnlyPoints(m_needsUpdating[iIn]) == false)
	{
		int startB = m_needsUpdating[iIn] == 0 ? 0 : start;
		int endB = m_needsUpdating[iIn] >= m_dataArray.size() - 1 ? m_bakedValues.size() : end;
		// process line effect
		// if it is enabled
		qDebug("getValues run prucessEffect run prucessEffect run prucessEffect run prucessEffect");
		for (int j = startB; j < endB; j++)
		{
			m_bakedValues[j] = processEffect(m_needsUpdating[iIn], m_bakedValues[j], 0, effectorOutputIn->operator[](j));
		}
	}
	// clamp
	for (int j = start; j < end; j++)
	{
		if (m_bakedValues[j] > 1.0f)
		{
			m_bakedValues[j] = 1.0f;
		}
		else if (m_bakedValues[j] < -1.0f)
		{
			m_bakedValues[j] = -1.0f;
		}
	}
	if (m_isNonNegative == true)
	{
		int startB = m_needsUpdating[iIn] == 0 ? 0 : start;
		int endB = m_needsUpdating[iIn] >= m_dataArray.size() - 1 ? m_bakedValues.size() : end;
		for (int j = startB; j < endB; j++)
		{
			m_bakedValues[j] = m_bakedValues[j] / 2.0f + 0.5f;
		}
	}
}

bool VectorGraphDataArray::isEffectedPoint(unsigned int locationIn)
{
	// loops througth all the effects
	// return true when 1 or more effects are active
	bool output = false;
	for (unsigned int i = 0; i <= 8; i++)
	{
		if (getEffect(locationIn, i) == true)
		{
			output = true;
			break;
		}
	}
	return output;
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
