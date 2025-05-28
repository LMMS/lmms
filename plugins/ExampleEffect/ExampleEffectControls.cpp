/*
 * ExampleEffectControls.cpp - Example effect control boilerplate code
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

#include "ExampleEffectControls.h"
#include "ExampleEffect.h"

namespace lmms
{

ExampleEffectControls::ExampleEffectControls(ExampleEffectEffect* effect) :
	EffectControls(effect),
	m_effect(effect),
	m_volumeModel(100.0f, 0.0f, 200.0f, 0.001f, this, tr("Volume")),
	m_decayModel(1.0f, 0.0f, 1.0f, 0.001f, this, tr("Decay")),
	m_invertModel(false, this, tr("Invert")),
	m_numberModel(1, 1, 10, this, tr("Number")),
	m_faderModel(0.0f, -10.0f, 10.0f, 0.001f, this, tr("Fader"))
{
}


void ExampleEffectControls::loadSettings(const QDomElement& parent)
{
	m_volumeModel.loadSettings(parent, "volume");
	m_decayModel.loadSettings(parent, "decay");
	m_invertModel.loadSettings(parent, "invert");
	m_numberModel.loadSettings(parent, "num");
	m_faderModel.loadSettings(parent, "fader");
}


void ExampleEffectControls::saveSettings(QDomDocument& doc, QDomElement& parent)
{
	m_volumeModel.saveSettings(doc, parent, "volume");
	m_decayModel.saveSettings(doc, parent, "volume");
	m_invertModel.saveSettings(doc, parent, "invert");
	m_numberModel.saveSettings(doc, parent, "num");
	m_faderModel.saveSettings(doc, parent, "fader");
}


} // namespace lmms
