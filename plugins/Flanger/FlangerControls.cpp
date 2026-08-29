/*
 * flangercontrols.cpp - defination of FlangerControls class.
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


#include "FlangerControls.h"
#include "FlangerEffect.h"
#include "Engine.h"
#include "Song.h"

namespace lmms
{


FlangerControls::FlangerControls( FlangerEffect *effect ) :
	EffectControls ( effect ),
	m_effect ( effect ),
	m_delayTimeModel(0.001f, 0.0001f, 0.050f, 0.0001f, this, tr("Delay samples")),
	m_lfoFrequencyModel(0.25f, 0.01f, 60, 0.0001f, 60000.f, this, tr("LFO frequency")),
	m_lfoAmountModel(0.f, 0.f, 0.0025f, 0.0001f, this, tr("Amount")),
	m_lfoPhaseModel(90.f, 0.f, 360.f, 0.0001f, this, tr("Stereo phase")),
	m_feedbackModel(0.f, -1.f, 1.f, 0.0001f, this, tr("Feedback")),
	m_whiteNoiseAmountModel(0.f, 0.f, 0.05f, 0.0001f, this, tr("Noise")),
	m_invertFeedbackModel ( false, this, tr( "Invert" ) )

{
	connect( Engine::audioEngine(), SIGNAL( sampleRateChanged() ), this, SLOT( changedSampleRate() ) );
	connect( Engine::getSong(), SIGNAL( playbackStateChanged() ), this, SLOT( changedPlaybackState() ) );
}




void FlangerControls::loadSettings( const QDomElement &_this )
{
	m_delayTimeModel.loadSettings( _this, "DelayTimeSamples" );
	m_lfoFrequencyModel.loadSettings( _this, "LfoFrequency" );
	m_lfoAmountModel.loadSettings( _this, "LfoAmount" );
	m_lfoPhaseModel.loadSettings( _this, "LfoPhase" );
	m_feedbackModel.loadSettings( _this, "Feedback" );
	m_whiteNoiseAmountModel.loadSettings( _this, "WhiteNoise" );
	m_invertFeedbackModel.loadSettings( _this, "Invert" );

}




void FlangerControls::saveSettings( QDomDocument &doc, QDomElement &parent )
{
	m_delayTimeModel.saveSettings( doc , parent, "DelayTimeSamples" );
	m_lfoFrequencyModel.saveSettings( doc, parent , "LfoFrequency" );
	m_lfoAmountModel.saveSettings( doc, parent , "LfoAmount" );
	m_lfoPhaseModel.saveSettings( doc, parent , "LfoPhase" );
	m_feedbackModel.saveSettings( doc, parent, "Feedback" ) ;
	m_whiteNoiseAmountModel.saveSettings( doc, parent , "WhiteNoise" ) ;
	m_invertFeedbackModel.saveSettings( doc, parent, "Invert" );
}




void FlangerControls::changedSampleRate()
{
	m_effect->changeSampleRate();
}




void FlangerControls::changedPlaybackState()
{
	m_effect->restartLFO();
}


} // namespace lmms
