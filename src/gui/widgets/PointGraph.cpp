

#include <vector>
#include <cmath>
#include <QPainter>
#include <QMouseEvent>
#include <QInputDialog>
#include <algorithm>

#include "PointGraph.h"


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

	m_selectedLocation = 0;
	m_selectedArray = 0;
	m_isSelected = false;

	m_lastTrackPoint.first = 0;
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

std::pair<unsigned int, float> PointGraphView::getSelectedData()
{
	std::pair<unsigned int, float> output(0, -2.0f);
	if (m_isSelected == true)
	{
		output = *model()->getDataArray(m_selectedArray)->getData(m_selectedLocation);
	}
	return output;
}
void PointGraphView::setSelectedData(std::pair<unsigned int, float> dataIn)
{
	if (m_isSelected == true)
	{
		qDebug("setSelectedData");
		model()->getDataArray(m_selectedArray)->setValue(m_selectedLocation, dataIn.second);
		qDebug("set value done");
		model()->getDataArray(m_selectedArray)->setPos(m_selectedLocation, dataIn.first);
		qDebug("set position done");
		m_isSelected = false;
	}
}

void PointGraphView::mousePressEvent(QMouseEvent* me)
{
	// get position
	int x = me->x();
	int y = me->y();
	if (me->button() == Qt::LeftButton)
	{
		// add
		m_addition = true;
		// show new inputDialog to change data if control is pressed
		if ((me->modifiers() & Qt::ControlModifier) == true)
		{
			selectData(x, y);
			// if selecting was successful
			if (m_isSelected == true)
			{
				// display dialog
				std::pair<unsigned int, float> curData = showInputDialog();
				// change data
				setSelectedData(curData);
			}
		}
		else
		{
			m_mousePress = true;
		}
	}
	else if (me->button() == Qt::RightButton)
	{
		// delete
		m_addition = false;
		m_mousePress = true;
	}
	m_mouseDown = true;
	qDebug("mousePressEnd ---------");
}

void PointGraphView::mouseMoveEvent(QMouseEvent* me)
{
	// get position
	int x = me->x();
	int y = me->y();
	m_mousePress = false;
}

void PointGraphView::mouseReleaseEvent(QMouseEvent* me)
{
	// get position
	int x = me->x();
	int y = me->y();
	if (m_mousePress == true)
	{
		// add/delete point
		selectData(x, y);
		if (m_isSelected == false && m_addition == true)
		{
			// if selection failed and addition
			// get the first editable daraArray and add value
			qDebug("release size: %ld", model()->getDataArraySize());
			for(unsigned int i = 0; i < model()->getDataArraySize(); i++)
			{
				std::pair<unsigned int, float> curMouseData = mapMousePos(x, y,
					model()->getDataArray(i)->getNonNegative()); // TODO optimize
				int location = model()->getDataArray(i)->add(curMouseData.first);
				// if adding was successful
				if (location >= 0)
				{
	qDebug("mouseRelease added %d", location);
					model()->getDataArray(i)->setValue(location, curMouseData.second);
					break;
				}
			}
		}
		else if (m_isSelected == true && m_addition == false)
		{
			// if selection was successful and deletion
			model()->getDataArray(m_selectedArray)->del(m_selectedLocation);
		}

		m_mousePress = false;
	}
	else
	{
		// add/set/delete line end

	}
	m_addition = false;
	m_mouseDown = false;
	m_mouseDrag = false;
	// triggering paintEvent
	qDebug("mouseReleaseEnd");
	update();
	emit drawn();
}

void PointGraphView::mouseDoubleClickEvent(QMouseEvent * me)
{
	// if a data/sample is selected then show input dialog to change the data
	if (m_isSelected == true && me->button() == Qt::LeftButton)
	{
		// display dialog
		std::pair<unsigned int, float> curData = showInputDialog();
		// change data
		setSelectedData(curData);
	}
}

void PointGraphView::paintEvent(QPaintEvent* pe)
{
	QPainter p(this);
	//QPainterPath pt(); // TODO

	qDebug("paintEvent");

	for (unsigned int i = 0; i < model()->getDataArraySize(); i++)
	{
		PointGraphDataArray* dataArray = model()->getDataArray(i);
		p.setPen(QPen(*dataArray->getLineColor(), 1));
		QColor gcolor = QColor(dataArray->getLineColor()->red(), dataArray->getLineColor()->green(),
			dataArray->getLineColor()->blue(), 100);

		unsigned int length = dataArray->size();

		if (m_isSelected == true)
		{
		p.drawLine(10, 10, 20, 20);
		}

		std::pair<int, int> posB(0, 0);
		if (length > 0)
		{
			std::pair<int, int> posA = mapDataPos(*dataArray->getData(0), dataArray->getNonNegative());

			// if first data/sample > 0, draw a line to the first data/sample from 0
			if (posA.first > 0)
			{
				//p.drawLine(0, posA.second, posA.first, posA.second);
			}
			// drawing lines
	qDebug("paint size: %d", length);
			for (unsigned int j = 0; j < length - 1; j++)
			{
				std::pair<int, int> posB = mapDataPos(*dataArray->getData(j + 1), dataArray->getNonNegative());
				// x1, y1, x2, y2
	//qDebug("paint positions: x: %d, y: %d, x2: %d, y2: %d", posA.first, posA.second, posB.first, posB.second);
				p.drawLine(posA.first, posA.second, posB.first, posB.second);
				posA = posB;
			}
			if (posA.first < width())
			{
				//p.drawLine(posA.first, posA.second, width(), posA.second);
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

std::pair<unsigned int, float> PointGraphView::mapMousePos(int xIn, int yIn, bool nonNegativeIn)
{
	if (nonNegativeIn == true)
	{
		// mapping the position to 0 - m_maxLength, 0 - 1 using qWidget width and height
		return std::pair<unsigned int, float>(
			static_cast<unsigned int>(xIn * (model()->getMaxLength() - 1) / width()),
			static_cast<float>(yIn / (float)height()));
	}
	else
	{
		// mapping the position to 0 - m_maxLength, -1 - 1 using qWidget width and height
		return std::pair<unsigned int, float>(
			static_cast<unsigned int>(xIn * (model()->getMaxLength() - 1) / width()),
			static_cast<float>((yIn * 2.0f / (float)height()) - 1.0f));
	}
}
std::pair<int, int> PointGraphView::mapDataPos(std::pair<unsigned int, float> posIn,
	bool nonNegativeIn)
{
	if (nonNegativeIn == true)
	{
		// mapping the point/sample positon to mouse/view position
		return std::pair<int, int>(
			static_cast<int>(posIn.first * width() / model()->getMaxLength()),
			static_cast<int>(posIn.second * height()));
	}
	else
	{
		// mapping the point/sample positon to mouse/view position
		return std::pair<int, int>(
			static_cast<int>(posIn.first * width() / model()->getMaxLength()),
			static_cast<int>((posIn.second / 2.0f + 0.5f) * height()));
	}
}

float PointGraphView::getDistance(int xAIn, int yAIn, int xBIn, int yBIn)
{
	return std::sqrt(static_cast<float>((xAIn - xBIn) * (xAIn - xBIn) + (yAIn - yBIn) * (yAIn - yBIn)));
}

std::pair<unsigned int, float> PointGraphView::showInputDialog()
{
	std::pair<unsigned int, float> curData(0, 0.0f);
	if (m_isSelected == true)
	{
		curData = getSelectedData();
		double minValue = model()->getDataArray(m_selectedArray)->getNonNegative() == true ? 0.0 : -1.0;

		// show position input dialog
		bool ok;
		double changedPos = QInputDialog::getDouble(this, tr("Set value"),
			tr("Please enter a new value between 0 and ") + QString::number(model()->getMaxLength()),
			static_cast<double>(curData.first),
			0.0, static_cast<double>(model()->getMaxLength()), 0, &ok);
		if (ok == true)
		{
			curData.first = static_cast<unsigned int>(changedPos);
		}

		double changedValue = QInputDialog::getDouble(this, tr("Set value"),
			tr("Please enter a new value between") + QString::number(minValue) + tr(" and 1.0"),
			static_cast<double>(curData.second),
			minValue, 1.0, 2, &ok);
		if (ok == true)
		{
			curData.second = static_cast<unsigned int>(changedValue);
		}
	}
	return curData;
}

void PointGraphView::selectData(int mouseXIn, int mouseYIn)
{
	qDebug("selectData");
	//float minDistance = m_pointSize + 1.0;
	m_selectedLocation = 0;
	m_selectedArray = 0;
	m_isSelected = false;

	std::pair<unsigned int, float> transformedMouseA = mapMousePos(mouseXIn, mouseYIn, false);
	std::pair<unsigned int, float> transformedMouseB = mapMousePos(mouseXIn, mouseYIn, true);
	std::pair<unsigned int, float>* transformedMouse = &transformedMouseA;

	for (unsigned int i = 0; i < model()->getDataArraySize(); i++)
	{
		PointGraphDataArray* dataArray = model()->getDataArray(i);
		if (dataArray->getNonNegative() == true)
		{
			transformedMouse = &transformedMouseB;
		}
		else
		{
			transformedMouse = &transformedMouseA;
		}
		if (dataArray->getSelectable() == true)
		{
			// we dont use this bool
			bool found = false;
			// get the nearest data to the mouse pos (x) in an optimalized way
			int location = dataArray->getNearestLocation(transformedMouse->first, &found);
			qDebug("selected location: %d", location);

			// if getNearestLocation was successful
			if (location >= 0)
			{
				std::pair<int, int> transformedData = mapDataPos(*dataArray->getData(location),
					dataArray->getNonNegative());
				// check distance against the pos (x)
				qDebug("selected x distance: %d", std::abs(transformedData.first - mouseXIn));
				if (std::abs(static_cast<int>(transformedData.first) - mouseXIn) <= m_pointSize * 2)
				{
					// calculate real distance (x and y)
				qDebug("selected cords: %d, %d, %d, %d", mouseXIn, mouseYIn,
						transformedData.first, transformedData.second);
					float curDistance = getDistance(mouseXIn, mouseYIn,
						transformedData.first, transformedData.second);

				qDebug("selected full distance: %f", curDistance);
					if (curDistance <= m_pointSize)
					{
	qDebug("selection Made!");
						m_selectedLocation = location;
						m_selectedArray = i;
						m_isSelected = true;
						break;
					}
				}
			}
/*
			for (unsigned int j = 0; j < dataArray->size(); j++)
			{
				std::pair<unsigned int, float>* data = dataArray->getData(j);
				std::pair<int, int> transformedData = mapDataPos(*data, dataArray->getNonNegative())
				float curDistance = getDistance(transformedData.first, mouseXIn,
					transformedData.second, mouseYIn);
				if (curDistance <= m_pointSize)// && curDistance < minDistance)
				{
					//minDistance = curDistance;
					m_selectedLocation = j;
					m_selectedArray = i;
					m_isSelected = true;
					break;
				}
			}
			if (m_isSelected == true)
			{
				break;
			}*/
		}
	}
	qDebug("selectDataEnd");
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

unsigned int PointGraphModel::addArray(std::vector<std::pair<unsigned int, float>>* arrayIn)
{
	unsigned int location = addArray();
	m_dataArrays[location].setDataArray(arrayIn);
	return location;
}

unsigned int PointGraphModel::addArray()
{
	PointGraphDataArray tempArray(&m_maxLength,
		false, false, false, false, false, false, false, this);
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

// PointGraphDataArray ------

PointGraphDataArray::PointGraphDataArray()
{
	m_maxLength = nullptr;
	m_isFixedSize = false;
	m_isFixedValue = false;
	m_isFixedPos = false;
	m_isFixedEndPoints = false;
	m_isSelectable = false;
	m_isEditableAttrib = false;
	m_nonNegative = false;
	
	m_lineColor = QColor(200, 200, 200, 255);
	m_activeColor = QColor(255, 255, 255, 255);
	// is not enabled by default
	m_fillColor = QColor(0, 0, 0, 0);

	m_dataArray = {};
}

PointGraphDataArray::PointGraphDataArray(unsigned int* maxLengthIn,
	bool isFixedSizeIn, bool isFixedValueIn, bool isFixedPosIn, bool nonNegativeIn,
	bool isFixedEndPointsIn, bool isSelectableIn, bool isEditableAttribIn, PointGraphModel* parentIn)
{
	m_maxLength = maxLengthIn;
	m_isFixedSize = isFixedSizeIn;
	m_isFixedValue = isFixedValueIn;
	m_isFixedPos = isFixedPosIn;
	m_isFixedEndPoints = isFixedEndPointsIn;
	m_isSelectable = isSelectableIn;
	m_isEditableAttrib = isEditableAttribIn;
	m_nonNegative = nonNegativeIn;
	
	m_lineColor = QColor(200, 200, 200, 255);
	m_activeColor = QColor(255, 255, 255, 255);
	// is not enabled by default
	m_fillColor = QColor(0, 0, 0, 0);

	m_dataArray = {};

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

void PointGraphDataArray::setFixedSize(bool valueIn)
{
	m_isFixedSize = valueIn;
	dataChanged();
}
void PointGraphDataArray::setFixedValue(bool valueIn)
{
	m_isFixedValue = valueIn;
	dataChanged();
}
void PointGraphDataArray::setFixedPos(bool valueIn)
{
	m_isFixedPos = valueIn;
	dataChanged();
}
void PointGraphDataArray::setFixedEndPoints(bool valueIn)
{
	m_isFixedEndPoints = valueIn;
	formatDataArrayEndPoints();
	dataChanged();
}
void PointGraphDataArray::setSelectable(bool valueIn)
{
	m_isSelectable = valueIn;
	dataChanged();
}
void PointGraphDataArray::setEditableAttrib(bool valueIn)
{
	m_isEditableAttrib = valueIn;
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
void PointGraphDataArray::setMaxLength(unsigned int* maxLengthIn)
{
	m_maxLength = maxLengthIn;
}

bool PointGraphDataArray::getFixedSize()
{
	return m_isFixedSize;
}
bool PointGraphDataArray::getFixedValue()
{
	return m_isFixedValue;
}
bool PointGraphDataArray::getFixedPos()
{
	return m_isFixedPos;
}
bool PointGraphDataArray::getFixedEndPoints()
{
	return m_isFixedEndPoints;
}
bool PointGraphDataArray::getSelectable()
{
	return m_isSelectable;
}
bool PointGraphDataArray::getEditableAttrib()
{
	return m_isEditableAttrib;
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

// array:

int PointGraphDataArray::add(unsigned int posIn)
{
	int location = -1;
	if (posIn < *m_maxLength)
	{
	qDebug("add 1. success");
		bool found = false;
		location = getNearestLocation(posIn, &found);
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
				if (m_dataArray[location].first < posIn)
				{
					// we are adding one value, so dataArray.size() will be a valid location
					if (targetLocation < m_dataArray.size())
					{
						targetLocation++;
					}
				}
				m_dataArray.push_back(std::pair<unsigned int, float>(posIn, 0.0f));
	qDebug("add 4. success, target: %d", targetLocation);
				swap(m_dataArray.size() - 1, targetLocation, true);
				dataChangedVal = true;
			}
			else if (m_dataArray.size() <= 0)
			{
	qDebug("add 5. success");
				m_dataArray.push_back(std::pair<unsigned int, float>(posIn, 0.0f));
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
			if (m_dataArray[i].first > *m_maxLength)
			{
				m_dataArray[i].first = *m_maxLength;
			}
			if (m_dataArray[i].second > 1)
			{
				m_dataArray[i].second = 1;
			}
			if (m_nonNegative == true)
			{
				if (m_dataArray[i].second < 0)
				{
					m_dataArray[i].second = 0;
				}
			}
			else
			{
				if (m_dataArray[i].second < -1)
				{
					m_dataArray[i].second = -1;
				}
			}
		}
		formatDataArrayEndPoints();
	}

	// sort
	if (sortIn == true)
	{
		std::sort(m_dataArray.begin(), m_dataArray.end(),
			[](std::pair<unsigned int, float> a, std::pair<unsigned int, float> b)
			{
				return a.first > b.first;
			});
	}

	// delete duplicates
	unsigned int lastPos = 0;
	if (m_dataArray.size() > 0)
	{
		lastPos = m_dataArray[0].first;
	}
	for (unsigned int i = 1; i < m_dataArray.size(); i++)
	{
		if (m_dataArray[i].first == lastPos)
		{
			del(i);
		}
		else
		{
			lastPos = m_dataArray[i].first;
		}
	}
}

int PointGraphDataArray::getLocation(unsigned int posIn)
{
	bool found = false;
	int location = getNearestLocation(posIn, &found);
	if (found == false)
	{
		return -1;
	}
	return location;
}

int PointGraphDataArray::getNearestLocation(unsigned int posIn, bool* foundOut)
{
	if (posIn < *m_maxLength && m_dataArray.size() > 0)
	{
		int start = 0;
		int end = m_dataArray.size() - 1;
		int mid = 0;
		// binary search
		while (start < end)
		{
			mid = start + (end - start) / 2;
	qDebug("getNearestLocation, mid: %d, start: %d, end: %d", mid, start, end);
	qDebug("getNearestLocation, val: %d, pos: %d", m_dataArray[mid].first, posIn);
			if (m_dataArray[mid].first == posIn)
			{
				*foundOut = true;
				return mid;
			}
			else if (m_dataArray[mid].first < posIn)
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
		if (m_dataArray[mid].first > posIn && mid > 0)
		{
			mid = mid - 1;
		}
		if (mid + 1 < m_dataArray.size() &&
			std::abs(static_cast<int>(m_dataArray[mid].first) - static_cast<int>(posIn)) >
			std::abs(static_cast<int>(m_dataArray[mid + 1].first) - static_cast<int>(posIn)))
		{
			outputDif = 1;
		}
	qDebug("getNearestLocation, outputDif: %d", outputDif);
		*foundOut = false;
		return mid + outputDif;
	}
	qDebug("getNearestLocation, posIn: %d", posIn);
	*foundOut = false;
	return -1;
}

float PointGraphDataArray::getValueAtPositon(float posIn)
{
	float posB = posIn / static_cast<float>(*m_maxLength);
	bool found = false;
	unsigned int location = getNearestLocation(static_cast<unsigned int>(posIn), &found);
	if (location >= 0)
	{
		if (found == true)
		{
			return m_dataArray[location].second;
		}
		else if (m_dataArray.size() == 1)
		{
			return m_dataArray[0].second;
		}
		else
		{
			float posC = posB - static_cast<float>(static_cast<int>(posB));
			int slide = 1;
			float value = 0.0f;
			if (location == m_dataArray.size() - 1)
			{
				slide = -1;
			}
			/*
			if (m_graphStyle == Style::Linear && m_graphStyle == Style::LinearPoints)
			{
				// if linear
				value = m_dataArray[location].second * posC + m_dataArray[location + slide].second * (1.0f - posC);
			}
			else
			{
				// if curved TODO

			}
			*/
			return value;
		}
	}
}

void PointGraphDataArray::setValue(unsigned int locationIn, float valueIn)
{
	if (m_isFixedValue == false)
	{
		if (m_isFixedEndPoints == true && locationIn < m_dataArray.size() - 1
			&& locationIn > 0 || m_isFixedEndPoints == false)
		{
			m_dataArray[locationIn].second = valueIn;
			dataChanged();
		}
	}
}

unsigned int PointGraphDataArray::setPos(unsigned int locationIn, unsigned int posIn)
{
	int location = locationIn;
	if (m_isFixedPos == false && posIn < *m_maxLength)
	{
		bool found = false;
		location = getNearestLocation(posIn, &found);
		// if an other point was not found at posIn
		// and if getNearestLocation returned a value
		// and if dataArray end points are changeable
		if (found == false && m_isFixedEndPoints == true &&
			locationIn < m_dataArray.size() - 1 && location > 0 ||
			found == false && m_isFixedEndPoints == false)
		{
			int targetLocation = location;
			bool dataChangedVal = false;
			// if getNearestLocation returned a value
			if (location >= 0)
			{
				// slide the new data if the closest data pos is bigger
	qDebug("set 3. success, location: %d", targetLocation);
				if (m_dataArray[location].first < posIn)
				{
					if (targetLocation + 1 < m_dataArray.size())
					{
						//targetLocation++;
					}
				}
	qDebug("set 4. success, target: %d", targetLocation);
				m_dataArray[locationIn].first = posIn;
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
				qDebug("   - i: %d  -  x: %d", i, m_dataArray[i].first);
			}
			
			if (locationAIn < locationBIn)
			{
				std::pair<unsigned int, float> swap = m_dataArray[locationAIn];
				for (unsigned int i = locationAIn; i < locationBIn; i++)
				{
					m_dataArray[i] = m_dataArray[i + 1];
				}
				m_dataArray[locationBIn] = swap;
			}
			else
			{
				std::pair<unsigned int, float> swap = m_dataArray[locationAIn];
				for (unsigned int i = locationAIn; i > locationBIn; i--)
				{
					m_dataArray[i] = m_dataArray[i - 1];
				}
				m_dataArray[locationBIn] = swap;
			}
			
			qDebug(" --------- ");
			for (unsigned int i = 0; i < m_dataArray.size(); i++)
			{
				qDebug("   - i: %d  -  x: %d", i, m_dataArray[i].first);
			}
		}
		else
		{
			std::pair<unsigned int, float> swap = m_dataArray[locationAIn];
			m_dataArray[locationAIn] = m_dataArray[locationBIn];
			m_dataArray[locationBIn] = swap;
		}
		dataChanged();
	}
}
void PointGraphDataArray::formatDataArrayEndPoints()
{
	if (m_isFixedEndPoints == true && m_dataArray.size() > 0)
	{
		m_dataArray[m_dataArray.size() - 1].first = 1;
		m_dataArray[m_dataArray.size() - 1].second = 1.0f;
		m_dataArray[0].first = 0;
		m_dataArray[0].second = m_nonNegative == true ? 0.0f : -1.0f;
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
