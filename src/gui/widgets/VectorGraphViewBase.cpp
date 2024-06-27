/*
 * VectorGraphViewBase.cpp - contains implementations of lmms widget classes for VectorGraph
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
#include "VectorGraph.h"

#include <vector>
#include <QInputDialog> // showInputDialog()
#include <QMenu> // context menu
#include <QLayout>
#include <QLabel>
#include <QPushButton>


#include "StringPairDrag.h"
#include "CaptionMenu.h" // context menu
#include "embed.h" // context menu
#include "MainWindow.h" // getting main window for context menu
#include "GuiApplication.h" // getGUI
#include "SimpleTextFloat.h"
#include "AutomatableModel.h"
#include "ControllerConnectionDialog.h"
#include "ControllerConnection.h"
#include "Knob.h"
#include "ComboBox.h"


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
	m_hintText->moveGlobal(thisWidget, QPoint(0, 0));
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
	m_curAutomationModel = automationModel;
	CaptionMenu contextMenu(displayName + QString(" - ") + controlName);
	addDefaultActions(&contextMenu, controlName);
	contextMenu.exec(point);
}
void VectorGraphViewBase::addDefaultActions(QMenu* menu, QString controlDisplayText)
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
		this, SLOT(contextMenuExecConnectionDialog()));
	if(m_curAutomationModel != nullptr && m_curAutomationModel->controllerConnection() != nullptr)
	{
		Controller* cont = m_curAutomationModel->controllerConnection()->getController();
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

void VectorGraphViewBase::contextMenuExecConnectionDialog()
{
	if (m_curAutomationModel != nullptr)
	{
		gui::ControllerConnectionDialog dialog(getGUI()->mainWindow(), m_curAutomationModel);

		if (dialog.exec() == 1)
		{
			// Actually chose something
			if (dialog.chosenController() != nullptr)
			{
				// Update
				if (m_curAutomationModel->controllerConnection() != nullptr)
				{
					m_curAutomationModel->controllerConnection()->setController(dialog.chosenController());
				}
				else
				{
					// New
					auto cc = new ControllerConnection(dialog.chosenController());
					m_curAutomationModel->setControllerConnection(cc);
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


VectorGraphCotnrolDialog::VectorGraphCotnrolDialog(QWidget* _parent, VectorGraphView* targetVectorGraphModel) :
	QMdiSubWindow(_parent),
	m_vectorGraphView(targetVectorGraphModel),
	m_curAutomationModel(nullptr),
	m_curAutomationModelKnob(nullptr),
	m_automationLayout(nullptr),
	m_lineTypeModel(nullptr, "", false),
	m_automatedAttribModel(nullptr, "", false),
	m_effectedAttribModel(nullptr, "", false),
	m_effectModelA(nullptr, "", false),
	m_effectModelB(nullptr, "", false),
	m_effectModelC(nullptr, "", false)
{

	auto makeKnob = [this](const QString& label, const QString& hintText, const QString& unit, FloatModel* model)
	{
        Knob* newKnob = new Knob(KnobType::Bright26, this);
        newKnob->setModel(model);
        newKnob->setLabel(label);
        newKnob->setHintText(hintText, unit);
        newKnob->setVolumeKnob(false);
        return newKnob;
    };

	/*
	setWindowIcon(embed::getIconPixmap("setup_audio"));
	setWindowTitle(tr("Connection Settings"));
	//setModal(true);
*/
	setWindowTitle(tr("vector graph settings"));
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

	qDebug("VectorGraphControllerDialog Running");

	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	resize(300, 500);

	if (layout() != nullptr)
	{
		delete layout();
		//m_subWindow->layout()->setSizeConstraint(QLayout::SetFixedSize);
	}

	QHBoxLayout* mainLayout = new QHBoxLayout(this);
	mainLayout->setContentsMargins(5, 5, 5, 5);
	setLayout(mainLayout);

	QVBoxLayout* knobLayout = new QVBoxLayout(nullptr);
	mainLayout->addLayout(knobLayout);
	
	for (size_t i = 0; i < m_controlFloatText.size(); i += 2)
	{
		FloatModel* curModel = new FloatModel(0.0f, (i == 0 ? 0.0f : -1.0f), 1.0f, 0.01f, nullptr, QString(), false);
		m_controlModelArray.push_back(curModel);
		Knob* newKnob = makeKnob(m_controlFloatText[i + 1], m_controlFloatText[i], "%", curModel);
		knobLayout->addWidget(newKnob);
		knobLayout->setAlignment(newKnob, Qt::AlignHCenter);

		connect(curModel, &AutomatableModel::setValueEvent,
				this, &VectorGraphCotnrolDialog::controlValueChanged);
		/*
		connect(curModel, SIGNAL(setValueEvent()),
				this, SLOT(controlValueChanged()));
				*/
	}
	
	QVBoxLayout* settingLayout = new QVBoxLayout(nullptr);
	mainLayout->addLayout(settingLayout);

	for (size_t i = 0; i < m_controlLineTypeText.size(); i++)
	{
		m_lineTypeModel.addItem(m_controlLineTypeText[i], nullptr);
	}
	m_automatedAttribModel.addItem(m_controlFloatText[2], nullptr);
	m_automatedAttribModel.addItem(m_controlFloatText[4], nullptr);
	m_automatedAttribModel.addItem(m_controlFloatText[6], nullptr);
	m_automatedAttribModel.addItem(m_controlFloatText[8], nullptr);
	m_effectedAttribModel.addItem(m_controlFloatText[2], nullptr);
	m_effectedAttribModel.addItem(m_controlFloatText[4], nullptr);
	m_effectedAttribModel.addItem(m_controlFloatText[6], nullptr);
	m_effectedAttribModel.addItem(m_controlFloatText[8], nullptr);
	for (size_t i = 0; i < m_controlLineEffectText.size(); i++)
	{
		m_effectModelA.addItem(m_controlLineEffectText[i], nullptr);
		m_effectModelB.addItem(m_controlLineEffectText[i], nullptr);
		m_effectModelC.addItem(m_controlLineEffectText[i], nullptr);
	}



	QLabel* lineTypeLabel = new QLabel(tr("line type:"));
	lineTypeLabel->setFixedSize(100, 20);
	settingLayout->addWidget(lineTypeLabel);
	settingLayout->setAlignment(lineTypeLabel, Qt::AlignHCenter);
	ComboBox* lineTypeComboBox = new ComboBox(this);
	lineTypeComboBox->setModel(&m_lineTypeModel);
	lineTypeComboBox->setFixedSize(100, 20);
	lineTypeComboBox->show();
	settingLayout->addWidget(lineTypeComboBox);
	settingLayout->setAlignment(lineTypeComboBox, Qt::AlignHCenter);

	QLabel* automatedAttribLabel = new QLabel(tr("automated attribute:"));
	automatedAttribLabel->setFixedSize(130, 20);
	settingLayout->addWidget(automatedAttribLabel);
	settingLayout->setAlignment(automatedAttribLabel, Qt::AlignHCenter);
	ComboBox* automatedAttribComboBox = new ComboBox(this);
	automatedAttribComboBox->setModel(&m_automatedAttribModel);
	automatedAttribComboBox->setFixedSize(100, 20);
	automatedAttribComboBox->show();
	settingLayout->addWidget(automatedAttribComboBox);
	settingLayout->setAlignment(automatedAttribComboBox, Qt::AlignHCenter);

	QLabel* effectedAttribLabel = new QLabel(tr("effected attribute:"));
	effectedAttribLabel->setFixedSize(130, 20);
	settingLayout->addWidget(effectedAttribLabel);
	settingLayout->setAlignment(effectedAttribLabel, Qt::AlignHCenter);
	ComboBox* effectedAttribComboBox = new ComboBox(this);
	effectedAttribComboBox->setModel(&m_effectedAttribModel);
	effectedAttribComboBox->setFixedSize(100, 20);
	effectedAttribComboBox->show();
	settingLayout->addWidget(effectedAttribComboBox);
	settingLayout->setAlignment(effectedAttribComboBox, Qt::AlignHCenter);

	QLabel* effectedLabelA = new QLabel(tr("1. effect:"));
	effectedLabelA->setFixedSize(100, 20);
	settingLayout->addWidget(effectedLabelA);
	settingLayout->setAlignment(effectedLabelA, Qt::AlignHCenter);
	ComboBox* effectedComboBoxA = new ComboBox(this);
	effectedComboBoxA->setModel(&m_effectModelA);
	effectedComboBoxA->setFixedSize(100, 20);
	effectedComboBoxA->show();
	settingLayout->addWidget(effectedComboBoxA);
	settingLayout->setAlignment(effectedComboBoxA, Qt::AlignHCenter);

	QLabel* effectedLabelB = new QLabel(tr("2. effect:"));
	effectedLabelB->setFixedSize(100, 20);
	settingLayout->addWidget(effectedLabelB);
	settingLayout->setAlignment(effectedLabelB, Qt::AlignHCenter);
	ComboBox* effectedComboBoxB = new ComboBox(this);
	effectedComboBoxB->setModel(&m_effectModelB);
	effectedComboBoxB->setFixedSize(100, 20);
	effectedComboBoxB->show();
	settingLayout->addWidget(effectedComboBoxB);
	settingLayout->setAlignment(effectedComboBoxB, Qt::AlignHCenter);

	QLabel* effectedLabelC = new QLabel(tr("3. effect:"));
	effectedLabelC->setFixedSize(100, 20);
	settingLayout->addWidget(effectedLabelC);
	settingLayout->setAlignment(effectedLabelC, Qt::AlignHCenter);
	ComboBox* effectedComboBoxC = new ComboBox(this);
	effectedComboBoxC->setModel(&m_effectModelC);
	effectedComboBoxC->setFixedSize(100, 20);
	effectedComboBoxC->show();
	settingLayout->addWidget(effectedComboBoxC);
	settingLayout->setAlignment(effectedComboBoxC, Qt::AlignHCenter);


	m_automationLayout = new QVBoxLayout(nullptr);
	mainLayout->addLayout(m_automationLayout);

	QPushButton* effectPointButton = new QPushButton(tr("effect point"), this);
	m_automationLayout->addWidget(effectPointButton);
	m_automationLayout->setAlignment(effectPointButton, Qt::AlignHCenter);

	QPushButton* effectLineButton = new QPushButton(tr("effect line"), this);
	m_automationLayout->addWidget(effectLineButton);
	m_automationLayout->setAlignment(effectLineButton, Qt::AlignHCenter);

	QLabel* automationModelLabel = new QLabel(tr("only this\nkonb can be\nautomated:"));
	automationModelLabel->setFixedSize(100, 60);
	m_automationLayout->addWidget(automationModelLabel);
	m_automationLayout->setAlignment(automationModelLabel, Qt::AlignHCenter);

	show();

	connect(&m_lineTypeModel, &AutomatableModel::setValueEvent,
			this, &VectorGraphCotnrolDialog::controlValueChanged);
	connect(&m_automatedAttribModel, &AutomatableModel::setValueEvent,
			this, &VectorGraphCotnrolDialog::controlValueChanged);
	connect(&m_effectedAttribModel, &AutomatableModel::setValueEvent,
			this, &VectorGraphCotnrolDialog::controlValueChanged);
	connect(&m_effectModelA, &AutomatableModel::setValueEvent,
			this, &VectorGraphCotnrolDialog::controlValueChanged);
	connect(&m_effectModelB, &AutomatableModel::setValueEvent,
			this, &VectorGraphCotnrolDialog::controlValueChanged);
	connect(&m_effectModelC, &AutomatableModel::setValueEvent,
			this, &VectorGraphCotnrolDialog::controlValueChanged);

	/*
	connect(m_lineTypeModel, SIGNAL(setValueEvent()),
			this, SLOT(controlValueChanged()));
	connect(m_automatedAttribModel, SIGNAL(setValueEvent()),
			this, SLOT(controlValueChanged()));
	connect(m_effectedAttribModel, SIGNAL(setValueEvent()),
			this, SLOT(controlValueChanged()));
	connect(m_effectModelA, SIGNAL(setValueEvent()),
			this, SLOT(controlValueChanged()));
	connect(m_effectModelB, SIGNAL(setValueEvent()),
			this, SLOT(controlValueChanged()));
	connect(m_effectModelC, SIGNAL(setValueEvent()),
			this, SLOT(controlValueChanged()));
			*/

		/*
	connect(effectPointButton, SIGNAL(clicked()),
			this, &VectorGraphCotnrolDialog::controlValueChanged);
			*/
	QObject::connect(effectPointButton, SIGNAL(clicked(bool)),
			//this, &VectorGraphCotnrolDialog::effectedPointClicked);
			this, SLOT(effectedPointClicked(bool)));
	QObject::connect(effectLineButton, SIGNAL(clicked(bool)),
			this, SLOT(effectedLineClicked(bool)));
	/*
	QObject::connect(effectPointButton, SIGNAL(clicked()),
			this, SLOT(pointEffectedButton));
	QObject::connect(effectLineButton, SIGNAL(clicked()),
			this, SLOT(lineEffectedButton)); //&VectorGraphCotnrolDialog::lineEffectedButton);
			*/
}

VectorGraphCotnrolDialog::~VectorGraphCotnrolDialog()
{
	for (auto i : m_controlModelArray)
	{
		if (i != nullptr)
		{
			delete i;
		}
	}
}

void VectorGraphCotnrolDialog::hideAutomation()
{
	if (m_curAutomationModelKnob != nullptr)
	{
		// taking control of m_curAutomationModelKnob
		m_automationLayout->removeWidget(m_curAutomationModelKnob);
		delete m_curAutomationModelKnob;
		m_curAutomationModelKnob = nullptr;
	}
}
void VectorGraphCotnrolDialog::switchPoint(unsigned int selectedArray, unsigned int selectedLocation)
{
	m_curSelectedArray = selectedArray;
	m_curSelectedLocation = selectedLocation;
	m_vectorGraphView->model()->getDataArray(selectedArray)->setAutomated(selectedLocation, true);
	m_curAutomationModel = m_vectorGraphView->model()->getDataArray(selectedArray)->getAutomationModel(selectedLocation);

	if (m_curAutomationModel == nullptr) { hideAutomation(); return; }

	if (m_curAutomationModelKnob != nullptr)
	{
		if (m_curAutomationModel != m_curAutomationModelKnob->model())
		{
			m_curAutomationModelKnob->setModel(m_curAutomationModel);
		}
	}
	else
	{
		m_curAutomationModelKnob = new Knob(KnobType::Bright26, this);
		m_curAutomationModelKnob->setModel(m_curAutomationModel);
		m_curAutomationModelKnob->setLabel(tr("automation knob"));
		m_curAutomationModelKnob->setHintText(tr("automate this to automate a value of the vector graph"), "%");
		m_curAutomationModelKnob->setVolumeKnob(false);

		m_automationLayout->addWidget(m_curAutomationModelKnob);
		m_automationLayout->setAlignment(m_curAutomationModelKnob, Qt::AlignHCenter);
	}
	updateControls();
}

void VectorGraphCotnrolDialog::controlValueChanged()
{
	qDebug("val changed B");
	updateVectorGraphAttribs();
}

void VectorGraphCotnrolDialog::effectedPointClicked(bool isChecked)
{
	qDebug("clikced A");
}

void VectorGraphCotnrolDialog::effectedLineClicked(bool isChecked)
{
	qDebug("clikced B");
}

void VectorGraphCotnrolDialog::updateControls()
{
	if (m_curAutomationModel == nullptr) { return; }

	bool unusedBool = false;
	for (size_t i = 0; i < m_controlModelArray.size(); i++)
	{
		m_controlModelArray[i]->setAutomatedValue(m_vectorGraphView->getInputAttribValue(i, &unusedBool));
	}

	m_lineTypeModel.setAutomatedValue(m_vectorGraphView->getInputAttribValue(5, &unusedBool));
	m_automatedAttribModel.setAutomatedValue(m_vectorGraphView->getInputAttribValue(6, &unusedBool));
	m_effectedAttribModel.setAutomatedValue(m_vectorGraphView->getInputAttribValue(7, &unusedBool));
	//m_effectModelA;
	//m_effectModelB;
	//m_effectModelC;
}

void VectorGraphCotnrolDialog::updateVectorGraphAttribs()
{
	if (m_curAutomationModel == nullptr) { return; }

	qDebug("set input attrib value");
	for (size_t i = 0; i < m_controlModelArray.size(); i++)
	{
		m_vectorGraphView->setInputAttribValue(static_cast<unsigned int>(i), m_controlModelArray[i]->value(), false);
		//m_controlModelArray[i]->setAutomatedValue(m_vectorGraphView->getInputAttribValue(i, &unusedBool));
	}

}


} // namespace gui
} // namespace lmms
