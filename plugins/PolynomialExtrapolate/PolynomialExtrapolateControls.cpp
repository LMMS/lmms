/*
 * PolynomialExtrapolateControls.cpp - controls for PolynomialExtrapolate effect
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

#include "PolynomialExtrapolateControls.h"
#include "PolynomialExtrapolate.h"

namespace lmms
{

PolynomialExtrapolateControls::PolynomialExtrapolateControls(PolynomialExtrapolateEffect* effect) :
	EffectControls(effect),
	m_effect(effect),
	m_mixModel(1.0f, 0.0f, 1.0f, 0.001f, this, tr("Mix")),
	m_degreeModel(1, 1, 40, this, tr("Degree")),
	m_gapModel(1, 1, 500, this, tr("Gap")),
	m_predictionCountModel(1, 1, 500, this, tr("Prediction count")),
	m_xMultiplierModel(1.0f, 0.0f, 5.0f, 0.001f, this, tr("X multiplier")),
	m_feedbackModel(0.0f, 0.0f, 1.0f, 0.001f, this, tr("feedback")),
	m_reusePercentModel(0.0f, 0.0f, 1.0f, 0.001f, this, tr("reuse")),
	m_volumeModel(1.0f, 0.0f, 1.0f, 0.001f, this, tr("volume"))
{
}


void PolynomialExtrapolateControls::loadSettings(const QDomElement& parent)
{
	m_mixModel.loadSettings(parent, "mix");
	m_degreeModel.loadSettings(parent, "degree");
	m_gapModel.loadSettings(parent, "gap");
	m_predictionCountModel.loadSettings(parent, "predictionCount");
	m_xMultiplierModel.loadSettings(parent, "xMultiplier");
	m_feedbackModel.loadSettings(parent, "feedback");
	m_reusePercentModel.loadSettings(parent, "reuse");
	m_volumeModel.loadSettings(parent, "volume");
}


void PolynomialExtrapolateControls::saveSettings(QDomDocument& doc, QDomElement& parent)
{
	m_mixModel.saveSettings(doc, parent, "mix"); 
	m_degreeModel.saveSettings(doc, parent, "degree");
	m_gapModel.saveSettings(doc, parent, "gap");
	m_predictionCountModel.saveSettings(doc, parent, "predictionCount");
	m_xMultiplierModel.saveSettings(doc, parent, "xMultiplier");
	m_feedbackModel.saveSettings(doc, parent, "feedback");
	m_reusePercentModel.saveSettings(doc, parent, "reuse");
	m_volumeModel.saveSettings(doc, parent, "volume");
}


} // namespace lmms
