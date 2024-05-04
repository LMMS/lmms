/*
 * VectorGraph.cpp - Vector graph widget, model, helper class implementation
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


#include "VectorGraphViewBase.h"

#include <vector>
#include <QInputDialog> // showInputDialog()
#include <QMenu> // context menu


#include "StringPairDrag.h"
#include "CaptionMenu.h" // context menu
#include "embed.h" // context menu
#include "MainWindow.h" // getting main window for context menu
#include "GuiApplication.h" // getGUI
#include "SimpleTextFloat.h"
#include "AutomatableModel.h"
#include "ControllerConnectionDialog.h"
#include "ControllerConnection.h"


namespace lmms
{

namespace gui
{
VectorGraphViewBase::VectorGraphViewBase(QWidget* parent) :
	QWidget(parent),
	m_hintText(new SimpleTextFloat)
{
}
VectorGraphViewBase::~VectorGraphViewBase()
{
	delete m_hintText;
}

void VectorGraphViewBase::showHintText(QWidget* thisWidget, QString hintText, int msecBeforeDisplay, int msecDisplayTime)
{
	m_hintText->setText(hintText);
	m_hintText->moveGlobal(thisWidget, QPoint(width() + 2, 0));
	m_hintText->showWithDelay(msecBeforeDisplay, msecDisplayTime);
}
void VectorGraphViewBase::hideHintText()
{
	m_hintText->hide();
}

void VectorGraphViewBase::connectToAutomationTrack(QMouseEvent* me, FloatModel* automationModel, QWidget* thisWidget)
{
	if (automationModel != nullptr)
	{
		qDebug("mousePress automation sent");
		new gui::StringPairDrag("automatable_model", QString::number(automationModel->id()), QPixmap(), thisWidget);
		me->accept();
	}
}

void VectorGraphViewBase::showContextMenu(const QPoint point, FloatModel* automationModel, QString displayName, QString controlName)
{
	CaptionMenu contextMenu(displayName + QString(" - ") + controlName);
	addDefaultActions(&contextMenu, automationModel, controlName);
	contextMenu.exec(point);
}
void VectorGraphViewBase::addDefaultActions(QMenu* menu, FloatModel* automationModel, QString controlDisplayText)
{
	// context menu settings
	menu->addAction(embed::getIconPixmap("reload"),
		tr("name: ") + controlDisplayText,
		this, SLOT(contextMenuRemoveAutomation()));
	menu->addAction(embed::getIconPixmap("reload"),
		tr("remove automation"),
		this, SLOT(contextMenuRemoveAutomation()));
	menu->addSeparator();

	QString controllerTxt;

	menu->addAction(embed::getIconPixmap("controller"),
		tr("Connect to controller..."),
		this, SLOT(execConnectionDialog(automationModel)));
	if(automationModel != nullptr && automationModel->controllerConnection() != nullptr)
	{
		
		Controller* cont = automationModel->controllerConnection()->getController();
		if(cont)
		{
			controllerTxt = AutomatableModel::tr( "Connected to %1" ).arg( cont->name() );
		}
		else
		{
			controllerTxt = AutomatableModel::tr( "Connected to controller" );
		}


		QMenu* contMenu = menu->addMenu(embed::getIconPixmap("controller"), controllerTxt);

		contMenu->addAction(embed::getIconPixmap("cancel"),
			tr("Remove connection"),
			this, SLOT(contextMenuRemoveAutomation()));
	}
}

void VectorGraphViewBase::contextMenuExecConnectionDialog(FloatModel* automationModel)
{
	if (automationModel != nullptr)
	{
		gui::ControllerConnectionDialog dialog(getGUI()->mainWindow(), automationModel);

		if (dialog.exec() == 1)
		{
			// Actually chose something
			if (dialog.chosenController() != nullptr)
			{
				// Update
				if (automationModel->controllerConnection() != nullptr)
				{
					automationModel->controllerConnection()->setController(dialog.chosenController());
				}
				else
				{
					// New
					auto cc = new ControllerConnection(dialog.chosenController());
					automationModel->setControllerConnection(cc);
				}
			}
			else
			{
				// no controller, so delete existing connection
				contextMenuRemoveAutomation();
			}
		}
		else
		{
			// did not return 1 -> delete the created floatModel
			contextMenuRemoveAutomation();
		}
	}
}

std::pair<float, float> VectorGraphViewBase::showCoordInputDialog(std::pair<float, float> pointPosition)
{
	// show position input dialog
	bool ok;
	double changedX = QInputDialog::getDouble(this, tr("Set value"),
		tr("Please enter a new value between 0 and 100"),
		static_cast<double>(pointPosition.first * 100.0f),
		0.0, 100.0, 2, &ok);
	if (ok == true)
	{
		pointPosition.first = static_cast<float>(changedX) / 100.0f;
	}

	double changedY = QInputDialog::getDouble(this, tr("Set value"),
		tr("Please enter a new value between -100 and 100"),
		static_cast<double>(pointPosition.second * 100.0f),
		-100.0, 100.0, 2, &ok);
	if (ok == true)
	{
		pointPosition.second = static_cast<float>(changedY) / 100.0f;
	}
	return pointPosition;
}
float VectorGraphViewBase::showInputDialog(float curInputValue)
{
	float output = 0.0f;

	bool ok;
	double changedPos = QInputDialog::getDouble(this, tr("Set value"),
		tr("Please enter a new value between -100 and 100"),
		static_cast<double>(curInputValue * 100.0f),
		-100.0, 100.0, 2, &ok);
	if (ok == true)
	{
		output = static_cast<float>(changedPos) / 100.0f;
	}

	return output;
}


} // namespace gui
} // namespace lmms
