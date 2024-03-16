
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
	//		TODO: connect FloatModels connect with getter		Done
	// TODO: setPointAutomatedAttrib() --> changes the type value between y pos, curve, valueA, valueB			Done
	// TODO: setPointType(unsigned int type)			Done
	// TODO: add effector(PointGraphDataArray) int location to the PointGraphDataArray class			Done
	// TODO: add effector line attributes to the nested class			Done
	// TODO: add effect implementation		Done

	// TODO: clear array when 2. last point is deleted in the widget		IGNORE
	// TODO: event when a dataArray's size gets to 0
	// TODO: ability to scale displayed coords in PointGraphView (not 0 - 100) (add scalers)
	// TODO: check PointGraphDataArray signals
	// TODO: journalling in PointGraphModel
	// TODO: m_maxLenght* should be replaced with m_parent->getMaxLength()			Done
	// TODO: setDataArray keep attributes option
	// TODO: PointGraphDataArray shouldSaveAll and shouldSavePointAttributesOnly (for saving only editable graphs) option			Done
	// TODO: baked values in PointGraphPoint
	// TODO: rename class to VectorGraph
	// TODO: make std::vector<float> last used values			Done
	//		TODO: make update logic (isChanged and update only automation / effected lines)
	// TODO: effector location (same as automation location)
	// TODO: PointGraphView isSimplified			Done
	// TODO: new automated color			Done
	// TODO: context menu in gui (clear automation, connect to controller)
	// TODO: display hints (full text) in the editing
	// TODO: ability to edit multiple graphs using m_isLastSelectedArray

	PointGraphView(QWidget * parentIn,
		int widthIn, int heightIn,
		unsigned int pointSizeIn, unsigned int maxLengthIn);
	~PointGraphView();

	void setLineColor(QColor colorIn, unsigned int dataArrayLocationIn);
	void setActiveColor(QColor colorIn, unsigned int dataArrayLocationIn);
	void setFillColor(QColor colorIn, unsigned int dataArrayLocationIn);
	void setAutomatedColor(QColor colorIn, unsigned int dataArrayLocationIn);

	inline PointGraphModel* model()
	{
		return castModel<PointGraphModel>();
	}

	void setIsSimplified(bool isSimplifiedIn);

	// returns -1.0f at first when nothing is selected
	std::pair<float, float> getSelectedData();
	// returns -1 it can not return a array location
	int getLastSelectedArray();
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
	void mouseDoubleClickEvent(QMouseEvent* me) override; //TODO
protected slots:
	void updateGraph();
private:
	void modelChanged() override;

	std::pair<float, float> mapMousePos(int xIn, int yIn, bool nonNegativeIn);
	// calculate curve position
	std::pair<float, float> mapDataCurvePos(float xAIn, float yAIn, float xBIn, float yBIn, float curveIn);
	std::pair<int, int>mapDataCurvePos(int xAIn, int yAIn, int xBIn, int yBIn, float curveIn);
	std::pair<int, int> mapDataPos(float xIn, float yIn, bool nonNegativeIn);
	int mapInputPos(float inputValueIn, unsigned int displayLengthIn);

	float getDistance(int xAIn, int yAIn, int xBIn, int yBIn);
	float getDistance(float xAIn, float yAIn, float xBIn, float yBIn);

	// returns true if the graph was clicked
	bool isGraphPressed(int mouseYIn);
	// returns -1 if no attribute was clicked
	int getPressedInput(int mouseXIn, int mouseYIn, unsigned int inputCountIn);
	// returns a float attrib value, valueOut = attrib value if it is a bool
	float getInputAttribValue(unsigned int editingArrayLocationIn, bool* valueOut);
	// sets the attrib to floatValueIn it it is float, else it sets the attrib to boolValueIn
	void setInputAttribValue(unsigned int editingArrayLocationIn, float floatValueIn, bool boolValueIn);
	// calculates the ideal text color
	QColor getTextColorFromBaseColor(QColor baseColorIn);
	// returns the first x char that fits in the displayedLength(in pixel)
	// cuts the string to displayedLength(in px) size
	QString getTextFromDisplayLength(QString textIn, unsigned int displayLengthIn);

	std::pair<float, float> showCoordInputDialog();
	float showInputDialog(float curInputValueIn);
	// searches arrays to select
	// clicked datapoint
	void selectData(int mouseXIn, int mouseYIn);
	// searches for data in a given array
	// returns found location, when a data
	// was found in the given distance
	// else it returns -1
	int searchForData(int mouseXIn, int mouseYIn, float maxDistanceIn, PointGraphDataArray* arrayIn, bool curvedIn);

	bool m_mouseDown;
	bool m_mouseDrag;
	// if the mouse is not moved
	bool m_mousePress;
	// decides addition or deletion
	bool m_addition;

	// radius, rx = ry
	unsigned int m_pointSize;
	// draw simplified lines
	bool m_isSimplified;

	unsigned int m_selectedLocation;
	unsigned int m_selectedArray;
	bool m_isSelected;
	bool m_isCurveSelected;
	// if m_selectedArray was the last array selected
	bool m_isLastSelectedArray;

	unsigned int m_graphHeight;
	unsigned int m_editingHeight;
	// displayed attrib count
	unsigned int m_editingInputCount;
	unsigned int m_editingDisplayPage;
	bool m_isEditingActive;
	std::vector<QString> m_editingText;
	std::vector<QString> m_editingLineEffectText;
	std::vector<bool> m_editingInputIsFloat;

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
	bool isFixedEndPointsIn, bool isSelectableIn, bool isEditableAttribIn, bool isAutomatableEffectableIn,
	bool isSaveableIn, PointGraphModel* parentIn);
	~PointGraphDataArray();

	void updateConnections(PointGraphModel* parentIn);

	void setIsFixedSize(bool valueIn);
	void setIsFixedValue(bool valueIn);
	void setIsFixedPos(bool valueIn);
	void setIsFixedEndPoints(bool valueIn);
	void setIsSelectable(bool valueIn);
	void setIsEditableAttrib(bool valueIn);
	void setIsAutomatableEffectable(bool valueIn);
	void setIsSaveable(bool valueIn);
	void setNonNegative(bool valueIn);
	void setLineColor(QColor colorIn);
	void setActiveColor(QColor colorIn);
	void setFillColor(QColor colorIn);
	void setAutomatedColor(QColor colorIn);
	// returns true if successful
	bool setEffectorArrayLocation(unsigned int locationIn);

	bool getIsFixedSize();
	bool getIsFixedValue();
	bool getIsFixedPos();
	bool getIsFixedEndPoints();
	bool getIsSelectable();
	bool getIsEditableAttrib();
	bool getIsAutomatableEffectable();
	bool getIsSaveable();
	bool getNonNegative();
	QColor* getLineColor();
	QColor* getActiveColor();
	QColor* getFillColor();
	QColor* getAutomatedColor();
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
	inline float getX(unsigned int locationIn)
	{
		return m_dataArray[locationIn].m_x;
	}
	inline float getY(unsigned int locationIn)
	{
		return m_dataArray[locationIn].m_y;
	}
	inline float getC(unsigned int locationIn)
	{
		return m_dataArray[locationIn].m_c;
	}
	inline float getValA(unsigned int locationIn)
	{
		return m_dataArray[locationIn].m_valA;
	}
	inline float getValB(unsigned int locationIn)
	{
		return m_dataArray[locationIn].m_valB;
	}
	inline unsigned int getType(unsigned int locationIn)
	{
		return m_dataArray[locationIn].m_type;
	}
	// returns attribLocation: 0 = y, 1 = c, 2 = valA, 3 = valB
	unsigned int getAutomatedAttribLocation(unsigned int locationIn);
	unsigned int getEffectedAttribLocation(unsigned int locationIn);
	inline bool getEffectOnlyPoints(unsigned int locationIn)
	{
		return m_dataArray[locationIn].m_effectOnlyPoints;
	}
	// returns if the [effectNumberIn] effect is active based on effectNumberIn
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
	int getNearestLocation(float xIn, bool* foundOut, bool* isBeforeOut);

	float getValueAtPosition(float xIn); // TODO


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
	// 
	void setValB(unsigned int locationIn, float valueIn);
	// 
	void setType(unsigned int locationIn, unsigned int typeIn);
	// 
	void setAutomatedAttrib(unsigned int locationIn, unsigned int attribLocationIn);
	void setEffectedAttrib(unsigned int locationIn, unsigned int attribLocationIn);
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
			m_automatedEffectedAttribLocations = 0;

			m_effectOnlyPoints = false;
			m_effectAdd = true;
			m_effectSubtract = false;
			m_effectMultiply = false;
			m_effectDivide = false;
			m_effectPower = false;
			m_effectLog = false;
			m_effectSine = false;

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
			m_automatedEffectedAttribLocations = 0;

			m_effectOnlyPoints = false;
			m_effectAdd = true;
			m_effectSubtract = false;
			m_effectMultiply = false;
			m_effectDivide = false;
			m_effectPower = false;
			m_effectLog = false;
			m_effectSine = false;

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
		// 0 (or -1) - 1, getAutomatedAttrib() -> 0
		float m_y;
		// curve, -1 - 1, getAutomatedAttrib() -> 1
		float m_c;
		// valueA, -1 - 1, getAutomatedAttrib() -> 2
		float m_valA;
		// valueB, -1 - 1, getAutomatedAttrib() -> 3
		float m_valB;
		// line type:
		// 0 - none
		// 1 - sine
		// 2 - sineB
		// 3 - peak
		// 4 - steps
		// 5 - random
		unsigned int m_type;
		// the automated attrib location and
		// the effected attrib location is
		// stored here
		// use getAutomatedAttrib or getEffectedAttrib to get it
		unsigned int m_automatedEffectedAttribLocations;

		bool m_effectOnlyPoints;

		bool m_effectAdd;
		bool m_effectSubtract;
		bool m_effectMultiply;
		bool m_effectDivide;
		bool m_effectPower;
		bool m_effectLog;
		bool m_effectSine;

		// automation: connecting to floatmodels, nullptr when it isn't conntected'
		FloatModel* m_automationModel;
	};
	// swapping values, "slide" moves the values (between) once left or right
	// handle m_isFixedEndPoints when using this
	void swap(unsigned int locationAIn, unsigned int locationBIn, bool slide);
	// applys the effect on a given value, does clamp TODO clamp
	float processEffect(float valueIn, float effectValueIn,
		PointGraphDataArray* effectArrayIn, unsigned int effectLocationIn, float lowerLimitIn);
	// returns the curve value at a given x coord
	float processCurve(float valueBeforeIn, float valueAfterIn, float curveIn, float xIn);
	// returns a PointGraphPoint with modified attributes
	float processAutomation(unsigned int locationIn, unsigned int attribLocationIn);
	// line effects / types, m_type is used for this
	// valA: amp, valB: freq, fadeOutStartIn: from what xIn value should the line type fade out
	float processLineTypeSine(float xIn, float valAIn, float valBIn, float fadeOutStartIn);
	// curveIn: phase
	float processLineTypeSineB(float xIn, float valAIn, float valBIn, float curveIn, float fadeOutStartIn);
	//std::vector<float> processLineTypeArraySine(std::vector<float> xIn, unsigned int startIn, unsigned int endIn, float valAIn, float valBIn);
	// valA: amp, valB: x coord, curve: width
	float processLineTypePeak(float xIn, float valAIn, float valBIn, float curveIn, float fadeOutStartIn);
	// y: calculate steps from, valA: y count, valB: curve
	float processLineTypeSteps(float xIn, float yIn, float valAIn, float valBIn, float fadeOutStartIn);
	// valA: amp, valB: random number count, curveIn: seed
	float processLineTypeRandom(float xIn, float valAIn, float valBIn, float curveIn, float fadeOutStartIn);

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
	// every attribute outside of x and y
	bool m_isEditableAttrib;
	// can the points be automated or effected
	bool m_isAutomatableEffectable;
	// if PointGraphDataArray is allowed to save this
	bool m_isSaveable;

	// can values be less than 0
	bool m_nonNegative;
	QColor m_lineColor;
	QColor m_activeColor;
	QColor m_fillColor;
	QColor m_automatedColor;

	PointGraphModel* m_parent;

	// which PointGraphDataArray can effect this one, -1 if not effected
	int m_effectorLocation;

	// ordered array of PointGraphPoints
	std::vector<PointGraphPoint> m_dataArray;
	
	// baking
	bool m_isDataChanged;
	// array containing output final float values for optimalization
	std::vector<float> m_bakedValues;
};

} // namespace lmms

#endif // LMMS_POINTGRAPH_H
