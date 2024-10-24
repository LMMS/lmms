/*
 * VectorGraphViewBase.h - contains implementations of lmms widget classes for VectorGraph
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

#ifndef LMMS_GUI_VECTORGRAPHVIEWBASE_H
#define LMMS_GUI_VECTORGRAPHVIEWBASE_H

#include <QWidget>
#include <QCursor>
#include <QMenu>
#include <QLayout>

#include "ModelView.h"
#include "SimpleTextFloat.h"
#include "AutomatableModel.h"
#include "Knob.h"
#include "ComboBoxModel.h"
#include "ComboBox.h"

namespace lmms
{

class VectorGraphView;
class FloatModel;

namespace gui
{
class LMMS_EXPORT VectorGraphViewBase : public QWidget
{
	Q_OBJECT
public:
	VectorGraphViewBase(QWidget* parent);
	~VectorGraphViewBase();

protected:
	// hints, SimpleTextFloat
	void showHintText(QWidget* thisWidget, QString hintText, int msecBeforeDesplay, int msecDisplayTime);
	void hideHintText();
	// AutomationTrack
	void connectToAutomationTrack(QMouseEvent* me, FloatModel* automationModel, QWidget* thisWidget);

	// context menu
	//void showContextMenu(const QPoint point, FloatModel* automationModel, QString displayName, QString controlName);

	// inputDialog
	std::pair<float, float> showCoordInputDialog(std::pair<float, float> pointPosition);

private:
	// context menu
	//void addDefaultActions(QMenu* menu, QString controlDisplayText);

	SimpleTextFloat* m_hintText;
};

class VectorGraphView;

class LMMS_EXPORT VectorGraphCotnrolDialog : public QWidget, public ModelView
{
	Q_OBJECT
public:
	VectorGraphCotnrolDialog(QWidget* parent, VectorGraphView* targetVectorGraphView);
	~VectorGraphCotnrolDialog();

	// deletes m_curAutomationModelKnob connected to the VectorGraphDataArray::m_automationModelArray
	void hideAutomation();
	// connects or adds m_curAutomationModelKnob to the gui, sets the selected point
	void switchPoint(unsigned int selectedArray, unsigned int selectedLocation);

public slots:
	void controlValueChanged();
	// slots for buttons
	void effectedPointClicked(bool isChecked);
	void effectedLineClicked(bool isChecked);
	void deleteAutomationClicked(bool isChecked);
protected slots:
	// needs to be used
	// to not delete this control dialog widget when closing
	void closeEvent(QCloseEvent * ce);
private:
	// loads in the selected point's values, settings and attributes
	void updateControls();
	// sets the selected point's values to the control knob's values
	void updateVectorGraphAttribs();

	VectorGraphView* m_vectorGraphView;

	FloatModel* m_curAutomationModel;
	Knob* m_curAutomationModelKnob;
	QVBoxLayout* m_automationLayout;
	// VectorGraphView should run hideAutomation()
	// to notify that these are invalid
	// selected VectorGraphDataArray
	unsigned int m_curSelectedArray;
	// selected VectorGraphPoint
	unsigned int m_curSelectedLocation;
	bool m_isValidSelection;

	ComboBoxModel m_lineTypeModel;
	ComboBoxModel m_automatedAttribModel;
	ComboBoxModel m_effectedAttribModel;
	ComboBoxModel m_effectModelA;
	ComboBoxModel m_effectModelB;
	ComboBoxModel m_effectModelC;

	const std::array<QString, 10> m_controlFloatText =
	{
		tr("x coordinate"), "x",  tr("y coordinate"), "y", tr("curve"), tr("curve"), tr("1. attribute value"), tr("1. attribute"),
		tr("2. attribute value"), tr("2. attribute")
	};
	const std::array<QString, 7> m_controlLineTypeText = {
		tr("none"),
		tr("bézier"),
		tr("sine"),
		tr("phase changable sine"),
		tr("peak"),
		tr("steps"),
		tr("random")
	};
	const std::array<QString, 10> m_controlLineEffectText = {
		tr("none"), tr("\"add\" effect"), tr("\"subtract\" effect"), tr("\"multiply\" effect"), tr("\"divide\" effect"),
		tr("\"power\" effect"), tr("\"log\" effect"), tr("\"sine\" effect"), tr("\"clamp lower\" effect"), tr("\"clamp upper\" effect")
	};


	std::vector<FloatModel*> m_controlModelArray;
	std::vector<Knob*> m_hideableKnobs;
	std::vector<ComboBox*> m_hideableComboBoxes;
};

} // namespace gui
} // namespace lmms

#endif // LMMS_GUI_VECTORGRAPHVIEWBASE_H
