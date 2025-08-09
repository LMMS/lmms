/*
 * delaycontrols.cpp - definition of DelayControls class.
 *
 * Copyright (c) 2014 David French <dave/dot/french3/at/googlemail/dot/com>
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


#include "DelayControls.h"
#include "DelayEffect.h"
#include "Engine.h"

namespace lmms
{

DelayControls::DelayControls( DelayEffect* effect ):
	EffectControls( effect ),
	m_effect ( effect ),
	m_delayTimeModel(0.5f, 0.01f, 5.f, 0.0001f, 5000.f, this, tr("Delay samples")) ,
	m_feedbackModel(0.0f, 0.0f, 1.0f, 0.01f, this, tr("Feedback")),
	m_lfoTimeModel(2.f, 0.01f, 5.f, 0.0001f, 20000.f, this, tr("LFO frequency")),
	m_lfoAmountModel(0.f, 0.f, 0.5f, 0.0001f, 2000.f, this, tr("LFO amount")),
	m_outGainModel(0.f, -60.f, 20.f, 0.01f, this, tr("Output gain"))
{
	connect( Engine::audioEngine(), SIGNAL( sampleRateChanged() ), this, SLOT( changeSampleRate() ) );
	m_outPeakL = 0.0;
	m_outPeakR = 0.0;
}




void DelayControls::loadSettings( const QDomElement &_this )
{
	m_delayTimeModel.loadSettings(_this, "DelayTimeSamples" );
	m_feedbackModel.loadSettings( _this, "FeebackAmount" );
	m_lfoTimeModel.loadSettings( _this , "LfoFrequency");
	m_lfoAmountModel.loadSettings( _this, "LfoAmount");
	m_outGainModel.loadSettings( _this, "OutGain" );
}




void DelayControls::saveSettings( QDomDocument& doc, QDomElement& _this )
{
	m_delayTimeModel.saveSettings( doc, _this, "DelayTimeSamples" );
	m_feedbackModel.saveSettings( doc, _this ,"FeebackAmount" );
	m_lfoTimeModel.saveSettings( doc, _this, "LfoFrequency" );
	m_lfoAmountModel.saveSettings( doc, _this ,"LfoAmount" );
	m_outGainModel.saveSettings( doc, _this, "OutGain" );
}



void DelayControls::changeSampleRate()
{
	m_effect->changeSampleRate();
}


} // namespace lmms
