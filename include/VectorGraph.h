/*
 * VecorGraph.h - Vector graph widget and model implementation
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

#ifndef LMMS_GUI_VECTORGRAPH_H
#define LMMS_GUI_VECTORGRAPH_H

#include <vector>
#include <QPainterPath>
#include <QWidget>
#include <QCursor>
#include <QMenu>

#include "Model.h"
#include "ModelView.h"
#include "lmms_basics.h"
#include "AutomatableModel.h"
#include "JournallingObject.h"

namespace lmms
{

class VectorGraphModel;
class VectorGraphDataArray;
class FloatModel;

namespace gui
{
class LMMS_EXPORT VectorGraphView : public QWidget, public ModelView
{
	Q_OBJECT
public:
	VectorGraphView(QWidget * parentIn,
		int widthIn, int heightIn,
		unsigned int pointSizeIn, unsigned int maxLengthIn);
	~VectorGraphView();

	void setLineColor(QColor colorIn, unsigned int dataArrayLocationIn);
	void setActiveColor(QColor colorIn, unsigned int dataArrayLocationIn);
	void setFillColor(QColor colorIn, unsigned int dataArrayLocationIn);
	void setAutomatedColor(QColor colorIn, unsigned int dataArrayLocationIn);

	inline VectorGraphModel* model()
	{
		return castModel<VectorGraphModel>();
	}

	void setIsSimplified(bool isSimplifiedIn);

	// returns -1.0f at first when nothing is selected
	std::pair<float, float> getSelectedData();
	// returns -1 it can not return a array location
	int getLastSelectedArray();
	void setSelectedData(std::pair<float, float> dataIn);
	void setBackground(const QPixmap backgoundIn);
	
	void useGetLastValues();
signals:
	void drawn();
protected:
	void paintEvent(QPaintEvent* pe) override;
	void mousePressEvent(QMouseEvent* me) override;
	void mouseMoveEvent(QMouseEvent* me) override;
	void mouseReleaseEvent(QMouseEvent* me) override;
	void mouseDoubleClickEvent(QMouseEvent* me) override;
protected slots:
	void updateGraph();

	void execConnectionDialog();
	void removeAutomation();
	void removeController();
private:
	void paintGraph(QPainter* pIn, unsigned int locationIn, std::vector<int>* alreadyUpdatedDataArraysIn);
	void paintEditing(QPainter* pIn);

	void modelChanged() override;

	// utility
	std::pair<float, float> mapMousePos(int xIn, int yIn);
	// calculate curve position
	std::pair<float, float> mapDataCurvePos(float xAIn, float yAIn, float xBIn, float yBIn, float curveIn);
	std::pair<int, int>mapDataCurvePos(int xAIn, int yAIn, int xBIn, int yBIn, float curveIn);
	std::pair<int, int> mapDataPos(float xIn, float yIn, bool nonNegativeIn);
	int mapInputPos(float inputValueIn, unsigned int displayLengthIn);

	float getDistance(int xAIn, int yAIn, int xBIn, int yBIn);
	float getDistance(float xAIn, float yAIn, float xBIn, float yBIn);

	bool addPoint(unsigned int locationIn, int mouseXIn, int mouseYIn);

	// editing menu / controls
	// returns true if the graph was clicked
	bool isGraphPressed(int mouseXIn, int mouseYIn);
	// returns true if the control window was clicked while in editing mode
	bool isControlWindowPressed(int mouseYIn);
	void processControlWindowPressed(int mouseXIn, int mouseYIn, bool isDraggingIn, bool startMovingIn, int curXIn, int curYIn);
	// returns -1 if no attribute was clicked
	int getPressedInput(int mouseXIn, int mouseYIn, unsigned int inputCountIn);
	// returns a float attrib value, valueOut = attrib value if it is a bool
	float getInputAttribValue(unsigned int controlArrayLocationIn, bool* valueOut);
	// sets the attrib to floatValueIn it it is float, else it sets the attrib to boolValueIn
	void setInputAttribValue(unsigned int controlArrayLocationIn, float floatValueIn, bool boolValueIn);
	// calculates the ideal text color
	QColor getTextColorFromBaseColor(QColor baseColorIn);
	// calculates a replacement fill color
	QColor getFillColorFromBaseColor(QColor baseColorIn);
	// returns the first x char that fits in the displayedLength(in pixel)
	// cuts the string to displayedLength(in px) size
	QString getTextFromDisplayLength(QString textIn, unsigned int displayLengthIn);
	void addDefaultActions(QMenu* menu, QString controlDisplayTextIn);

	// inputDialog
	std::pair<float, float> showCoordInputDialog();
	float showInputDialog(float curInputValueIn);

	// selection
	// searches arrays to select
	// clicked datapoint
	void selectData(int mouseXIn, int mouseYIn);
	// searches for data in a given array
	// returns found location, when a data
	// was found in the given distance
	// else it returns -1
	int searchForData(int mouseXIn, int mouseYIn, float maxDistanceIn, VectorGraphDataArray* arrayIn, bool curvedIn);

	bool m_mouseDown;
	bool m_mouseDrag;
	// if the mouse is not moved
	bool m_mousePress;
	// decides addition or deletion
	bool m_addition;

	// radius, rx = ry
	unsigned int m_pointSize;
	unsigned int m_fontSize;
	// draw simplified lines
	bool m_isSimplified;
	QPixmap m_background;
	// for 1 draw, it will use the VectorGraphDataArray
	// m_bakedValues without calling GetValues()
	bool m_useGetLastValues;

	// if m_isLastSelectedArray == true then
	// m_selectedArray can be used
	// else if m_isSelected == false then
	// m_selectedLocation amd m_selectedArray should not be used
	unsigned int m_selectedLocation;
	unsigned int m_selectedArray;
	bool m_isSelected;
	bool m_isCurveSelected;
	// if m_selectedArray was the last array selected
	bool m_isLastSelectedArray;

	unsigned int m_graphHeight;
	unsigned int m_controlHeight;
	// displayed attrib count
	unsigned int m_controlDisplayCount;
	unsigned int m_controlDisplayPage;
	bool m_isEditingActive;
	std::vector<QString> m_controlText;
	std::vector<QString> m_controlLineEffectText;
	std::vector<bool> m_controlIsFloat;

	std::pair<int, int> m_lastTrackPoint;
	std::pair<int, int> m_lastScndTrackPoint;
};

} // namespace gui

class LMMS_EXPORT VectorGraphModel : public Model, public JournallingObject
{
Q_OBJECT
public:
	VectorGraphModel(unsigned int maxLengthIn, Model* parentIn, bool defaultConstructedIn);
	~VectorGraphModel();

	inline size_t getDataArraySize()
	{
		return m_dataArrays.size();
	}
	inline VectorGraphDataArray* getDataArray(unsigned int locationIn)
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
	unsigned int addArray();
	// preservs the order
	void delArray(unsigned int locationIn);
	inline void clearArray()
	{
		m_dataArrays.clear();
	}
	// if the id is not found then it will return 0
	int getDataArrayLocationFromId(int idIn);
	int getDataArrayNewId();

	// save, load
	QString nodeName() const override
	{
		return "VectorGraphModel";
	}
	virtual void saveSettings(QDomDocument& doc, QDomElement& element, const QString& name);
	virtual void loadSettings(const QDomElement& element, const QString& name);
	virtual void saveSettings(QDomDocument& doc, QDomElement& element);
	virtual void loadSettings(const QDomElement& element);
	// read locations from saved data attributes
	//int readLoc(unsigned int startIn, QString dataIn);
signals:
	// data changed inside m_dataArray or m_maxLength changed
	void dataChanged();
	// signals when a dataArray gets to 0 element
	// locationIn is the location of the dataArray
	// locationIn can be -1
	void clearedEvent(int locationIn);
	// style changed inside m_dataArray
	void styleChanged();
	// m_dataArrays length changed
	void lengthChanged();
public slots:
	void dataArrayChanged();
	void dataArrayClearedEvent(int idIn);
	void dataArrayStyleChanged();
private:
	std::vector<VectorGraphDataArray> m_dataArrays;
	unsigned int m_maxLength;

	//friend class gui::VectorGraphView;
};

class LMMS_EXPORT VectorGraphDataArray
{

public:
	// avoid using this or run updateConnections() after initialization
	VectorGraphDataArray();
	VectorGraphDataArray(
	bool isFixedSizeIn, bool isFixedXIn, bool isFixedYIn, bool isNonNegativeIn,
	bool isFixedEndPointsIn, bool isSelectableIn, bool isEditableAttribIn, bool isAutomatableEffectableIn,
	bool isSaveableIn, VectorGraphModel* parentIn, int idIn);
	~VectorGraphDataArray();

	void updateConnections(VectorGraphModel* parentIn);

	void setIsFixedSize(bool valueIn);
	void setIsFixedX(bool valueIn);
	void setIsFixedY(bool valueIn);
	void setIsFixedEndPoints(bool valueIn);
	void setIsSelectable(bool valueIn);
	void setIsEditableAttrib(bool valueIn);
	void setIsAutomatableEffectable(bool valueIn);
	void setIsSaveable(bool valueIn);
	void setIsNonNegative(bool valueIn);
	void setLineColor(QColor colorIn);
	void setActiveColor(QColor colorIn);
	void setFillColor(QColor colorIn);
	void setAutomatedColor(QColor colorIn);
	// returns true if successful

	bool setEffectorArrayLocation(int locationIn, bool callDataChangedIn);

	bool getIsFixedSize();
	bool getIsFixedX();
	bool getIsFixedY();
	bool getIsFixedEndPoints();
	bool getIsSelectable();
	bool getIsEditableAttrib();
	bool getIsAutomatableEffectable();
	bool getIsSaveable();
	bool getIsNonNegative();
	QColor* getLineColor();
	QColor* getActiveColor();
	QColor* getFillColor();
	QColor* getAutomatedColor();
	// returns -1 if it has no effector

	int getEffectorArrayLocation();
	int getId();


	// array: -------------------
	// returns the location of added/found point, -1 if not found and can not be added
	int add(float xIn);
	// deletes the data/sample if m_isFixedSize is disabled
	void del(unsigned int locationIn);
	// clears data/sample array without any checks
	inline void clear()
	{
		m_dataArray.clear();
		clearedEvent();
		getUpdatingFromPoint(-1);
		dataChanged();
	}
	inline size_t size()
	{
		return m_dataArray.size();
	}
	// clamps down the values to 0 - 1, -1 - 1
	// does check m_isFixedSize, m_isFixedValue, m_isFixedPos,
	// sorts array, removes duplicated positions, calls dataChanged()
	// clampIn: should clamp, sortIn: should sort
	void formatArray(std::vector<std::pair<float, float>>* dataArrayIn, bool clampIn, bool rescaleIn, bool sortIn, bool callDataChangedIn);


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
	// returns true when m_effectOnlyPoints is true or
	// when getEffectedAttribLocation > 0 (y is uneffected)
	// -> when the current point CAN effect lines before it
	bool getEffectOnlyPoints(unsigned int locationIn);
	// returns if the [effectNumberIn] effect is active based on effectNumberIn
	bool getEffect(unsigned int locationIn, unsigned int effectNumberIn);
	bool getIsAutomationValueChanged(unsigned int locationIn);
	inline FloatModel* getAutomationModel(unsigned int locationIn);


	// get: -------------------
	// returns -1 when position is not found
	int getLocation(float xIn);
	// gets the nearest data location to the position,
	// foundOut is true when the nearest position = posIn,
	// reurns -1 when search failed
	int getNearestLocation(float xIn, bool* foundOut, bool* isBeforeOut);


	// returns the latest updated graph values
	// countIn is the retuned vector's size
	std::vector<float> getValues(unsigned int countIn);
	std::vector<float> getValues(unsigned int countIn, bool* isChangedOut, std::vector<unsigned int>* updatingValuesOut);
	// returns m_bakedValues without updating
	std::vector<float> getLastValues();
	std::vector<int> getEffectorArrayLocations();


	// set: -------------------
	// sets data array without any checks
	// inport x and y coords
	void setDataArray(std::vector<std::pair<float, float>>* dataArrayIn, bool isCurvedIn, bool clearIn, bool clampIn, bool rescaleIn, bool sortIn, bool callDataChangedIn);
	// inport y coords
	void setDataArray(std::vector<float>* dataArrayIn, bool isCurvedIn, bool clearIn, bool clampIn, bool rescaleIn, bool callDataChangedIn);


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
	void setAutomated(unsigned int locationIn, bool isAutomatedIn);


// signals: // not qt
	void dataChanged();
	// runs when m_dataArray.size() gets to 0
	void clearedEvent();
	// color
	void styleChanged();
protected:
	// returns m_automationModelArray
	std::vector<FloatModel*>* getAutomationModelArray();
	QString getSavedDataArray();
	void loadDataArray(QString dataIn, unsigned int sizeIn);
private:
	class VectorGraphPoint
	{
	public:
		inline VectorGraphPoint()
		{
			m_x = 0.0f;
			m_y = 0.0f;
			m_c = 0.0f;
			m_valA = 0.0f;
			m_valB = 0.0f;
			m_type = 0;
			m_automatedEffectedAttribLocations = 0;

			m_effectOnlyPoints = false;
			m_effectAdd = false;
			m_effectSubtract = false;
			m_effectMultiply = false;
			m_effectDivide = false;
			m_effectPower = false;
			m_effectLog = false;
			m_effectSine = false;
			m_effectClampLower = false;
			m_effectClampUpper = false;

			m_bufferedAutomationValue = 0.0f;
			m_automationModel = -1;
		}
		inline VectorGraphPoint(float xIn, float yIn)
		{
			m_x = xIn;
			m_y = yIn;
			m_c = 0.0f;
			m_valA = 0.0f;
			m_valB = 0.0f;
			m_type = 0;
			m_automatedEffectedAttribLocations = 0;

			m_effectOnlyPoints = false;
			m_effectAdd = false;
			m_effectSubtract = false;
			m_effectMultiply = false;
			m_effectDivide = false;
			m_effectPower = false;
			m_effectLog = false;
			m_effectSine = false;
			m_effectClampLower = false;
			m_effectClampUpper = false;

			m_bufferedAutomationValue = 0.0f;
			m_automationModel = -1;
		}
		inline ~VectorGraphPoint()
		{
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

		// if the point's line should effect only points
		// getEffectOnlyPoints() will return true when
		// effected attrib location > 0
		bool m_effectOnlyPoints;

		bool m_effectAdd;
		bool m_effectSubtract;
		bool m_effectMultiply;
		bool m_effectDivide;
		bool m_effectPower;
		bool m_effectLog;
		bool m_effectSine;
		bool m_effectClampLower;
		bool m_effectClampUpper;

		// stores m_automationModel->value(), used in updating
		float m_bufferedAutomationValue;
		// automation: connecting to floatmodels, -1 when it isn't conntected'
		int m_automationModel;
	};
	// deletes the point's automation model
	// locationIn = point location
	void delAutomationModel(unsigned int modelLocationIn, bool callDataChangedIn);
	// swapping values, "slide" moves the values (between) once left or right
	// handle m_isFixedEndPoints when using this
	void swap(unsigned int locationAIn, unsigned int locationBIn, bool slide);
	// returns the curve value at a given x coord
	float processCurve(float valueBeforeIn, float valueAfterIn, float curveIn, float xIn);
	// applys the effect on a given value, does clamp
	float processEffect(unsigned int locationIn, float attribValueIn, unsigned int attribLocationIn, float effectValueIn);
	// returns a VectorGraphPoint with modified attributes
	float processAutomation(float attribValueIn, unsigned int locationIn, unsigned int attribLocationIn);

	// line effects / types, m_type is used for this
	// valA: amp, valB: freq, fadeInStartIn: from what xIn value should the line type fade out
	//float processLineTypeSine(float xIn, float valAIn, float valBIn, float fadeInStartIn);
	std::vector<float> processLineTypeArraySine(std::vector<float>* xIn, unsigned int startIn, unsigned int endIn,
		float valAIn, float valBIn, float fadeInStartIn);
	// curveIn: phase
	//float processLineTypeSineB(float xIn, float valAIn, float valBIn, float curveIn, float fadeInStartIn);
	std::vector<float> processLineTypeArraySineB(std::vector<float>* xIn, unsigned int startIn, unsigned int endIn,
		float valAIn, float valBIn, float curveIn, float fadeInStartIn);
	// valA: amp, valB: x coord, curve: width
	//float processLineTypePeak(float xIn, float valAIn, float valBIn, float curveIn, float fadeInStartIn);
	std::vector<float> processLineTypeArrayPeak(std::vector<float>* xIn, unsigned int startIn, unsigned int endIn,
		float valAIn, float valBIn, float curveIn, float fadeInStartIn);
	// y: calculate steps from, valA: y count, valB: curve
	//float processLineTypeSteps(float xIn, float yIn, float valAIn, float valBIn, float fadeInStartIn);
	std::vector<float> processLineTypeArraySteps(std::vector<float>* xIn, unsigned int startIn, unsigned int endIn,
		std::vector<float>* yIn, float valAIn, float valBIn, float fadeInStartIn);
	// valA: amp, valB: random number count, curveIn: seed
	//float processLineTypeRandom(float xIn, float valAIn, float valBIn, float curveIn, float fadeInStartIn);
	std::vector<float> processLineTypeArrayRandom(std::vector<float>* xIn, unsigned int startIn, unsigned int endIn,
		float valAIn, float valBIn, float curveIn, float fadeInStartIn);

	// updating
	// adds the points that are
	// effected by the effector's values changing
	// ONLY WORKS IN SORTED ARRAYS
	void getUpdatingFromEffector(std::vector<unsigned int>* updatingValuesIn);
	// if locationIn > 0 -> adds the location to m_needsUpdating
	// else it will update the whole m_dataArray and m_bakedValues
	// changes in the size of m_dataArray (addtition, deletion, ect.)
	// needs to cause a full update
	void getUpdatingFromPoint(int locationIn);
	// adds the points that are changed because their
	// automation is changed
	void getUpdatingFromAutomation();
	// recalculates and sorts m_needsUpdating so
	// every point is in there only once
	void getUpdatingOriginals();
	//void getValuesLocations(VectorGraphDataArray* effectorIn, std::vector<std::pair<unsigned int, unsigned int>>* effectorDataOut);
	void getValuesUpdateLines(VectorGraphDataArray* effectorIn, std::vector<float>* effectorOutputIn,
		std::vector<float>* outputXLocationsIn, unsigned int iIn, float stepSizeIn);
	bool isEffectedPoint(unsigned int locationIn);

	// checks m_isFixedEndPoints, does not call dataChanged()
	void formatDataArrayEndPoints();

	// can new data be added or removed
	bool m_isFixedSize;
	// can the positions be changed
	bool m_isFixedX;
	// can the values be changed
	bool m_isFixedY;
	// if true then it makes the last position coordinate 1, 1, the first point coordinate to -1 (ot 0), 0
	bool m_isFixedEndPoints;
	// can VectorGraphView select this
	bool m_isSelectable;
	// can the point attributes be edited
	// every attribute outside of x and y
	// automation can be changed
	bool m_isEditableAttrib;
	// can the points be automated or effected
	// (can these settings be changed)
	bool m_isAutomatableEffectable;
	// if VectorGraphDataArray is allowed to save this
	bool m_isSaveable;
	// can values be less than 0
	bool m_isNonNegative;

	QColor m_lineColor;
	QColor m_activeColor;
	QColor m_fillColor;
	QColor m_automatedColor;

	VectorGraphModel* m_parent;
	// simple id system for setEffectorArrayLocation
	int m_id;

	// which VectorGraphDataArray can effect this one, -1 if not effected
	int m_effectorLocation;

	// ordered array of VectorGraphPoints
	std::vector<VectorGraphPoint> m_dataArray;
	

	// baking

	// getValues() will return m_bakedValues if a line is unchanged
	// else it will recalculate the line's values, update m_bakedValues
	// getValues() needs to know where did lines change so it updates
	// m_needsUpdating by running getUpdatingFromEffector()
	// if m_isDataChanged is true, then getValues adds all the points
	// to m_needsUpdating before running
	// getValues() clears m_needsUpdating after it has run
	// every change is only applyed to the point's line (line started by the point)
	// changes in y will cause multiple points to update

	// if we want to update all
	bool m_isDataChanged;
	// array containing output final float values for optimalization
	std::vector<float> m_bakedValues;
	// unsorted array of locations in m_dataArray
	// that need to be updated
	std::vector<unsigned int> m_needsUpdating;

	// this stores all the FloatModels
	// used for automation
	std::vector<FloatModel*> m_automationModelArray;

	// used for saving
	friend class lmms::VectorGraphModel;
};

} // namespace lmms

#endif // LMMS_VECTORGRAPH_H
