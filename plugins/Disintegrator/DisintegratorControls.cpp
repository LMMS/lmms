/*
 * DisintegratorControls.cpp
 *
 * Copyright (c) 2019 Lost Robot <r94231@gmail.com>
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

#include "DisintegratorControls.h"
#include "Disintegrator.h"

#include "Engine.h"
#include "Song.h"


DisintegratorControls::DisintegratorControls(DisintegratorEffect* effect) :
	EffectControls(effect),
	m_effect(effect),
	m_lowCutModel(20.0f, 20.0f, 20000.0f, 0.01f, this, tr("Low Cut Frequency")),
	m_highCutModel(20000.0f, 20.0f, 20000.0f, 0.01f, this, tr("High Cut Frequency")),
	m_amountModel(1.6f, 0.0f, 10.0f, 0.001f, this, tr("Amount")),
	m_typeModel(this, tr("Type")),
	m_freqModel(100.0f, 1.0f, 21050.0f, 0.01f, this, tr("Frequency"))
{
	// All of these are much easier to tweak when logarithmic
	m_lowCutModel.setScaleLogarithmic(true);
	m_highCutModel.setScaleLogarithmic(true);
	m_amountModel.setScaleLogarithmic(true);
	m_freqModel.setScaleLogarithmic(true);

	m_typeModel.addItem(tr("Mono Noise"));
	m_typeModel.addItem(tr("Stereo Noise"));
	m_typeModel.addItem(tr("Sine Wave"));
	m_typeModel.addItem(tr("Self-Modulation"));

	connect(Engine::mixer(), SIGNAL(sampleRateChanged()), this, SLOT(sampleRateChanged()));
}


void DisintegratorControls::saveSettings(QDomDocument& doc, QDomElement& _this)
{
	m_lowCutModel.saveSettings(doc, _this, "lowCut"); 
	m_highCutModel.saveSettings(doc, _this, "highCut");
	m_amountModel.saveSettings(doc, _this, "amount");
	m_typeModel.saveSettings(doc, _this, "type");
	m_freqModel.saveSettings(doc, _this, "freq");
}



void DisintegratorControls::loadSettings(const QDomElement& _this)
{
	m_lowCutModel.loadSettings(_this, "lowCut");
	m_highCutModel.loadSettings(_this, "highCut");
	m_amountModel.loadSettings(_this, "amount");
	m_typeModel.loadSettings(_this, "type");
	m_freqModel.loadSettings(_this, "freq");

	m_effect->m_needsUpdate = true;
	m_effect->clearFilterHistories();
}



void DisintegratorControls::sampleRateChanged()
{
	m_effect->sampleRateChanged();
}
