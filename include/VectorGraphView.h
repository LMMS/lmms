/*
 * VecorGraph.h - Vector graph widget implementation
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

#ifndef LMMS_GUI_VECTORGRAPHVIEW_H
#define LMMS_GUI_VECTORGRAPHVIEW_H

#include <vector>
#include <array>
#include <QPainterPath>
#include <QWidget>
#include <QCursor>
#include <QMenu>
#include <QMutex>

#include "VectorGraphViewBase.h"
#include "VectorGraphModel.h"
#include "Model.h"
#include "ModelView.h"
#include "lmms_basics.h"
#include "AutomatableModel.h"
#include "JournallingObject.h"
#include "SubWindow.h"

namespace lmms
{

//class VectorGraphView;
class VectorGraphModel;
class VectorGraphDataArray;
//class FloatModel;

using PointF = std::pair<float, float>;
using PointInt = std::pair<int, int>;

namespace gui
{
class LMMS_EXPORT VectorGraphView : public VectorGraphViewBase, public ModelView
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

	VectorGraphView(QWidget* parent, int widgetWidth, int widgetHeight, unsigned int pointSize,
		unsigned int controlHeight, bool shouldApplyDefaultVectorGraphColors);
	~VectorGraphView();

	void setLineColor(QColor color, unsigned int dataArrayLocation);
	void setActiveColor(QColor color, unsigned int dataArrayLocation);
	void setFillColor(QColor color, unsigned int dataArrayLocation);
	void setAutomatedColor(QColor color, unsigned int dataArrayLocation);
	void applyDefaultColors();
	void setPointSize(unsigned int pointSize);
	void setControlHeight(unsigned int controlHeight);


	inline VectorGraphModel* model()
	{
		return castModel<VectorGraphModel>();
	}


	// draws estimated line, does not call getSamples()
	// does not fill graphs with VectorGraphDataArray FillColor
	void setIsSimplified(bool isSimplified);
	bool getIsSimplified();

	// returns -1.0f at .first when nothing is selected
	PointF getSelectedData();
	// returns -1 it can not return an array location
	int getLastSelectedArray();
	// sets the position of the currently selected point
	void setSelectedData(PointF data);
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

	void leaveEvent(QEvent *event) override;
protected slots:
	void updateGraph();
	void updateGraph(bool shouldUseGetLastSamples);
	void updateDefaultColors();
private:
	void paintGraph(QPainter* p, unsigned int arrayLocation, std::vector<float>* sampleBuffer);
	void paintEditing(QPainter* p);

	void modelChanged() override;

	// utility
	// calculate graph coords from screen space coords
	PointF mapMousePos(int x, int y);
	// calculate gui curve point's position
	PointF mapDataCurvePosF(float xA, float yA, float xB, float yB, float curve);
	PointInt mapDataCurvePos(int xA, int yA, int xB, int yB, float curve);
	// calculate screen space coords from graph coords
	// isNonNegative can only be true when graph line / getSamples() is mapped
	PointInt mapDataPos(float x, float y, bool isNonNegative);
	// map where each Control is displayed when m_isEdtitingActive is true
	int mapControlInputX(float inputValue, unsigned int displayLength);

	float getDistance(int xA, int yA, int xB, int yB);
	float getDistanceF(float xA, float yA, float xB, float yB);

	// adds point to the selected VectorGraphDataArray
	bool addPoint(unsigned int arrayLocation, int mouseX, int mouseY);

	// editing menu / controls
	// returns true if the graph was clicked
	bool isGraphPressed(int mouseY);
	// returns true if the control window was clicked while in editing mode
	bool isControlWindowPressed(int mouseY);
	void processControlWindowPressed(int mouseX, int mouseY, bool isDragging, bool startMoving);
	// returns -1 if no control / input was clicked
	// returns displayed absolute control / input location based on inputCount
	int getPressedControlInput(int mouseX, int mouseY, size_t controlCount);
	// returns a float attrib value
	float getInputAttribValue(unsigned int controlArrayLocation);
	// sets the selected point's attrib (controlArrayLocation) to floatValue
	void setInputAttribValue(unsigned int controlArrayLocation, float floatValue);
	// calculates the ideal text color
	QColor getTextColorFromBaseColor(QColor baseColor);
	// calculates a replacement background fill color
	QColor getFillColorFromBaseColor(QColor baseColor);

	SubWindow* m_controlDialog;

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
	// displayed control count
	size_t m_controlDisplayCount;
	bool m_isEditingActive;
	const std::array<QString, 2> m_controlText =
	{
		tr("edit point"), tr("switch graph")
	};

	PointInt m_lastTrackPoint;
	PointInt m_lastScndTrackPoint;

	// default VectorGraphDataArray colors
	// applyed in constructor
	QColor m_vectorGraphDefaultAutomatedColor;

	QColor m_vectorGraphDefaultLineColor;
	QColor m_vectorGraphDefaultActiveColor;
	QColor m_vectorGraphDefaultFillColor;
	QColor m_vectorGraphSecondaryLineColor;
	QColor m_vectorGraphSecondaryActiveColor;
	QColor m_vectorGraphSecondaryFillColor;

	friend class lmms::gui::VectorGraphCotnrolDialog;
};

} // namespace gui
} // namespace lmms

#endif // LMMS_GUI_VECTORGRAPHVIEW_H
