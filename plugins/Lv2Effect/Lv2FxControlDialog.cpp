/*
 * Lv2FxControlDialog.cpp - Lv2FxControlDialog implementation
 *
 * Copyright (c) 2018-2023 Johannes Lorenz <jlsf2013$users.sourceforge.net, $=@>
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

#include <QPushButton>

#include "Lv2FxControls.h"

namespace lmms::gui
{


Lv2FxControlDialog::Lv2FxControlDialog(Lv2FxControls *controls) :
	EffectControlDialog(controls),
	Lv2ViewBase(this, controls)
{
	if (m_reloadPluginButton) {
		connect(m_reloadPluginButton, &QPushButton::clicked,
				this, [this](){ lv2Controls()->reload(); });
	}
	if (m_toggleUIButton) {
		connect(m_toggleUIButton, &QPushButton::toggled,
			this, [this](){ toggleUI(); });
	}
	if (m_helpButton) {
		connect(m_helpButton, &QPushButton::toggled,
			this, [this](bool visible){ toggleHelp(visible); });
	}
	// for Effects, modelChanged only goes to the top EffectView
	// we need to call it manually
	modelChanged();
}




Lv2FxControls *Lv2FxControlDialog::lv2Controls()
{
	return static_cast<Lv2FxControls *>(m_effectControls);
}




void Lv2FxControlDialog::modelChanged()
{
	Lv2ViewBase::modelChanged(lv2Controls());
	connect(lv2Controls(), &Lv2FxControls::modelChanged,
		this, [this](){ this->modelChanged();} );
}




void Lv2FxControlDialog::hideEvent(QHideEvent *event)
{
	closeHelpWindow();
	QWidget::hideEvent(event);
}


} // namespace lmms::gui
