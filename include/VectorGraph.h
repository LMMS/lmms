/*
 * VecorGraph.h - Vector graph widget and model implementation
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

#ifndef LMMS_GUI_VECTORGRAPH_H
#define LMMS_GUI_VECTORGRAPH_H

#include <vector>
#include <mutex>
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
	// set default VectorGraph css colors and styles
	Q_PROPERTY(QColor vectorGraphDefaultAutomatedColor MEMBER m_vectorGraphDefaultAutomatedColor NOTIFY changedDefaultColors)
	Q_PROPERTY(QColor vectorGraphDefaultLineColor MEMBER m_vectorGraphDefaultLineColor NOTIFY changedDefaultColors)
	Q_PROPERTY(QColor vectorGraphDefaultActiveColor MEMBER m_vectorGraphDefaultActiveColor NOTIFY changedDefaultColors)
	Q_PROPERTY(QColor vectorGraphDefaultFillColor MEMBER m_vectorGraphDefaultFillColor NOTIFY changedDefaultColors)
	Q_PROPERTY(QColor vectorGraphSecondaryLineColor MEMBER m_vectorGraphSecondaryLineColor NOTIFY changedDefaultColors)
	Q_PROPERTY(QColor vectorGraphSecondaryActiveColor MEMBER m_vectorGraphSecondaryActiveColor NOTIFY changedDefaultColors)
	Q_PROPERTY(QColor vectorGraphSecondaryFillColor MEMBER m_vectorGraphSecondaryFillColor NOTIFY changedDefaultColors)

	Q_PROPERTY(int fontSize MEMBER m_fontSize)
public:

	VectorGraphView(QWidget * parentIn, int widthIn, int heightIn,
		unsigned int pointSizeIn, unsigned int maxLengthIn, bool shouldApplyDefaultVectorGraphColorsIn);
	~VectorGraphView();

	void setLineColor(QColor colorIn, unsigned int dataArrayLocationIn);
	void setActiveColor(QColor colorIn, unsigned int dataArrayLocationIn);
	void setFillColor(QColor colorIn, unsigned int dataArrayLocationIn);
	void setAutomatedColor(QColor colorIn, unsigned int dataArrayLocationIn);
	void applyDefaultColors();

	inline VectorGraphModel* model()
	{
		return castModel<VectorGraphModel>();
	}

	// draws estimated line, does not call getValues()
	// does not fill graphs with VectorGraphDataArray FillColor
	void setIsSimplified(bool isSimplifiedIn);

	// returns -1.0f at .first when nothing is selected
	std::pair<float, float> getSelectedData();
	// returns -1 it can not return an array location
	int getLastSelectedArray();
	// sets the position of the currently selected point
	void setSelectedData(std::pair<float, float> dataIn);
	// sets the background pixmap
	void setBackground(const QPixmap backgoundIn);
	
	// if this function is called
	// paintEvent will not call getValues() (optimization)
	// insted calls getLastValues
	// resets after every paint event
	void useGetLastValues();
signals:
	// emited after paintEvent
	void drawn();
	void changedDefaultColors();
protected:
	void paintEvent(QPaintEvent* pe) override;
	void mousePressEvent(QMouseEvent* me) override;
	void mouseMoveEvent(QMouseEvent* me) override;
	void mouseReleaseEvent(QMouseEvent* me) override;
	void mouseDoubleClickEvent(QMouseEvent* me) override;
protected slots:
	void updateGraph();
	void updateGraph(bool shouldUseGetLastValuesIn);
	void updateDefaultColors();

	void execConnectionDialog();
	void removeAutomation();
	void removeController();
private:
	void paintGraph(QPainter* pIn, unsigned int locationIn, std::vector<int>* alreadyUpdatedDataArraysIn);
	void paintEditing(QPainter* pIn);

	void modelChanged() override;

	// utility
	// calculate graph coords from screen space coords
	std::pair<float, float> mapMousePos(int xIn, int yIn);
	// calculate gui curve point's position
	std::pair<float, float> mapDataCurvePos(float xAIn, float yAIn, float xBIn, float yBIn, float curveIn);
	std::pair<int, int>mapDataCurvePos(int xAIn, int yAIn, int xBIn, int yBIn, float curveIn);
	// calculate screen space coords from graph coords
	// isNonNegativeIn can only be true when graph line / getValues() is mapped
	std::pair<int, int> mapDataPos(float xIn, float yIn, bool isNonNegativeIn);
	// map where each Control is displayed when m_isEdtitingActive is true
	int mapControlInputX(float inputValueIn, unsigned int displayLengthIn);

	float getDistance(int xAIn, int yAIn, int xBIn, int yBIn);
	float getDistance(float xAIn, float yAIn, float xBIn, float yBIn);

	// adds point to the selected VectorGraphDataArray
	bool addPoint(unsigned int locationIn, int mouseXIn, int mouseYIn);

	// editing menu / controls
	// returns true if the graph was clicked
	bool isGraphPressed(int mouseXIn, int mouseYIn);
	// returns true if the control window was clicked while in editing mode
	bool isControlWindowPressed(int mouseYIn);
	void processControlWindowPressed(int mouseXIn, int mouseYIn, bool isDraggingIn, bool startMovingIn, int curXIn, int curYIn);
	// returns -1 if no control / input was clicked
	// returns displayed absolute control / input location based on inputCountIn
	int getPressedControlInput(int mouseXIn, int mouseYIn, unsigned int inputCountIn);
	// returns a float attrib value, valueOut = attrib value if it is a bool
	float getInputAttribValue(unsigned int controlArrayLocationIn, bool* valueOut);
	// sets the selected point's attrib to floatValueIn it it is float, else it sets the attrib to boolValueIn
	void setInputAttribValue(unsigned int controlArrayLocationIn, float floatValueIn, bool boolValueIn);
	// calculates the ideal text color
	QColor getTextColorFromBaseColor(QColor baseColorIn);
	// calculates a replacement background fill color
	QColor getFillColorFromBaseColor(QColor baseColorIn);
	// cuts the string to displayedLength(in px) size (estimated)
	QString getTextFromDisplayLength(QString textIn, unsigned int displayLengthIn);
	// context menu actions
	void addDefaultActions(QMenu* menu, QString controlDisplayTextIn);

	// inputDialog
	std::pair<float, float> showCoordInputDialog();
	float showInputDialog(float curInputValueIn);

	// selection
	// searches VectorGraphDataArray-s to select
	// near clicked location
	void selectData(int mouseXIn, int mouseYIn);
	// searches for point in a given VectorGraphDataArray
	// returns found location, when a point
	// was found in the given distance
	// else it returns -1
	int searchForData(int mouseXIn, int mouseYIn, float maxDistanceIn, VectorGraphDataArray* arrayIn, bool curvedIn);

	// if the mouse is not moved
	bool m_mousePress;
	// decides addition or deletion
	bool m_addition;

	// radius, rx = ry
	unsigned int m_pointSize;
	unsigned int m_fontSize;
	// draw simplified lines
	bool m_isSimplified;
	// true when applyDefaultColors is called, used when defaultColors are loading
	bool m_isDefaultColorsApplyed;
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
	// displayed control count (+1 because of the ">>" button in editing mode)
	unsigned int m_controlDisplayCount;
	unsigned int m_controlDisplayPage;
	bool m_isEditingActive;
	std::vector<QString> m_controlText;
	std::vector<QString> m_controlLineEffectText;
	std::vector<bool> m_controlIsFloat;

	std::pair<int, int> m_lastTrackPoint;
	std::pair<int, int> m_lastScndTrackPoint;

	
	// default VectorGraphDataArray colors
	// applyed in constructor
	QColor m_vectorGraphDefaultAutomatedColor;

	QColor m_vectorGraphDefaultLineColor;
	QColor m_vectorGraphDefaultActiveColor;
	QColor m_vectorGraphDefaultFillColor;
	QColor m_vectorGraphSecondaryLineColor;
	QColor m_vectorGraphSecondaryActiveColor;
	QColor m_vectorGraphSecondaryFillColor;
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
			emit updateGraphView(false);
		}
	}
	// returns added VectorGraphDataArray location
	unsigned int addArray();
	// deletes VectorGraphDataArray at locationIn
	// preservs the order
	void delArray(unsigned int locationIn);
	inline void clearArray()
	{
		m_dataArrays.clear();
		emit dataChanged();
		emit updateGraphView(false);
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
	// read locations from saved data attributes unused but implemeted
	//int readLoc(unsigned int startIn, QString dataIn);
	void lockGetValuesAccess();
	void unlockGetValuesAccess();
	void lockBakedValuesAccess();
	void unlockBakedValuesAccess();
signals:
	// point changed inside VectorGraphDataArray m_dataArray or m_maxLength changed
	void dataChanged();
	void updateGraphView(bool shouldUseGetLastValuesIn);
	// signals when a dataArray gets to 0 element size
	// locationIn is the location of the VectorGraphDataArray
	// locationIn can be -1
	void clearedEvent(int locationIn);
	// style changed inside m_dataArray
	void styleChanged();
public slots:
	void dataArrayChanged();
	void updateGraphModel(bool shouldUseGetLastValuesIn);
	void dataArrayClearedEvent(int idIn);
	void dataArrayStyleChanged();
private:
	std::vector<VectorGraphDataArray> m_dataArrays;
	unsigned int m_maxLength;

	// block threads that want to access
	// a dataArray's getValues() at the same time
	std::mutex m_getValuesAccess;
	std::mutex m_bakedValuesAccess;
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

	// see descriptions in privete
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
	// if callDataChangedIn then it will call dataChanged() --> paintEvent()
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
	// returns the location of added/found point, -1 if not found or can not be added
	int add(float xIn);
	// deletes the point in locationIn location if m_isFixedSize is disabled
	void del(unsigned int locationIn);
	// clears m_dataArray without any checks
	inline void clear()
	{
		m_dataArray.clear();
		m_needsUpdating.clear();
		// m_automationModelArray sould not be cleared without destruction
		clearedEvent();
		getUpdatingFromPoint(-1);
		dataChanged();
	}
	inline size_t size()
	{
		return m_dataArray.size();
	}
	// clamps down the values to 0 - 1, -1 - 1
	// sorts array, removes duplicated positions, calls dataChanged() if callDataChangedIn
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
	// returns attribLocation: 0 = m_y, 1 = m_c, 2 = m_valA, 3 = m_valB (int VectorGraphPoint)
	unsigned int getAutomatedAttribLocation(unsigned int locationIn);
	unsigned int getEffectedAttribLocation(unsigned int locationIn);
	// returns true when m_effectOnlyPoints is true or
	// when getEffectedAttribLocation > 0 (y is uneffected)
	// -> when the current point CAN effect lines before it
	bool getEffectOnlyPoints(unsigned int locationIn);
	// returns if the effectNumberIn-th effect is active
	bool getEffect(unsigned int locationIn, unsigned int effectNumberIn);
	// true when the automationModel's value changed since last check
	bool getIsAutomationValueChanged(unsigned int locationIn);
	// can return nullptr
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
	// returns m_bakedValues without updating
	void getLastValues(std::vector<float>* copyBufferOut);
	std::vector<int> getEffectorArrayLocations();


	// set: -------------------
	// sets / adds m_dataArray points
	// .first = x, .second = y coords
	// isCurvedIn -> should set curve automatically
	// clearIn -> clear m_dataArray before setting
	// clampIn -> clamp input positions
	// rescaleIn -> scale input positions
	// sortIn -> sort input positions
	// callDataChangedIn -> call dataChanged() after -> paintEvent()
	void setDataArray(std::vector<std::pair<float, float>>* dataArrayIn, bool isCurvedIn, bool clearIn, bool clampIn, bool rescaleIn, bool sortIn, bool callDataChangedIn);
	void setDataArray(std::vector<float>* dataArrayIn, bool isCurvedIn, bool clearIn, bool clampIn, bool rescaleIn, bool callDataChangedIn);
	void setDataArray(float* dataArrayIn, unsigned int sizeIn, bool isCurvedIn, bool clearIn, bool clampIn, bool rescaleIn, bool callDataChangedIn);


	// set attribute: -------------------
	// sets position when m_isFixedX is disabled, returns final location
	unsigned int setX(unsigned int locationIn, float xIn);
	// sets value when m_isFixedY is disabled
	void setY(unsigned int locationIn, float yIn);
	// sets value when m_isEditableAttrib is enabled
	void setC(unsigned int locationIn, float cIn);
	// sets value when m_isEditableAttrib is enabled
	void setValA(unsigned int locationIn, float valueIn);
	// sets value when m_isEditableAttrib is enabled
	void setValB(unsigned int locationIn, float valueIn);
	// sets value when m_isEditableAttrib is enabled
	void setType(unsigned int locationIn, unsigned int typeIn);
	// sets attribute settings when m_isEditableAttrib and m_isAutomatableEffectable is enabled
	void setAutomatedAttrib(unsigned int locationIn, unsigned int attribLocationIn);
	void setEffectedAttrib(unsigned int locationIn, unsigned int attribLocationIn);
	void setEffectOnlyPoints(unsigned int locationIn, bool boolIn);
	void setEffect(unsigned int locationIn, unsigned int effectNumberIn, bool boolIn);
	// if isAutomatedIn is true then make a new FloatModel and connect it, else delete
	// the currently used FloatModel
	// runs if m_isAutomatableEffectable is enabled
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
	// delete automationModels in m_automationModelArray
	// that are not used by points (there should be 0 cases like this)
	void delUnusedAutomation();
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
	// if locationIn == point location
	void delAutomationModel(unsigned int modelLocationIn, bool callDataChangedIn);
	// swapping values, "slide" moves the values (between) once left or right
	// handle m_isFixedEndPoints when using this
	void swap(unsigned int locationAIn, unsigned int locationBIn, bool slide);
	// returns the curve value at a given x coord, does clamp
	float processCurve(float valueBeforeIn, float valueAfterIn, float curveIn, float xIn);
	// applys the effect on a given value, does clamp
	float processEffect(unsigned int locationIn, float attribValueIn, unsigned int attribLocationIn, float effectValueIn);
	// returns a VectorGraphPoint with modified attributes, does clamp
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
	// if locationIn >= 0 -> adds the location to m_needsUpdating
	// else it will update the whole m_dataArray and m_bakedValues
	// changes in the size of m_dataArray (addtition, deletion, ect.)
	// should to cause a full update
	void getUpdatingFromPoint(int locationIn);
	// adds the points that are changed because their
	// automation is changed
	void getUpdatingFromAutomation();
	// recalculates and sorts m_needsUpdating so
	// every point is in there only once
	void getUpdatingOriginals();

	// real getValues processing
	std::vector<float> getValues(unsigned int countIn, bool* isChangedOut, std::vector<unsigned int>* updatingValuesOut);
	// gets every m_needsUpdating point's line's start and end effector point's location in the effector dataArray
	// .first = start, .second = line end location (effector dataArray)
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
	// changes in position will cause multiple points to update

	// if we want to update all (the full line in getValues())
	bool m_isDataChanged;
	// array containing output final float values for optimalization
	std::vector<float> m_bakedValues;
	// used for updating m_bakedValues fast
	std::vector<float> m_updatingBakedValues;
	// unsorted array of locations in m_dataArray
	// that need to be updated
	// sorted in getUpdatingOriginals() because some functions need this to be sorted
	std::vector<unsigned int> m_needsUpdating;

	// this stores all the FloatModels
	// used for automation
	std::vector<FloatModel*> m_automationModelArray;

	// used for saving
	friend class lmms::VectorGraphModel;
};

} // namespace lmms

#endif // LMMS_GUI_VECTORGRAPH_H
