/*
 * VectorGraphViewBase.cpp - contains implementations of lmms widget classes for VectorGraph
 *
 * Copyright (c) 2024 - 2025 szeli1 </at/gmail/dot/com> TODO
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
#include <QLayout>
#include <QLabel>
#include <QPushButton>

#include "AutomatableModel.h"
#include "ComboBox.h"
#include "embed.h" // context menu
#include "GuiApplication.h" // getGUI
#include "Knob.h"
#include "MainWindow.h"
#include "SimpleTextFloat.h"
#include "StringPairDrag.h"
#include "VectorGraphModel.h"
#include "VectorGraphView.h"


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
		new gui::StringPairDrag("automatable_model", QString::number(automationModel->id()), QPixmap(), thisWidget);
		me->accept();
	}
}

std::pair<float, float> VectorGraphViewBase::showCoordInputDialog(std::pair<float, float> pointPosition)
{
	// show position input dialog
	bool ok;
	double changedX = QInputDialog::getDouble(this, tr("Set value"),
		tr("X pos: Please enter a new value between 0 and 100"),
		static_cast<double>(pointPosition.first * 100.0f),
		0.0, 100.0, 2, &ok);
	if (ok == true)
	{
		pointPosition.first = static_cast<float>(changedX) / 100.0f;
	}

	double changedY = QInputDialog::getDouble(this, tr("Set value"),
		tr("Y pos: Please enter a new value between -100 and 100"),
		static_cast<double>(pointPosition.second * 100.0f),
		-100.0, 100.0, 2, &ok);
	if (ok == true)
	{
		pointPosition.second = static_cast<float>(changedY) / 100.0f;
	}
	return pointPosition;
}

VectorGraphControlDialog::VectorGraphControlDialog(QWidget* parent, VectorGraphView* targetVectorGraphView) :
	QWidget(parent),
	ModelView(nullptr, this),
	m_vectorGraphView(targetVectorGraphView),
	m_curAutomationModel(nullptr),
	m_curAutomationModelKnob(nullptr),
	m_automationLayout(nullptr),
	m_isValidSelection(false),
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

	//m_controlModelArray
	//m_hideableKnobs
	//m_hideableComboBoxes

	setWindowTitle(tr("vector graph settings"));

	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	setFixedSize(300, 500);

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
		m_hideableKnobs.push_back(newKnob);

		connect(curModel, &AutomatableModel::setValueEvent,
				this, &VectorGraphControlDialog::controlValueChanged);
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
	lineTypeComboBox->setFixedSize(100, ComboBox::DEFAULT_HEIGHT);
	lineTypeComboBox->show();
	settingLayout->addWidget(lineTypeComboBox);
	settingLayout->setAlignment(lineTypeComboBox, Qt::AlignHCenter);
	m_hideableComboBoxes.push_back(lineTypeComboBox);

	QLabel* automatedAttribLabel = new QLabel(tr("automated attribute:"));
	automatedAttribLabel->setFixedSize(130, 20);
	settingLayout->addWidget(automatedAttribLabel);
	settingLayout->setAlignment(automatedAttribLabel, Qt::AlignHCenter);
	ComboBox* automatedAttribComboBox = new ComboBox(this);
	automatedAttribComboBox->setModel(&m_automatedAttribModel);
	automatedAttribComboBox->setFixedSize(100, ComboBox::DEFAULT_HEIGHT);
	automatedAttribComboBox->show();
	settingLayout->addWidget(automatedAttribComboBox);
	settingLayout->setAlignment(automatedAttribComboBox, Qt::AlignHCenter);
	m_hideableComboBoxes.push_back(automatedAttribComboBox);

	QLabel* effectedAttribLabel = new QLabel(tr("effected attribute:"));
	effectedAttribLabel->setFixedSize(130, 20);
	settingLayout->addWidget(effectedAttribLabel);
	settingLayout->setAlignment(effectedAttribLabel, Qt::AlignHCenter);
	ComboBox* effectedAttribComboBox = new ComboBox(this);
	effectedAttribComboBox->setModel(&m_effectedAttribModel);
	effectedAttribComboBox->setFixedSize(100, ComboBox::DEFAULT_HEIGHT);
	effectedAttribComboBox->show();
	settingLayout->addWidget(effectedAttribComboBox);
	settingLayout->setAlignment(effectedAttribComboBox, Qt::AlignHCenter);
	m_hideableComboBoxes.push_back(effectedAttribComboBox);

	QLabel* effectedLabelA = new QLabel(tr("1. effect:"));
	effectedLabelA->setFixedSize(100, 20);
	settingLayout->addWidget(effectedLabelA);
	settingLayout->setAlignment(effectedLabelA, Qt::AlignHCenter);
	ComboBox* effectedComboBoxA = new ComboBox(this);
	effectedComboBoxA->setModel(&m_effectModelA);
	effectedComboBoxA->setFixedSize(100, ComboBox::DEFAULT_HEIGHT);
	effectedComboBoxA->show();
	settingLayout->addWidget(effectedComboBoxA);
	settingLayout->setAlignment(effectedComboBoxA, Qt::AlignHCenter);
	m_hideableComboBoxes.push_back(effectedComboBoxA);

	QLabel* effectedLabelB = new QLabel(tr("2. effect:"));
	effectedLabelB->setFixedSize(100, 20);
	settingLayout->addWidget(effectedLabelB);
	settingLayout->setAlignment(effectedLabelB, Qt::AlignHCenter);
	ComboBox* effectedComboBoxB = new ComboBox(this);
	effectedComboBoxB->setModel(&m_effectModelB);
	effectedComboBoxB->setFixedSize(100, ComboBox::DEFAULT_HEIGHT);
	effectedComboBoxB->show();
	settingLayout->addWidget(effectedComboBoxB);
	settingLayout->setAlignment(effectedComboBoxB, Qt::AlignHCenter);
	m_hideableComboBoxes.push_back(effectedComboBoxB);

	QLabel* effectedLabelC = new QLabel(tr("3. effect:"));
	effectedLabelC->setFixedSize(100, 20);
	settingLayout->addWidget(effectedLabelC);
	settingLayout->setAlignment(effectedLabelC, Qt::AlignHCenter);
	ComboBox* effectedComboBoxC = new ComboBox(this);
	effectedComboBoxC->setModel(&m_effectModelC);
	effectedComboBoxC->setFixedSize(100, ComboBox::DEFAULT_HEIGHT);
	effectedComboBoxC->show();
	settingLayout->addWidget(effectedComboBoxC);
	settingLayout->setAlignment(effectedComboBoxC, Qt::AlignHCenter);
	m_hideableComboBoxes.push_back(effectedComboBoxC);


	m_automationLayout = new QVBoxLayout(nullptr);
	mainLayout->addLayout(m_automationLayout);

	QPushButton* helpButton = new QPushButton(tr("?"), this);
	m_automationLayout->addWidget(helpButton);
	m_automationLayout->setAlignment(helpButton, Qt::AlignHCenter);
	connect(helpButton, SIGNAL(clicked()), this, SLOT(showHelpWindowClicked()));

	QPushButton* effectPointButton = new QPushButton(tr("effect point"), this);
	m_automationLayout->addWidget(effectPointButton);
	m_automationLayout->setAlignment(effectPointButton, Qt::AlignHCenter);

	QPushButton* effectLineButton = new QPushButton(tr("effect line"), this);
	m_automationLayout->addWidget(effectLineButton);
	m_automationLayout->setAlignment(effectLineButton, Qt::AlignHCenter);

	QPushButton* deleteAutomationButton = new QPushButton(tr("delete\nautomation"), this);
	m_automationLayout->addWidget(deleteAutomationButton);
	m_automationLayout->setAlignment(deleteAutomationButton, Qt::AlignHCenter);

	QLabel* automationModelLabel = new QLabel(tr("only this\nkonb can be\nautomated:"));
	automationModelLabel->setFixedSize(100, 60);
	m_automationLayout->addWidget(automationModelLabel);
	m_automationLayout->setAlignment(automationModelLabel, Qt::AlignHCenter);

	show();

	connect(&m_lineTypeModel, &AutomatableModel::setValueEvent,
			this, &VectorGraphControlDialog::controlValueChanged);
	connect(&m_automatedAttribModel, &AutomatableModel::setValueEvent,
			this, &VectorGraphControlDialog::controlValueChanged);
	connect(&m_effectedAttribModel, &AutomatableModel::setValueEvent,
			this, &VectorGraphControlDialog::controlValueChanged);
	connect(&m_effectModelA, &AutomatableModel::setValueEvent,
			this, &VectorGraphControlDialog::controlValueChanged);
	connect(&m_effectModelB, &AutomatableModel::setValueEvent,
			this, &VectorGraphControlDialog::controlValueChanged);
	connect(&m_effectModelC, &AutomatableModel::setValueEvent,
			this, &VectorGraphControlDialog::controlValueChanged);

	QObject::connect(effectPointButton, SIGNAL(clicked(bool)),
			this, SLOT(effectedPointClicked(bool)));
	QObject::connect(effectLineButton, SIGNAL(clicked(bool)),
			this, SLOT(effectedLineClicked(bool)));
	QObject::connect(deleteAutomationButton, SIGNAL(clicked(bool)),
			this, SLOT(deleteAutomationClicked(bool)));
}

VectorGraphControlDialog::~VectorGraphControlDialog()
{
	hideAutomation();
	for (auto i : m_controlModelArray)
	{
		if (i != nullptr)
		{
			delete i;
		}
	}
}

void VectorGraphControlDialog::hideAutomation()
{
	m_isValidSelection = false;
	m_curAutomationModel = nullptr;
	if (m_curAutomationModelKnob != nullptr)
	{
		// taking control of m_curAutomationModelKnob
		m_automationLayout->removeWidget(m_curAutomationModelKnob);
		delete m_curAutomationModelKnob;
		m_curAutomationModelKnob = nullptr;
	}
}

void VectorGraphControlDialog::switchPoint(size_t selectedArray, size_t selectedLocation)
{
	// set current point location
	m_curSelectedArray = selectedArray;
	m_curSelectedLocation = selectedLocation;
	// enable all of the control dialog settings by turning this on
	m_isValidSelection = true;
	// get a FloatModel for the selected point
	m_vectorGraphView->model()->getDataArray(selectedArray)->setAutomated(selectedLocation, true);
	m_curAutomationModel = m_vectorGraphView->model()->getDataArray(selectedArray)->getAutomationModel(selectedLocation);

	if (m_curAutomationModel != nullptr)
	{
		if (m_curAutomationModelKnob != nullptr)
		{
			// if m_curAutomationKnob exists
			// update its model to the selected point's automationModel
			if (m_curAutomationModel != m_curAutomationModelKnob->model())
			{
				m_curAutomationModelKnob->setModel(m_curAutomationModel);
			}
		}
		else
		{
			// if m_curAutomationKnob doesn't exist
			// create new
			m_curAutomationModelKnob = new Knob(KnobType::Bright26, this);
			m_curAutomationModelKnob->setModel(m_curAutomationModel);
			m_curAutomationModelKnob->setLabel(tr("automation knob"));
			m_curAutomationModelKnob->setHintText(tr("automate this to automate a value of the vector graph"), "%");
			m_curAutomationModelKnob->setVolumeKnob(false);

			m_automationLayout->addWidget(m_curAutomationModelKnob);
			m_automationLayout->setAlignment(m_curAutomationModelKnob, Qt::AlignHCenter);
		}
	}
	else
	{
		// if can not be automated
		hideAutomation();
		// turn back on this
		// so ohter functions can still work
		m_isValidSelection = true;
	}
	updateControls();
}

void VectorGraphControlDialog::controlValueChanged()
{
	updateVectorGraphAttribs();
}

void VectorGraphControlDialog::effectedPointClicked(bool isChecked)
{
	float currentValue = m_vectorGraphView->getInputAttribValue(11);
	m_vectorGraphView->setInputAttribValue(11, currentValue >= 0.5f ? 0.0f : 1.0f);
}

void VectorGraphControlDialog::effectedLineClicked(bool isChecked)
{
	float currentValue = m_vectorGraphView->getInputAttribValue(12);
	m_vectorGraphView->setInputAttribValue(12, currentValue >= 0.5f ? 0.0f : 1.0f);
}

void VectorGraphControlDialog::deleteAutomationClicked(bool isChecked)
{
	if (m_isValidSelection == false) { hideAutomation(); return; }
	
	bool swapIsValidSelection = m_isValidSelection;
	hideAutomation();
	// remove automationModel (FloatModel) from the selected point
	m_vectorGraphView->model()->getDataArray(m_curSelectedArray)->setAutomated(m_curSelectedLocation, false);
	m_isValidSelection = swapIsValidSelection;
}

void VectorGraphControlDialog::showHelpWindowClicked()
{
	VectorGraphHelpView::getInstance()->close();
	VectorGraphHelpView::getInstance()->show();
}

void VectorGraphControlDialog::closeEvent(QCloseEvent * ce)
{
	// we need to ignore this event
	// because Qt::WA_DeleteOnClose was activated by MainWindow::addWindowedWidget
	// or else this widget will be deleted
	ce->ignore();

	parentWidget()->hide();
}

void VectorGraphControlDialog::updateControls()
{
	if (m_isValidSelection == false) { hideAutomation(); return; }

	// show every control
	for (Knob* i : m_hideableKnobs)
	{
		if (i != nullptr)
		{
			i->show();
		}
	}
	for (ComboBox* i : m_hideableComboBoxes)
	{
		if (i != nullptr)
		{
			i->show();
		}
	}

	// hiding different parts of the gui
	// to not confuse the user
	if (m_vectorGraphView->model()->getDataArray(m_curSelectedArray)->getIsFixedX() == true && m_hideableKnobs[0] != nullptr)
	{
		m_hideableKnobs[0]->hide();
	}
	if (m_vectorGraphView->model()->getDataArray(m_curSelectedArray)->getIsFixedY() == true && m_hideableKnobs[1] != nullptr)
	{
		m_hideableKnobs[1]->hide();
	}
	if (m_vectorGraphView->model()->getDataArray(m_curSelectedArray)->getIsEditableAttrib() == false)
	{
		if (m_hideableKnobs[1] != nullptr) { m_hideableKnobs[1]->hide(); }
		if (m_hideableKnobs[2] != nullptr) { m_hideableKnobs[2]->hide(); }
		if (m_hideableKnobs[3] != nullptr) { m_hideableKnobs[3]->hide(); }
		if (m_hideableKnobs[4] != nullptr) { m_hideableKnobs[4]->hide(); }
		if (m_hideableComboBoxes[0] != nullptr) { m_hideableComboBoxes[0]->hide(); }
		if (m_hideableComboBoxes[1] != nullptr) { m_hideableComboBoxes[1]->hide(); }
		if (m_hideableComboBoxes[2] != nullptr) { m_hideableComboBoxes[2]->hide(); }
	}
	if (m_vectorGraphView->model()->getDataArray(m_curSelectedArray)->getIsAutomatable() == false)
	{
		if (m_hideableComboBoxes[1] != nullptr) { m_hideableComboBoxes[1]->hide(); }
	}
	if (m_vectorGraphView->model()->getDataArray(m_curSelectedArray)->getIsEffectable() == false)
	{
		if (m_hideableComboBoxes[2] != nullptr) { m_hideableComboBoxes[2]->hide(); }
		if (m_hideableComboBoxes[3] != nullptr) { m_hideableComboBoxes[3]->hide(); }
		if (m_hideableComboBoxes[4] != nullptr) { m_hideableComboBoxes[4]->hide(); }
		if (m_hideableComboBoxes[5] != nullptr) { m_hideableComboBoxes[5]->hide(); }
	}

	// load selected point's values into knobs
	for (size_t i = 0; i < m_controlModelArray.size(); i++)
	{
		m_controlModelArray[i]->setAutomatedValue(m_vectorGraphView->getInputAttribValue(i));
	}

	m_lineTypeModel.setAutomatedValue(m_vectorGraphView->getInputAttribValue(5));
	m_automatedAttribModel.setAutomatedValue(m_vectorGraphView->getInputAttribValue(6));
	m_effectedAttribModel.setAutomatedValue(m_vectorGraphView->getInputAttribValue(7));
	m_effectModelA.setAutomatedValue(m_vectorGraphView->getInputAttribValue(8));
	m_effectModelB.setAutomatedValue(m_vectorGraphView->getInputAttribValue(9));
	m_effectModelC.setAutomatedValue(m_vectorGraphView->getInputAttribValue(10));
}

void VectorGraphControlDialog::updateVectorGraphAttribs()
{
	if (m_isValidSelection == false) { hideAutomation(); return; }

	// set / load knob's values into the selected point's values
	for (size_t i = 0; i < m_controlModelArray.size(); i++)
	{
		m_vectorGraphView->setInputAttribValue(static_cast<size_t>(i), m_controlModelArray[i]->value());
	}

	m_vectorGraphView->setInputAttribValue(5, m_lineTypeModel.value());
	m_vectorGraphView->setInputAttribValue(6, m_automatedAttribModel.value());
	m_vectorGraphView->setInputAttribValue(7, m_effectedAttribModel.value());
	m_vectorGraphView->setInputAttribValue(8, m_effectModelA.value());
	m_vectorGraphView->setInputAttribValue(9, m_effectModelB.value());
	m_vectorGraphView->setInputAttribValue(10, m_effectModelC.value());
}


QString VectorGraphHelpView::s_helpText =
"<div style='text-align: center;'>"
"<b> Vector graph </b><br>"
"</div>"
"<h3>Basics:</h3>"
"<b>x</b> - represents the x coordinate of the point<br>"
"<b>y</b> - represents the y coordinate of the point<br>"
"<b>curve</b> - sets the curve amount between the selected point and the point after that, functionality depends on line type<br>"
"<b>1. attribute</b> - adjusts an additional parameter of the line, functionality depends on line type<br>"
"<b>2. attribute</b> - adjusts an additional parameter of the line, functionality depends on line type<br>"
"<b>note:</b> all variables are between -1 and 1, except 0 &lt;= x &lt;= 1 and in some cases 0 &lt;= y &lt;= 1<br>"


"<h3>Line type:</h3>"
"<h5>none:</h5>"
"<ul>"
"<li><b>description</b> - draws a simple line</li>"
"<li><b>curve</b> - changes the curve (default)</li>"
"<li><b>1. attribute</b> - none</li>"
"<li><b>1. attribute</b> - none</li>"
"</ul>"
"<h5>bézier:</h5>"
"<ul>"
"<li><b>description</b> - draws a bézier curve</li>"
"<li><b>curve</b> - changes the curve strength</li>"
"<li><b>1. attribute</b> - curve direction for the selected point</li>"
"<li><b>1. attribute</b> - curve direction for the point after the selected point</li>"
"</ul>"
"<h5>sine:</h5>"
"<ul>"
"<li><b>description</b> - draws a sine curve</li>"
"<li><b>curve</b> - default</li>"
"<li><b>1. attribute (a)</b> - the amplitude of the sine (a * sine, -1 &lt;= a &lt;= 1)</li>"
"<li><b>1. attribute (b)</b> - the freq of the sine (sine(x * 200 * pi * b), 0 &lt;= x &lt;= 1, -1 &lt;= b &lt;= 1)</li>"
"</ul>"
"<h5>phase changeable sine:</h5>"
"<ul>"
"<li><b>description</b> - draws a sine curve, but the phase is changeable</li>"
"<li><b>curve (c)</b> - the phase of the sine (sine(x + c * 100))</li>"
"<li><b>1. attribute (a)</b> - the amplitude of the sine (a * sine, -1 &lt;= a &lt;= 1)</li>"
"<li><b>1. attribute (b)</b> - the freq of the sine (sine(x * 200 * pi * b), 0 &lt;= x &lt;= 1, -1 &lt;= b &lt;= 1)</li>"
"</ul>"
"<h5>peak:</h5>"
"<ul>"
"<li><b>description</b> - draws a shape similar to a peak filter</li>"
"<li><b>curve</b> - peak width</li>"
"<li><b>1. attribute</b> - peak amplitude</li>"
"<li><b>1. attribute</b> - peak x location</li>"
"</ul>"
"<h5>steps:</h5>"
"<ul>"
"<li><b>description</b> - draws a line but the y values will be rounded down to some common y values</li>"
"<li><b>curve</b> - default</li>"
"<li><b>1. attribute (a)</b> - step count (count = (1 + a) / 2 * 19 + 1, -1 &lt;= a &lt;= 1)</li>"
"<li><b>1. attribute</b> - smoothens the edges</li>"
"</ul>"
"<h5>random:</h5>"
"<ul>"
"<li><b>description</b> - draws a line with random values added to it</li>"
"<li><b>curve</b> - random seed, this setting will slide the random values between 20 seeds gradually</li>"
"<li><b>1. attribute</b> - random value amplitude</li>"
"<li><b>1. attribute (b)</b> - random value count (count = 200 * b, -1 &lt;= b &lt;= 1)</li>"
"</ul>"

"<h3>automation:</h3>"
"<b>automated attribute</b> - decides what point attribute can be automated, only 1 can be automated<br>"
"<b>effected attribute</b> - decides what point attribute can be effected, only 1 can be automated<br>"
"<h3>effects:</h3>"
"if more than 1 wave shape is available for the widget, effects decide how the wave shapes interact. For example one waveshape's y values can be added to an other one's y values, meaning more complex shapes can be made. These settings may not be available."
"<h5>effects:</h5>"
"<b>note:</b> a = selected attribute, b = effector grap's y value, -1 &lt;= a &lt;= 1, -1 &lt;= b &lt;= 1<br>"
"<ul>"
"<li><b>none</b> - skips this step</li>"
"<li><b>add</b> - a + b</li>"
"<li><b>subtract</b> - a - b</li>"
"<li><b>multiply</b> - a * 5 * b</li>"
"<li><b>divide</b> - a / 5 / b</li>"
"<li><b>power</b> - a ^ (b * 5)</li>"
"<li><b>log</b> - log(a) / log(b)</li>"
"<li><b>sine</b> - a + sin(b * 100)</li>"
"<li><b>clamp lower</b> - b &lt;= a &lt;= 1</li>"
"<li><b>clamp upper</b> - -1 &lt;= a &lt;= b</li>"
"</ul>"

"<b>effect point</b> - can the point's attributes be effected (for example if the y attribute is effected, should the point move up and down)<br>"
"<b>effect line</b> - can the line between the selected point and the point after that be effected (only avaiable if y attribute is effected, for example should the effector graph be added to the line's values)<br>"
;

VectorGraphHelpView::VectorGraphHelpView():QTextEdit(s_helpText)
{
#if (QT_VERSION < QT_VERSION_CHECK(5,12,0))
	// Bug workaround: https://codereview.qt-project.org/c/qt/qtbase/+/225348
	using ::operator|;
#endif
	setWindowTitle(tr("VectorGraph Help"));
	setTextInteractionFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);
	getGUI()->mainWindow()->addWindowedWidget(this);
	parentWidget()->setAttribute(Qt::WA_DeleteOnClose, false);
	//parentWidget()->setWindowIcon(PLUGIN_NAME::getIconPixmap("logo"));
	
	// No maximize button
	Qt::WindowFlags flags = parentWidget()->windowFlags();
	flags &= ~Qt::WindowMaximizeButtonHint;
	parentWidget()->setWindowFlags(flags);
}


} // namespace gui
} // namespace lmms
