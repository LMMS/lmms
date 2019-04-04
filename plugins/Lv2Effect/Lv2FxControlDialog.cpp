/*
 * Lv2ControlDialog.cpp - control dialog for amplifier effect
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

#include "Lv2FxControlDialog.h"

#include <QDebug>
#include <QDragEnterEvent>
#include <QGridLayout>
#include <QGroupBox>
#include <QMimeData>
#include <QPushButton>
#include <QVBoxLayout>
#include <lv2.h>

#include "Knob.h"
#include "LcdSpinBox.h"
#include "LedCheckbox.h"
#include "Lv2Effect.h"
#include "Lv2FxControls.h"
#include "embed.h"
#include "gui_templates.h"


Lv2FxControlDialog::Lv2FxControlDialog(Lv2FxControls *controls) :
	EffectControlDialog(controls),
	Lv2ViewBase(this, controls)
{
	if (m_reloadPluginButton) {
		connect(m_reloadPluginButton, SIGNAL(toggled(bool)),
			this, SLOT(reloadPlugin()));
	}
	if (m_toggleUIButton) {
		connect(m_toggleUIButton, SIGNAL(toggled(bool)),
			this, SLOT(toggleUI()));
	}
	if (m_helpButton) {
		connect(m_helpButton, SIGNAL(toggled(bool)),
			this, SLOT(toggleHelp(bool)));
	}
	// for Effects, modelChanged only goes to the top EffectView
	// we need to call it manually
	modelChanged();
}




void Lv2FxControlDialog::reloadPlugin() { lv2Controls()->reloadPlugin(); }




void Lv2FxControlDialog::toggleUI()
{
}




void Lv2FxControlDialog::toggleHelp(bool visible)
{
	Lv2ViewBase::toggleHelp(visible);
}




Lv2FxControls *Lv2FxControlDialog::lv2Controls()
{
	return static_cast<Lv2FxControls *>(m_effectControls);
}




void Lv2FxControlDialog::modelChanged()
{
	Lv2ViewBase::modelChanged(lv2Controls());
}


