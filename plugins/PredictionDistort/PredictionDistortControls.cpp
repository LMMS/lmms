/*
 * PredictionDistortControls.cpp - controls for PredictionDistort effect
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

#include "PredictionDistortControls.h"
#include "PredictionDistort.h"

namespace lmms
{

PredictionDistortControls::PredictionDistortControls(PredictionDistortEffect* effect) :
	EffectControls(effect),
	m_effect(effect),
	m_mixModel(1.0f, 0.0f, 1.0f, 0.001f, this, tr("Mix")),
	m_decayModel(1, 1, 40, this, tr("Decay")),
	m_rangeModel(1, 1, 500, this, tr("Range")),
	m_isReverseModel(false, this, tr("Reversed"))
{
}


void PredictionDistortControls::loadSettings(const QDomElement& parent)
{
	m_mixModel.loadSettings(parent, "mix");
	m_decayModel.loadSettings(parent, "decay");
	m_rangeModel.loadSettings(parent, "range");
	m_isReverseModel.loadSettings(parent, "reversed");
}


void PredictionDistortControls::saveSettings(QDomDocument& doc, QDomElement& parent)
{
	m_mixModel.saveSettings(doc, parent, "mix"); 
	m_decayModel.saveSettings(doc, parent, "decay");
	m_rangeModel.saveSettings(doc, parent, "range");
	m_isReverseModel.saveSettings(doc, parent, "reversed");
}


} // namespace lmms
