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
#include "LedCheckbox.h"
#include "SpaControlBase.h"

SpaViewBase::SpaViewBase(QWidget* meAsWidget, SpaControlBase *ctrlBase,
			const char* reloadPluginSlot)
{
	QGridLayout *grid = new QGridLayout(meAsWidget);

	m_reloadPluginButton = new QPushButton(QObject::tr("Reload Plugin"),
		meAsWidget);
	grid->addWidget(m_reloadPluginButton, 0, 0, 1, 3);

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
		grid->addWidget(m_toggleUIButton, 0, 3, 1, 3);
	}

	//	setAcceptDrops(true); // TODO?

	const int rowNum = 6; // just some guess for what might look good
	int wdgNum = 0;
	for (SpaControlBase::LmmsPorts::TypedPorts &ports :
		ctrlBase->m_ports.m_userPorts)
	{
		QWidget *wdg;
		switch (ports.m_type)
		{
		case 'f':
		{
			Knob *k = new Knob(meAsWidget);
			k->setModel(ports.m_connectedModel.m_floatModel);
			wdg = k;
			break;
		}
		case 'i':
		{
			Knob *k = new Knob(meAsWidget);
			k->setModel(ports.m_connectedModel.m_intModel);
			wdg = k;
			break;
		}
		case 'b':
		{
			LedCheckBox *l = new LedCheckBox(meAsWidget);
			l->setModel(ports.m_connectedModel.m_boolModel);
			wdg = l;
			break;
		}
		default:
			wdg = nullptr;
			break;
		}

		if (wdg)
		{
			// start in row one, add widgets cell by cell
			grid->addWidget(
				wdg, 1 + wdgNum / rowNum, wdgNum % rowNum);
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

	// move non-model values to widgets
	m_toggleUIButton->setChecked(ctrlBase->m_hasGUI);
}

#endif // LMMS_HAVE_SPA

