/*
 * BassBooster.cpp - bass booster effect plugin
 *
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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

#include "BassBooster.h"

#include "embed.cpp"


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT bassbooster_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"BassBooster",
	QT_TRANSLATE_NOOP( "pluginBrowser", "Boost your bass the fast and simple way" ),
	"Tobias Doerffel <tobydox/at/users.sf.net>",
	0x0100,
	Plugin::Effect,
	new PluginPixmapLoader( "logo" ),
	NULL,
	NULL
} ;

}



BassBoosterEffect::BassBoosterEffect( Model* parent, const Descriptor::SubPluginFeatures::Key* key ) :
	Effect( &bassbooster_plugin_descriptor, parent, key ),
	m_frequencyChangeNeeded( false ),
	m_bbFX( DspEffectLibrary::FastBassBoost( 70.0f, 1.0f, 2.8f ) ),
	m_bbControls( this )
{
	changeFrequency();
	changeGain();
	changeRatio();
}




BassBoosterEffect::~BassBoosterEffect()
{
}




bool BassBoosterEffect::processAudioBuffer( sampleFrame* buf, const fpp_t frames )
{
	if( !isEnabled() || !isRunning () )
	{
		return( false );
	}
	// check out changed controls
	if( m_frequencyChangeNeeded || m_bbControls.m_freqModel.isValueChanged() )
	{
		changeFrequency();
		m_frequencyChangeNeeded = false;
	}
	if( m_bbControls.m_gainModel.isValueChanged() ) { changeGain(); }
	if( m_bbControls.m_ratioModel.isValueChanged() ) { changeRatio(); }

	double outSum = 0.0;
	const float d = dryLevel();
	const float w = wetLevel();
	for( fpp_t f = 0; f < frames; ++f )
	{
		outSum += buf[f][0]*buf[f][0] + buf[f][1]*buf[f][1];

		sample_t s[2] = { buf[f][0], buf[f][1] };
		m_bbFX.nextSample( s[0], s[1] );

		buf[f][0] = d * buf[f][0] + w * s[0];
		buf[f][1] = d * buf[f][1] + w * s[1];
	}

	checkGate( outSum / frames );

	return isRunning();
}


inline void BassBoosterEffect::changeFrequency()
{
	const sample_t fac = engine::mixer()->processingSampleRate() / 44100.0f;

	m_bbFX.leftFX().setFrequency( m_bbControls.m_freqModel.value() * fac );
	m_bbFX.rightFX().setFrequency( m_bbControls.m_freqModel.value() * fac );
}




inline void BassBoosterEffect::changeGain()
{
	m_bbFX.leftFX().setGain( m_bbControls.m_gainModel.value() );
	m_bbFX.rightFX().setGain( m_bbControls.m_gainModel.value() );
}




inline void BassBoosterEffect::changeRatio()
{
	m_bbFX.leftFX().setRatio( m_bbControls.m_ratioModel.value() );
	m_bbFX.rightFX().setRatio( m_bbControls.m_ratioModel.value() );
}




extern "C"
{

// necessary for getting instance out of shared lib
Plugin * PLUGIN_EXPORT lmms_plugin_main( Model* parent, void* data )
{
	return new BassBoosterEffect( parent, static_cast<const Plugin::Descriptor::SubPluginFeatures::Key *>( data ) );
}

}

