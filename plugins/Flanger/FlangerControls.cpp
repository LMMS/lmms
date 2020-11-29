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

#include <QtXml/QDomElement>

#include "FlangerControls.h"
#include "FlangerEffect.h"
#include "Engine.h"
#include "Song.h"



FlangerControls::FlangerControls( FlangerEffect *effect ) :
	EffectControls ( effect ),
	m_effect ( effect ),
	m_delayTimeModel(0.001, 0.0001, 0.050, 0.0001,  this, tr( "Delay samples" ) ) ,
	m_lfoFrequencyModel( 0.25, 0.01, 60, 0.0001, 60000.0 ,this, tr( "LFO frequency" ) ),
	m_lfoAmountModel( 0.0, 0.0, 0.0025 , 0.0001 , this , tr( "Seconds" ) ),
	m_feedbackModel( 0.0 , 0.0 , 1.0 , 0.0001, this, tr( "Regen" ) ),
	m_whiteNoiseAmountModel( 0.0 , 0.0 , 0.05 , 0.0001, this, tr( "Noise" ) ),
	m_invertFeedbackModel ( false , this, tr( "Invert" ) )

{
	connect( Engine::mixer(), SIGNAL( sampleRateChanged() ), this, SLOT( changedSampleRate() ) );
	connect( Engine::getSong(), SIGNAL( playbackStateChanged() ), this, SLOT( changedPlaybackState() ) );
}




void FlangerControls::loadSettings( const QDomElement &_this )
{
	m_delayTimeModel.loadSettings( _this, "DelayTimeSamples" );
	m_lfoFrequencyModel.loadSettings( _this, "LfoFrequency" );
	m_lfoAmountModel.loadSettings( _this, "LfoAmount" );
	m_feedbackModel.loadSettings( _this, "Feedback" );
	m_whiteNoiseAmountModel.loadSettings( _this, "WhiteNoise" );
	m_invertFeedbackModel.loadSettings( _this, "Invert" );

}




void FlangerControls::saveSettings( QDomDocument &doc, QDomElement &parent )
{
	m_delayTimeModel.saveSettings( doc , parent, "DelayTimeSamples" );
	m_lfoFrequencyModel.saveSettings( doc, parent , "LfoFrequency" );
	m_lfoAmountModel.saveSettings( doc, parent , "LfoAmount" );
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
