/*
 * TemplateEffectControlDialog.cpp - Example effect gui boilerplate code
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "TemplateEffectControlDialog.h"
#include "TemplateEffectControls.h"
#include "embed.h"
#include "Knob.h"
#include "Fader.h"
#include "LedCheckBox.h"
#include "LcdSpinBox.h"

#include <QHBoxLayout>

namespace lmms::gui
{

TemplateEffectControlDialog::TemplateEffectControlDialog(TemplateEffectControls* controls) :
	EffectControlDialog(controls)
{
	setAutoFillBackground(true);
	/* Uncomment this if you want to use a background image
	QPalette pal;
	pal.setBrush(backgroundRole(), PLUGIN_NAME::getIconPixmap("artwork"));
	setPalette(pal);
	setFixedSize(100, 110);
	*/

	QHBoxLayout* mainLayout = new QHBoxLayout(this);
	mainLayout->setContentsMargins(20,20,20,20);

	TEMPLATE_KNOB_LOOP_START
	Knob* knobTEMPLATE_KNOB_NUMBER = new Knob(KnobType::Bright26, this);
	knobTEMPLATE_KNOB_NUMBER->setModel(&controls->m_modelTEMPLATE_KNOB_NUMBER);
	knobTEMPLATE_KNOB_NUMBER->setLabel(tr("Knob TEMPLATE_KNOB_NUMBER"));
	knobTEMPLATE_KNOB_NUMBER->setHintText(tr("Value description:"), " units");
	mainLayout->addWidget(knobTEMPLATE_KNOB_NUMBER);

	TEMPLATE_KNOB_LOOP_END

}

} // namespace lmms::gui
