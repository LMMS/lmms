/*
 * TemplateEffectControls.cpp - Example effect control boilerplate code
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QDomElement>

#include "TemplateEffectControls.h"
#include "TemplateEffect.h"

namespace lmms
{

TemplateEffectControls::TemplateEffectControls(TemplateEffectEffect* effect) :
	EffectControls(effect),
	TEMPLATE_KNOB_LOOP_START
	m_modelTEMPLATE_KNOB_NUMBER(0.5f, 0.0f, 1.0f, 0.00001f, this, tr("Knob TEMPLATE_KNOB_NUMBER")),
	TEMPLATE_KNOB_LOOP_END
	m_effect(effect)
{
}


void TemplateEffectControls::loadSettings(const QDomElement& parent)
{
	TEMPLATE_KNOB_LOOP_START
	m_modelTEMPLATE_KNOB_NUMBER.loadSettings(parent, "modelTEMPLATE_KNOB_NUMBER");
	TEMPLATE_KNOB_LOOP_END
}


void TemplateEffectControls::saveSettings(QDomDocument& doc, QDomElement& parent)
{
	TEMPLATE_KNOB_LOOP_START
	m_modelTEMPLATE_KNOB_NUMBER.saveSettings(doc, parent, "modelTEMPLATE_KNOB_NUMBER");
	TEMPLATE_KNOB_LOOP_END
}


} // namespace lmms
