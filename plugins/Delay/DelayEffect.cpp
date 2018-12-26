/*
 * delayeffect.cpp - definition of the DelayEffect class. The Delay Plugin
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

#include "DelayEffect.h"
#include "Engine.h"
#include "embed.h"
#include "interpolation.h"
#include "plugin_export.h"

extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT delay_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"Delay",
	QT_TRANSLATE_NOOP( "pluginBrowser", "A native delay plugin" ),
	"Dave French <contact/dot/dave/dot/french3/at/googlemail/dot/com>",
	0x0100,
	Plugin::Effect,
	new PluginPixmapLoader("logo"),
	NULL,
	NULL
} ;




DelayEffect::DelayEffect( Model* parent, const Plugin::Descriptor::SubPluginFeatures::Key* key ) :
	Effect( &delay_plugin_descriptor, parent, key ),
	m_delayControls( this )
{
	m_delay = 0;
	m_delay = new StereoDelay( 20, Engine::mixer()->processingSampleRate() );
	m_lfo = new Lfo( Engine::mixer()->processingSampleRate() );
	m_outGain = 1.0;
}




DelayEffect::~DelayEffect()
{
	if( m_delay )
	{
		delete m_delay;
	}
	if( m_lfo )
	{
		delete m_lfo;
	}
}




bool DelayEffect::processAudioBuffer( sampleFrame* buf, const fpp_t frames )
{
	if( !isEnabled() || !isRunning () )
	{
		return( false );
	}
	double outSum = 0.0;
	const float sr = Engine::mixer()->processingSampleRate();
	const float d = dryLevel();
	const float w = wetLevel();
	sample_t dryS[2];
	float lPeak = 0.0;
	float rPeak = 0.0;
	float length = m_delayControls.m_delayTimeModel.value();
	float amplitude = m_delayControls.m_lfoAmountModel.value() * sr;
	float lfoTime = 1.0 / m_delayControls.m_lfoTimeModel.value();
	float feedback =  m_delayControls.m_feedbackModel.value();
	ValueBuffer *lengthBuffer = m_delayControls.m_delayTimeModel.valueBuffer();
	ValueBuffer *feedbackBuffer = m_delayControls.m_feedbackModel.valueBuffer();
	ValueBuffer *lfoTimeBuffer = m_delayControls.m_lfoTimeModel.valueBuffer();
	ValueBuffer *lfoAmountBuffer = m_delayControls.m_lfoAmountModel.valueBuffer();
	int lengthInc = lengthBuffer ? 1 : 0;
	int amplitudeInc = lfoAmountBuffer ? 1 : 0;
	int lfoTimeInc = lfoTimeBuffer ? 1 : 0;
	int feedbackInc = feedbackBuffer ? 1 : 0;
	float *lengthPtr = lengthBuffer ? &( lengthBuffer->values()[ 0 ] ) : &length;
	float *amplitudePtr = lfoAmountBuffer ? &( lfoAmountBuffer->values()[ 0 ] ) : &amplitude;
	float *lfoTimePtr = lfoTimeBuffer ? &( lfoTimeBuffer->values()[ 0 ] ) : &lfoTime;
	float *feedbackPtr = feedbackBuffer ? &( feedbackBuffer->values()[ 0 ] ) : &feedback;

	if( m_delayControls.m_outGainModel.isValueChanged() )
	{
		m_outGain = dbfsToAmp( m_delayControls.m_outGainModel.value() );
	}
	int sampleLength;
	for( fpp_t f = 0; f < frames; ++f )
	{
		dryS[0] = buf[f][0];
		dryS[1] = buf[f][1];

		m_delay->setFeedback( *feedbackPtr );
		m_lfo->setFrequency( *lfoTimePtr );
		sampleLength = *lengthPtr * Engine::mixer()->processingSampleRate();
		m_currentLength = sampleLength;
		m_delay->setLength( m_currentLength + ( *amplitudePtr * ( float )m_lfo->tick() ) );
		m_delay->tick( buf[f] );

		buf[f][0] *= m_outGain;
		buf[f][1] *= m_outGain;

		lPeak = buf[f][0] > lPeak ? buf[f][0] : lPeak;
		rPeak = buf[f][1] > rPeak ? buf[f][1] : rPeak;

		buf[f][0] = ( d * dryS[0] ) + ( w * buf[f][0] );
		buf[f][1] = ( d * dryS[1] ) + ( w * buf[f][1] );
		outSum += buf[f][0]*buf[f][0] + buf[f][1]*buf[f][1];

		lengthPtr += lengthInc;
		amplitudePtr += amplitudeInc;
		lfoTimePtr += lfoTimeInc;
		feedbackPtr += feedbackInc;
	}
	checkGate( outSum / frames );
	m_delayControls.m_outPeakL = lPeak;
	m_delayControls.m_outPeakR = rPeak;

	return isRunning();
}

void DelayEffect::changeSampleRate()
{
	m_lfo->setSampleRate( Engine::mixer()->processingSampleRate() );
	m_delay->setSampleRate( Engine::mixer()->processingSampleRate() );
}




extern "C"
{

//needed for getting plugin out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model* parent, void* data )
{
	return new DelayEffect( parent , static_cast<const Plugin::Descriptor::SubPluginFeatures::Key *>( data ) );
}

}}

