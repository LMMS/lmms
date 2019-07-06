/*
 * SpaControlDialog.cpp - control dialog for amplifier effect
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

#include "SpaFxControlDialog.h"

#include <QPushButton>

#include "SpaFxControls.h"

SpaFxControls *SpaFxControlDialog::spaControls()
{
	return static_cast<SpaFxControls *>(m_effectControls);
}

SpaFxControlDialog::SpaFxControlDialog(SpaFxControls *controls) :
	EffectControlDialog(controls),
	SpaViewBase(this, controls)
{
	connect(m_reloadPluginButton, SIGNAL(toggled(bool)),
		this, SLOT(reloadPlugin()));
	if(m_toggleUIButton)
		connect(m_toggleUIButton, SIGNAL(toggled(bool)),
			this, SLOT(toggleUI()));
	// for Effects, modelChanged only goes to the top EffectView
	// we need to call it manually
	modelChanged();
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
	SpaViewBase::modelChanged(spaControls());
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
