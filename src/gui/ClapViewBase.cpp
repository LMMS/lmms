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
#include "ClapInstance.h"
#include "ClapManager.h"
#include "ComboBox.h"
#include "ControlLayout.h"
#include "CustomTextKnob.h"
#include "embed.h"
#include "Engine.h"
#include "FontHelper.h"
#include "GuiApplication.h"
#include "lmms_math.h"
#include "MainWindow.h"
#include "PixmapButton.h"
#include "PresetSelector.h"
#include "SubWindow.h"

namespace lmms::gui
{

ClapViewParameters::ClapViewParameters(QWidget* parent, ClapInstance* instance, int colNum)
	: QWidget{parent}
	, m_instance{instance}
	, m_layout{new ControlLayout{this}}
{
	setFocusPolicy(Qt::StrongFocus);

	auto addControl = [&](std::unique_ptr<Control> ctrl, std::string_view display) {
		if (!ctrl) { return; }

		auto box = new QWidget{this};
		auto boxLayout = new QHBoxLayout{box};
		boxLayout->addWidget(ctrl->topWidget());

		// Required, so the Layout knows how to sort/filter widgets by string
		box->setObjectName(QString::fromUtf8(display.data(), display.size()));
		m_layout->addWidget(box);

		m_widgets.push_back(std::move(ctrl));
	};

	for (auto param : m_instance->params().parameters())
	{
		if (!param || !param->model()) { continue; }

		std::unique_ptr<Control> control;

		switch (param->valueType())
		{
		case ClapParameter::ValueType::Bool:
			control = std::make_unique<CheckControl>();
			break;
		case ClapParameter::ValueType::Integer: [[fallthrough]];
		case ClapParameter::ValueType::Enum:
		{
			const int digits = std::max(
				numDigitsAsInt(param->info().min_value),
				numDigitsAsInt(param->info().max_value));

			if (digits < 3)
			{
				control = std::make_unique<LcdControl>(digits);
			}
			else
			{
				control = std::make_unique<KnobControl>();
			}
			break;
		}
		// TODO: Add support for enum controls
		case ClapParameter::ValueType::Float:
		{
			control = std::make_unique<CustomTextKnobControl>();

			// CustomTextKnob calls this lambda to update value text
			auto customTextKnob = dynamic_cast<CustomTextKnob*>(control->modelView());
			customTextKnob->setValueText([=, this] {
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

		addControl(std::move(control), param->displayName());
	}
}

ClapViewBase::ClapViewBase(QWidget* pluginWidget, ClapInstance* instance)
{
	constexpr int controlsPerRow = 6;
	auto grid = new QGridLayout{pluginWidget};

	auto btnBox = std::make_unique<QHBoxLayout>();
	if (ClapManager::debugging())
	{
		m_reloadPluginButton = new QPushButton{QObject::tr("Reload Plugin"), pluginWidget};
		btnBox->addWidget(m_reloadPluginButton);
	}

	if (instance->gui().supported())
	{
		m_toggleUIButton = new QPushButton{QObject::tr("Show GUI"), pluginWidget};
		m_toggleUIButton->setCheckable(true);
		m_toggleUIButton->setChecked(false);
		m_toggleUIButton->setIcon(embed::getIconPixmap("zoom"));
		m_toggleUIButton->setFont(adjustedToPixelSize(m_toggleUIButton->font(), SMALL_FONT_SIZE));
		btnBox->addWidget(m_toggleUIButton);
	}

	if (instance->presetLoader().supported())
	{
		auto presetBox = std::make_unique<QHBoxLayout>();
		m_presetSelector = new PresetSelector{&instance->presetLoader(), pluginWidget};
		presetBox->addWidget(m_presetSelector);
		grid->addLayout(presetBox.release(), static_cast<int>(Rows::PresetRow), 0, 1, 1);
	}

	if (instance->audioPorts().hasMonoPort())
	{
		m_portConfig = new ComboBox{pluginWidget};
		m_portConfig->setFixedSize(128, ComboBox::DEFAULT_HEIGHT);

		QString inputType;
		switch (instance->audioPorts().inputPortType())
		{
			case PluginPortConfig::PortType::None: break;
			case PluginPortConfig::PortType::Mono:
				inputType += QObject::tr("mono in"); break;
			case PluginPortConfig::PortType::Stereo:
				inputType += QObject::tr("stereo in"); break;
			default: break;
		}

		QString outputType;
		switch (instance->audioPorts().outputPortType())
		{
			case PluginPortConfig::PortType::None: break;
			case PluginPortConfig::PortType::Mono:
				outputType += QObject::tr("mono out"); break;
			case PluginPortConfig::PortType::Stereo:
				outputType += QObject::tr("stereo out"); break;
			default: break;
		}

		QString pluginType;
		if (inputType.isEmpty()) { pluginType = outputType; }
		else if (outputType.isEmpty()) { pluginType = inputType; }
		else { pluginType = QString{"%1, %2"}.arg(inputType, outputType); }

		m_portConfig->setToolTip(QObject::tr("L/R channel config for %1 plugin").arg(pluginType));
		m_portConfig->setModel(instance->audioPorts().model());
		btnBox->addWidget(m_portConfig);
	}

	btnBox->addStretch(1);

	pluginWidget->setAcceptDrops(true);

	if (m_reloadPluginButton || m_toggleUIButton || m_portConfig)
	{
		grid->addLayout(btnBox.release(), static_cast<int>(Rows::ButtonRow), 0, 1, controlsPerRow);
	}

	m_parametersView = new ClapViewParameters{pluginWidget, instance, controlsPerRow};
	grid->addWidget(m_parametersView, static_cast<int>(Rows::ParametersRow), 0);
}


ClapViewBase::~ClapViewBase()
{
	// TODO: hide UI if required
}

void ClapViewBase::toggleUI()
{
}

void ClapViewBase::modelChanged(ClapInstance* instance)
{
	// reconnect models
	if (m_toggleUIButton)
	{
		m_toggleUIButton->setChecked(instance->gui().supported());
	}

	//m_presetsView->modelChanged(&ctrlBase->presetsGroup());

	// TODO: How to handle presets?
}

} // namespace lmms::gui

#endif // LMMS_HAVE_CLAP
