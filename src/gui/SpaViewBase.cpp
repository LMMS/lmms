/*
 * SpaViewBase.cpp - base class for SPA plugin views
 *
 * Copyright (c) 2018-2019 Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>
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

#include "SpaViewBase.h"

#ifdef LMMS_HAVE_SPA

#include <QDebug>
#include <QGridLayout>
#include <QPushButton>

#include "Controls.h"
#include "embed.h"
#include "gui_templates.h"
#include "SpaControlBase.h"
#include "LedCheckbox.h"

SpaViewBase::SpaViewBase(QWidget* meAsWidget, SpaControlBase *ctrlBase)
	: LinkedModelGroupsView (ctrlBase)
{
	m_grid = new QGridLayout(meAsWidget);

	m_reloadPluginButton = new QPushButton(QObject::tr("Reload Plugin"),
		meAsWidget);
	m_grid->addWidget(m_reloadPluginButton, Rows::ButtonRow, 0, 1, 3);

	if (ctrlBase->hasUi())
	{
		m_toggleUIButton = new QPushButton(QObject::tr("Show GUI"),
						meAsWidget);
		m_toggleUIButton->setCheckable(true);
		m_toggleUIButton->setChecked(false);
		m_toggleUIButton->setIcon(embed::getIconPixmap("zoom"));
		m_toggleUIButton->setFont(
			pointSize<8>(m_toggleUIButton->font()));
		m_toggleUIButton->setWhatsThis(
			QObject::tr("Click here to show or hide the "
				"graphical user interface (GUI) of SPA."));
		m_grid->addWidget(m_toggleUIButton, Rows::ButtonRow, 3, 1, 3);
	}

	meAsWidget->setAcceptDrops(true);

	std::size_t nProcs = ctrlBase->controls().size();
	Q_ASSERT(m_colNum % nProcs == 0);
	std::size_t colsEach = m_colNum / nProcs;
	for (std::size_t i = 0; i < nProcs; ++i)
	{
			SpaViewProc* vpr = new SpaViewProc(meAsWidget,
					ctrlBase->controls()[i].get(),
					colsEach, nProcs);
			m_grid->addWidget(vpr, Rows::ProcRow, static_cast<int>(i));
			m_procViews.push_back(vpr);
	}

	LedCheckBox* led = globalLinkLed();
	if (led)
	{
			m_grid->addWidget(led, Rows::LinkChannelsRow, 0,
				1, static_cast<int>(m_colNum));
	}
}

SpaViewBase::~SpaViewBase() {}

void SpaViewBase::modelChanged(SpaControlBase *ctrlBase)
{
	// reconnect models
	if(m_toggleUIButton)
	{
		m_toggleUIButton->setChecked(ctrlBase->m_hasGUI);
	}
	LinkedModelGroupsView::modelChanged(ctrlBase);
}

SpaViewProc::SpaViewProc(QWidget* parent, SpaProc *proc,
	std::size_t colNum, std::size_t nProcs)
	: LinkedModelGroupView(parent, proc, colNum, nProcs)
{
#if 0
	class SetupWidget : public Lv2Ports::Visitor
	{
	public:
		QWidget* m_par; // input
		const AutoLilvNode* m_commentUri; // input
		ControlBase* m_control = nullptr; // output
		void visit(Lv2Ports::Control& port) override
		{
			if (port.m_flow == Lv2Ports::Flow::Input)
			{
				using PortVis = Lv2Ports::Vis;

				switch (port.m_vis)
				{
					case PortVis::None:
						m_control = new KnobControl(m_par);
						break;
					case PortVis::Integer:
						m_control = new LcdControl((port.m_max <= 9.0f) ? 1 : 2,
													m_par);
						break;
					case PortVis::Enumeration:
						m_control = new ComboControl(m_par);
						break;
					case PortVis::Toggled:
						m_control = new CheckControl(m_par);
						break;
				}
				m_control->setText(port.name());

				LilvNodes* props = lilv_port_get_value(
					port.m_plugin, port.m_port, m_commentUri->get());
				LILV_FOREACH(nodes, itr, props)
				{
					const LilvNode* nod = lilv_nodes_get(props, itr);
					m_control->topWidget()->setToolTip(lilv_node_as_string(nod));
					break;
				}
				lilv_nodes_free(props);
			}
		}
	};

	AutoLilvNode commentUri = uri(LILV_NS_RDFS "comment");
	for (std::unique_ptr<Lv2Ports::PortBase>& port : ctrlBase->getPorts())
	{
		SetupWidget setup;
		setup.m_par = this;
		setup.m_commentUri = &commentUri;
		port->accept(setup);

		if (setup.m_control) { addControl(setup.m_control); }
	}
#endif
	Control* control = nullptr;
	for (SpaProc::LmmsPorts::TypedPorts &ports :
		proc->m_ports.m_userPorts)
	{
		//QWidget* wdg;
		//AutomatableModelView* modelView;
		switch (ports.m_type)
		{
			case 'f':
			{
				control = new KnobControl(this);
				control->setText(ports.m_connectedModel.m_floatModel->displayName());
			//	wdg = k;
				//modelView = k;
				break;
			}
			case 'i':
			{
				// TODO: check max
				control = new LcdControl(2, this);
				control->setText(ports.m_connectedModel.m_intModel->displayName());
			//	wdg = l;
			//	modelView = l;
				break;
			}
			case 'b':
			{
				control = new CheckControl(this);
				control->setText(ports.m_connectedModel.m_boolModel->displayName());
	//			wdg = l;
		//		modelView = l;
				break;
			}
			default:
			//	wdg = nullptr;
			//	modelView = nullptr;
				control = nullptr;
				break;
		}

		if (control)
		{
			// start in row one, add widgets cell by cell
//			m_modelViews.push_back(modelView);
			/*m_grid->addWidget(
				wdg,
				m_firstModelRow + wdgNum / m_rowNum,
					wdgNum % m_rowNum);
			++wdgNum;*/
			addControl(control);
		}
		else
		{
			qCritical() << "this should never happen...";
		}
	}
}

LinkedModelGroupView *SpaViewBase::getGroupView(std::size_t idx)
{
	return m_procViews[static_cast<int>(idx)];
}

#endif // LMMS_HAVE_SPA


