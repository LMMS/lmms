/*
 * VectorGraph.cpp - Vector graph widget class implementation
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

#include "VectorGraphView.h"
#include "VectorGraphViewBase.h"

#include <array>
#include <vector>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

#include "AutomatableModel.h"
#include "GuiApplication.h" // getGUI
#include "MainWindow.h" // getting main window for control dialog
#include "ProjectJournal.h"

//#define VECTORGRAPH_DEBUG_USER_INTERACTION
//#define VECTORGRAPH_DEBUG_PAINT_EVENT

namespace lmms
{

namespace gui
{
VectorGraphView::VectorGraphView(QWidget * parent, int widgetWidth, int widgetHeight, unsigned int pointSize,
	unsigned int controlHeight, bool shouldApplyDefaultVectorGraphColors) :
		VectorGraphViewBase(parent),
		ModelView(new VectorGraphModel(2048, nullptr, false), this),
		m_controlDialog(getGUI()->mainWindow()->addWindowedWidget(new VectorGraphCotnrolDialog(getGUI()->mainWindow(), this)))
{
	resize(widgetWidth, widgetHeight);

	m_controlDialog->hide();
	Qt::WindowFlags flags = m_controlDialog->windowFlags();
	flags &= ~Qt::WindowMaximizeButtonHint;
	m_controlDialog->setWindowFlags(flags);

	m_mousePress = false;
	m_addition = false;

	m_pointSize = pointSize;
	// gets set in style
	//m_fontSize = 12; // set in css
	m_isSimplified = false;
	m_isDefaultColorsApplyed = false;
	//m_background;
	m_useGetLastSamples = false;

	m_selectedLocation = 0;
	m_selectedArray = 0;
	m_isSelected = false;
	m_isCurveSelected = false;
	m_isLastSelectedArray = false;

	m_graphHeight = height();
	m_controlHeight = controlHeight;
	m_controlDisplayCount = 2;
	m_isEditingActive = false;
	// set in .h
	//m_controlText
	//m_controlLineTypeText
	//m_controlIsFloat

	m_lastTrackPoint.first = -1;
	m_lastTrackPoint.second = 0;
	m_lastScndTrackPoint.first = 0;
	m_lastScndTrackPoint.second = 0;

	setCursor(Qt::CrossCursor);

	modelChanged();

	// connect loading of default colors to applyDefaultColors
	QObject::connect(this, &VectorGraphView::changedDefaultColors,
		this, &VectorGraphView::updateDefaultColors);

	if (shouldApplyDefaultVectorGraphColors == true)
	{
		applyDefaultColors();
	}
}
VectorGraphView::~VectorGraphView()
{
	if (m_controlDialog != nullptr)
	{
		delete m_controlDialog;
	}
}

void VectorGraphView::setLineColor(QColor color, unsigned int dataArrayLocation)
{
	if (model()->getDataArraySize() > dataArrayLocation)
	{
		model()->getDataArray(dataArrayLocation)->setLineColor(color);
		updateGraph();
	}
}
void VectorGraphView::setActiveColor(QColor color, unsigned int dataArrayLocation)
{
	if (model()->getDataArraySize() > dataArrayLocation)
	{
		model()->getDataArray(dataArrayLocation)->setActiveColor(color);
		updateGraph();
	}
}
void VectorGraphView::setFillColor(QColor color, unsigned int dataArrayLocation)
{
	if (model()->getDataArraySize() > dataArrayLocation)
	{
		model()->getDataArray(dataArrayLocation)->setFillColor(color);
		updateGraph();
	}
}
void VectorGraphView::setAutomatedColor(QColor color, unsigned int dataArrayLocation)
{
	if (model()->getDataArraySize() > dataArrayLocation)
	{
		model()->getDataArray(dataArrayLocation)->setAutomatedColor(color);
		updateGraph();
	}
}
void VectorGraphView::applyDefaultColors()
{
	m_isDefaultColorsApplyed = true;
	unsigned int size = model()->getDataArraySize();
	if (size > 0)
	{
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
void VectorGraphView::setPointSize(unsigned int pointSize)
{
	m_pointSize = pointSize;
	updateGraph();
}
void VectorGraphView::setControlHeight(unsigned int controlHeight)
{
	m_controlHeight = controlHeight;
	updateGraph();
}

void VectorGraphView::setIsSimplified(bool isSimplified)
{
	m_isSimplified = isSimplified;
#ifdef VECTORGRAPH_DEBUG_USER_INTERACTION
	qDebug("setIsSimplified %d", m_isSimplified);
#endif
}
bool VectorGraphView::getIsSimplified()
{
	return m_isSimplified;
}

PointF VectorGraphView::getSelectedData()
{
	PointF output(-1.0f, 0.00);
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
void VectorGraphView::setSelectedData(PointF data)
{
	if (m_isSelected == true)
	{
		model()->getDataArray(m_selectedArray)->setY(m_selectedLocation, data.second);
		m_selectedLocation = model()->getDataArray(m_selectedArray)->setX(m_selectedLocation, data.first);
#ifdef VECTORGRAPH_DEBUG_USER_INTERACTION
		qDebug("setSelectedData (x, y): %f, %f", data.first, data.second);
#endif
	}
}
void VectorGraphView::setBackground(const QPixmap background)
{
	m_background = background;
}
void VectorGraphView::useGetLastSamples()
{
	m_useGetLastSamples = true;
}

void VectorGraphView::mousePressEvent(QMouseEvent* me)
{
#ifdef VECTORGRAPH_DEBUG_USER_INTERACTION
	qDebug("\n\nmousePressEvent start ---------");
#endif
	// get position
	int x = me->x();
	int y = me->y();
	m_addition = false;
	m_mousePress = false;

	// a point's FloatModel might be deleted after this
	// cleaning up connected Knob in the dialog
	reinterpret_cast<VectorGraphCotnrolDialog*>(m_controlDialog->widget())->hideAutomation();
	m_controlDialog->hide();
	if (m_isSelected == true)
	{
		FloatModel* curFloatModel = model()->getDataArray(m_selectedArray)->getAutomationModel(m_selectedLocation);
		if (curFloatModel != nullptr && curFloatModel->isAutomatedOrControlled() == false)
		{
			model()->getDataArray(m_selectedArray)->setAutomated(m_selectedLocation, false);
		}
	}

	if(me->button() == Qt::LeftButton && me->modifiers() & Qt::ControlModifier && m_isSelected == true)
	{
#ifdef VECTORGRAPH_DEBUG_USER_INTERACTION
		qDebug("mousePressEvent automation connecting");
#endif
		// connect to AutomationTrack
		model()->getDataArray(m_selectedArray)->setAutomated(m_selectedLocation, true);
		FloatModel* curFloatModel = model()->getDataArray(m_selectedArray)->getAutomationModel(m_selectedLocation);
		// (VectorGraphViewBase method:)
		connectToAutomationTrack(me, curFloatModel, widget());
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
		if (isGraphPressed(m_graphHeight - y) == true)
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

			model()->modelAddJournalCheckPoint();
		}
	}

	// if the mouse was not moved a lot
	if (m_mousePress == true) { return; }

	if (isGraphPressed(m_lastScndTrackPoint.second) == true)
	{
		if (m_isSelected == true && m_addition == true)
		{
			if (m_isCurveSelected == false)
			{
#ifdef VECTORGRAPH_DEBUG_USER_INTERACTION
				qDebug("mouseMoveEvent point drag");
#endif
				// dragging point
				PointF convertedCoords = mapMousePos(x, m_graphHeight - y);
				convertedCoords.first = std::clamp(convertedCoords.first, 0.0f, 1.0f);
				convertedCoords.second = std::clamp(convertedCoords.second, -1.0f, 1.0f);
				setSelectedData(convertedCoords);
			}
			else if (model()->getDataArray(m_selectedArray)->getIsEditableAttrib() == true)
			{
#ifdef VECTORGRAPH_DEBUG_USER_INTERACTION
				qDebug("mouseMoveEvent curve drag: mouse (x, y): (%d, %d), last track point (x, y): (%d, %d)", x, (m_graphHeight - y), m_lastTrackPoint.first, m_lastTrackPoint.second);
#endif
				// dragging curve
				PointF convertedCoords = mapMousePos(x - m_lastTrackPoint.first, (m_graphHeight - y) - m_lastTrackPoint.second + m_graphHeight / 2);
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
					model()->getDataArray(m_selectedArray)->deletePoint(m_selectedLocation);
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
		processControlWindowPressed(m_lastTrackPoint.first, m_graphHeight - m_lastScndTrackPoint.second, true, startMoving);
	}
}

void VectorGraphView::mouseReleaseEvent(QMouseEvent* me)
{
#ifdef VECTORGRAPH_DEBUG_USER_INTERACTION
	qDebug("mouseReleaseEvent start");
#endif
	// get position
	int x = me->x();
	int y = me->y();
	// if did not drag and graph is pressed
	if (m_mousePress == true && isGraphPressed(m_graphHeight - y) == true)
	{
		model()->modelAddJournalCheckPoint();
		// add/delete point
		if (m_isSelected == false && m_addition == true)
		{
#ifdef VECTORGRAPH_DEBUG_USER_INTERACTION
			qDebug("mouseReleaseEvent add point, dataArray count: %d", static_cast<int>(model()->getDataArraySize()));
#endif
			// if selection failed and addition
			// get the first editable daraArray and add value
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
#ifdef VECTORGRAPH_DEBUG_USER_INTERACTION
			qDebug("mouseReleaseEvent delete point");
#endif
			// if selection was successful -> deletion
			model()->getDataArray(m_selectedArray)->deletePoint(m_selectedLocation);
			m_isSelected = false;
			m_isEditingActive = false;
		}
	}
	else if (m_mousePress == true && isControlWindowPressed(m_graphHeight - y) == true)
	{
		model()->modelAddJournalCheckPoint();
		processControlWindowPressed(x, m_graphHeight - y, false, false);
	}
	m_mousePress = false;
	m_addition = false;
	// reset trackpoint
	m_lastTrackPoint.first = -1;
	updateGraph(false);
#ifdef VECTORGRAPH_DEBUG_USER_INTERACTION
	qDebug("mouseReleaseEvent end");
#endif
}

void VectorGraphView::mouseDoubleClickEvent(QMouseEvent * me)
{
#ifdef VECTORGRAPH_DEBUG_USER_INTERACTION
	qDebug("mouseDoubleClickEvent start");
#endif
	// get position
	//int x = me->x();
	int y = me->y();

	// if a data/sample is selected then show input dialog to change the data
	if (isGraphPressed(m_graphHeight - y) == true)
	{
		if (m_isSelected == true && me->button() == Qt::LeftButton)
		{
			// display dialog
			PointF curData = showCoordInputDialog(getSelectedData());
			// change data
			setSelectedData(curData);
		}
	}
}
void VectorGraphView::leaveEvent(QEvent *event)
{
	hideHintText();
}

void VectorGraphView::paintEvent(QPaintEvent* pe)
{
#ifdef VECTORGRAPH_DEBUG_PAINT_EVENT
	qDebug("paintEvent start");
#endif
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing, true);

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

	std::vector<float> sampleBuffer;

	// updating the VectorGraphDataArray samples before draw
	if (m_useGetLastSamples == false || m_isSimplified == true)
	{
		// updating arrays that do not effect other arrays first
		// (this step will run getSamples() on effector arrays (-> every array will be updated about once))
		for (unsigned int i = 0; i < model()->getDataArraySize(); i++)
		{
			if (model()->getDataArray(i)->getIsAnEffector() == false)
			{
				// this updates the array and its effector arrays
				// with width() sample count
				model()->getDataArray(i)->getSamples(width(), nullptr);
			}
		}
	}

	// draw the updated VectorGraphDataArray samples in order
	// sometimes an other getSamples() call elsewhere can
	// change the sample count between updating and drawing
	// causing badly drawn output (if sample count is low)
	for (int i = model()->getDataArraySize() - 1; i >= 0; i--)
	{
		paintGraph(&p, i, &sampleBuffer);
	}

	paintEditing(&p);

	m_useGetLastSamples = false;
#ifdef VECTORGRAPH_DEBUG_PAINT_EVENT
	qDebug("paintEvent end");
#endif
	emit drawn();
}

void VectorGraphView::paintGraph(QPainter* p, unsigned int arrayLocation, std::vector<float>* sampleBuffer)
{
#ifdef VECTORGRAPH_DEBUG_PAINT_EVENT
	qDebug("paintGraph start: data arrayLocation: %d", arrayLocation);
#endif
	VectorGraphDataArray* dataArray = model()->getDataArray(arrayLocation);
	unsigned int length = dataArray->size();
	if (length <= 0) { return; }

	p->setPen(QPen(*dataArray->getLineColor(), 2));
	p->setBrush(QBrush(*dataArray->getLineColor(), Qt::NoBrush));

	PointInt posA(0, 0);
	PointInt posB(0, 0);
	PointInt startPos(mapDataPos(0.0f, dataArray->getY(0), false));
	// draw line
	if (m_isSimplified == false)
	{
		QPainterPath pt;
		posA = startPos;
		pt.moveTo(startPos.first + 1, m_graphHeight - startPos.second);

		// get the currently drawed VectorGraphDataArray samples
		dataArray->getLastSamples(sampleBuffer);

		for (unsigned int j = 0; j < sampleBuffer->size(); j++)
		{
			// if nonNegative then only the dataArray output (getDataValues)
			// is bigger than 0 so it matters only here
			posB = mapDataPos(0, (*sampleBuffer)[j], dataArray->getIsNonNegative());
			posB.first = static_cast<int>((j * width()) / static_cast<float>(sampleBuffer->size()));

			if (posA.first != posB.first)
			{
				pt.lineTo(posB.first, m_graphHeight - posB.second);
				// pt replaces drawing with path
				//p->drawLine(posA.first, m_graphHeight - posA.second, posB.first, m_graphHeight - posB.second);
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
			p->fillPath(pt, QBrush(*dataArray->getFillColor()));
			// draw line
			p->drawPath(ptline);
		}
		else
		{
			p->drawPath(pt);
		}
	}

	// draw points
	if (dataArray->getIsSelectable() == true || m_isSimplified == true)
	{
		posA = startPos;

		int squareSize = m_pointSize;

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
					p->setPen(QPen(*dataArray->getAutomatedColor(), 2));
					p->setBrush(QBrush(automatedFillColor, Qt::SolidPattern));
					resetColor = true;
				}
				else if (m_isSelected == true && m_selectedArray == arrayLocation && m_selectedLocation == j)
				{
					// if selected
					p->setBrush(QBrush(*dataArray->getFillColor(), Qt::SolidPattern));
					resetColor = true;
				}

				p->drawEllipse(posB.first - m_pointSize, m_graphHeight - posB.second - m_pointSize, m_pointSize * 2, m_pointSize * 2);

				// reset point color
				if (resetColor == true)
				{
					p->setPen(QPen(*dataArray->getLineColor(), 2));
					p->setBrush(Qt::NoBrush);
					resetColor = false;
				}

				if (j > 0)
				{
					if (dataArray->getIsEditableAttrib() == true)
					{
						PointInt posC = mapDataCurvePos(posA.first, posA.second, posB.first, posB.second, dataArray->getC(j - 1));
						p->drawRect(posC.first - squareSize / 2,
							m_graphHeight - posC.second - squareSize / 2, squareSize, squareSize);
					}
				}
			}

			// draw simplified line
			if (m_isSimplified == true)
			{
				p->drawLine(posA.first, m_graphHeight - posA.second, posB.first, m_graphHeight - posB.second);
			}
			posA = posB;
		}
	}
	// draw last simplified line
	if (m_isSimplified == true)
	{
		p->drawLine(posB.first, m_graphHeight - posB.second, width(), m_graphHeight - posB.second);
	}
}
void VectorGraphView::paintEditing(QPainter* p)
{
	p->setFont(QFont("Arial", m_fontSize));
	if (m_isEditingActive == true)
	{
		VectorGraphDataArray* dataArray = model()->getDataArray(m_selectedArray);
		QColor textColor = getTextColorFromBaseColor(*dataArray->getLineColor());
		// background of float values
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

		int segmentLength = width() / (m_controlDisplayCount);
		// draw inputs
		p->setPen(QPen(textColor, 1));
		for (unsigned int i = 0; i < m_controlDisplayCount; i++)
		{
			QColor curForeColor = *dataArray->getFillColor();
			p->fillRect(i * segmentLength, m_graphHeight, segmentLength, m_controlHeight, curForeColor);
			p->drawText(i * segmentLength, m_graphHeight + (m_controlHeight - m_fontSize) / 2 + m_fontSize, m_controlText[i]);
		}

		// draw outline
		p->setPen(QPen(*dataArray->getLineColor(), 1));
		p->drawLine(0, m_graphHeight, width(), m_graphHeight);
		for (unsigned int i = 1; i < m_controlDisplayCount; i++)
		{
			if (i < controlTextCount || i >= m_controlDisplayCount)
			{
				p->drawLine(i * segmentLength, m_graphHeight, i * segmentLength, height());
			}
		}
	}
	if (m_isLastSelectedArray == true)
	{
		// draw selected array number
		p->setPen(QPen(QColor(127, 127, 127, 255), 2));
		p->drawText(2, (m_controlHeight - m_fontSize) / 2 + m_fontSize, QString::number(m_selectedArray));
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
void VectorGraphView::updateGraph(bool shouldUseGetLastSamples)
{
	m_useGetLastSamples = shouldUseGetLastSamples;
	update();
}
void VectorGraphView::updateDefaultColors()
{
	if (m_isDefaultColorsApplyed == true)
	{
		applyDefaultColors();
	}
}

PointF VectorGraphView::mapMousePos(int x, int y)
{
	// mapping the position to 0 - 1, -1 - 1 using qWidget width and height
	return PointF(
		static_cast<float>(x / static_cast<float>(width())),
		static_cast<float>(y) * 2.0f / static_cast<float>(m_graphHeight) - 1.0f);
}
PointInt VectorGraphView::mapDataPos(float x, float y, bool isNonNegative)
{
	// mapping the point/sample positon to mouse/view position
	if (isNonNegative == true)
	{
		return PointInt(
			static_cast<int>(x * width()),
			static_cast<int>(y * m_graphHeight));
	}
	else
	{
		return PointInt(
			static_cast<int>(x * width()),
			static_cast<int>((y + 1.0f) * static_cast<float>(m_graphHeight) / 2.0f));
	}
}
PointF VectorGraphView::mapDataCurvePosF(float xA, float yA, float xB, float yB, float curve)
{
	return PointF(
		(xA + xB) / 2.0f,
		yA + (curve / 2.0f + 0.5f) * (yB - yA));
}
PointInt VectorGraphView::mapDataCurvePos(int xA, int yA, int xB, int yB, float curve)
{
	return PointInt(
		(xA + xB) / 2,
		yA + static_cast<int>((curve / 2.0f + 0.5f) * (yB - yA)));
}
int VectorGraphView::mapControlInputX(float inputValue, unsigned int displayLength)
{
	return (inputValue / 2.0f + 0.5f) * displayLength;
}

float VectorGraphView::getDistance(int xA, int yA, int xB, int yB)
{
	return std::sqrt(static_cast<float>((xA - xB) * (xA - xB) + (yA - yB) * (yA - yB)));
}
float VectorGraphView::getDistanceF(float xA, float yA, float xB, float yB)
{
	return std::sqrt((xA - xB) * (xA - xB) + (yA - yB) * (yA - yB));
}

bool VectorGraphView::addPoint(unsigned int arrayLocation, int mouseX, int mouseY)
{
#ifdef VECTORGRAPH_DEBUG_USER_INTERACTION
	qDebug("addPoint: arrayLocation: %d, position (x, y): %d, %d", arrayLocation, mouseX, mouseY);
#endif
	// mouseY is calculated like this:
	// m_graphHeight - y
	bool output = false;
	PointF curMouseCoords = mapMousePos(mouseX, mouseY);
	curMouseCoords.first = std::clamp(curMouseCoords.first, 0.0f, 1.0f);
	curMouseCoords.second = std::clamp(curMouseCoords.second, -1.0f, 1.0f);
	int location = model()->getDataArray(arrayLocation)->add(curMouseCoords.first);

	// if adding was not successful
	if (location < 0) { return output; }

	output = true;
	model()->getDataArray(arrayLocation)->setY(location, curMouseCoords.second);
#ifdef VECTORGRAPH_DEBUG_USER_INTERACTION
	qDebug("addPoint: point location: %d, positionF (x, y): %f, %f, new dataArray size: %d",
		location, curMouseCoords.first, curMouseCoords.second, static_cast<int>(model()->getDataArray(arrayLocation)->size()));
#endif
	return output;
}

bool VectorGraphView::isGraphPressed(int mouseY)
{
	return !isControlWindowPressed(mouseY);
}
bool VectorGraphView::isControlWindowPressed(int mouseY)
{
	bool output = false;
	// mouseY is calculated like this:
	// m_graphHeight - y
	if (m_isEditingActive == true && mouseY <= 0)
	{
		output = true;
	}
	return output;
}
void VectorGraphView::processControlWindowPressed(int mouseX, int mouseY, bool isDragging, bool startMoving)
{
	// mouseY is calculated like this:
	// m_graphHeight - y
	setCursor(Qt::ArrowCursor);

#ifdef VECTORGRAPH_DEBUG_USER_INTERACTION
	qDebug("processControlWindowPressed: mouse (x, y): %d, %d, isDragging: %d, startMoving: %d", mouseX, mouseY, isDragging, startMoving);
#endif

	if (m_isEditingActive == false) { return; }

	int pressLocation = getPressedControlInput(mouseX, m_graphHeight - mouseY, m_controlDisplayCount);
#ifdef VECTORGRAPH_DEBUG_USER_INTERACTION
	qDebug("processControlWindowPressed: pressLocation: %d", pressLocation);
#endif
	if (isDragging == false || (isDragging == true && startMoving == false))
	{
		if (pressLocation == 0)
		{
			m_isEditingActive = false;
			m_controlDialog->show();
			if (m_isSelected == true)
			{
				reinterpret_cast<VectorGraphCotnrolDialog*>(m_controlDialog->widget())->switchPoint(m_selectedArray, m_selectedLocation);
			}
			hideHintText();
		}
		else if (pressLocation == 1)
		{
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
#ifdef VECTORGRAPH_DEBUG_USER_INTERACTION
				qDebug("mouseReleaseEvent select dataArray: i: [%d], m_selectedArray: %d, oldSelectedArray: %d", i, m_selectedArray, oldSelectedArray);
#endif
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
#ifdef VECTORGRAPH_DEBUG_USER_INTERACTION
			qDebug("mouseReleaseEvent select dataArray final: %d", m_selectedArray);
#endif

			// hint text
			if (m_selectedArray != oldSelectedArray)
			{
				showHintText(widget(), tr("selected graph changed"), 5, 3500);
			}
			else
			{
				showHintText(widget(), tr("unable to select other graph"), 5, 3500);
			}
		}
	}
}
int VectorGraphView::getPressedControlInput(int mouseX, int mouseY, unsigned int controlCount)
{
	int output = -1;
	if (m_isEditingActive == true && mouseY > m_graphHeight)
	{
		output = mouseX * controlCount / width();
	}
	if (output > controlCount)
	{
		output = controlCount;
	}
	return output;
}
float VectorGraphView::getInputAttribValue(unsigned int controlArrayLocation)
{
	float output = 0.0f;
	if (m_isSelected == false) { return output; }

	switch (controlArrayLocation)
	{
		case 0:
			output = model()->getDataArray(m_selectedArray)->getX(m_selectedLocation);
			break;
		case 1:
			output = model()->getDataArray(m_selectedArray)->getY(m_selectedLocation);
			break;
		case 2:
			output = model()->getDataArray(m_selectedArray)->getC(m_selectedLocation);
			break;
		case 3:
			output = model()->getDataArray(m_selectedArray)->getValA(m_selectedLocation);
			break;
		case 4:
			output = model()->getDataArray(m_selectedArray)->getValB(m_selectedLocation);
			break;
		case 5:
			// type
			output = model()->getDataArray(m_selectedArray)->getType(m_selectedLocation);
			break;
		case 6:
			// automation location
			output = model()->getDataArray(m_selectedArray)->getAutomatedAttribLocation(m_selectedLocation);
			break;
		case 7:
			// effect location
			output = model()->getDataArray(m_selectedArray)->getEffectedAttribLocation(m_selectedLocation);
			break;
		case 8:
			output = model()->getDataArray(m_selectedArray)->getEffect(m_selectedLocation, 0);
			break;
		case 9:
			output = model()->getDataArray(m_selectedArray)->getEffect(m_selectedLocation, 1);
			break;
		case 10:
			output = model()->getDataArray(m_selectedArray)->getEffect(m_selectedLocation, 2);
			break;
		case 11:
			output = model()->getDataArray(m_selectedArray)->getEffectPoints(m_selectedLocation) ? 1.0f : 0.0f;
			break;
		case 12:
			output = model()->getDataArray(m_selectedArray)->getEffectLines(m_selectedLocation) ? 1.0f : 0.0f;
			break;
	}
	return output;
}
void VectorGraphView::setInputAttribValue(unsigned int controlArrayLocation, float floatValue)
{
#ifdef VECTORGRAPH_DEBUG_USER_INTERACTION
	qDebug("setInputAttribute start: control input: %d, set floatValue: %f", controlArrayLocation, floatValue);
#endif
	if (m_isSelected == false) { return; }

	float clampedValue = std::clamp(floatValue, -1.0f, 1.0f);
	unsigned int clampedValueB = 0;
	switch (controlArrayLocation)
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
			clampedValueB = static_cast<unsigned int>(std::clamp(floatValue, 0.0f, 6.0f));
			model()->getDataArray(m_selectedArray)->setType(m_selectedLocation, clampedValueB);
			break;
		case 6:
			// automation location
			clampedValueB = static_cast<unsigned int>(std::clamp(floatValue, 0.0f, 4.0f));
			model()->getDataArray(m_selectedArray)->setAutomatedAttrib(m_selectedLocation, clampedValueB);
			break;
		case 7:
			// effect location
			clampedValueB = static_cast<unsigned int>(std::clamp(floatValue, 0.0f, 4.0f));
			model()->getDataArray(m_selectedArray)->setEffectedAttrib(m_selectedLocation, clampedValueB);
			break;
		case 8:
			clampedValueB = static_cast<unsigned int>(floatValue);
			model()->getDataArray(m_selectedArray)->setEffect(m_selectedLocation, 0, clampedValueB);
			break;
		case 9:
			clampedValueB = static_cast<unsigned int>(floatValue);
			model()->getDataArray(m_selectedArray)->setEffect(m_selectedLocation, 1, clampedValueB);
			break;
		case 10:
			clampedValueB = static_cast<unsigned int>(floatValue);
			model()->getDataArray(m_selectedArray)->setEffect(m_selectedLocation, 2, clampedValueB);
			break;
		case 11:
			model()->getDataArray(m_selectedArray)->setEffectPoints(m_selectedLocation, floatValue >= 0.5f);
			break;
		case 12:
			model()->getDataArray(m_selectedArray)->setEffectLines(m_selectedLocation, floatValue >= 0.5f);
			break;
	}
}
QColor VectorGraphView::getTextColorFromBaseColor(QColor baseColor)
{
	QColor output(255, 255, 255, 255);
	int colorSum = baseColor.red() + baseColor.green() + baseColor.blue();
	// > 127 * 3
	if (colorSum > 382)
	{
		output = QColor(0, 0, 0, 255);
	}
	return output;
}
QColor VectorGraphView::getFillColorFromBaseColor(QColor baseColor)
{
	QColor output;
	int colorSum = baseColor.red() + baseColor.green() + baseColor.blue();
	int brighten = 0;
	int alpha = baseColor.alpha();
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
		output = QColor(static_cast<int>(static_cast<float>(baseColor.red()) * 0.42f + colorSum * 0.09f) - brighten,
			static_cast<int>(static_cast<float>(baseColor.green()) * 0.42f + colorSum * 0.09f) - brighten,
			static_cast<int>(static_cast<float>(baseColor.blue()) * 0.42f + colorSum * 0.09f) - brighten, 255);
	}
	else
	{
		// (red * 0.6f + avg * 0.4f / 3.0f) * 1.3
		output = QColor(static_cast<int>(static_cast<float>(baseColor.red()) * 0.78f + colorSum * 0.17f) + brighten,
			static_cast<int>(static_cast<float>(baseColor.green()) * 0.78f + colorSum * 0.17f) + brighten,
			static_cast<int>(static_cast<float>(baseColor.blue()) * 0.78f + colorSum * 0.17f) + brighten, 255);
	}
	return output;
}

void VectorGraphView::selectData(int mouseX, int mouseY)
{
#ifdef VECTORGRAPH_DEBUG_USER_INTERACTION
	qDebug("selectData start: mouse (x, y): %d, %d", mouseX, mouseY);
#endif

	m_isSelected = false;

	// trying to select the last selected array
	if (m_isLastSelectedArray == true)
	{
		VectorGraphDataArray* dataArray = model()->getDataArray(m_selectedArray);
		int location = searchForData(mouseX, mouseY, static_cast<float>(m_pointSize) / width(), dataArray, false);
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
				int location = searchForData(mouseX, mouseY, static_cast<float>(m_pointSize) / width(), dataArray, false);
				if (location > -1)
				{
					m_selectedLocation = location;
					m_selectedArray = i;
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
					int location = searchForData(mouseX, mouseY, static_cast<float>(m_pointSize) / width(), dataArray, true);
					if (location > -1)
					{
						m_selectedLocation = location;
						m_selectedArray = i;
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
#ifdef VECTORGRAPH_DEBUG_USER_INTERACTION
	qDebug("selectData end: m_selectedArray: %d, m_selectedLocation %d", m_selectedArray, m_selectedLocation);
#endif
}

int VectorGraphView::searchForData(int mouseX, int mouseY, float maxDistance, VectorGraphDataArray* dataArray, bool isCurved)
{
	int output = -1;
	maxDistance = maxDistance * 2.0f;

	PointF transformedMouse = mapMousePos(mouseX, mouseY);

	// unused bool
	bool found = false;
	bool isBefore = false;
	// get the nearest data to the mouse pos (x) in an optimalized way
	int location = dataArray->getNearestLocation(transformedMouse.first, &found, &isBefore);

	// if getNearestLocation was not successful
	if (location < 0) { return output; }

	float dataX = dataArray->getX(location);
	float dataY = dataArray->getY(location);
	// this is set to one when isCurved == true
	// and isBefore == false
	int curvedBefore = 0;
	// if isCurved then get the closest curved coord
	if (isCurved == true && dataArray->size() > 1)
	{
		if (isBefore == false && 1 < location)
		{
			curvedBefore = 1;
		}
		if (location - curvedBefore < dataArray->size()  - 1)
		{
			PointF curvedDataCoords = mapDataCurvePosF(
				dataArray->getX(location - curvedBefore), dataArray->getY(location - curvedBefore),
				dataArray->getX(location - curvedBefore + 1), dataArray->getY(location - curvedBefore + 1),
				dataArray->getC(location - curvedBefore));
			dataX = curvedDataCoords.first;
			dataY = curvedDataCoords.second;
		}
	}
	// check distance against x coord
	if (std::abs(dataX - transformedMouse.first) <= maxDistance)
	{
		// calculate real distance (x and y)
		float curDistance = getDistanceF(transformedMouse.first * 2.0f, transformedMouse.second,
			dataX * 2.0f, dataY);

		if (curDistance <= maxDistance)
		{
			output = location - curvedBefore;
		}
		else
		{
			// sometimes the mouse x and the nearest point x
			// coordinates are close but the y coords are not
			// calculating and testing all near by point distances
			int searchStart = 0;
			int searchEnd = dataArray->size() - 1;
			// from where we need to search the data
			for (int i = location - curvedBefore - 1; i > 0; i--)
			{
				if (std::abs(dataArray->getX(i) - transformedMouse.first) > maxDistance)
				{
					// if it is isCurved, then subtract 1
					// add 1 to i because [i] > maxDistance
					searchStart = i + 1 - (i > 0 && isCurved == true ? 1 : 0);
					break;
				}
			}
			// getting where the search needs to end
			for (int i = location - curvedBefore + 1; i < dataArray->size(); i++)
			{
				if (std::abs(dataArray->getX(i) - transformedMouse.first) > maxDistance)
				{
					searchEnd = i - 1 - (i > 0 && isCurved == true ? 1 : 0);
					break;
				}
			}
			// calculating real distances from the point coords
			for (int i = searchStart; i <= searchEnd; i++)
			{
				if (i != location)
				{
					dataX = dataArray->getX(i);
					dataY = dataArray->getY(i);
					if (isCurved == true && dataArray->size() > 1)
					{
						if (dataArray->size() - 1 > i)
						{
							PointF curvedDataCoords = mapDataCurvePosF(
								dataArray->getX(i), dataArray->getY(i), dataArray->getX(i + 1), dataArray->getY(i + 1),
								dataArray->getC(i));
							dataX = curvedDataCoords.first;
							dataY = curvedDataCoords.second;
						}
					}
					curDistance = getDistanceF(transformedMouse.first * 2.0f, transformedMouse.second,
						dataX * 2.0f, dataY);
					if (curDistance <= maxDistance)
					{
						output = i;
						break;
					}
				}
			}
		}
	}
	return output;
}

} // namespace gui

} // namespace lmms
