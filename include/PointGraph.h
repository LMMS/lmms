
#ifndef LMMS_GUI_POINTGRAPH_H
#define LMMS_GUI_POINTGRAPH_H

#include <QPainterPath>
#include <QWidget>
#include <QCursor>
#include <vector>

#include "Model.h"
#include "ModelView.h"
#include "lmms_basics.h"
#include "AutomatableModel.h"

namespace lmms
{

class PointGraphModel;
class PointGraphDataArray;
class FloatModel;

namespace gui
{

class LMMS_EXPORT PointGraphView : public QWidget, public ModelView
{
	Q_OBJECT
public:
	// TODO: remove styles			Done
	// TODO: change x unsigned int to float			Done
	// TODO: make a new class inside PointGraphDataArray to store the point data			Done
	// TODO: add is selectable			Done
	// TODO: add new setting to make the last point cord 1, 1			Done
	// TODO: flip mouse y position			Done
	// TODO: function to get multiple values
	// TODO: rewrite comments
	// TODO: rename functions and values

	// TODO: automation:
	// TODO: add 4 new values to the nested class: curve, type, valueA, valueB (1 type is 4 value long)			Done
	// TODO: add automation support
	//		TODO: make FloatModel pointer array		Done
	//		TODO: allocate FloatModels with new		Done
	//		TODO: delete FloatModels in destructor		Done
	//		TODO: save FloatModels (run saveSettings)
	//		TODO: getter for the FloatModels for saving		Done
	//		TODO: connect FloatModels connect with getter
	// TODO: setPointAutomatedAttrib() --> changes the type value between y pos, curve, valueA, valueB			Done
	// TODO: setPointType(unsigned int type)			Done
	// TODO: add effector(PointGraphDataArray) int location to the PointGraphDataArray class			Done
	// TODO: add effector line attributes to the nested class			Done
	// TODO: add effect implementation

	// TODO: clear array when 2. last point is deleted in the widget		IGNORE
	// TODO: event when a dataArray's size gets to 0
	// TODO: ability to scale displayed coords in PointGraphView (not 0 - 100) (add scalers)
	// TODO: check PointGraphDataArray signals
	// TODO: journalling in PointGraphModel
	// TODO: m_maxLenght* should be replaced with m_parent->getMaxLength()			Done

	PointGraphView(QWidget * parentIn,
		int widthIn, int heightIn,
		unsigned int pointSizeIn, unsigned int maxLengthIn);
	~PointGraphView();

	void setLineColor(QColor colorIn, unsigned int dataArrayLocationIn);
	void setActiveColor(QColor colorIn, unsigned int dataArrayLocationIn);
	void setFillColor(QColor colorIn, unsigned int dataArrayLocationIn);

	inline PointGraphModel* model()
	{
		return castModel<PointGraphModel>();
	}
	
	// returns -2.0f at second when nothing is selected
	std::pair<float, float> getSelectedData();
	void setSelectedData(std::pair<float, float> dataIn);
	
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

	std::pair<float, float> mapMousePos(int xIn, int yIn, bool nonNegativeIn);
	std::pair<int, int> mapDataPos(float xIn, float yIn, bool nonNegativeIn);

	float getDistance(int xAIn, int yAIn, int xBIn, int yBIn);

	std::pair<float, float> showInputDialog(); //TODO
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
};

} // namespace gui

class LMMS_EXPORT PointGraphModel : public Model//, public JournallingObject
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
	unsigned int addArray(std::vector<std::pair<float, float>>* arrayIn, bool isCurvedIn);
	// returns location
	unsigned int addArray(std::vector<float>* arrayIn, bool isCurvedIn);
	// returns location
	unsigned int addArray();
	// preservs the order
	void delArray(unsigned int locationIn);
	inline void clearArray()
	{
		m_dataArrays.clear();
	}
	unsigned int getDataArrayLocation(PointGraphDataArray* dataArrayIn);

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
	// avoid using this or run updateConnections() after initialization
	PointGraphDataArray();
	PointGraphDataArray(
	bool isFixedSizeIn, bool isFixedValueIn, bool isFixedPosIn, bool nonNegativeIn,
	bool isFixedEndPointsIn, bool isSelectableIn, bool isEditableAttribIn, PointGraphModel* parentIn);
	~PointGraphDataArray();

	void updateConnections(PointGraphModel* parentIn);

	void setFixedSize(bool valueIn);
	void setFixedValue(bool valueIn);
	void setFixedPos(bool valueIn);
	void setFixedEndPoints(bool valueIn);
	void setSelectable(bool valueIn);
	void setEditableAttrib(bool valueIn);
	void setNonNegative(bool valueIn);
	void setLineColor(QColor colorIn);
	void setActiveColor(QColor colorIn);
	void setFillColor(QColor colorIn);
	// returns true if successful
	bool setEffectorArrayLocation(unsigned int locationIn);

	bool getFixedSize();
	bool getFixedValue();
	bool getFixedPos();
	bool getFixedEndPoints();
	bool getSelectable();
	bool getEditableAttrib();
	bool getNonNegative();
	QColor* getLineColor();
	QColor* getActiveColor();
	QColor* getFillColor();
	// returns -1 if it has no effector
	int getEffectorArrayLocation();


	// array: -------------------
	// returns the location of added/found point, -1 if not found and can not be added
	int add(float xIn);
	// deletes the data/sample if m_isFixedSize is disabled
	void del(unsigned int locationIn);
	// clears data/sample array without any checks
	inline void clear()
	{
		m_dataArray.clear();
		dataChanged();
	}
	inline size_t size()
	{
		return m_dataArray.size();
	}
	// clamps down the values to 0 - 1, -1 - 1
	// does check m_isFixedSize, m_isFixedValue, m_isFixedPos,
	// sorts array, removes duplicated positions,
	// clampIn: should clamp, sortIn: should sort
	void formatArray(bool clampIn, bool sortIn);


	// get attribute: -------------------
	inline float* getX(unsigned int locationIn)
	{
		return &m_dataArray[locationIn].m_x;
	}
	inline float* getY(unsigned int locationIn)
	{
		return &m_dataArray[locationIn].m_y;
	}
	inline float* getC(unsigned int locationIn)
	{
		return &m_dataArray[locationIn].m_c;
	}
	inline float* getValA(unsigned int locationIn)
	{
		return &m_dataArray[locationIn].m_valA;
	}
	inline float* getValB(unsigned int locationIn)
	{
		return &m_dataArray[locationIn].m_valB;
	}
	unsigned int getType(unsigned int locationIn);
	unsigned int getAutomatedAttrib(unsigned int locationIn);
	inline bool getEffectOnlyPoints(unsigned int locationIn)
	{
		return &m_dataArray[locationIn].m_effectOnlyPoints;
	}
	bool getEffect(unsigned int locationIn, unsigned int effectNumberIn);
	inline FloatModel* getAutomationModel(unsigned int locationIn)
	{
		return m_dataArray[locationIn].m_automationModel;
	}


	// get: -------------------
	// returns -1 when position is not found
	int getLocation(float xIn);
	// gets the nearest data location to the position,
	// foundOut is true when the nearest position = posIn,
	// reurns -1 when search failed
	int getNearestLocation(float xIn, bool* foundOut);

	float getValueAtPositon(float xIn); // TODO


	// set: -------------------
	// sets data array without any checks
	// inport x and y coords
	void setDataArray(std::vector<std::pair<float, float>>* dataArrayIn, bool isCurvedIn);
	// inport y coords
	void setDataArray(std::vector<float>* dataArrayIn, bool isCurvedIn);


	// set attribute: -------------------
	// sets position when m_isFixedPos is disabled, returns final location
	unsigned int setX(unsigned int locationIn, float xIn);
	// sets value when m_isFixedValue is disabled
	void setY(unsigned int locationIn, float yIn);
	// sets value when m_isFixedValue is disabled
	void setC(unsigned int locationIn, float cIn);
	// sets value when m_isFixedValue is disabled
	void setValA(unsigned int locationIn, float valueIn);
	// sets value when m_isFixedValue is disabled
	void setValB(unsigned int locationIn, float valueIn);
	// sets value when m_isFixedValue is disabled
	void setType(unsigned int locationIn, unsigned int typeIn);
	// sets value when m_isFixedValue is disabled
	void setAutomatedAttrib(unsigned int locationIn, unsigned int attribLocationIn);
	void setEffectOnlyPoints(unsigned int locationIn, bool boolIn);
	void setEffect(unsigned int locationIn, unsigned int effectNumberIn, bool boolIn);
	// if isAutomatedIn is true then make a new FloatModel and connect it, else delete
	// the currently used FloatModel
	void setAutomated(unsigned int locationIn, bool isAutomatedIn); // TODO


// signals: // not qt
	// m_dataArray
	void dataChanged();
	// color
	void styleChanged();
private:
	class PointGraphPoint
	{
	public:
		inline PointGraphPoint()
		{
			m_x = 0.0f;
			m_y = 0.0f;
			m_c = 0.0f;
			m_valA = 0.0f;
			m_valB = 0.0f;
			m_type = 0;

			m_effectOnlyPoints = false;
			m_effectAdd = true;
			m_effectSubtract = false;
			m_effectMultiply = false;
			m_effectDivide = false;
			m_effectPower = false;
			m_effectLog = false;

			m_automationModel = nullptr;
		}
		inline PointGraphPoint(float xIn, float yIn)
		{
			m_x = xIn;
			m_y = yIn;
			m_c = 0.0f;
			m_valA = 0.0f;
			m_valB = 0.0f;
			m_type = 0;

			m_effectOnlyPoints = false;
			m_effectAdd = true;
			m_effectSubtract = false;
			m_effectMultiply = false;
			m_effectDivide = false;
			m_effectPower = false;
			m_effectLog = false;

			m_automationModel = nullptr;
		}
		inline ~PointGraphPoint()
		{
			if (m_automationModel != nullptr)
			{
				delete m_automationModel;
			}
		}
		// 0 - 1
		float m_x;
		// 0 (or -1) - 1, the automatin value is scaled if needed TODO
		float m_y;
		// curve, -1 - 1
		float m_c;
		// valueA, -1 - 1
		float m_valA;
		// valueB, -1 - 1
		float m_valB;
		// line type, 0 -
		unsigned int m_type;

		bool m_effectOnlyPoints;

		bool m_effectAdd;
		bool m_effectSubtract;
		bool m_effectMultiply;
		bool m_effectDivide;
		bool m_effectPower;
		bool m_effectLog;

		// automation: connecting to floatmodels, nullptr when it isn't conntected'
		FloatModel* m_automationModel;
	};
	// swapping values, "slide" moves the values (between) once left or right
	// handle m_isFixedEndPoints when using this
	void swap(unsigned int locationAIn, unsigned int locationBIn, bool slide);
	// checks m_isFixedEndPoints, does not call dataChanged()
	void formatDataArrayEndPoints();

	// can new data be added or removed
	bool m_isFixedSize;
	// can the values be changed
	bool m_isFixedValue;
	// can the positions be changed
	bool m_isFixedPos;
	// if true then it makes the last position coordinate 1, 1, the first point coordinate to -1 (ot 0), 0
	bool m_isFixedEndPoints;
	// can PointGraphView select this
	bool m_isSelectable;
	// can PointGraphView edit the point attributes
	bool m_isEditableAttrib;

	// can values be less than 0
	bool m_nonNegative;
	QColor m_lineColor;
	QColor m_activeColor;
	QColor m_fillColor;

	PointGraphModel* m_parent;

	// which PointGraphDataArray can effect this one, -1 if not effected
	int m_effectorLocation;

	// ordered array of PointGraphPoints
	std::vector<PointGraphPoint> m_dataArray;
};

} // namespace lmms

#endif // LMMS_POINTGRAPH_H
