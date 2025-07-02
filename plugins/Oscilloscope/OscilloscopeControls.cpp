/*
 * OscilloscopeControls.cpp - Oscilloscope effect controls/models
 *
 * Copyright (c) 2025 Keratin
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

#include "Oscilloscope.h"
#include "OscilloscopeControls.h"

namespace lmms
{

OscilloscopeControls::OscilloscopeControls(Oscilloscope* effect) :
	EffectControls(effect),
	m_ampModel(100.0f, 0.0f, 1000.0f, 0.00001f, this, tr("Amplitude")),
	m_lengthModel(10000.0f, 10.0f, Oscilloscope::BUFFER_SIZE, 1.0f, this, tr("Length")),
	m_phaseModel(1.0f, 0.0f, 1.0f, 0.00001f, this, tr("Phase")),
	m_pauseModel(false, this, tr("Pause")),
	m_stereoModel(false, this, tr("Stereo")),
	m_effect(effect)
{
	m_lengthModel.setScaleLogarithmic(true);
}


void OscilloscopeControls::loadSettings(const QDomElement& parent)
{
	m_ampModel.loadSettings(parent, "amp");
	m_lengthModel.loadSettings(parent, "length");
	m_phaseModel.loadSettings(parent, "phase");
	m_pauseModel.loadSettings(parent, "pause");
	m_stereoModel.loadSettings(parent, "stereo");
}


void OscilloscopeControls::saveSettings(QDomDocument& doc, QDomElement& parent)
{
	m_ampModel.saveSettings(doc, parent, "amp");
	m_lengthModel.saveSettings(doc, parent, "length");
	m_phaseModel.saveSettings(doc, parent, "phase");
	m_pauseModel.saveSettings(doc, parent, "pause");
	m_stereoModel.saveSettings(doc, parent, "stereo");
}


} // namespace lmms
