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
#include <spa/spa.h>

#include "Knob.h"
#include "embed.h"
#include "gui_templates.h"
#include "LcdSpinBox.h"
#include "LedCheckbox.h"
#include "SpaControlBase.h"

SpaViewBase::SpaViewBase(QWidget* meAsWidget, SpaControlBase *ctrlBase)
{
	m_grid = new QGridLayout(meAsWidget);

	m_reloadPluginButton = new QPushButton(QObject::tr("Reload Plugin"),
		meAsWidget);
	m_grid->addWidget(m_reloadPluginButton, 0, 0, 1, 3);

	if (ctrlBase->m_spaDescriptor->ui_ext())
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
		m_grid->addWidget(m_toggleUIButton, 0, 3, 1, 3);
	}

	meAsWidget->setAcceptDrops(true);

	int wdgNum = 0;
	for (SpaControlBase::LmmsPorts::TypedPorts &ports :
		ctrlBase->m_ports.m_userPorts)
	{
		QWidget* wdg;
		AutomatableModelView* modelView;
		switch (ports.m_type)
		{
			case 'f':
			{
				Knob* k = new Knob(meAsWidget);
				wdg = k;
				modelView = k;
				break;
			}
			case 'i':
			{
				LcdSpinBox *l = new LcdSpinBox(3/*log(range)*/,
					meAsWidget);
				wdg = l;
				modelView = l;
				break;
			}
			case 'b':
			{
				LedCheckBox *l = new LedCheckBox(meAsWidget);
				wdg = l;
				modelView = l;
				break;
			}
			default:
				wdg = nullptr;
				modelView = nullptr;
				break;
		}

		if (wdg && modelView)
		{
			// start in row one, add widgets cell by cell
			m_modelViews.push_back(modelView);
			m_grid->addWidget(
				wdg,
				m_firstModelRow + wdgNum / m_rowNum,
					wdgNum % m_rowNum);
			++wdgNum;
		}
		else
		{
			qDebug() << "this should never happen...";
		}
	}
}

void SpaViewBase::modelChanged(SpaControlBase *ctrlBase)
{
	// reconnect models
	QVector<AutomatableModelView*>::Iterator itr = m_modelViews.begin();
	for (SpaControlBase::LmmsPorts::TypedPorts &ports :
		ctrlBase->m_ports.m_userPorts)
	{
		switch (ports.m_type)
		{
			case 'f':
				(*itr)->setModel(ports.m_connectedModel.m_floatModel);
				break;
			case 'i':
				(*itr)->setModel(ports.m_connectedModel.m_intModel);
				break;
			case 'b':
				(*itr)->setModel(ports.m_connectedModel.m_boolModel);
				break;
		}
		++itr;
	}

	// move non-model values to widgets
	if(m_toggleUIButton)
		m_toggleUIButton->setChecked(ctrlBase->m_hasGUI);
}

#endif // LMMS_HAVE_SPA

