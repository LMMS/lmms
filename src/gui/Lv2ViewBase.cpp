/*
 * Lv2ViewBase.cpp - base class for Lv2 plugin views
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

#include "Lv2ViewBase.h"

#ifdef LMMS_HAVE_LV2

#include <QGridLayout>
#include <QGroupBox>
#include <QMdiSubWindow>
#include <QPushButton>
#include <QHBoxLayout>
#include <lilv/lilv.h>

#include "Controls.h"
#include "Engine.h"
#include "GuiApplication.h"
#include "embed.h"
#include "gui_templates.h"
#include "LedCheckbox.h"
#include "Lv2ControlBase.h"
#include "Lv2Manager.h"
#include "Lv2Proc.h"
#include "Lv2Ports.h"
#include "MainWindow.h"
#include "SubWindow.h"




Lv2ViewProc::Lv2ViewProc(QWidget* parent, Lv2Proc* ctrlBase,
	int colNum, int curProc, int nProc, const QString& name) :
	LinkedModelGroupViewBase (parent, ctrlBase->isLinking(), colNum,
								curProc, nProc, name)
{
	class SetupWidget : public Lv2Ports::Visitor
	{
		AutoLilvNode commentUri = uri(LILV_NS_RDFS "comment");
	public:
		QWidget* par; // input
		ControlBase* control = nullptr; // output
		void visit(Lv2Ports::Control& port) override
		{
			if(port.m_flow == Lv2Ports::Flow::Input)
			{
				using PortVis = Lv2Ports::Vis;

				switch (port.m_vis)
				{
					case PortVis::None:
						control = new KnobControl(par);
						break;
					case PortVis::Integer:
						control = new LcdControl((port.m_max <= 9.0f) ? 1 : 2,
													par);
						break;
					case PortVis::Enumeration:
						control = new ComboControl(par);
						break;
					case PortVis::Toggled:
						control = new CheckControl(par);
						break;
				}
				control->setText(port.name());

				LilvNodes* props = lilv_port_get_value(
					port.m_plugin, port.m_port, commentUri.get());
				LILV_FOREACH(nodes, itr, props)
				{
					const LilvNode* nod = lilv_nodes_get(props, itr);
					control->topWidget()->setToolTip(lilv_node_as_string(nod));
					break;
				}
				lilv_nodes_free(props);
			}
		}
	};

	for (Lv2Ports::PortBase* port : ctrlBase->getPorts())
	{
		SetupWidget setup;
		setup.par = this;
		port->accept(setup);

		if(setup.control)
		{
			addControl(setup.control);
		}
	}
}




Lv2ViewProc::~Lv2ViewProc() {}




AutoLilvNode Lv2ViewProc::uri(const char *uriStr)
{
	return Engine::getLv2Manager()->uri(uriStr);
}




Lv2ViewBase::Lv2ViewBase(QWidget* meAsWidget, Lv2ControlBase *ctrlBase)
	: LinkedModelGroupsViewBase (ctrlBase)
{
	QGridLayout* grid = new QGridLayout(meAsWidget);

	QHBoxLayout* btnBox = new QHBoxLayout();
	grid->addLayout(btnBox, Rows::ButtonRow, 0, 1, m_colNum);
	if(/* DISABLES CODE */ (false)) {
		m_reloadPluginButton = new QPushButton(QObject::tr("Reload Plugin"),
			meAsWidget);
		btnBox->addWidget(m_reloadPluginButton, 0);
	}

	if (/* DISABLES CODE */ (false)) // TODO: check if the plugin has the UI extension
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
				"Lv2 graphical user interface (GUI)."));
		btnBox->addWidget(m_toggleUIButton, 0);
	}
	btnBox->addStretch(1);

	meAsWidget->setAcceptDrops(true);

	// note: the lifetime of C++ objects ends after the top expression in the
	// expression syntax tree, so the AutoLilvNode gets freed after the function
	// has been called
	LilvNodes* props = lilv_plugin_get_value(ctrlBase->getPlugin(),
				uri(LILV_NS_RDFS "comment").get());
	LILV_FOREACH(nodes, itr, props)
	{
		const LilvNode* node = lilv_nodes_get(props, itr);
		QLabel* infoLabel = new QLabel(lilv_node_as_string(node));
		infoLabel->setWordWrap(true);
		infoLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

		m_helpButton = new QPushButton(QObject::tr("Help"));
		m_helpButton->setCheckable(true);
		btnBox->addWidget(m_helpButton);

		m_helpWindow = gui->mainWindow()->addWindowedWidget(infoLabel);
		m_helpWindow->setSizePolicy(QSizePolicy::Minimum,
									QSizePolicy::Expanding);
		m_helpWindow->hide();

		break;
	}
	lilv_nodes_free(props);

	int nProcs = static_cast<int>(ctrlBase->controls().size());
	Q_ASSERT(m_colNum % nProcs == 0);
	int colsEach = m_colNum / nProcs;
	for (int i = 0; i < nProcs; ++i)
	{
		Lv2ViewProc* vpr = new Lv2ViewProc(meAsWidget,
			ctrlBase->controls()[static_cast<std::size_t>(i)],
			colsEach, i, nProcs);
		grid->addWidget(vpr, Rows::ProcRow, i);
		m_procViews.push_back(vpr);
	}

	LedCheckBox* led = globalLinkLed();
	if(led)
		grid->addWidget(led, Rows::LinkChannelsRow, 0, 1, m_colNum);
}




Lv2ViewBase::~Lv2ViewBase() {}




void Lv2ViewBase::toggleHelp(bool visible)
{
	if( m_helpWindow )
	{
		if( visible )
		{
			m_helpWindow->show();
			m_helpWindow->raise();
		}
		else
		{
			m_helpWindow->hide();
		}
	}
}




void Lv2ViewBase::modelChanged(Lv2ControlBase *ctrlBase)
{
	// reconnect models
	if(m_toggleUIButton)
	{
		m_toggleUIButton->setChecked(ctrlBase->hasGui());
	}

	LinkedModelGroupsViewBase::modelChanged(ctrlBase);
}




AutoLilvNode Lv2ViewBase::uri(const char *uriStr)
{
	return Engine::getLv2Manager()->uri(uriStr);
}


#endif // LMMS_HAVE_LV2
