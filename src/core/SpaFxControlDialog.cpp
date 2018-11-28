/*
 * SpaControlDialog.cpp - control dialog for amplifier effect
 *
 * Copyright (c) 2018-2018 Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>
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

#include "SpaFxControlDialog.h"

#include <QDebug>
#include <QDragEnterEvent>
#include <QGridLayout>
#include <QGroupBox>
#include <QMimeData>
#include <QPushButton>
#include <QVBoxLayout>
#include <spa/audio.h>

#include "Knob.h"
#include "LedCheckbox.h"
#include "SpaEffect.h"
#include "SpaFxControls.h"
#include "embed.h"
#include "gui_templates.h"

SpaFxControls *SpaFxControlDialog::spaControls()
{
	return static_cast<SpaFxControls *>(m_effectControls);
}

SpaFxControlDialog::SpaFxControlDialog(SpaFxControls *controls) :
	EffectControlDialog(controls)
{
	QGridLayout *grid = new QGridLayout(this);

	m_reloadPluginButton = new QPushButton(tr("Reload Plugin"), this);
	grid->addWidget(m_reloadPluginButton, 0, 0, 1, 3);

	connect(m_reloadPluginButton, SIGNAL(toggled(bool)), this,
		SLOT(reloadPlugin()));

	if (spaControls()->m_spaDescriptor->ui_ext())
	{
		m_toggleUIButton = new QPushButton(tr("Show GUI"), this);
		m_toggleUIButton->setCheckable(true);
		m_toggleUIButton->setChecked(false);
		m_toggleUIButton->setIcon(embed::getIconPixmap("zoom"));
		m_toggleUIButton->setFont(
			pointSize<8>(m_toggleUIButton->font()));
		connect(m_toggleUIButton, SIGNAL(toggled(bool)), this,
			SLOT(toggleUI()));
		m_toggleUIButton->setWhatsThis(
			tr("Click here to show or hide the graphical user "
			   "interface "
			   "(GUI) of Osc."));
		grid->addWidget(m_toggleUIButton, 0, 3, 1, 3);
	}

	//	setAcceptDrops(true); // TODO?

	const int rowNum = 6; // just some guess for what might look good
	int wdgNum = 0;
	for (SpaControlBase::LmmsPorts::TypedPorts &ports :
		controls->m_ports.m_otherPorts)
	{
		QWidget *wdg;
		switch (ports.m_type)
		{
		case 'f':
		{
			Knob *k = new Knob(this);
			k->setModel(ports.m_connectedModel.m_floatModel);
			wdg = k;
			break;
		}
		case 'i':
		{
			Knob *k = new Knob(this);
			k->setModel(ports.m_connectedModel.m_intModel);
			wdg = k;
			break;
		}
		case 'b':
		{
			LedCheckBox *l = new LedCheckBox(this);
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

/*
// TODO: common UI class..., as this must be usable for instruments, too
SpaControlDialog::~SpaControlDialog()
{
	SpaEffect *model = castModel<SpaEffect>();

	if (model && spaControls()->m_spaDescriptor->ui_ext() &&
spaControls()->m_hasGUI)
	{
		qDebug() << "shutting down UI...";
		model->m_plugin->ui_ext_show(false);
	}
}
*/

void SpaFxControlDialog::modelChanged()
{
	/*	// set models for controller knobs
		m_portamento->setModel( &m->m_portamentoModel ); */

	m_toggleUIButton->setChecked(spaControls()->m_hasGUI);
}

void SpaFxControlDialog::toggleUI()
{
#if 0
	SpaEffect *model = castModel<SpaEffect>();
	if (model->m_spaDescriptor->ui_ext() &&
		model->m_hasGUI != m_toggleUIButton->isChecked())
	{
		model->m_hasGUI = m_toggleUIButton->isChecked();
		model->m_plugin->ui_ext_show(model->m_hasGUI);
		ControllerConnection::finalizeConnections();
	}
#endif
}

void SpaFxControlDialog::reloadPlugin() { spaControls()->reloadPlugin(); }
