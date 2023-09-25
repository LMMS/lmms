/*
 * ClapViewBase.cpp - Base class for CLAP plugin views
 *
 * Copyright (c) 2023 Dalton Messmer <messmer.dalton/at/gmail.com>
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
#include <QPushButton>
#include <QHBoxLayout>
#include <QLabel>
#include <clap/clap.h>

#include "AudioEngine.h"
#include "Controls.h"
#include "Engine.h"
#include "GuiApplication.h"
#include "embed.h"
#include "gui_templates.h"
#include "ClapControlBase.h"
#include "ClapManager.h"
#include "ClapInstance.h"
#include "ClapParam.h"
#include "MainWindow.h"
#include "SubWindow.h"
#include "CustomTextKnob.h"


namespace lmms::gui
{


ClapViewInstance::ClapViewInstance(QWidget* parent, ClapInstance* instance, int colNum)
	: LinkedModelGroupView(parent, instance, colNum)
{
	/*
	class SetupWidget : public ClapPorts::ConstVisitor
	{
	public:
		QWidget* m_par; // input
		const LilvNode* m_commentUri; // input
		Control* m_control = nullptr; // output
		void visit(const ClapPorts::Control& port) override
		{
			if (port.m_flow == ClapPorts::Flow::Input)
			{
				using PortVis = ClapPorts::Vis;

				switch (port.m_vis)
				{
					case PortVis::Generic:
						m_control = new KnobControl(m_par);
						break;
					case PortVis::Integer:
					{
						sample_rate_t sr = Engine::audioEngine()->processingSampleRate();
						m_control = new LcdControl((port.max(sr) <= 9.0f) ? 1 : 2,
													m_par);
						break;
					}
					case PortVis::Enumeration:
						m_control = new ComboControl(m_par);
						break;
					case PortVis::Toggled:
						m_control = new CheckControl(m_par);
						break;
				}
				m_control->setText(port.name());

				AutoLilvNodes props(lilv_port_get_value(
					port.m_plugin, port.m_port, m_commentUri));
				LILV_FOREACH(nodes, itr, props.get())
				{
					const LilvNode* nod = lilv_nodes_get(props.get(), itr);
					m_control->topWidget()->setToolTip(lilv_node_as_string(nod));
					break;
				}
			}
		}
	};

	AutoLilvNode commentUri = uri(LILV_NS_RDFS "comment");
	instance->foreach_port(
		[this, &commentUri](const Lv2Ports::PortBase* port)
		{
			if(!lilv_port_has_property(port->m_plugin, port->m_port,
										uri(LV2_PORT_PROPS__notOnGUI).get()))
			{
				SetupWidget setup;
				setup.m_par = this;
				setup.m_commentUri = commentUri.get();
				port->accept(setup);

				if (setup.m_control)
				{
					addControl(setup.m_control,
						lilv_node_as_string(lilv_port_get_symbol(
							port->m_plugin, port->m_port)),
						port->name().toUtf8().data(),
						false);
				}
			}
		});

	*/

	for (auto param : instance->params())
	{
		if (!param || !param->model()) { continue; }

		Control* control = nullptr;

		switch (param->valueType())
		{
		case ClapParam::ParamType::Bool:
			control = new CheckControl{this};
			break;
		case ClapParam::ParamType::Integer:
			// TODO: What if more digits are needed? Lv2 uses KnobControl in this case.
			control = new LcdControl{(param->info().max_value <= 9.0) ? 1 : 2, this};
			break;
		case ClapParam::ParamType::Float:
		{
			auto derived = new CustomTextKnobControl{this};

			// CustomTextKnob calls this lambda to update value text
			auto customTextKnob = dynamic_cast<CustomTextKnob*>(derived->modelView());
			customTextKnob->setValueText([instance, param](){
					return QString::fromUtf8(instance->getParamValueText(param).c_str());
			});

			control = derived;
			break;
		}
		default:
			throw std::runtime_error{"Invalid CLAP param value type"};
		}

		if (!control) { continue; }

		// This is the param name seen in the GUI
		control->setText(QString::fromUtf8(param->displayName().data()));

		if (param->info().module[0] != '\0')
		{
			control->topWidget()->setToolTip(QString::fromUtf8(param->info().module));
		}

		addControl(control, param->id().data(), param->displayName().data(), false);
	}

}

/*
auto ClapViewInstance::uri(const char* uriStr) -> AutoLilvNode
{
	return Engine::getClapManager()->uri(uriStr);
}
*/

ClapViewBase::ClapViewBase(QWidget* meAsWidget, ClapControlBase* ctrlBase)
{
	auto grid = new QGridLayout(meAsWidget);

	auto btnBox = new QHBoxLayout();
	if (/* DISABLES CODE */ (false))
	{
		m_reloadPluginButton = new QPushButton(QObject::tr("Reload Plugin"), meAsWidget);
		btnBox->addWidget(m_reloadPluginButton, 0);
	}

	if (/* DISABLES CODE */ (false)) // TODO: check if the plugin has the UI extension
	{
		m_toggleUIButton = new QPushButton(QObject::tr("Show GUI"), meAsWidget);
		m_toggleUIButton->setCheckable(true);
		m_toggleUIButton->setChecked(false);
		m_toggleUIButton->setIcon(embed::getIconPixmap("zoom"));
		m_toggleUIButton->setFont(pointSize<8>(m_toggleUIButton->font()));
		btnBox->addWidget(m_toggleUIButton, 0);
	}
	btnBox->addStretch(1);

	meAsWidget->setAcceptDrops(true);

	// note: the lifetime of C++ objects ends after the top expression in the
	// expression syntax tree, so the AutoLilvNode gets freed after the function
	// has been called

	/*
	AutoLilvNodes props(lilv_plugin_get_value(ctrlBase->getPlugin(),
				uri(LILV_NS_RDFS "comment").get()));
	LILV_FOREACH(nodes, itr, props.get())
	{
		const LilvNode* node = lilv_nodes_get(props.get(), itr);
		auto infoLabel = new QLabel(lilv_node_as_string(node));
		infoLabel->setWordWrap(true);
		infoLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

		m_helpButton = new QPushButton(QObject::tr("Help"));
		m_helpButton->setCheckable(true);
		btnBox->addWidget(m_helpButton);

		m_helpWindow = getGUI()->mainWindow()->addWindowedWidget(infoLabel);
		m_helpWindow->setSizePolicy(QSizePolicy::Minimum,
									QSizePolicy::Expanding);
		m_helpWindow->setAttribute(Qt::WA_DeleteOnClose, false);
		m_helpWindow->hide();

		break;
	}
	*/

	if (m_reloadPluginButton || m_toggleUIButton || m_helpButton)
	{
		grid->addLayout(btnBox, Rows::ButtonRow, 0, 1, s_colNum);
	}
	else { delete btnBox; }

	m_procView = new ClapViewInstance(meAsWidget, ctrlBase->control(0), s_colNum);
	grid->addWidget(m_procView, Rows::ProcRow, 0);
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

	LinkedModelGroupsView::modelChanged(ctrlBase);
}

/*
auto ClapViewBase::uri(const char* uriStr) -> AutoLilvNode
{
	return Engine::getClapManager()->uri(uriStr);
}
*/


} // namespace lmms::gui

#endif // LMMS_HAVE_CLAP
