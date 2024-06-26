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
		FloatModel* curModel = new FloatModel(0.0f, -1.0f, 1.0f, 0.01f, nullptr, QString(), false);
		m_controlModelArray.push_back(curModel);
		Knob* newKnob = makeKnob(m_controlFloatText[i + 1], m_controlFloatText[i], "%", curModel);
		knobLayout->addWidget(newKnob);
		knobLayout->setAlignment(newKnob, Qt::AlignHCenter);
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


	/*
	// Midi stuff
	m_midiGroupBox = new GroupBox( tr( "MIDI CONTROLLER" ), this );
	m_midiGroupBox->setGeometry( 8, 10, 240, 80 );
	connect( m_midiGroupBox->model(), SIGNAL(dataChanged()),
			this, SLOT(midiToggled()));
	
	m_midiChannelSpinBox = new LcdSpinBox( 2, m_midiGroupBox,
			tr( "Input channel" ) );
	m_midiChannelSpinBox->addTextForValue( 0, "--" );
	m_midiChannelSpinBox->setLabel( tr( "CHANNEL" ) );
	m_midiChannelSpinBox->move( 8, 24 );

	m_midiControllerSpinBox = new LcdSpinBox( 3, m_midiGroupBox,
			tr( "Input controller" ) );
	m_midiControllerSpinBox->addTextForValue(MidiController::NONE, "---" );
	m_midiControllerSpinBox->setLabel( tr( "CONTROLLER" ) );
	m_midiControllerSpinBox->move( 68, 24 );
	

	m_midiAutoDetectCheckBox =
			new LedCheckBox( tr("Auto Detect"),
				m_midiGroupBox, tr("Auto Detect") );
	m_midiAutoDetectCheckBox->setModel( &m_midiAutoDetect );
	m_midiAutoDetectCheckBox->move( 8, 60 );
	connect( &m_midiAutoDetect, SIGNAL(dataChanged()),
			this, SLOT(autoDetectToggled()));

	// when using with non-raw-clients we can provide buttons showing
	// our port-menus when being clicked
	if( !Engine::audioEngine()->midiClient()->isRaw() )
	{
		m_readablePorts = new MidiPortMenu( MidiPort::Mode::Input );
		connect( m_readablePorts, SIGNAL(triggered(QAction*)),
				this, SLOT(enableAutoDetect(QAction*)));
		auto rp_btn = new ToolButton(m_midiGroupBox);
		rp_btn->setText( tr( "MIDI-devices to receive "
						"MIDI-events from" ) );
		rp_btn->setIcon( embed::getIconPixmap( "piano" ) );
		rp_btn->setGeometry( 160, 24, 32, 32 );
		rp_btn->setMenu( m_readablePorts );
		rp_btn->setPopupMode( QToolButton::InstantPopup );
	}


	// User stuff
	m_userGroupBox = new GroupBox( tr( "USER CONTROLLER" ), this );
	m_userGroupBox->setGeometry( 8, 100, 240, 60 );
	connect( m_userGroupBox->model(), SIGNAL(dataChanged()),
			this, SLOT(userToggled()));

	m_userController = new ComboBox( m_userGroupBox, "Controller" );
	m_userController->setGeometry( 10, 24, 200, ComboBox::DEFAULT_HEIGHT );
	for (Controller * c : Engine::getSong()->controllers())
	{
		m_userController->model()->addItem( c->name() );
	}
	connect( m_userController->model(), SIGNAL(dataUnchanged()),
			this, SLOT(userSelected()));
	connect( m_userController->model(), SIGNAL(dataChanged()),
			this, SLOT(userSelected()));


	// Mapping functions
	m_mappingBox = new TabWidget( tr( "MAPPING FUNCTION" ), this );
	m_mappingBox->setGeometry( 8, 170, 240, 64 );
	m_mappingFunction = new QLineEdit( m_mappingBox );
	m_mappingFunction->setGeometry( 10, 20, 170, 16 );
	m_mappingFunction->setText( "input" );
	m_mappingFunction->setReadOnly( true );


	// Buttons
	auto buttons = new QWidget(this);
	buttons->setGeometry( 8, 240, 240, 32 );

	auto btn_layout = new QHBoxLayout(buttons);
	btn_layout->setSpacing( 0 );
	btn_layout->setContentsMargins(0, 0, 0, 0);

	auto select_btn = new QPushButton(embed::getIconPixmap("add"), tr("OK"), buttons);
	connect( select_btn, SIGNAL(clicked()), 
				this, SLOT(selectController()));

	auto cancel_btn = new QPushButton(embed::getIconPixmap("cancel"), tr("Cancel"), buttons);
	connect( cancel_btn, SIGNAL(clicked()),
				this, SLOT(reject()));

	btn_layout->addStretch();
	btn_layout->addSpacing( 10 );
	btn_layout->addWidget( select_btn );
	btn_layout->addSpacing( 10 );
	btn_layout->addWidget( cancel_btn );
	btn_layout->addSpacing( 10 );

	setFixedSize( 256, 280 );

	// Crazy MIDI View stuff
	
	// TODO, handle by making this a model for the Dialog "view"
	ControllerConnection * cc = nullptr;
	if( m_targetModel )
	{
		cc = m_targetModel->controllerConnection();

		if( cc && cc->getController()->type() != Controller::ControllerType::Dummy && Engine::getSong() )
		{
			if ( cc->getController()->type() == Controller::ControllerType::Midi )
			{
				m_midiGroupBox->model()->setValue( true );
				// ensure controller is created
				midiToggled();

				auto cont = (MidiController*)(cc->getController());
				m_midiChannelSpinBox->model()->setValue( cont->m_midiPort.inputChannel() );
				m_midiControllerSpinBox->model()->setValue( cont->m_midiPort.inputController() );

				m_midiController->subscribeReadablePorts( static_cast<MidiController*>( cc->getController() )->m_midiPort.readablePorts() );
			}
			else
			{
				auto& controllers = Engine::getSong()->controllers();
				auto it = std::find(controllers.begin(), controllers.end(), cc->getController());

				if (it != controllers.end())
				{
					int idx = std::distance(controllers.begin(), it);
					m_userGroupBox->model()->setValue( true );
					m_userController->model()->setValue( idx );
				}
			}
		}
	}

	if( !cc )
	{
		m_midiGroupBox->model()->setValue( true );
	}
*/
	show();
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
		m_automationLayout->removeWidget(m_curAutomationModelKnob);
		delete m_curAutomationModelKnob;
		m_curAutomationModelKnob = nullptr;
	}
}
void VectorGraphCotnrolDialog::switchPoint(unsigned int selectedArray, unsigned int selectedLocation)
{
	qDebug("switch point 0: %d, %d", selectedArray, selectedLocation);
	m_vectorGraphView->model()->getDataArray(selectedArray)->setAutomated(selectedLocation, true);
	qDebug("switch point 0.5");
	m_curAutomationModel = m_vectorGraphView->model()->getDataArray(selectedArray)->getAutomationModel(selectedLocation);
	qDebug("switch point 1");

	if (m_curAutomationModel == nullptr) { hideAutomation(); return; }

	qDebug("switch point 2");
	if (m_curAutomationModelKnob != nullptr)
	{
		if (m_curAutomationModel != m_curAutomationModelKnob->model())
		{
			qDebug("switch point 3");
			m_curAutomationModelKnob->setModel(m_curAutomationModel);
		}
	}
	else
	{
		qDebug("switch point 4");
		m_curAutomationModelKnob = new Knob(KnobType::Bright26, this);
		m_curAutomationModelKnob->setModel(m_curAutomationModel);
		m_curAutomationModelKnob->setLabel(tr("automation knob"));
		m_curAutomationModelKnob->setHintText(tr("automate this to automate a value of the vector graph"), "%");
		m_curAutomationModelKnob->setVolumeKnob(false);

		m_automationLayout->addWidget(m_curAutomationModelKnob);
		m_automationLayout->setAlignment(m_curAutomationModelKnob, Qt::AlignHCenter);
		//m_curAutomationModelKnob->show();
	}
}


} // namespace gui
} // namespace lmms
