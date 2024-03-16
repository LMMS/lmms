

#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <QPainter>
#include <QMouseEvent>
#include <QInputDialog>

#include "PointGraph.h"
#include "StringPairDrag.h"


namespace lmms
{

namespace gui
{

PointGraphView::PointGraphView(QWidget * parentIn,
		int widthIn, int heightIn,
		unsigned int pointSizeIn, unsigned int maxLengthIn) :
		QWidget(parentIn),
		ModelView(new PointGraphModel(maxLengthIn, nullptr, false), this)
{
	resize(widthIn, heightIn);

	m_mouseDown = false;
	m_mouseDrag = false;
	m_mousePress = false;
	m_addition = false;

	m_pointSize = pointSizeIn;
	m_isSimplified = false;

	m_selectedLocation = 0;
	m_selectedArray = 0;
	m_isSelected = false;
	m_isCurveSelected = false;
	m_isLastSelectedArray = false;

	m_graphHeight = height();
	m_editingHeight = 30;
	m_editingInputCount = 4;
	m_editingDisplayPage = 0;
	m_isEditingActive = false;
	m_editingText = {
		tr("x coordinate"), tr("y coordinate"), tr("curve"), tr("1. attribute value"),
		tr("2. attribute value"), tr("switch graph line type"), tr("switch graph automated value"),
		tr("switch graph effected value"), tr("can only effect graph points"), tr("\"add\" effect"), tr("\"subtract\" effect"),
		tr("\"multiply\" effect"), tr("\"divide\" effect"), tr("\"power\" effect"), tr("\"log\" effect"),
		tr("\"sine\" effect")
	};
	m_editingLineEffectText = {
		tr("none"),
		tr("sine"),
		tr("phase changable sine"),
		tr("peak"),
		tr("steps"),
		tr("random")
	};
	m_editingInputIsFloat = {
		true, true, true, true,
		true, false, false,
		false, false, false, false,
		false, false, false, false,
		false
	};

	m_lastTrackPoint.first = -1;
	m_lastTrackPoint.second = 0;
	m_lastScndTrackPoint.first = 0;
	m_lastScndTrackPoint.second = 0;

	setCursor(Qt::CrossCursor);

	auto gModel = model();

	QObject::connect(gModel, SIGNAL(dataChanged()),
			this, SLOT(updateGraph()));
	QObject::connect(gModel, SIGNAL(lengthChanged()),
			this, SLOT(updateGraph()));
}
PointGraphView::~PointGraphView()
{
	m_editingText.clear();
	m_editingInputIsFloat.clear();
	m_editingLineEffectText.clear();
}

void PointGraphView::setLineColor(QColor colorIn, unsigned int dataArrayLocationIn)
{
	if (model()->getDataArraySize() > dataArrayLocationIn)
	{
		model()->getDataArray(dataArrayLocationIn)->setLineColor(colorIn);
		update();
	}
}
void PointGraphView::setActiveColor(QColor colorIn, unsigned int dataArrayLocationIn)
{
	if (model()->getDataArraySize() > dataArrayLocationIn)
	{
		model()->getDataArray(dataArrayLocationIn)->setActiveColor(colorIn);
		update();
	}
}
void PointGraphView::setFillColor(QColor colorIn, unsigned int dataArrayLocationIn)
{
	if (model()->getDataArraySize() > dataArrayLocationIn)
	{
		model()->getDataArray(dataArrayLocationIn)->setFillColor(colorIn);
		update();
	}
}
void PointGraphView::setAutomatedColor(QColor colorIn, unsigned int dataArrayLocationIn)
{
	if (model()->getDataArraySize() > dataArrayLocationIn)
	{
		model()->getDataArray(dataArrayLocationIn)->setAutomatedColor(colorIn);
		update();
	}
}

void PointGraphView::setIsSimplified(bool isSimplifiedIn)
{
	m_isSimplified = isSimplifiedIn;
}

std::pair<float, float> PointGraphView::getSelectedData()
{
	std::pair<float, float> output(-1.0f, 0.00);
	if (m_isSelected == true)
	{
		output.first = model()->getDataArray(m_selectedArray)->getX(m_selectedLocation);
		output.second = model()->getDataArray(m_selectedArray)->getY(m_selectedLocation);
	}
	return output;
}
int PointGraphView::getLastSelectedArray()
{
	if (m_isLastSelectedArray == true)
	{
		return m_selectedArray;
	}
	return -1;
}
void PointGraphView::setSelectedData(std::pair<float, float> dataIn)
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

void PointGraphView::mousePressEvent(QMouseEvent* me)
{
	// get position
	int x = me->x();
	int y = me->y();
	m_addition = false;

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
	else if (isGraphPressed(y) == true)
	{
		// try selecting the clicked point (if it is near)
		selectData(x, m_graphHeight - y);

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
	}
	else
	{
		m_mousePress = true;
	}
	m_mouseDown = true;
	qDebug("mousePressEnd ---------");
}

void PointGraphView::mouseMoveEvent(QMouseEvent* me)
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
		if (isGraphPressed(m_graphHeight - m_lastScndTrackPoint.second) == true)
		{
			if (m_isSelected == true && m_addition == true)
			{
				if (m_isCurveSelected == false)
				{
					std::pair<float, float> convertedCoords = mapMousePos(x, m_graphHeight - y, model()->getDataArray(m_selectedArray)->getNonNegative());
					convertedCoords.first = convertedCoords.first > 1.0f ? 1.0f : convertedCoords.first < 0.0f ? 1.0f : convertedCoords.first;
					convertedCoords.second = convertedCoords.second > 1.0f ? 1.0f : convertedCoords.second < -1.0f ? -1.0f : convertedCoords.second;
					setSelectedData(convertedCoords);
				}
				else if (model()->getDataArray(m_selectedArray)->getIsEditableAttrib() == true)
				{
					std::pair<float, float> convertedCoords = mapMousePos(x - m_lastTrackPoint.first, m_graphHeight - y + m_lastTrackPoint.second,
						model()->getDataArray(m_selectedArray)->getNonNegative());
					float curveValue = convertedCoords.second + convertedCoords.first * 0.1f;
					curveValue = curveValue > 1.0f ? 1.0f : curveValue < -1.0f ? -1.0f : curveValue;
					model()->getDataArray(m_selectedArray)->setC(m_selectedLocation, curveValue);
				}
			}
			else if (m_addition == false)
			{
				// TODO deletion
			}
			else
			{
				float curDistance = getDistance(x, m_graphHeight - y,
					m_lastTrackPoint.first, m_lastTrackPoint.second);
				if (curDistance > m_pointSize * 2)
				{
					// TODO drawing
					m_lastTrackPoint.first = x;
					m_lastTrackPoint.second = m_graphHeight - y;
				}
				// else m_mousePress does not change
			}
		}
		else
		{
			// if editing inputs were pressed (not graph)
			int pressLocation = getPressedInput(m_lastTrackPoint.first, m_graphHeight - m_lastScndTrackPoint.second, m_editingInputCount + 1);
			if (pressLocation >= 0 && pressLocation < m_editingInputCount)
			{
				// pressLocation should always be >= 0
				unsigned int location = m_editingInputCount * m_editingDisplayPage + pressLocation;
				// if the input type is a float
				if (location < m_editingText.size() && m_editingInputIsFloat[location] == true)
				{
					// if the user just started to move the mouse it is pressed
					if (startMoving == true)
					{
						// unused bool
						bool isTrue = false;
						// set m_lastScndTrackPoint.first to the current input value
						m_lastScndTrackPoint.first = mapInputPos(getInputAttribValue(location, &isTrue), m_graphHeight);

						m_lastTrackPoint.first = x;
						m_lastTrackPoint.second = m_graphHeight - y;
		qDebug("get last value: %d, lasttrack: %d, x: %d, y: %d, x2: %d, y2: %d, location: %d", m_lastScndTrackPoint.first, m_lastScndTrackPoint.second, x, (m_graphHeight - y), m_lastTrackPoint.first, m_lastTrackPoint.second, pressLocation);
					}
					std::pair<float, float> convertedCoords = mapMousePos(0, m_lastScndTrackPoint.first + static_cast<int>(m_graphHeight - y - m_lastTrackPoint.second) / 2,
						model()->getDataArray(m_selectedArray)->getNonNegative());
					setInputAttribValue(location, convertedCoords.second, false);
				}
			}
		}
	}
}

void PointGraphView::mouseReleaseEvent(QMouseEvent* me)
{
	// get position
	int x = me->x();
	int y = me->y();
	if (isGraphPressed(m_graphHeight - y) == true)
	{
	qDebug("mouseMove graphPressed: %d", m_lastTrackPoint.first);
		if (m_mousePress == true)
		{
			// add/delete point
			if (m_isSelected == false && m_addition == true)
			{
				// if selection failed and addition
				// get the first editable daraArray and add value
				qDebug("release size: %ld", model()->getDataArraySize());
				for(unsigned int i = 0; i < model()->getDataArraySize(); i++)
				{
					std::pair<float, float> curMouseData = mapMousePos(x, m_graphHeight - y,
						model()->getDataArray(i)->getNonNegative()); // TODO optimize
					int location = model()->getDataArray(i)->add(curMouseData.first);
					// if adding was successful
					if (location >= 0)
					{
		qDebug("mouseRelease added %d   %f,%d", location, curMouseData.second, m_graphHeight);
						model()->getDataArray(i)->setY(location, curMouseData.second);
						break;
					}
				}
			}
			else if (m_isSelected == true && m_addition == false)
			{
				// if selection was successful and deletion
				model()->getDataArray(m_selectedArray)->del(m_selectedLocation);
				m_isSelected = false;
				m_isEditingActive = false;
			}

			m_mousePress = false;
		}
		else
		{
			// add/set/delete line end

		}
	}
	else if (m_mousePress == true)
	{
	qDebug("mouseMove 7: %d", m_lastTrackPoint.first);
		int pressLocation = getPressedInput(x, m_graphHeight - y, m_editingInputCount + 1);
		if (pressLocation == m_editingInputCount)
		{
			// if the last button was pressed

			// how many inputs are there
			int editingTextCount = m_editingText.size();
			if (m_isSelected == true)
			{
				if (model()->getDataArray(m_selectedLocation)->getIsEditableAttrib() == false)
				{
					// x, y
					editingTextCount = 2;
				}
				else if (model()->getDataArray(m_selectedLocation)->getIsAutomatableEffectable() == false)
				{
					// x, y, curve, valA, valB, switch type
					editingTextCount = 6;
				}
			}

			m_editingDisplayPage++;
			if (m_editingInputCount * m_editingDisplayPage >= editingTextCount)
			{
				m_editingDisplayPage = 0;
			}
	qDebug("mouseMove editingPage: %d", m_editingDisplayPage);
		}
		else if (pressLocation >= 0)
		{
			// pressLocation should always be >= 0
			unsigned int location = m_editingInputCount * m_editingDisplayPage + pressLocation;
			// setting the boolean values
			if (location < m_editingText.size() && m_editingInputIsFloat[location] == false)
			{
				bool curBoolValue = true;
				//if (location >= 5 && location <= 7)
				//{
				//	curBoolValue = true;
				//}
				//else
				// if location is not at "set type" or "set automation location" or "set effect location"
				// (else curBoolValue = true -> setInputAttribValue will add 1 to the attribute)
				if (location < 5 || location > 7)
				{
					getInputAttribValue(location, &curBoolValue);
					curBoolValue = !curBoolValue;
				}
				setInputAttribValue(location, 0.0f, curBoolValue);
			}
		}
	}
	m_addition = false;
	m_mouseDown = false;
	m_mouseDrag = false;
	// reset trackpoint
	m_lastTrackPoint.first = -1;
	// triggering paintEvent
	qDebug("mouseReleaseEnd");
	update();
	emit drawn();
}

void PointGraphView::mouseDoubleClickEvent(QMouseEvent * me)
{
	// get position
	int x = me->x();
	int y = me->y();

	// if a data/sample is selected then show input dialog to change the data
	if (isGraphPressed(m_graphHeight - y) == true)
	{
		if (m_isSelected == true && me->button() == Qt::LeftButton)
		{
			// display dialog
			std::pair<float, float> curData = showCoordInputDialog();
			// change data
			setSelectedData(curData);
		}
	}
	else
	{
		int pressLocation = getPressedInput(x, m_graphHeight - y, m_editingInputCount + 1);
		if (pressLocation >= 0 && pressLocation != m_editingInputCount)
		{
			unsigned int location = m_editingInputCount * m_editingDisplayPage + pressLocation;
			if (location < m_editingText.size() && m_editingInputIsFloat[location] == true)
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

void PointGraphView::paintEvent(QPaintEvent* pe)
{
	QPainter p(this);
	//QPainterPath pt(); // TODO
	qDebug("paintEvent");
	m_graphHeight = m_isEditingActive == true ? height() - m_editingHeight : height();

	PointGraphDataArray* dataArray = nullptr;
	for (unsigned int i = 0; i < model()->getDataArraySize(); i++)
	{
		dataArray = model()->getDataArray(i);
		p.setPen(QPen(*dataArray->getLineColor(), 1));
		QColor gcolor = QColor(dataArray->getLineColor()->red(), dataArray->getLineColor()->green(),
			dataArray->getLineColor()->blue(), 100);

		unsigned int length = dataArray->size();

		if (m_isSelected == true)
		{
		p.drawLine(10, 10, 20, 20);
		}

		if (length > 0)
		{

			std::pair<int, int> posA(0, 0);
			std::pair<int, int> posB(0, 0);
			// if first data/sample > 0, draw a line to the first data/sample from 0
			//if (posA.first > 0)
			//{
				//p.drawLine(0, posA.second, posA.first, posA.second);
			//}
			// drawing lines
	qDebug("paint size: %d", length);
			if (dataArray->getIsSelectable() == true)
			{
				for (unsigned int j = 0; j < length; j++)
				{
					//qDebug("draw: x: %f, y: %f", dataArray->getX(j), dataArray->getY(j));
					posB = mapDataPos(dataArray->getX(j), dataArray->getY(j), dataArray->getNonNegative());
					p.drawEllipse(posB.first - m_pointSize, m_graphHeight - posB.second - m_pointSize, m_pointSize * 2, m_pointSize * 2);
					if (j > 0)
					{
						if (dataArray->getIsEditableAttrib() == true)
						{
							std::pair<int, int> posC = mapDataCurvePos(posA.first, posA.second, posB.first, posB.second, dataArray->getC(j - 1));
							p.drawRect(posC.first - m_pointSize / 2, m_graphHeight - posC.second - m_pointSize / 2, m_pointSize, m_pointSize);
						}
						if (m_isSimplified == true)
						{
							p.drawLine(posA.first, m_graphHeight - posA.second, posB.first, m_graphHeight - posB.second);
						}
					}
					posA = posB;
				}
			}
			if (m_isSimplified == false)
			{
				posA = mapDataPos(dataArray->getX(0), dataArray->getY(0), dataArray->getNonNegative());
				for (unsigned int j = 0; j < width(); j++)
				{
					//posB = mapDataPos(*dataArray->getX(j + 1), dataArray->getY(j + 1), dataArray->getNonNegative());
					posB = mapDataPos(0, dataArray->getValueAtPosition(static_cast<float>(j) / static_cast<float>(width())), dataArray->getNonNegative());
					//posB = dataArray->getValueAtPosition(static_cast<float>(j) / static_cast<float>(width()));
					posB.first = j;
		//qDebug("paint positions: x: %d, y: %d", posB.first, posB.second);
					// x1, y1, x2, y2
		//qDebug("paint positions: x: %d, y: %d, x2: %d, y2: %d", posA.first, posA.second, posB.first, posB.second);
					p.drawLine(posA.first, m_graphHeight - posA.second, posB.first, m_graphHeight - posB.second);
					posA = posB;
				}
			}
			//if (posA.first < width())
			//{
				//p.drawLine(posA.first, posA.second, width(), posA.second);
			//}
		}
	}

	if (m_isEditingActive == true)
	{
		dataArray = model()->getDataArray(m_selectedArray);
		QColor textColor = getTextColorFromBaseColor(*dataArray->getLineColor());
		// background of float values
		QColor backColor(25, 25, 25, 255);
		QColor foreColor = *dataArray->getLineColor();
		if (dataArray->getFillColor()->alpha() > 0)
		{
			backColor = QColor(dataArray->getFillColor()->red() / 2, dataArray->getFillColor()->green() / 2,
				dataArray->getFillColor()->blue() / 2, 255);
			foreColor = *dataArray->getFillColor();
		}

		int editingTextCount = m_editingText.size();
		if (dataArray->getIsEditableAttrib() == false)
		{
			// x, y
			editingTextCount = 2;
		}
		else if (dataArray->getIsAutomatableEffectable() == false)
		{
			// x, y, curve, valA, valB, switch type
			editingTextCount = 6;
		}

		int segmentLength = width() / (m_editingInputCount + 1);
		// draw inputs
		p.setPen(textColor);
		for (unsigned int i = 0; i < m_editingInputCount; i++)
		{
			if (m_editingInputCount * m_editingDisplayPage + i < editingTextCount)
			{
				if (m_editingInputIsFloat[m_editingInputCount * m_editingDisplayPage + i] == true)
				{
					QColor curForeColor = foreColor;
					// unused bool
					bool isTrue = false;
					float inputValue = getInputAttribValue(m_editingInputCount * m_editingDisplayPage + i, &isTrue);
					if (dataArray->getAutomationModel(m_selectedLocation) != nullptr  && static_cast<int>(getInputAttribValue(6, &isTrue)) == i - 1)
					{
						curForeColor = *dataArray->getAutomatedColor();
					}
					else if (dataArray->getIsAutomatableEffectable() == true && static_cast<int>(getInputAttribValue(7, &isTrue)) == i - 1)
					{
						curForeColor = *dataArray->getActiveColor();
					}
					p.fillRect(i * segmentLength, m_graphHeight, segmentLength, m_editingHeight, backColor);
					p.fillRect(i * segmentLength, m_graphHeight, mapInputPos(inputValue, segmentLength), m_editingHeight, curForeColor);
					p.drawText(i * segmentLength, m_graphHeight + m_editingHeight / 2,
						getTextFromDisplayLength(m_editingText[m_editingInputCount * m_editingDisplayPage + i], segmentLength));
				}
				else
				{
					QColor curForeColor = *dataArray->getFillColor();
					bool isTrue = false;
					// unused float
					float inputValue = getInputAttribValue(m_editingInputCount * m_editingDisplayPage + i, &isTrue);
					if (isTrue == true)
					{
						curForeColor = *dataArray->getActiveColor();
					}
					p.fillRect(i * segmentLength, m_graphHeight, segmentLength, m_editingHeight, curForeColor);
					p.drawText(i * segmentLength, m_graphHeight + m_editingHeight / 2,
						getTextFromDisplayLength(m_editingText[m_editingInputCount * m_editingDisplayPage + i], segmentLength));
				}
			}
		}

		// draw "next page" button
		p.fillRect(m_editingInputCount * segmentLength, m_graphHeight, segmentLength, m_editingHeight, *dataArray->getFillColor());
		p.setPen(textColor);
		p.drawText(m_editingInputCount * segmentLength, m_graphHeight + m_editingHeight / 2, ">>");
		// draw outline
		p.setPen(*dataArray->getLineColor());
		p.drawLine(0, m_graphHeight, width(), m_graphHeight);
		for (unsigned int i = 1; i < m_editingInputCount + 1; i++)
		{
			if (m_editingInputCount * m_editingDisplayPage + i < editingTextCount || i >= m_editingInputCount)
			{
				p.drawLine(i * segmentLength, m_graphHeight, i * segmentLength, height());
			}
		}
	}
}

void PointGraphView::modelChanged()
{
	auto gModel = model();
	QObject::connect(gModel, SIGNAL(dataChanged()),
			this, SLOT(updateGraph()));
	QObject::connect(gModel, SIGNAL(lengthChanged()),
			this, SLOT(updateGraph()));
}

void PointGraphView::updateGraph()
{
	update();
}

std::pair<float, float> PointGraphView::mapMousePos(int xIn, int yIn, bool nonNegativeIn)
{
	if (nonNegativeIn == true)
	{
		// mapping the position to 0 - 1, 0 - 1 using qWidget width and height
		return std::pair<float, float>(
			static_cast<float>(xIn / (float)width()),
			static_cast<float>(yIn / (float)m_graphHeight));
	}
	else
	{
		// mapping the position to 0 - 1, -1 - 1 using qWidget width and height
		return std::pair<float, float>(
			static_cast<float>(xIn / (float)width()),
			static_cast<float>((yIn * 2.0f / (float)m_graphHeight) - 1.0f));
	}
}
std::pair<int, int> PointGraphView::mapDataPos(float xIn, float yIn, bool nonNegativeIn)
{
	if (nonNegativeIn == true)
	{
		// mapping the point/sample positon to mouse/view position
		return std::pair<int, int>(
			static_cast<int>(xIn * width()),
			static_cast<int>(yIn * m_graphHeight));
	}
	else
	{
		// mapping the point/sample positon to mouse/view position
		return std::pair<int, int>(
			static_cast<int>(xIn * width()),
			static_cast<int>((yIn / 2.0f + 0.5f) * m_graphHeight));
	}
}
std::pair<float, float> PointGraphView::mapDataCurvePos(float xAIn, float yAIn, float xBIn, float yBIn, float curveIn)
{
	return std::pair<float, float>(
		(xAIn + xBIn) / 2.0f,
		yAIn + (curveIn / 2.0f + 0.5f) * (yBIn - yAIn));
}
std::pair<int, int> PointGraphView::mapDataCurvePos(int xAIn, int yAIn, int xBIn, int yBIn, float curveIn)
{
	return std::pair<int, int>(
		(xAIn + xBIn) / 2,
		yAIn + static_cast<int>((curveIn / 2.0f + 0.5f) * (yBIn - yAIn)));
}
int PointGraphView::mapInputPos(float inputValueIn, unsigned int displayLengthIn)
{
	return (inputValueIn / 2.0f + 0.5f) * displayLengthIn;
}

float PointGraphView::getDistance(int xAIn, int yAIn, int xBIn, int yBIn)
{
	return std::sqrt(static_cast<float>((xAIn - xBIn) * (xAIn - xBIn) + (yAIn - yBIn) * (yAIn - yBIn)));
}
float PointGraphView::getDistance(float xAIn, float yAIn, float xBIn, float yBIn)
{
	return std::sqrt((xAIn - xBIn) * (xAIn - xBIn) + (yAIn - yBIn) * (yAIn - yBIn));
}

bool PointGraphView::isGraphPressed(int mouseYIn)
{
	if (m_isEditingActive == true && mouseYIn > m_graphHeight)
	{
		return false;
	}
	return true;
}
int PointGraphView::getPressedInput(int mouseXIn, int mouseYIn, unsigned int inputCountIn)
{
	int output = -1;
	if (m_isEditingActive == true && mouseYIn > m_graphHeight)
	{
		output = mouseXIn * inputCountIn / width();
	}
	if (output > inputCountIn)
	{
		output = inputCountIn;
qDebug("getPressedInput x location ERRROR: %d", mouseXIn);
	}
	return output;
}
float PointGraphView::getInputAttribValue(unsigned int editingArrayLocationIn, bool* valueOut)
{
	float output = 0.0f;
	if (m_isSelected == true)
	{
		switch (editingArrayLocationIn)
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
		}
	}
	return output;
}
void PointGraphView::setInputAttribValue(unsigned int editingArrayLocationIn, float floatValueIn, bool boolValueIn)
{
	qDebug("setInputAttribValue started");
	if (m_isSelected == true)
	{
		float clampedValue = floatValueIn < -1.0f ? -1.0f : floatValueIn > 1.0f ? 1.0f : floatValueIn;
		unsigned int clampedValueB = 0;
		switch (editingArrayLocationIn)
		{
			case 0:
				m_selectedLocation = model()->getDataArray(m_selectedArray)->setX(m_selectedLocation, clampedValue < 0.0f ? 0.0f : clampedValue);
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
					clampedValueB = static_cast<unsigned int>((floatValueIn < 0.0f ? 0.0f : floatValueIn > 5.0f ? 5.0f : floatValueIn));
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
					clampedValueB = static_cast<unsigned int>((floatValueIn < 0.0f ? 0.0f : floatValueIn > 4.0f ? 4.0f : floatValueIn));
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
					clampedValueB = static_cast<unsigned int>((floatValueIn < 0.0f ? 0.0f : floatValueIn > 4.0f ? 4.0f : floatValueIn));
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
		}
	}
	qDebug("setInputAttribValue finished");
}
QColor PointGraphView::getTextColorFromBaseColor(QColor baseColorIn)
{
	QColor output(255, 255, 255, 255);
	int colorSum = baseColorIn.red() + baseColorIn.green() + baseColorIn.blue();
	if (colorSum > 382)
	{
		output = QColor(0, 0, 0, 255);
	}
	return output;
}
QString PointGraphView::getTextFromDisplayLength(QString textIn, unsigned int displayLengthIn)
{
	int charLength = 4;
	QString output = "";
	int targetSize = displayLengthIn / charLength < textIn.size() ? displayLengthIn / charLength : textIn.size();
	for (unsigned int i = 0; i < targetSize; i++)
	{
		if (i + 3 < targetSize)
		{
			output = output + textIn[i];
		}
		else
		{
			output = output + QString(".");
		}
	}
	return output;
}

std::pair<float, float> PointGraphView::showCoordInputDialog()
{
	std::pair<float, float> curData(0.0f, 0.0f);
	if (m_isSelected == true)
	{
		curData = getSelectedData();
		double minValue = model()->getDataArray(m_selectedArray)->getNonNegative() == true ? 0.0 : -100.0;

		// show position input dialog
		bool ok;
		double changedX = QInputDialog::getDouble(this, tr("Set value"),
			tr("Please enter a new value between 0 and 100"),
			static_cast<double>(curData.first * 100.0f),
			0.0, 100.0, 0, &ok);
		if (ok == true)
		{
			curData.first = static_cast<float>(changedX) / 100.0f;
		}

		double changedY = QInputDialog::getDouble(this, tr("Set value"),
			tr("Please enter a new value between ") + QString::number(minValue) + tr(" and 100"),
			static_cast<double>(curData.second * 100.0f),
			minValue, 100.0, 2, &ok);
		if (ok == true)
		{
			curData.second = static_cast<float>(changedY) / 100.0f;
		}
	}
	return curData;
}
float PointGraphView::showInputDialog(float curInputValueIn)
{
	float output = 0.0f;

	bool ok;
	double changedPos = QInputDialog::getDouble(this, tr("Set value"),
		tr("Please enter a new value between -100 and 100"),
		static_cast<double>(curInputValueIn * 100.0f),
		-100.0, 100.0, 0, &ok);
	if (ok == true)
	{
		output = static_cast<float>(changedPos) / 100.0f;
	}

	return output;
}

void PointGraphView::selectData(int mouseXIn, int mouseYIn)
{
	qDebug("selectData");
	m_selectedLocation = 0;
	m_selectedArray = 0;
	m_isSelected = false;
	m_isCurveSelected = false;
	m_isEditingActive = false;

	for (unsigned int i = 0; i < model()->getDataArraySize(); i++)
	{
		PointGraphDataArray* dataArray = model()->getDataArray(i);
		if (dataArray->getIsSelectable() == true)
		{
			int location = searchForData(mouseXIn, mouseYIn, static_cast<float>(m_pointSize) / width(), dataArray, false);
			if (location > -1)
			{
	qDebug("selected data!");
				m_selectedLocation = location;
				m_selectedArray = i;
	qDebug("selected data location: %d, %d", location, i);
				m_isSelected = true;
				m_isCurveSelected = false;
				m_isEditingActive = dataArray->getIsEditableAttrib();
				break;
			}
		}
	}
	if (m_isSelected == false)
	{
		for (unsigned int i = 0; i < model()->getDataArraySize(); i++)
		{
			PointGraphDataArray* dataArray = model()->getDataArray(i);
			if (dataArray->getIsSelectable() == true)
			{
				int location = searchForData(mouseXIn, mouseYIn, static_cast<float>(m_pointSize) / width(), dataArray, true);
				if (location > -1)
				{
		qDebug("selected data curve!");
					m_selectedLocation = location;
					m_selectedArray = i;
		qDebug("selected data curve location: %d, %d", location, i);
					m_isSelected = true;
					m_isCurveSelected = true;
					m_isEditingActive = dataArray->getIsEditableAttrib();
					break;
				}
			}
		}
	}
	qDebug("selectDataEnd");
}

int PointGraphView::searchForData(int mouseXIn, int mouseYIn, float maxDistanceIn, PointGraphDataArray* arrayIn, bool curvedIn)
{
	int output = -1;
	float maxDistance = maxDistanceIn * 2.0f;
	qDebug("searchData");

	std::pair<float, float> transformedMouse = mapMousePos(mouseXIn, mouseYIn, arrayIn->getNonNegative());

	// we dont use this bool
	bool found = false;
	bool isBefore = false;
	// get the nearest data to the mouse pos (x) in an optimalized way
	int location = arrayIn->getNearestLocation(transformedMouse.first, &found, &isBefore);
	qDebug("selected location: %d", location);

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
		qDebug("selected x distance: %f", std::abs(dataX - transformedMouse.first));
		if (std::abs(dataX - transformedMouse.first) <= maxDistance)
		{
			// calculate real distance (x and y)
			float curDistance = getDistance(transformedMouse.first * 2.0f, transformedMouse.second,
				dataX * 2.0f, dataY);

			qDebug("selected full distance: %f  (%d, %d)", curDistance, location, (location - curvedBefore));
			if (curDistance <= maxDistance)
			{
				qDebug("search successful");
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
				qDebug("search V2AAA temp, start: %d, end: %d", searchStart, searchEnd);
				// getting where the search needs to end
				for (int i = location - curvedBefore + 1; i < arrayIn->size(); i++)
				{
					if (std::abs(arrayIn->getX(i) - transformedMouse.first) > maxDistance)
					{
						searchEnd = i - 1 - (i > 0 && curvedIn == true ? 1 : 0);
						break;
					}
				}
							qDebug("search V2, start: %d, end: %d", searchStart, searchEnd);
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
			qDebug("search v2 full distance %d: %f  / %f     y: %f, my: %f    y:%f  size:%d", i, curDistance, maxDistance, dataY, transformedMouse.second, arrayIn->getY(i), arrayIn->size());
						if (curDistance <= maxDistance)
						{
							qDebug("search successful V2");
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

PointGraphModel::PointGraphModel(unsigned int maxLengthIn, Model* parentIn, bool defaultConstructedIn) :
	Model(parentIn, tr("PointGraphModel"), defaultConstructedIn)
{
	m_maxLength = maxLengthIn;
	m_dataArrays = {};
}

PointGraphModel::~PointGraphModel()
{
	m_dataArrays.clear();
}

unsigned int PointGraphModel::addArray(std::vector<std::pair<float, float>>* arrayIn, bool isCurvedIn)
{
	unsigned int location = addArray();
	m_dataArrays[location].setDataArray(arrayIn, isCurvedIn);
	return location;
}

unsigned int PointGraphModel::addArray(std::vector<float>* arrayIn, bool isCurvedIn)
{
	unsigned int location = addArray();
	m_dataArrays[location].setDataArray(arrayIn, isCurvedIn);
	return location;
}

unsigned int PointGraphModel::addArray()
{
	PointGraphDataArray tempArray(
		false, false, false, false, false, false, false, false, true, this);
	m_dataArrays.push_back(tempArray);
	return m_dataArrays.size() - 1;
}

void PointGraphModel::delArray(unsigned int locationIn)
{
	for (unsigned int i = locationIn; i < m_dataArrays.size() - 1; i++)
	{
		m_dataArrays[i] = m_dataArrays[i + 1];
	}
	m_dataArrays.pop_back();
}

void PointGraphModel::dataArrayChanged()
{
	emit dataChanged();
}

void PointGraphModel::dataArrayStyleChanged()
{
	emit styleChanged();
}
unsigned int PointGraphModel::getDataArrayLocation(PointGraphDataArray* dataArrayIn)
{
	return reinterpret_cast<std::uintptr_t>(&dataArrayIn) -
		reinterpret_cast<std::uintptr_t>(&m_dataArrays) / sizeof(PointGraphDataArray);
}

// PointGraphDataArray ------

PointGraphDataArray::PointGraphDataArray()
{
	m_isFixedSize = false;
	m_isFixedValue = false;
	m_isFixedPos = false;
	m_isFixedEndPoints = false;
	m_isSelectable = false;
	m_isEditableAttrib = false;
	m_isAutomatableEffectable = false;
	m_isSaveable = false;
	m_nonNegative = false;
	
	m_lineColor = QColor(200, 200, 200, 255);
	m_activeColor = QColor(255, 255, 255, 255);
	// is not enabled by default
	m_fillColor = QColor(0, 0, 0, 0);
	m_automatedColor = QColor(0, 0, 0, 0);

	m_effectorLocation = -1;

	m_dataArray = {};
	m_isDataChanged = true;
	m_bakedValues = {};
}

PointGraphDataArray::PointGraphDataArray(
	bool isFixedSizeIn, bool isFixedValueIn, bool isFixedPosIn, bool nonNegativeIn,
	bool isFixedEndPointsIn, bool isSelectableIn, bool isEditableAttribIn, bool isAutomatableEffectableIn,
	bool isSaveableIn, PointGraphModel* parentIn)
{
	m_isFixedSize = isFixedSizeIn;
	m_isFixedValue = isFixedValueIn;
	m_isFixedPos = isFixedPosIn;
	m_isFixedEndPoints = isFixedEndPointsIn;
	m_isSelectable = isSelectableIn;
	m_isEditableAttrib = isEditableAttribIn;
	m_isAutomatableEffectable = isAutomatableEffectableIn;
	m_isSaveable = isSaveableIn;
	m_nonNegative = nonNegativeIn;
	
	m_lineColor = QColor(200, 200, 200, 255);
	m_activeColor = QColor(255, 255, 255, 255);
	// is not enabled by default
	m_fillColor = QColor(0, 0, 0, 0);

	m_effectorLocation = -1;

	// m_dataArray = {};
	m_isDataChanged = true;
	// m_bakedValues = {};

	updateConnections(parentIn);
}

PointGraphDataArray::~PointGraphDataArray()
{
	m_dataArray.clear();
}

void PointGraphDataArray::updateConnections(PointGraphModel* parentIn)
{
	// call PointGraphModel signals without qt
	m_parent = parentIn;
}

void PointGraphDataArray::setIsFixedSize(bool valueIn)
{
	m_isFixedSize = valueIn;
	dataChanged();
}
void PointGraphDataArray::setIsFixedValue(bool valueIn)
{
	m_isFixedValue = valueIn;
	dataChanged();
}
void PointGraphDataArray::setIsFixedPos(bool valueIn)
{
	m_isFixedPos = valueIn;
	dataChanged();
}
void PointGraphDataArray::setIsFixedEndPoints(bool valueIn)
{
	m_isFixedEndPoints = valueIn;
	formatDataArrayEndPoints();
	dataChanged();
}
void PointGraphDataArray::setIsSelectable(bool valueIn)
{
	m_isSelectable = valueIn;
	dataChanged();
}
void PointGraphDataArray::setIsEditableAttrib(bool valueIn)
{
	m_isEditableAttrib = valueIn;
	dataChanged();
}
void PointGraphDataArray::setIsAutomatableEffectable(bool valueIn)
{
	m_isAutomatableEffectable = valueIn;
	if (valueIn == false)
	{
		setEffectorArrayLocation(-1);
	}
	else
	{
		// setEffectorArray will call dataChanged()
		dataChanged();
	}
}
void PointGraphDataArray::setIsSaveable(bool valueIn)
{
	m_isSaveable = valueIn;
	dataChanged();
}
void PointGraphDataArray::setNonNegative(bool valueIn)
{
	m_nonNegative = valueIn;
	dataChanged();
}
void PointGraphDataArray::setLineColor(QColor colorIn)
{
	m_lineColor = colorIn;
	styleChanged();
}
void PointGraphDataArray::setActiveColor(QColor colorIn)
{
	m_activeColor = colorIn;
	styleChanged();
}
void PointGraphDataArray::setFillColor(QColor colorIn)
{
	m_fillColor = colorIn;
	styleChanged();
}
void PointGraphDataArray::setAutomatedColor(QColor colorIn)
{
	m_automatedColor = colorIn;
	styleChanged();
}
bool PointGraphDataArray::setEffectorArrayLocation(unsigned int locationIn)
{
	bool found = true;
	if (locationIn >= 0)
	{
		unsigned int curLocation = m_parent->getDataArrayLocation(this);
		qDebug("setEffectorArrayLocation cur_loaction %d", curLocation);
		int arrayLocation = locationIn;
		found = false;
		for (unsigned int i = 0; i < m_parent->getDataArraySize(); i++)
		{
			arrayLocation = m_parent->getDataArray(arrayLocation)->getEffectorArrayLocation();
			if (arrayLocation == -1)
			{
				break;
			}
			else if(arrayLocation == curLocation)
			{
				found = true;
				break;
			}
		}
		if (found == false)
		{
			m_effectorLocation = locationIn;
			dataChanged();
		}
	}
	else
	{
		if (m_effectorLocation != -1)
		{
			dataChanged();
		}
		m_effectorLocation = -1;
	}
	return !found;
}

bool PointGraphDataArray::getIsFixedSize()
{
	return m_isFixedSize;
}
bool PointGraphDataArray::getIsFixedValue()
{
	return m_isFixedValue;
}
bool PointGraphDataArray::getIsFixedPos()
{
	return m_isFixedPos;
}
bool PointGraphDataArray::getIsFixedEndPoints()
{
	return m_isFixedEndPoints;
}
bool PointGraphDataArray::getIsSelectable()
{
	return m_isSelectable;
}
bool PointGraphDataArray::getIsEditableAttrib()
{
	return m_isEditableAttrib;
}
bool PointGraphDataArray::getIsAutomatableEffectable()
{
	return m_isAutomatableEffectable;
}
bool PointGraphDataArray::getIsSaveable()
{
	return m_isSaveable;
}
bool PointGraphDataArray::getNonNegative()
{
	return m_nonNegative;
}
QColor* PointGraphDataArray::getLineColor()
{
	return &m_lineColor;
}
QColor* PointGraphDataArray::getActiveColor()
{
	return &m_activeColor;
}
QColor* PointGraphDataArray::getFillColor()
{
	return &m_fillColor;
}
QColor* PointGraphDataArray::getAutomatedColor()
{
	return &m_automatedColor;
}
int PointGraphDataArray::getEffectorArrayLocation()
{
	return m_effectorLocation;
}

// array:

int PointGraphDataArray::add(float xIn)
{
	int location = -1;
	if (m_dataArray.size() < m_parent->getMaxLength())
	{
	qDebug("add 1. success");
		bool found = false;
		bool isBefore = false;
		location = getNearestLocation(xIn, &found, &isBefore);
		if (found == false && m_isFixedSize == false)
		{
	qDebug("add 2. success, nearest: %d", location);
			int targetLocation = -1;
			bool dataChangedVal = false;
			// if getNearestLocation returned a value
			if (location >= 0)
			{
	qDebug("add 3. success, nearest: %d", location);
				targetLocation = location;
				// slide the new data if the closest data pos is bigger
				if (isBefore == true)
				{
					// we are adding one value, so dataArray.size() will be a valid location
					if (targetLocation < m_dataArray.size())
					{
						targetLocation++;
					}
				}
				m_dataArray.push_back(PointGraphPoint(xIn, 0.0f));
	qDebug("add 4. success, target: %d", targetLocation);
				swap(m_dataArray.size() - 1, targetLocation, true);
				dataChangedVal = true;
			}
			else if (m_dataArray.size() <= 0)
			{
	qDebug("add 5. success");
				m_dataArray.push_back(PointGraphPoint(xIn, 0.0f));
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
				dataChanged();
			}
		}
	}
	return location;
}

void PointGraphDataArray::del(unsigned int locationIn)
{
	if (m_isFixedSize == false && locationIn < m_dataArray.size())
	{
		swap(locationIn, m_dataArray.size() - 1, true);
		m_dataArray.pop_back();
		if (locationIn == 0 || locationIn == m_dataArray.size() - 1)
		{
			formatDataArrayEndPoints();
		}
		dataChanged();
	}
}

// TODO input scaleing values
void PointGraphDataArray::formatArray(bool clampIn, bool sortIn)
{
	// clamp
	if (clampIn == true)
	{
		for (unsigned int i = 0; i < m_dataArray.size(); i++)
		{
			if (m_dataArray[i].m_x > 1)
			{
				m_dataArray[i].m_x = 1;
			}
			if (m_dataArray[i].m_y > 1)
			{
				m_dataArray[i].m_y = 1;
			}
			if (m_nonNegative == true)
			{
				if (m_dataArray[i].m_y < 0)
				{
					m_dataArray[i].m_y = 0;
				}
			}
			else
			{
				if (m_dataArray[i].m_y < -1)
				{
					m_dataArray[i].m_y = -1;
				}
			}
		}
		formatDataArrayEndPoints();
	}

	// sort
	if (sortIn == true)
	{
		std::sort(m_dataArray.begin(), m_dataArray.end(),
			[](PointGraphPoint a, PointGraphPoint b)
			{
				return a.m_x > b.m_x;
			});
	}

	// delete duplicates
	unsigned int lastPos = 0;
	if (m_dataArray.size() > 0)
	{
		lastPos = m_dataArray[0].m_x;
	}
	for (unsigned int i = 1; i < m_dataArray.size(); i++)
	{
		if (m_dataArray[i].m_x == lastPos)
		{
			del(i);
		}
		else
		{
			lastPos = m_dataArray[i].m_x;
		}
	}
}

int PointGraphDataArray::getLocation(float xIn)
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

int PointGraphDataArray::getNearestLocation(float xIn, bool* foundOut, bool* isBeforeOut)
{
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
		//if (mid + 1 < m_dataArray.size())
		//{
			//bool isBeforeOutB = xIn < m_dataArray[mid].m_x ? true : m_dataArray[mid + 1].m_x < xIn;
			// if (isBeforeOutB != *isBeforeOut)
			// {
			//	qDebug("getNearestLocation, BEFOREBUG xIn: %f", xIn);
			//}
		//}
		*isBeforeOut = xIn >= m_dataArray[mid + outputDif].m_x;
		return mid + outputDif;
	}
	//qDebug("getNearestLocation, xIn: %f", xIn);
	*foundOut = false;
	*isBeforeOut = false;
	return -1;
}

float PointGraphDataArray::getValueAtPosition(float xIn)
{
	float output = 0.0f;
	bool found = false;
	bool isBefore = false;
	int locationBefore = getNearestLocation(xIn, &found, &isBefore);
	// get values of points (process effect if it effects only points)
	// draw line
	// process effect (if it effects lines too)
	if (locationBefore >= 0)
	{
		// y limit for clamping
		float lowerLimit = m_nonNegative == true ? 0.0f : -1.0f;
		// location after should be location before + 1
		// or it is equal to locationBefore when it is on an edge
		locationBefore = isBefore == true ? locationBefore : locationBefore - 1;
		int locationAfter = locationBefore + 1;
		if (locationAfter >= m_dataArray.size())	
		{
			locationAfter = locationBefore;
		}
		if (locationBefore < 0)	
		{
			locationBefore++;
			locationAfter = locationBefore;
		}

		float pointEffectYBefore = processAutomation(locationBefore, 0);
		float pointEffectYAfter = processAutomation(locationAfter, 0);

	//qDebug("getVALUE, locationBefore1: %d", locationBefore);
	//qDebug("getVALUE, locationAfer: %d", locationAfter);
		// temp effecor data
		PointGraphDataArray* tempEArray = nullptr;
		bool tempEFound = false;
		bool tempEIsBefore = false;
		int tempELocation = -1;
		// if this data array has an effector data array
		if (m_effectorLocation >= 0)
		{
			tempEArray = m_parent->getDataArray(m_effectorLocation);

			// do not change order
			// locationAfter
			tempELocation = tempEArray->getNearestLocation(m_dataArray[locationAfter].m_x, &tempEFound, &tempEIsBefore);
			tempELocation = tempEIsBefore == true ? tempELocation : tempELocation > 0 ? tempELocation - 1 : tempELocation;
			if (tempELocation >= 0 && tempEArray->getEffectOnlyPoints(tempELocation) == true)
			{
				float effectValue = tempEArray->getValueAtPosition(m_dataArray[locationAfter].m_x);
				// apply effects
				pointEffectYAfter = processEffect(pointEffectYAfter,
					effectValue, tempEArray, tempELocation, lowerLimit);
			}

			// locationBefore
			tempELocation = tempEArray->getNearestLocation(m_dataArray[locationBefore].m_x, &tempEFound, &tempEIsBefore);
			tempELocation = tempEIsBefore == true ? tempELocation : tempELocation > 0 ? tempELocation - 1 : tempELocation;
			// if the effector point can only change points
			if (tempELocation >= 0 && tempEArray->getEffectOnlyPoints(tempELocation) == true)
			{
				float effectValue = tempEArray->getValueAtPosition(m_dataArray[locationBefore].m_x);
				// apply effects
				pointEffectYBefore = processEffect(pointEffectYBefore,
					effectValue, tempEArray, tempELocation, lowerLimit);
			}
		}

		if (found == true)
		{
			output = pointEffectYAfter;
		}
		else if (locationAfter == locationBefore)
		{
			// if the nearest point is an edge
			output = pointEffectYBefore;
		}
		else
		{
			float transformedX = (xIn - m_dataArray[locationBefore].m_x) / (m_dataArray[locationAfter].m_x - m_dataArray[locationBefore].m_x);
			// type effects
			unsigned int type = getType(locationBefore);
			float fadeOutStart = 0.95f;
			if (type == 0)
			{
				output = processCurve(pointEffectYBefore, pointEffectYAfter, processAutomation(locationBefore, 1), transformedX);
			}
			else if (type == 1)
			{
				output = processCurve(pointEffectYBefore, pointEffectYAfter, processAutomation(locationBefore, 1), transformedX);
				output = output + processLineTypeSine(transformedX, processAutomation(locationBefore, 2),
					processAutomation(locationBefore, 3), fadeOutStart);
			}
			else if (type == 2)
			{
				output = processCurve(pointEffectYBefore, pointEffectYAfter, 0.0f, transformedX);
				output = output + processLineTypeSineB(transformedX, processAutomation(locationBefore, 2),
					processAutomation(locationBefore, 3), processAutomation(locationBefore, 3), fadeOutStart);
			}
			else if (type == 3)
			{
				output = processCurve(pointEffectYBefore, pointEffectYAfter, 0.0f, transformedX);
				output = output + processLineTypePeak(transformedX, processAutomation(locationBefore, 2),
					processAutomation(locationBefore, 3), processAutomation(locationBefore, 1), fadeOutStart);
			}
			else if (type == 4)
			{
				output = processCurve(pointEffectYBefore, pointEffectYAfter, processAutomation(locationBefore, 1), transformedX);
				output = processCurve(pointEffectYBefore, pointEffectYAfter, m_dataArray[locationBefore].m_c, transformedX);
				output = output + processLineTypeSteps(transformedX, output, processAutomation(locationBefore, 2),
					processAutomation(locationBefore, 3), fadeOutStart);
			}
			else if (type == 5)
			{
				output = processCurve(pointEffectYBefore, pointEffectYAfter, 0.0f, transformedX);
				output = output + processLineTypeRandom(transformedX, processAutomation(locationBefore, 2),
					processAutomation(locationBefore, 3), processAutomation(locationBefore, 1), fadeOutStart);
			}
			//qDebug("getVALUE, value2: %f %f -  %d  %d  - %f   %f", output, xIn, locationBefore, locationAfter, transformedX, pointEffectBefore);
			//qDebug("getVALUE, transfrmedX: %f", transformedX);
			//qDebug("getVALUE, x: %f", xIn);
		}

		// if this data array has an effector data array
		// and it does not only effect data points
		if (m_effectorLocation >= 0 && tempELocation >= 0 && tempEArray->getEffectOnlyPoints(tempELocation) == true)
		{
			float effectValue = tempEArray->getValueAtPosition(xIn);
			output = processEffect(output,
				effectValue, tempEArray, tempELocation, lowerLimit);
		}
	}
	return output;
}

void PointGraphDataArray::setDataArray(std::vector<std::pair<float, float>>* dataArrayIn, bool isCurvedIn)
{
	m_dataArray.clear();
	for (unsigned int i = 0; i < dataArrayIn->size(); i++)
	{
		m_dataArray.push_back(PointGraphPoint(dataArrayIn->operator[](i).first, dataArrayIn->operator[](i).second));
		if (isCurvedIn == true)
		{
			// TODO
		}
	}
}
void PointGraphDataArray::setDataArray(std::vector<float>* dataArrayIn, bool isCurvedIn)
{
	m_dataArray.clear();
	for (unsigned int i = 0; i < dataArrayIn->size(); i++)
	{
		m_dataArray.push_back(PointGraphPoint((i / static_cast<float>(dataArrayIn->size())), dataArrayIn->operator[](i)));
		if (isCurvedIn == true)
		{
			// TODO
		}
	}
}

unsigned int PointGraphDataArray::setX(unsigned int locationIn, float xIn)
{
	int location = locationIn;
	if (m_isFixedPos == false && xIn <= 1.0f)
	{
		bool found = false;
		bool isBefore = false;
		location = getNearestLocation(xIn, &found, &isBefore);
		// if an other point was not found at xIn
		// and if getNearestLocation returned a value
		// and if dataArray end points are changeable
		if (found == false && ((m_isFixedEndPoints == true &&
			locationIn < m_dataArray.size() - 1 && location > 0) ||
			m_isFixedEndPoints == false))
		{
			int targetLocation = location;
			// bool dataChangedVal = false;
			// if getNearestLocation returned a value
			if (location >= 0)
			{
				// slide the new data if the closest data pos is bigger TODO test ifs
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

void PointGraphDataArray::setY(unsigned int locationIn, float yIn)
{
	if (m_isFixedValue == false)
	{
		if ((m_isFixedEndPoints == true && locationIn < m_dataArray.size() - 1 &&
			locationIn > 0) || m_isFixedEndPoints == false)
		{
			m_dataArray[locationIn].m_y = yIn;
			dataChanged();
		}
	}
}

void PointGraphDataArray::setC(unsigned int locationIn, float cIn)
{
	m_dataArray[locationIn].m_c = cIn;
	dataChanged();
}
void PointGraphDataArray::setValA(unsigned int locationIn, float valueIn)
{
	m_dataArray[locationIn].m_valA = valueIn;
	dataChanged();
}
void PointGraphDataArray::setValB(unsigned int locationIn, float valueIn)
{
	m_dataArray[locationIn].m_valB = valueIn;
	dataChanged();
}
void PointGraphDataArray::setType(unsigned int locationIn, unsigned int typeIn)
{
	// set the type without changing the automated attribute location
	m_dataArray[locationIn].m_type = typeIn;
	dataChanged();
}
void PointGraphDataArray::setAutomatedAttrib(unsigned int locationIn, unsigned int attribLocationIn)
{
	if (m_isAutomatableEffectable == true)
	{
		// only 4 attributes can be automated (y, c, valA, valB)
		attribLocationIn = attribLocationIn > 3 ? 0 : attribLocationIn;
		// set automated location correctly (effected_location = automatedEffectedLocation % 4)
		m_dataArray[locationIn].m_automatedEffectedAttribLocations = attribLocationIn * 4 + getEffectedAttribLocation(locationIn);
		dataChanged();
	}
}
void PointGraphDataArray::setEffectedAttrib(unsigned int locationIn, unsigned int attribLocationIn)
{
	if (m_isAutomatableEffectable == true)
	{
		// only 4 attributes can be effected (y, c, valA, valB)
		attribLocationIn = attribLocationIn > 3 ? 0 : attribLocationIn;
		// set effected location correctly
		m_dataArray[locationIn].m_automatedEffectedAttribLocations = attribLocationIn + getAutomatedAttribLocation(locationIn);
		dataChanged();
	}
}
unsigned int PointGraphDataArray::getAutomatedAttribLocation(unsigned int locationIn)
{
	return m_dataArray[locationIn].m_automatedEffectedAttribLocations / 4;
}
unsigned int PointGraphDataArray::getEffectedAttribLocation(unsigned int locationIn)
{
	return m_dataArray[locationIn].m_automatedEffectedAttribLocations % 4;
}
void PointGraphDataArray::setEffectOnlyPoints(unsigned int locationIn, bool boolIn)
{
	if (m_isAutomatableEffectable == true)
	{
		m_dataArray[locationIn].m_effectOnlyPoints = boolIn;
		// this change does not effect the output values of the dataArray
		// so dataChanged() is not called
	}
}
bool PointGraphDataArray::getEffect(unsigned int locationIn, unsigned int effectNumberIn)
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
	}
	return false;
}
void PointGraphDataArray::setEffect(unsigned int locationIn, unsigned int effectNumberIn, bool boolIn)
{
	if (m_isAutomatableEffectable == true)
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
		}
	}
}
void PointGraphDataArray::setAutomated(unsigned int locationIn, bool isAutomatedIn)
{
	if (m_isAutomatableEffectable == true)
	{
		if (isAutomatedIn == true)
		{
			if (m_dataArray[locationIn].m_automationModel == nullptr)
			{
				m_dataArray[locationIn].m_automationModel = new FloatModel(0.0f, -1.0f, 1.0f, 0.05f, nullptr, QString(), false);
			}
		}
		else
		{
			delete m_dataArray[locationIn].m_automationModel;
			m_dataArray[locationIn].m_automationModel = nullptr; // is this needed? TODO
		}
	}
}

void PointGraphDataArray::swap(unsigned int locationAIn, unsigned int locationBIn, bool slide)
{
	if (locationAIn != locationBIn)
	{
		if (slide == true)
		{
			qDebug("swap:    -------");
			qDebug("first.: %d, second.: %d", locationAIn, locationBIn);
			
			for (unsigned int i = 0; i < m_dataArray.size(); i++)
			{
				qDebug("   - i: %d  -  x: %f", i, m_dataArray[i].m_x);
			}
			
			if (locationAIn < locationBIn)
			{
				PointGraphPoint swap = m_dataArray[locationAIn];
				for (unsigned int i = locationAIn; i < locationBIn; i++)
				{
					m_dataArray[i] = m_dataArray[i + 1];
				}
				m_dataArray[locationBIn] = swap;
			}
			else
			{
				PointGraphPoint swap = m_dataArray[locationAIn];
				for (unsigned int i = locationAIn; i > locationBIn; i--)
				{
					m_dataArray[i] = m_dataArray[i - 1];
				}
				m_dataArray[locationBIn] = swap;
			}
			
			qDebug(" --------- ");
			for (unsigned int i = 0; i < m_dataArray.size(); i++)
			{
				qDebug("   - i: %d  -  x: %f", i, m_dataArray[i].m_x);
			}
		}
		else
		{
			PointGraphPoint swap = m_dataArray[locationAIn];
			m_dataArray[locationAIn] = m_dataArray[locationBIn];
			m_dataArray[locationBIn] = swap;
		}
		dataChanged();
	}
}
float PointGraphDataArray::processEffect(float valueIn, float effectValueIn,
	PointGraphDataArray* effectArrayIn, unsigned int effectLocationIn, float lowerLimitIn)
{
	float output = valueIn;
	if (effectArrayIn->getEffect(effectLocationIn, 6) == true)
	{
		// sine
		output = output + std::sin(effectValueIn * 100.0f);
	}
	if (effectArrayIn->getEffect(effectLocationIn, 4) == true)
	{
		// power
		output = std::pow(output, effectValueIn);
	}
	else if (effectArrayIn->getEffect(effectLocationIn, 5) == true)
	{
		// log
		output = std::log(output) / std::log(effectValueIn);
	}

	if (effectArrayIn->getEffect(effectLocationIn, 2) == true)
	{
		// multiply
		output = output * 5.0f * effectValueIn;
	}
	else if (effectArrayIn->getEffect(effectLocationIn, 3) == true)
	{
		output = output / 5.0f / effectValueIn;
		// divide
	}

	if (effectArrayIn->getEffect(effectLocationIn, 0) == true)
	{
		// add
		output += effectValueIn;
	}
	else if (effectArrayIn->getEffect(effectLocationIn, 1) == true)
	{
		// subtract
		output -= effectValueIn;
	}
	// clamp
	if (output  > 1.0f)
	{
		output = 1.0f;
	}
	else if (output < lowerLimitIn)
	{
		output = lowerLimitIn;
	}
	return output;
}
float PointGraphDataArray::processCurve(float valueBeforeIn, float valueAfterIn, float curveIn, float xIn)
{
	float absCurveIn = std::abs(curveIn);
	float pow = curveIn < 0.0f ? 1.0f - xIn : xIn;
	// float xVal = curveIn > 0.0f ? xIn + (1.0f - xIn) * absCurveIn : xIn * (1.0f - absCurveIn);
	// float xVal = curveIn > 0.0f ? 1.0 - xIn : xIn;
	pow = std::pow(pow, 1.0f - absCurveIn) - pow;

	//float output = valueBeforeIn + (valueAfterIn - valueBeforeIn) * xVal + log * (valueAfterIn - valueBeforeIn);

	float output = valueBeforeIn + (valueAfterIn - valueBeforeIn) * xIn;
	output = curveIn > 0.0f ? output + pow * (valueAfterIn - valueBeforeIn) : output - pow * (valueAfterIn - valueBeforeIn);
	if (valueBeforeIn > valueAfterIn)
	{
		output = output < valueAfterIn ? valueAfterIn : output > valueBeforeIn ? valueBeforeIn : output;
	}
	else
	{
		output = output < valueBeforeIn ? valueBeforeIn : output > valueAfterIn ? valueAfterIn : output;
	}
	return output;
}

float PointGraphDataArray::processAutomation(unsigned int locationIn, unsigned int attribLocationIn)
{
	float output = 0.0f;
	// if automated
	FloatModel* automationModel = getAutomationModel(locationIn);
	if (automationModel != nullptr)
	{
		unsigned int attribLocation = getAutomatedAttribLocation(locationIn);
		if (attribLocation = attribLocationIn)
		{
			output += automationModel->value();
			// qDebug("processAutomation -> value: %f", output);
		}
	}
	if (attribLocationIn == 0)
	{
		output += m_dataArray[locationIn].m_y;
	}
	else if (attribLocationIn == 1)
	{
		output += m_dataArray[locationIn].m_c;
	}
	else if (attribLocationIn == 2)
	{
		output += m_dataArray[locationIn].m_valA;
	}
	else if (attribLocationIn == 3)
	{
		output += m_dataArray[locationIn].m_valB;
	}
	
	output = output < -1.0f ? -1.0f : output > 1.0f ? 1.0f : output;
	return output;
}
float PointGraphDataArray::processLineTypeSine(float xIn, float valAIn, float valBIn, float fadeOutStartIn)
{
	// sine
	float output = valAIn * std::sin(xIn * valBIn);
	
	// fade out
	if (xIn > fadeOutStartIn)
	{
		output = output * (1.0f - xIn) / (1.0f - fadeOutStartIn);
	}
	return output;
}
float PointGraphDataArray::processLineTypeSineB(float xIn, float valAIn, float valBIn, float curveIn, float fadeOutStartIn)
{
	// sine
	float output = valAIn * std::sin(xIn * valBIn + curveIn * 10.0f);
	
	// fade out
	if (xIn > fadeOutStartIn)
	{
		output = output * (1.0f - xIn) / (1.0f - fadeOutStartIn);
	}
	return output;
}
float PointGraphDataArray::processLineTypePeak(float xIn, float valAIn, float valBIn, float curveIn, float fadeOutStartIn)
{
	// peak
	float output = std::pow((curveIn + 1.0f) * 0.2f + 0.01f, std::abs(xIn - (valBIn + 1.0f) * 0.5f) * 10.0f) * valAIn;

	// fade in
	float fadeInEnd = 1.0f - fadeOutStartIn;
	if (xIn < fadeInEnd)
	{
		output = output * xIn / fadeInEnd;
	}
	// fade out
	if (xIn > fadeOutStartIn)
	{
		output = output * (1.0f - xIn) / fadeInEnd;
	}
	return output;
}
float PointGraphDataArray::processLineTypeSteps(float xIn, float yIn, float valAIn, float valBIn, float fadeOutStartIn)
{
	// TODO opmtimalize
	// step
	float stepCount = std::floor(valAIn);
	float stepSize = 2.0f / stepCount;
	float curStep = std::floor(yIn / stepSize);
	float diff = curStep * stepSize - yIn;
	float output = diff * (valBIn + 1.0f) / 2.0f + stepSize * (2.0f - (valBIn + 1.0f) / 2.0f);

	// fade in
	float fadeInEnd = 1.0f - fadeOutStartIn;
	if (xIn < fadeInEnd)
	{
		output = output * xIn / fadeInEnd;
	}
	// fade out
	if (xIn > fadeOutStartIn)
	{
		output = output * (1.0f - xIn) / fadeInEnd;
	}
	return output;
}
float PointGraphDataArray::processLineTypeRandom(float xIn, float valAIn, float valBIn, float curveIn, float fadeOutStartIn)
{
	// randomize
	float blend = 50.0f + curveIn * 50.0f;
	int randomSeed = static_cast<int>(blend);
	blend = blend - randomSeed;
	std::srand(randomSeed);
	int curLocation = static_cast<int>(xIn * (1.0f + valBIn) * 50.0f);
	float output = 0.0f;
	// getting the current random value
	for (unsigned int i = 1; i <= curLocation; i++)
	{
		if (i == curLocation)
		{
			output = rand();
		}
		else
		{
			rand();
		}
	}
	output = output * blend * (blend - 2.0f) * - 1.0f - rand() * (1.0f - blend) * (-1.0f - blend);
	return output * valAIn;
}

void PointGraphDataArray::PointGraphDataArray::formatDataArrayEndPoints()
{
	if (m_isFixedEndPoints == true && m_dataArray.size() > 0)
	{
		m_dataArray[m_dataArray.size() - 1].m_x = 1;
		m_dataArray[m_dataArray.size() - 1].m_y = 1.0f;
		m_dataArray[0].m_x = 0;
		m_dataArray[0].m_y = m_nonNegative == true ? 0.0f : -1.0f;
	}
}

void PointGraphDataArray::dataChanged()
{
	m_parent->dataArrayChanged();
}
void PointGraphDataArray::styleChanged()
{
	m_parent->dataArrayStyleChanged();
}

} // namespace lmms
