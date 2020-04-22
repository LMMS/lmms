/*
 * Lv2ViewBase.cpp - base class for Lv2 plugin views
 *
 * Copyright (c) 2018-2020 Johannes Lorenz <jlsf2013$users.sourceforge.net, $=@>
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




Lv2ViewProc::Lv2ViewProc(QWidget* parent, Lv2Proc* ctrlBase, int colNum) :
	LinkedModelGroupView (parent, ctrlBase, colNum)
{
	class SetupWidget : public Lv2Ports::ConstVisitor
	{
	public:
		QWidget* m_par; // input
		const LilvNode* m_commentUri; // input
		Control* m_control = nullptr; // output
		void visit(const Lv2Ports::Control& port) override
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
	ctrlBase->foreach_port(
		[this, &commentUri](const Lv2Ports::PortBase* port)
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
		});
}




Lv2ViewProc::~Lv2ViewProc() {}




AutoLilvNode Lv2ViewProc::uri(const char *uriStr)
{
	return Engine::getLv2Manager()->uri(uriStr);
}




Lv2ViewBase::Lv2ViewBase(QWidget* meAsWidget, Lv2ControlBase *ctrlBase)
{
	QGridLayout* grid = new QGridLayout(meAsWidget);

	QHBoxLayout* btnBox = new QHBoxLayout();
	if (/* DISABLES CODE */ (false))
	{
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
	AutoLilvNodes props(lilv_plugin_get_value(ctrlBase->getPlugin(),
				uri(LILV_NS_RDFS "comment").get()));
	LILV_FOREACH(nodes, itr, props.get())
	{
		const LilvNode* node = lilv_nodes_get(props.get(), itr);
		QLabel* infoLabel = new QLabel(lilv_node_as_string(node));
		infoLabel->setWordWrap(true);
		infoLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

		m_helpButton = new QPushButton(QObject::tr("Help"));
		m_helpButton->setCheckable(true);
		btnBox->addWidget(m_helpButton);

		m_helpWindow = gui->mainWindow()->addWindowedWidget(infoLabel);
		m_helpWindow->setSizePolicy(QSizePolicy::Minimum,
									QSizePolicy::Expanding);
		m_helpWindow->setAttribute(Qt::WA_DeleteOnClose, false);
		m_helpWindow->hide();

		break;
	}

	if(m_reloadPluginButton || m_toggleUIButton || m_helpButton)
	{
		grid->addLayout(btnBox, Rows::ButtonRow, 0, 1, m_colNum);
	}
	else { delete btnBox; }

	m_procView = new Lv2ViewProc(meAsWidget, ctrlBase->control(0), m_colNum);
	grid->addWidget(m_procView, Rows::ProcRow, 0);
}




Lv2ViewBase::~Lv2ViewBase() {
	// TODO: hide UI if required
}




void Lv2ViewBase::toggleHelp(bool visible)
{
	if ( m_helpWindow )
	{
		if ( visible ) { m_helpWindow->show(); m_helpWindow->raise(); }
		else { m_helpWindow->hide(); }
	}
}




void Lv2ViewBase::modelChanged(Lv2ControlBase *ctrlBase)
{
	// reconnect models
	if (m_toggleUIButton)
	{
		m_toggleUIButton->setChecked(ctrlBase->hasGui());
	}

	LinkedModelGroupsView::modelChanged(ctrlBase);
}




AutoLilvNode Lv2ViewBase::uri(const char *uriStr)
{
	return Engine::getLv2Manager()->uri(uriStr);
}


#endif // LMMS_HAVE_LV2
