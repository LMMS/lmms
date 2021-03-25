/*
 * flangereffect.cpp - defination of FlangerEffect class.
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

#include "FlangerEffect.h"
#include "Engine.h"

#include "embed.h"
#include "plugin_export.h"

extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT flanger_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"Flanger",
	QT_TRANSLATE_NOOP( "PluginBrowser", "A native flanger plugin" ),
	"Dave French <contact/dot/dave/dot/french3/at/googlemail/dot/com>",
	0x0100,
	Plugin::Effect,
	new PluginPixmapLoader("logo"),
	NULL,
	NULL
} ;




FlangerEffect::FlangerEffect( Model *parent, const Plugin::Descriptor::SubPluginFeatures::Key *key ) :
	Effect( &flanger_plugin_descriptor, parent, key ),
	m_flangerControls( this )
{
	m_lfo = new QuadratureLfo( Engine::mixer()->processingSampleRate() );
	m_lDelay = new MonoDelay( 1, Engine::mixer()->processingSampleRate() );
	m_rDelay = new MonoDelay( 1, Engine::mixer()->processingSampleRate() );
	m_noise = new Noise;
}




FlangerEffect::~FlangerEffect()
{
	if(m_lDelay )
	{
		delete m_lDelay;
	}
	if( m_rDelay )
	{
		delete m_rDelay;
	}
	if( m_lfo )
	{
		delete m_lfo;
	}
	if(m_noise)
	{
		delete m_noise;
	}
}




bool FlangerEffect::processAudioBuffer( sampleFrame *buf, const fpp_t frames )
{
	if( !isEnabled() || !isRunning () )
	{
		return( false );
	}
	double outSum = 0.0;
	const float d = dryLevel();
	const float w = wetLevel();
	const float length = m_flangerControls.m_delayTimeModel.value() * Engine::mixer()->processingSampleRate();
	const float noise = m_flangerControls.m_whiteNoiseAmountModel.value();
	float amplitude = m_flangerControls.m_lfoAmountModel.value() * Engine::mixer()->processingSampleRate();
	bool invertFeedback = m_flangerControls.m_invertFeedbackModel.value();
	m_lfo->setFrequency(  1.0/m_flangerControls.m_lfoFrequencyModel.value() );
	m_lfo->setOffset( m_flangerControls.m_lfoPhaseModel.value() / 180 * D_PI );
	m_lDelay->setFeedback( m_flangerControls.m_feedbackModel.value() );
	m_rDelay->setFeedback( m_flangerControls.m_feedbackModel.value() );
	sample_t dryS[2];
	float leftLfo;
	float rightLfo;
	for( fpp_t f = 0; f < frames; ++f )
	{
		buf[f][0] += m_noise->tick() * noise;
		buf[f][1] += m_noise->tick() * noise;
		dryS[0] = buf[f][0];
		dryS[1] = buf[f][1];
		m_lfo->tick(&leftLfo, &rightLfo);
		m_lDelay->setLength( ( float )length + amplitude * (leftLfo+1.0)  );
		m_rDelay->setLength( ( float )length + amplitude * (rightLfo+1.0)  );
		if(invertFeedback)
		{
			m_lDelay->tick( &buf[f][1] );
			m_rDelay->tick(&buf[f][0] );
		} else
		{
			m_lDelay->tick( &buf[f][0] );
			m_rDelay->tick( &buf[f][1] );
		}

		buf[f][0] = ( d * dryS[0] ) + ( w * buf[f][0] );
		buf[f][1] = ( d * dryS[1] ) + ( w * buf[f][1] );
		outSum += buf[f][0]*buf[f][0] + buf[f][1]*buf[f][1];
	}
	checkGate( outSum / frames );
	return isRunning();
}




void FlangerEffect::changeSampleRate()
{
	m_lfo->setSampleRate( Engine::mixer()->processingSampleRate() );
	m_lDelay->setSampleRate( Engine::mixer()->processingSampleRate() );
	m_rDelay->setSampleRate( Engine::mixer()->processingSampleRate() );
}




void FlangerEffect::restartLFO()
{
	m_lfo->restart();
}




extern "C"
{

//needed for getting plugin out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model* parent, void* data )
{
	return new FlangerEffect( parent , static_cast<const Plugin::Descriptor::SubPluginFeatures::Key *>( data ) );
}

}}
