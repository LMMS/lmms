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
#include <array>
#include <QPainterPath>
#include <QWidget>
#include <QCursor>
#include <QMenu>
#include <QMutex>

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

	VectorGraphView(QWidget * parent, int widthIn, int heightIn, unsigned int pointSize,
		unsigned int controlHeight, unsigned int controlDisplayCount, bool shouldApplyDefaultVectorGraphColors);
	~VectorGraphView();

	void setLineColor(QColor color, unsigned int dataArrayLocation);
	void setActiveColor(QColor color, unsigned int dataArrayLocation);
	void setFillColor(QColor color, unsigned int dataArrayLocation);
	void setAutomatedColor(QColor color, unsigned int dataArrayLocation);
	void applyDefaultColors();
	void setPointSize(unsigned int pointSize);
	void setControlHeight(unsigned int controlHeight);
	void setControlDisplayCount(unsigned int controlDisplayCount);

	inline VectorGraphModel* model()
	{
		return castModel<VectorGraphModel>();
	}

	// draws estimated line, does not call getSamples()
	// does not fill graphs with VectorGraphDataArray FillColor
	void setIsSimplified(bool isSimplified);

	// returns -1.0f at .first when nothing is selected
	std::pair<float, float> getSelectedData();
	// returns -1 it can not return an array location
	int getLastSelectedArray();
	// sets the position of the currently selected point
	void setSelectedData(std::pair<float, float> data);
	// sets the background pixmap
	void setBackground(const QPixmap backgound);
	
	// if this function is called
	// paintEvent will not call getSamples() (optimization)
	// insted calls getLastSamples
	// resets after every paint event
	void useGetLastSamples();
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
	void updateGraph(bool shouldUseGetLastSamples);
	void updateDefaultColors();

	void execConnectionDialog();
	void removeAutomation();
	void removeController();
private:
	void paintGraph(QPainter* p, unsigned int arrayLocation, std::vector<float>* sampleBuffer);
	void paintEditing(QPainter* p);

	void modelChanged() override;

	// utility
	// calculate graph coords from screen space coords
	std::pair<float, float> mapMousePos(int x, int y);
	// calculate gui curve point's position
	std::pair<float, float> mapDataCurvePosF(float xA, float yA, float xB, float yB, float curve);
	std::pair<int, int>mapDataCurvePos(int xA, int yA, int xB, int yB, float curve);
	// calculate screen space coords from graph coords
	// isNonNegative can only be true when graph line / getSamples() is mapped
	std::pair<int, int> mapDataPos(float x, float y, bool isNonNegative);
	// map where each Control is displayed when m_isEdtitingActive is true
	int mapControlInputX(float inputValue, unsigned int displayLength);

	float getDistance(int xA, int yA, int xB, int yB);
	float getDistanceF(float xA, float yA, float xB, float yB);

	// adds point to the selected VectorGraphDataArray
	bool addPoint(unsigned int arrayLocation, int mouseX, int mouseY);

	// editing menu / controls
	// returns true if the graph was clicked
	bool isGraphPressed(int mouseX, int mouseY);
	// returns true if the control window was clicked while in editing mode
	bool isControlWindowPressed(int mouseY);
	void processControlWindowPressed(int mouseX, int mouseY, bool isDragging, bool startMoving, int curX, int curY);
	// returns -1 if no control / input was clicked
	// returns displayed absolute control / input location based on inputCount
	int getPressedControlInput(int mouseX, int mouseY, unsigned int controlCount);
	// returns a float attrib value, valueOut = attrib value if it is a bool
	float getInputAttribValue(unsigned int controlArrayLocation, bool* valueOut);
	// sets the selected point's attrib to floatValue it it is float, else it sets the attrib to boolValue
	void setInputAttribValue(unsigned int controlArrayLocation, float floatValue, bool boolValue);
	// calculates the ideal text color
	QColor getTextColorFromBaseColor(QColor baseColor);
	// calculates a replacement background fill color
	QColor getFillColorFromBaseColor(QColor baseColor);
	// cuts the string to displayedLength(in px) size (estimated)
	QString getTextFromDisplayLength(QString text, unsigned int displayLength);
	// context menu actions
	void addDefaultActions(QMenu* menu, QString controlDisplayText);

	// inputDialog
	std::pair<float, float> showCoordInputDialog();
	float showInputDialog(float curInputValue);

	// selection
	// searches VectorGraphDataArray-s to select
	// near clicked location
	void selectData(int mouseX, int mouseY);
	// searches for point in a given VectorGraphDataArray
	// returns found location, when a point
	// was found in the given distance
	// else it returns -1
	int searchForData(int mouseX, int mouseY, float maxDistance, VectorGraphDataArray* dataArray, bool isCurved);

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
	// m_bakedSamples without calling getSamples()
	bool m_useGetLastSamples;

	// if m_isLastSelectedArray == true then
	// m_selectedArray can be used
	// else if m_isSelected == false then
	// m_selectedLocation and m_selectedArray should not be used
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
	std::array<QString, 19> m_controlText =
	{
		tr("x coordinate"), tr("y coordinate"), tr("curve"), tr("1. attribute value"),
		tr("2. attribute value"), tr("switch graph line type"), tr("switch graph automated value"),
		tr("switch graph effected value"), tr("can only effect graph points"), tr("\"add\" effect"), tr("\"subtract\" effect"),
		tr("\"multiply\" effect"), tr("\"divide\" effect"), tr("\"power\" effect"), tr("\"log\" effect"),
		tr("\"sine\" effect"), tr("\"clamp lower\" effect"), tr("\"clamp upper\" effect")
	};
	std::array<QString, 6> m_controlLineEffectText = {
		tr("none"),
		tr("sine"),
		tr("phase changable sine"),
		tr("peak"),
		tr("steps"),
		tr("random")
	};
	std::array<bool, 19> m_controlIsFloat = {
		true, true, true, true,
		true, false, false,
		false, false, false, false,
		false, false, false, false,
		false, false, false
	};

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
	VectorGraphModel(unsigned int arrayMaxLength, Model* parentIn, bool defaultConstructed);
	~VectorGraphModel();

	inline size_t getDataArraySize()
	{
		return m_dataArrays.size();
	}
	inline VectorGraphDataArray* getDataArray(unsigned int arrayLocation)
	{
		return &m_dataArrays[arrayLocation];
	}
	inline unsigned int getMaxLength()
	{
		return m_maxLength;
	}
	inline void setMaxLength(unsigned int arrayMaxLength)
	{
		if (m_maxLength != arrayMaxLength)
		{
			m_maxLength = arrayMaxLength;
			emit dataChanged();
			emit updateGraphView(false);
		}
	}
	// returns added VectorGraphDataArray location
	unsigned int addArray();
	// deletes VectorGraphDataArray at arrayLocation
	// preservs the order
	void delArray(unsigned int arrayLocation);
	inline void clearArray()
	{
		m_dataArrays.clear();
		emit dataChanged();
		emit updateGraphView(false);
	}
	// if the id is not found then it will return 0
	int getDataArrayLocationFromId(int arrayId);
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
	void lockGetSamplesAccess();
	void unlockGetSamplesAccess();
	void lockBakedSamplesAccess();
	void unlockBakedSamplesAccess();
signals:
	// point changed inside VectorGraphDataArray m_dataArray or m_maxLength changed
	void dataChanged();
	void updateGraphView(bool shouldUseGetLastSamples);
	// signals when a dataArray gets to 0 element size
	// arrayLocation is the location of the VectorGraphDataArray
	// arrayLocation can be -1
	void clearedEvent(int arrayLocation);
	// style changed inside m_dataArray
	void styleChanged();
public slots:
	void dataArrayChanged();
	void updateGraphModel(bool shouldUseGetLastSamples);
	void dataArrayClearedEvent(int arrayId);
	void dataArrayStyleChanged();
private:
	std::vector<VectorGraphDataArray> m_dataArrays;
	unsigned int m_maxLength;

	// block threads that want to access
	// a dataArray's getSamples() at the same time
	QMutex m_getSamplesAccess;
	QMutex m_bakedSamplesAccess;
};

class LMMS_EXPORT VectorGraphDataArray
{

public:
	// avoid using this or run updateConnections() after initialization
	VectorGraphDataArray();
	VectorGraphDataArray(
	bool isFixedSize, bool isFixedX, bool isFixedY, bool isNonNegative,
	bool isFixedEndPoints, bool isSelectable, bool isEditableAttrib, bool isAutomatableEffectable,
	bool isSaveable, VectorGraphModel* parentIn, int arrayId);
	~VectorGraphDataArray();

	void updateConnections(VectorGraphModel* parentIn);

	// see descriptions in privete
	void setIsFixedSize(bool bValue);
	void setIsFixedX(bool bValue);
	void setIsFixedY(bool bValue);
	void setIsFixedEndPoints(bool bValue);
	void setIsSelectable(bool bValue);
	void setIsEditableAttrib(bool bValue);
	void setIsAutomatableEffectable(bool bValue);
	void setIsSaveable(bool bValue);
	void setIsNonNegative(bool bValue);
	void setLineColor(QColor color);
	void setActiveColor(QColor color);
	void setFillColor(QColor color);
	void setAutomatedColor(QColor color);

	// returns true if successful
	// if callDataChanged then it will call dataChanged() --> paintEvent()
	bool setEffectorArrayLocation(int arrayLocation, bool callDataChanged);

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
	// checks m_isFixedSize (== false) and m_maxLength
	// returns the location of added point, -1 if not found or can not be added
	// returns the location of found point if there is a point already at newX
	int add(float newX);
	// checks m_isFixedSize (== false)
	// deletes the point in pointLocation location
	void del(unsigned int pointLocation);
	// clears m_dataArray without any checks
	inline void clear()
	{
		m_dataArray.clear();
		m_needsUpdating.clear();
		// m_automationModelArray should not be cleared without FloatModel destruction
		clearedEvent();
		getUpdatingFromPoint(-1);
		dataChanged();
	}
	inline size_t size()
	{
		return m_dataArray.size();
	}
	// clamps down the values to 0 - 1, -1 - 1
	// sorts array, removes duplicated x positions, calls dataChanged() if callDataChanged
	// clamp: should clamp, sort: should sort
	void formatArray(std::vector<std::pair<float, float>>* dataArrayOut, bool shouldClamp, bool shouldRescale, bool shouldSort, bool callDataChanged);


	// get attribute: -------------------
	inline float getX(unsigned int pointLocation)
	{
		return m_dataArray[pointLocation].m_x;
	}
	inline float getY(unsigned int pointLocation)
	{
		return m_dataArray[pointLocation].m_y;
	}
	inline float getC(unsigned int pointLocation)
	{
		return m_dataArray[pointLocation].m_c;
	}
	inline float getValA(unsigned int pointLocation)
	{
		return m_dataArray[pointLocation].m_valA;
	}
	inline float getValB(unsigned int pointLocation)
	{
		return m_dataArray[pointLocation].m_valB;
	}
	inline unsigned int getType(unsigned int pointLocation)
	{
		return m_dataArray[pointLocation].m_type;
	}
	// returns attribLocation: 0 = m_y, 1 = m_c, 2 = m_valA, 3 = m_valB (int VectorGraphPoint)
	unsigned int getAutomatedAttribLocation(unsigned int pointLocation);
	unsigned int getEffectedAttribLocation(unsigned int pointLocation);
	// returns true when m_effectOnlyPoints is true or
	// when getEffectedAttribLocation > 0 (y is uneffected)
	// -> when the current point CAN effect lines before it
	bool getEffectOnlyPoints(unsigned int pointLocation);
	// returns if the effectId-th effect is active
	bool getEffect(unsigned int pointLocation, unsigned int effectId);
	// true when the automationModel's value changed since last check
	bool getIsAutomationValueChanged(unsigned int pointLocation);
	// can return nullptr
	inline FloatModel* getAutomationModel(unsigned int pointLocation);


	// get: -------------------
	// returns -1 when position is not found
	int getLocation(float searchX);
	// gets the nearest data location to the position,
	// foundOut is true when the nearest position = searchXIn,
	// reurns -1 when search failed
	int getNearestLocation(float searchXIn, bool* foundOut, bool* isBeforeOut);


	// returns the latest updated graph values
	// targetSizeIn is the retuned vector's size
	void getSamples(unsigned int targetSizeIn, std::vector<float>* sampleBufferOut);
	// returns m_bakedSamples without updating
	void getLastSamples(std::vector<float>* sampleBufferOut);
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
	void setDataArray(std::vector<std::pair<float, float>>* dataArrayIn, bool shouldCurve, bool shouldClear, bool shouldClamp, bool shouldRescale, bool shouldSort, bool callDataChanged);
	void setDataArray(std::vector<float>* dataArrayIn, bool shouldCurve, bool shouldClear, bool shouldClamp, bool shouldRescale, bool callDataChanged);
	void setDataArray(float* dataArrayIn, unsigned int sizeIn, bool shouldCurve, bool shouldClear, bool shouldClamp, bool shouldRescale, bool callDataChanged);


	// set attribute: -------------------
	// checks m_isFixedX (== false)
	// sets x position, returns final location
	// returns the location of found point if there is a point already at newX
	unsigned int setX(unsigned int pointLocation, float newX);
	// checks m_isFixedY (== false)
	// sets y position
	void setY(unsigned int pointLocation, float newY);
	// checks m_isEditableAttrib
	// sets curve
	void setC(unsigned int pointLocation, float newC);
	// checks m_isEditableAttrib
	// sets 1. attribute value
	void setValA(unsigned int pointLocation, float fValue);
	// checks m_isEditableAttrib
	// sets 2. attribute value
	void setValB(unsigned int pointLocation, float fValue);
	// checks m_isEditableAttrib
	// sets line type
	void setType(unsigned int pointLocation, unsigned int newType);
	// checks m_isAutomatableEffectable and m_isEditableAttrib
	// sets what attribute gets automated (by point's FloatModel)
	void setAutomatedAttrib(unsigned int pointLocation, unsigned int attribLocation);
	// checks m_isAutomatableEffectable and m_isEditableAttrib
	// sets what attribute gets effected (by effector array)
	void setEffectedAttrib(unsigned int pointLocation, unsigned int attribLocation);
	// checks m_isAutomatableEffectable and m_isEditableAttrib
	// if bValue is true then the effector array will not effect the line's individual samples
	void setEffectOnlyPoints(unsigned int pointLocation, bool bValue);
	// checks m_isAutomatableEffectable and m_isEditableAttrib
	// sets the point's effect type
	void setEffect(unsigned int pointLocation, unsigned int effectId, bool bValue);
	// checks m_isAutomatableEffectable
	// if bValue is true then make a new FloatModel and connect it, else delete
	// the currently used FloatModel
	void setAutomated(unsigned int pointLocation, bool bValue);


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
	// encodes m_dataArray to QString
	QString getSavedDataArray();
	// decodes and sets m_dataArray from QString
	void loadDataArray(QString data, unsigned int arraySize);
private:
	class VectorGraphPoint
	{
	public:
		inline VectorGraphPoint()
		{
		}
		inline VectorGraphPoint(float x, float y)
		{
			m_x = x;
			m_y = y;
		}
		// 0 - 1
		float m_x = 0.0f;
		// -1 - 1, getAutomatedAttrib() -> 0
		float m_y = 0.0f;
		// curve, -1 - 1, getAutomatedAttrib() -> 1
		float m_c = 0.0f;
		// valueA, -1 - 1, getAutomatedAttrib() -> 2
		float m_valA = 0.0f;
		// valueB, -1 - 1, getAutomatedAttrib() -> 3
		float m_valB = 0.0f;
		// line type:
		// 0 - none
		// 1 - sine
		// 2 - sineB
		// 3 - peak
		// 4 - steps
		// 5 - random
		unsigned int m_type = 0;
		// the automated attrib location and
		// the effected attrib location is
		// stored here
		// use getAutomatedAttrib or getEffectedAttrib to get it
		unsigned int m_automatedEffectedAttribLocations = 0;

		// if the point's line should effect only pointss
		// getEffectOnlyPoints() will return true when
		// effected attrib location > 0
		bool m_effectOnlyPoints = false;

		bool m_effectAdd = false;
		bool m_effectSubtract = false;
		bool m_effectMultiply = false;
		bool m_effectDivide = false;
		bool m_effectPower = false;
		bool m_effectLog = false;
		bool m_effectSine = false;
		bool m_effectClampLower = false;
		bool m_effectClampUpper = false;

		// stores m_automationModel->value(), used in getSamples() when updating
		float m_bufferedAutomationValue = 0.0f;
		// automation: connecting to floatmodels, -1 when it isn't conntected
		int m_automationModel = -1;
	};
	// deletes the point's automation model
	// if modelLocation == point location
	void delAutomationModel(unsigned int modelLocation, bool callDataChanged);
	// swapping values, "slide" moves the values (between) once left or right
	// handle m_isFixedEndPoints when using this
	void swap(unsigned int pointLocationA, unsigned int pointLocationB, bool slide);
	// returns the curve value at a given x coord, does clamp
	float processCurve(float yBefore, float yAfter, float curve, float xIn);
	// returns effected attribute value from base attribValue (input attribute value), does clamp
	// this function applies the point Effects (like m_effectAdd) based on attribValue and effectValue
	float processEffect(unsigned int pointLocation, float attribValue, unsigned int attribLocation, float effectValue);
	// returns automated attribute value from base attribValue (input attribute value), does clamp
	float processAutomation(unsigned int pointLocation, float attribValue, unsigned int attribLocation);

	// line types, m_type is used for this
	// valA: amp, valB: freq, fadeInStartLoc: from what xIn value should the line type fade out
	std::vector<float> processLineTypeArraySine(std::vector<float>* xArray, unsigned int startLoc, unsigned int endLoc,
		float valA, float valB, float fadeInStartLoc);
	// curve: phase
	std::vector<float> processLineTypeArraySineB(std::vector<float>* xArray, unsigned int startLoc, unsigned int endLoc,
		float valA, float valB, float curve, float fadeInStartLoc);
	// valA: amp, valB: x coord, curve: width
	std::vector<float> processLineTypeArrayPeak(std::vector<float>* xArray, unsigned int startLoc, unsigned int endLoc,
		float valA, float valB, float curve, float fadeInStartLoc);
	// y: calculate steps from, valA: y count, valB: curve
	std::vector<float> processLineTypeArraySteps(std::vector<float>* xArray, unsigned int startLoc, unsigned int endLoc,
		std::vector<float>* yArray, float valA, float valB, float fadeInStartLoc);
	// valA: amp, valB: random number count, curve: seed
	std::vector<float> processLineTypeArrayRandom(std::vector<float>* xArray, unsigned int startLoc, unsigned int endLoc,
		float valA, float valB, float curve, float fadeInStartLoc);

	// updating
	// adds the points that are
	// effected by the effector's values changing
	// ONLY WORKS IN SORTED ARRAYS
	void getUpdatingFromEffector(std::vector<unsigned int>* updatingPointLocations);
	// if pointLocation >= 0 -> adds the location to m_needsUpdating
	// else it will update the whole m_dataArray and m_bakedSamples
	// changes in the size of m_dataArray (addtition, deletion, ect.)
	// should to cause a full update
	void getUpdatingFromPoint(int pointLocation);
	// adds the points that are changed because their
	// automation is changed
	void getUpdatingFromAutomation();
	// recalculates and sorts m_needsUpdating so
	// every point is in there only once
	void getUpdatingOriginals();

	// real getSamples processing
	void getSamples(unsigned int targetSizeIn, bool* isChangedOut, std::vector<unsigned int>* updatingValuesOut, std::vector<float>* sampleBufferOut);
	// redraw lines
	void getSamplesUpdateLines(VectorGraphDataArray* effector, std::vector<float>* effectorSamples,
		std::vector<float>* outputXLocations, unsigned int iIn, float stepSize);
	bool isEffectedPoint(unsigned int pointLocation);

	// checks m_isFixedEndPoints, does not call dataChanged()
	void formatDataArrayEndPoints();

	// can new data be added or removed
	bool m_isFixedSize;
	// can the positions be changed
	bool m_isFixedX;
	// can the values be changed
	bool m_isFixedY;
	// if true then it makes the last point coordinate 1, 1, the first point coordinate -1, 0
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

	// getSamples() will return m_bakedSamples if lines are unchanged
	// else it will recalculate the changed line's values, update m_bakedSamples
	// getSamples() needs to know where did lines change so it updates
	// m_needsUpdating by running getUpdatingFromEffector()
	// if m_isDataChanged is true, then getSamples recalculates all the lines/samples
	// getSamples() clears m_needsUpdating after it has run
	// updating a line means recalculating m_bakedSamples in getSamples()
	// based on the changed points (stored in m_needsUpdating)
	// changes in a point will causes its line to update (line started by the point)
	// changes in position needs to cause multiple lines to update
	// addition or deletion needs to cause all the lines to update

	// if we want to update all (the full line in getSamples())
	bool m_isDataChanged;
	// array containing output final float values for optimalization
	std::vector<float> m_bakedSamples;
	// used for updating m_bakedSamples fast
	std::vector<float> m_updatingBakedSamples;
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
