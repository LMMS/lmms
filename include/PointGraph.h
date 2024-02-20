
#ifndef LMMS_GUI_POINTGRAPH_H
#define LMMS_GUI_POINTGRAPH_H

#include <QPainterPath>
#include <QWidget>
#include <QCursor>
#include <vector>

#include "Model.h"
#include "ModelView.h"
#include "lmms_basics.h"

namespace lmms
{

class PointGraphModel;
class PointGraphDataArray;

namespace gui
{

class LMMS_EXPORT PointGraphView : public QWidget, public ModelView
{
	Q_OBJECT
public:
	// fake styles, update setStyle when adding new styles
	enum class Style
	{
		Linear, // just lines
		LinearPoints, // linear + draw data/sample points
		Curved, // curved lines
		CurvedPoints, // curced lines + draw data/sample points
	};

	PointGraphView(QWidget * parentIn,
		int widthIn, int heightIn,
		unsigned int pointSizeIn, unsigned int maxLengthIn,
		Style styleIn);
	~PointGraphView();

	void setStyle(Style styleIn, unsigned int dataArrayLocationIn);
	void setLineColor(QColor colorIn, unsigned int dataArrayLocationIn);
	void setActiveColor(QColor colorIn, unsigned int dataArrayLocationIn);
	void setFillColor(QColor colorIn, unsigned int dataArrayLocationIn);

	inline PointGraphModel* model()
	{
		return castModel<PointGraphModel>();
	}
	
	// returns -2.0f at second when nothing is selected
	std::pair<unsigned int, float> getSelectedData();
	void setSelectedData(std::pair<unsigned int, float> dataIn);
	
signals:
	inline void drawn();
protected:
	void paintEvent(QPaintEvent* pe) override; //TODO
	//void dropEvent(QDropEvent* de) override; //ignore
	//void dragEnterEvent(QDragEnterEvent* dee) override; //ignore
	void mousePressEvent(QMouseEvent* me) override; //TODO
	void mouseMoveEvent(QMouseEvent* me) override; //TODO
	void mouseReleaseEvent(QMouseEvent* me) override; //TODO
	void mouseDoubleClickEvent(QMouseEvent* me) override;
protected slots:
	void updateGraph();
private:
	void modelChanged() override;

	std::pair<unsigned int, float> mapMousePos(int xIn, int yIn, bool nonNegativeIn);
	std::pair<int, int> mapDataPos(std::pair<unsigned int, float> posIn, bool nonNegativeIn);

	float getDistance(int xAIn, int yAIn, int xBIn, int yBIn);

	std::pair<unsigned int, float> showInputDialog(); //TODO
	void selectData(int mouseXIn, int mouseYIn);

	bool m_mouseDown;
	bool m_mouseDrag;
	// if the mouse is not moved
	bool m_mousePress;
	// decides addition or deletion
	bool m_addition;

	// radius, rx = ry
	unsigned int m_pointSize;

	unsigned int m_selectedLocation;
	unsigned int m_selectedArray;
	bool m_isSelected;
	
	std::pair<int, int> m_lastTrackPoint;
	std::pair<int, int> m_lastScndTrackPoint;

	Style m_defaultStyle; // TODO
};

} // namespace gui

class LMMS_EXPORT PointGraphModel : public Model
{
Q_OBJECT
public:
	PointGraphModel(unsigned int maxLengthIn, Model* parentIn, bool defaultConstructedIn);
	~PointGraphModel();

	inline size_t getDataArraySize()
	{
		return m_dataArrays.size();
	}
	inline PointGraphDataArray* getDataArray(unsigned int locationIn)
	{
		return &m_dataArrays[locationIn];
	}
	inline unsigned int getMaxLength()
	{
		return m_maxLength;
	}
	inline void setMaxLength(unsigned int maxLengthIn)
	{
		if (m_maxLength != maxLengthIn)
		{
			m_maxLength = maxLengthIn;
			emit dataChanged();
		}
	}
	// returns location
	unsigned int addArray(std::vector<std::pair<unsigned int, float>>* arrayIn);
	// returns location
	unsigned int addArray();
	// preservs the order
	void delArray(unsigned int locationIn); // TODO
	inline void clearArray()
	{
		m_dataArrays.clear();
	}

	// save, load
	//void saveSettings(QDomDocument& doc, QDomElement& element, const QString& name); //TODO
	//void loadSettings(const QDomElement& element, const QString& name); //TODO
signals:
	// data changed inside m_dataArray or m_maxLength changed
	void dataChanged();
	// style changed inside m_dataArray
	void styleChanged();
	// m_dataArrays length changed
	void lengthChanged();
public slots:
	void dataArrayChanged();
	void dataArrayStyleChanged();
private:
	std::vector<PointGraphDataArray> m_dataArrays;
	unsigned int m_maxLength;

	friend class gui::PointGraphView;
};

class LMMS_EXPORT PointGraphDataArray
{

public:
	enum class Style
	{
		Linear, // just lines
		LinearPoints, // linear + draw data/sample points
		Curved, // curved lines
		CurvedPoints, // curced lines + draw data/sample points
	};

	PointGraphDataArray();
	PointGraphDataArray(unsigned int* maxLengthIn, Style graphStyleIn,
		bool isFixedSizeIn, bool isFixedValueIn, bool isFixedPosIn, bool nonNegativeIn,
		PointGraphModel* parentIn);
	~PointGraphDataArray();

	void updateConnections(PointGraphModel* parentIn);

	void setFixedSize(bool valueIn);
	void setFixedValue(bool valueIn);
	void setFixedPos(bool valueIn);
	void setNonNegative(bool valueIn);
	void setLineColor(QColor colorIn);
	void setActiveColor(QColor colorIn);
	void setFillColor(QColor colorIn);
	void setStyle(Style graphStyleIn);
	void setMaxLength(unsigned int* maxLengthIn);

	bool getFixedSize();
	bool getFixedValue();
	bool getFixedPos();
	bool getNonNegative();
	QColor* getLineColor();
	QColor* getActiveColor();
	QColor* getFillColor();
	Style getStyle();

	// array:
	// returns the location of added/found point, -1 if not found and can not be added
	int add(unsigned int posIn);
	// deletes the data/sample if m_isFixedSize is disabled
	void del(unsigned int locationIn);
	// clears data/sample array without any checks
	inline void clear()
	{
		m_dataArray.clear();
	}
	inline size_t size()
	{
		return m_dataArray.size();
	}
	// clamps down the values to 0 - m_maxLength, -1 - 1, does not sort
	// does check m_isFixedSize, m_isFixedValue, m_isFixedPos,
	// sorts array, removes duplicated positions,
	// clampIn: should clamp, sortIn: should sort
	void formatArray(bool clampIn, bool sortIn);

	// get:
	inline std::pair<unsigned int, float>* getData(unsigned int locationIn)
	{
		return &m_dataArray[locationIn];
	}
	// returns -1 when position is not found
	int getLocation(unsigned int posIn);
	// gets the nearest data location to the position,
	// foundOut is true when the nearest position = posIn,
	// reurns -1 when search failed
	int getNearestLocation(unsigned int posIn, bool* foundOut);
	float getValueAtPositon(float posIn); // TODO

	// set:
	// sets data array without any checks
	inline void setDataArray(std::vector<std::pair<unsigned int, float>>* dataArrayIn)
	{
		m_dataArray = *dataArrayIn;
	}
	// sets value when m_isFixedValue is disabed
	void setValue(unsigned int locationIn, float valueIn);
	// sets position when m_isFixedPos is disabed, returns final location
	unsigned int setPos(unsigned int locationIn, unsigned int posIn);
// signals: // not qt
	// m_dataArray
	void dataChanged();
	// color and style
	void styleChanged();
private:
	// swapping values, slide moves the values ()between) once left or right
	void swap(unsigned int locationAIn, unsigned int locationBIn, bool slide);

	// can new data be added or removed
	bool m_isFixedSize;
	// can the values be changed
	bool m_isFixedValue;
	// can the positions be changed
	bool m_isFixedPos;

	// can values be less than 0
	bool m_nonNegative;

	QColor m_lineColor;
	QColor m_activeColor;
	QColor m_fillColor;

	Style m_graphStyle;

	PointGraphModel* m_parent;

	unsigned int* m_maxLength;

	// ordered array of 0 < position < max_Length, -1(or 0) < value < 1
	std::vector<std::pair<unsigned int, float>> m_dataArray;
};

} // namespace lmms

#endif // LMMS_POINTGRAPH_H
