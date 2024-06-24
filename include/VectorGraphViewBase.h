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
#include <QMdiSubWindow>

#include "ModelView.h"
#include "SimpleTextFloat.h"
#include "AutomatableModel.h"

namespace lmms
{

class VectorGraphModel;
class FloatModel;

namespace gui
{
class LMMS_EXPORT VectorGraphViewBase : public QWidget
{
	Q_OBJECT
public:
	VectorGraphViewBase(QWidget* parent);
	~VectorGraphViewBase();

protected slots:
	virtual void contextMenuRemoveAutomation() = 0;
	virtual void contextMenuExecConnectionDialog();
protected:
	// hints, SimpleTextFloat
	void showHintText(QWidget* thisWidget, QString hintText, int msecBeforeDesplay, int msecDisplayTime);
	void hideHintText();
	// AutomationTrack
	void connectToAutomationTrack(QMouseEvent* me, FloatModel* automationModel, QWidget* thisWidget);

	// context menu
	void showContextMenu(const QPoint point, FloatModel* automationModel, QString displayName, QString controlName);

	// inputDialog
	std::pair<float, float> showCoordInputDialog(std::pair<float, float> pointPosition);
	float showInputDialog(float curInputValue);

private:
	// context menu
	void addDefaultActions(QMenu* menu, QString controlDisplayText);

	SimpleTextFloat* m_hintText;
	FloatModel* m_curAutomationModel;
};

class VectorGraphView;

class LMMS_EXPORT VectorGraphCotnrolDialog : public QMdiSubWindow
{
	Q_OBJECT
public:
	VectorGraphCotnrolDialog(QWidget* parent, VectorGraphView* targetVectorGraphView);
	~VectorGraphCotnrolDialog();

	void hideControls();
	void switchPoint(unsigned int selectedArray, unsigned int selectedLocation);

signals:
	void controlValueChanged();

private:
	VectorGraphView* m_vectorGraphView;

	FloatModel* m_curAutomationModel;

	std::vector<FloatModel*> m_controlModelArray;
};

} // namespace gui
} // namespace lmms

#endif // LMMS_GUI_VECTORGRAPHVIEWBASE_H
