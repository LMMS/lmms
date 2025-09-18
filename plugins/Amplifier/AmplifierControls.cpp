/*
 * AmplifierControls.cpp - controls for amplifier effect
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


#include "AmplifierControls.h"
#include "Amplifier.h"

namespace lmms
{

AmplifierControls::AmplifierControls(AmplifierEffect* effect) :
	EffectControls(effect),
	m_effect(effect),
	m_volumeModel(100.0f, 0.0f, 200.0f, 0.1f, this, tr("Volume")),
	m_panModel(0.0f, -100.0f, 100.0f, 0.1f, this, tr("Panning")),
	m_leftModel(100.0f, 0.0f, 200.0f, 0.1f, this, tr("Left gain")),
	m_rightModel(100.0f, 0.0f, 200.0f, 0.1f, this, tr("Right gain"))
{
}


void AmplifierControls::loadSettings(const QDomElement& parent)
{
	m_volumeModel.loadSettings(parent, "volume");
	m_panModel.loadSettings(parent, "pan");
	m_leftModel.loadSettings(parent, "left");
	m_rightModel.loadSettings(parent, "right");
}


void AmplifierControls::saveSettings(QDomDocument& doc, QDomElement& parent)
{
	m_volumeModel.saveSettings(doc, parent, "volume"); 
	m_panModel.saveSettings(doc, parent, "pan");
	m_leftModel.saveSettings(doc, parent, "left");
	m_rightModel.saveSettings(doc, parent, "right");
}


} // namespace lmms
