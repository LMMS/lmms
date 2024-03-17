/*
 * ClapViewBase.cpp - Base class for CLAP plugin views
 *
 * Copyright (c) 2024 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#include "ClapViewBase.h"

#ifdef LMMS_HAVE_CLAP

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QToolBar>

#include "AudioEngine.h"
#include "ClapControlBase.h"
#include "ClapInstance.h"
#include "ClapManager.h"
#include "Controls.h"
#include "CustomTextKnob.h"
#include "embed.h"
#include "Engine.h"
#include "GuiApplication.h"
#include "gui_templates.h"
#include "MainWindow.h"
#include "PixmapButton.h"
#include "PresetSelector.h"
#include "SubWindow.h"

namespace lmms::gui
{

ClapViewParameters::ClapViewParameters(QWidget* parent, ClapInstance* instance, int colNum)
	: LinkedModelGroupView{parent, &instance->params(), static_cast<std::size_t>(colNum)}
	, m_instance{instance}
{
	for (auto param : m_instance->params().parameters())
	{
		if (!param || !param->model()) { continue; }

		Control* control = nullptr;

		switch (param->valueType())
		{
		case ClapParameter::ValueType::Bool:
			control = new CheckControl{this};
			break;
		case ClapParameter::ValueType::Integer:
			// TODO: What if more digits are needed? Lv2 uses KnobControl in this case.
			control = new LcdControl{(param->info().max_value <= 9.0) ? 1 : 2, this};
			break;
		// TODO: Add support for enum controls
		case ClapParameter::ValueType::Float:
		{
			control = new CustomTextKnobControl{this};

			// CustomTextKnob calls this lambda to update value text
			auto customTextKnob = dynamic_cast<CustomTextKnob*>(control->modelView());
			customTextKnob->setValueText([=] {
				return QString::fromUtf8(m_instance->params().getValueText(*param).c_str());
			});

			break;
		}
		default:
			throw std::runtime_error{"Invalid CLAP param value type"};
		}

		if (!control) { continue; }

		control->setModel(param->model()); // TODO?

		// This is the param name seen in the GUI
		control->setText(QString::fromUtf8(param->displayName().data()));

		// TODO: Group parameters according to module path?
		if (param->info().module[0] != '\0')
		{
			control->topWidget()->setToolTip(QString::fromUtf8(param->info().module));
		}

		addControl(control, param->id().data(), param->displayName().data(), false);
	}
}

ClapViewPresets::ClapViewPresets(QWidget* parent, ClapInstance* instance, int colNum)
	: LinkedModelGroupView{parent, &instance->presetLoader(), static_cast<std::size_t>(colNum)}
	, m_instance{instance}
{
	class PresetSelectorControl : public Control
	{
		QWidget* m_widget;
		PresetSelector* m_checkBox;
		QLabel* m_label;

	public:
		void setText(const QString& text) override { m_label->setText(text); }
		QWidget* topWidget() override { return m_widget; }

		void setModel(AutomatableModel* model) override
		{
			m_checkBox->setModel(model->dynamicCast<IntModel>(true));
		}
		IntModel* model() override { return m_checkBox->model(); }
		AutomatableModelView* modelView() override { return m_checkBox; }

		PresetSelectorControl(PluginPresets* presets, QWidget* parent = nullptr)
			: m_widget{new QWidget{parent}}
			, m_checkBox{new PresetSelector{presets, parent}}
			, m_label{new QLabel{m_widget}}
		{
			auto vbox = new QVBoxLayout(m_widget);
			vbox->addWidget(m_checkBox);
			vbox->addWidget(m_label);
		}

		~PresetSelectorControl() override = default;
	};

	auto control = new PresetSelectorControl{&instance->presetLoader(), parent};

	addControl(control, "preset", tr("Active preset").toStdString(), false);
}

ClapViewBase::ClapViewBase(QWidget* pluginWidget, ClapControlBase* ctrlBase)
{
	auto grid = new QGridLayout{pluginWidget};

	auto btnBox = std::make_unique<QHBoxLayout>();
	if (ClapManager::debugging())
	{
		m_reloadPluginButton = new QPushButton{QObject::tr("Reload Plugin"), pluginWidget};
		btnBox->addWidget(m_reloadPluginButton, 0);
	}

	if (ctrlBase->hasGui())
	{
		m_toggleUIButton = new QPushButton{QObject::tr("Show GUI"), pluginWidget};
		m_toggleUIButton->setCheckable(true);
		m_toggleUIButton->setChecked(false);
		m_toggleUIButton->setIcon(embed::getIconPixmap("zoom"));
		m_toggleUIButton->setFont(pointSize<8>(m_toggleUIButton->font()));
		btnBox->addWidget(m_toggleUIButton, 0);
	}

	if (ctrlBase->hasPresetSupport())
	{
		auto presetBox = std::make_unique<QHBoxLayout>();
		m_presetSelector = new PresetSelector{&ctrlBase->control(0)->presetLoader(), pluginWidget};
		presetBox->addWidget(m_presetSelector, 0);
		grid->addLayout(presetBox.release(), static_cast<int>(Rows::PresetRow), 0, 1, 1);
	}

	btnBox->addStretch(1);

	pluginWidget->setAcceptDrops(true);

	if (m_reloadPluginButton || m_toggleUIButton || m_helpButton)
	{
		grid->addLayout(btnBox.release(), static_cast<int>(Rows::ButtonRow), 0, 1, s_colNum);
	}

	m_parametersView = new ClapViewParameters{pluginWidget, ctrlBase->control(0), s_colNum};
	grid->addWidget(m_parametersView, static_cast<int>(Rows::ParametersRow), 0);
}


ClapViewBase::~ClapViewBase()
{
	// TODO: hide UI if required
}

void ClapViewBase::toggleUI()
{
}

void ClapViewBase::toggleHelp(bool visible)
{
	if (!m_helpWindow) { return; }

	if (visible)
	{
		m_helpWindow->show();
		m_helpWindow->raise();
	}
	else
	{
		m_helpWindow->hide();
	}
}

void ClapViewBase::modelChanged(ClapControlBase* ctrlBase)
{
	// reconnect models
	if (m_toggleUIButton)
	{
		m_toggleUIButton->setChecked(ctrlBase->hasGui());
	}

	//m_presetsView->modelChanged(&ctrlBase->presetsGroup());

	// TODO: How to handle presets?

	LinkedModelGroupsView::modelChanged(&ctrlBase->parametersGroup());
}

} // namespace lmms::gui

#endif // LMMS_HAVE_CLAP
