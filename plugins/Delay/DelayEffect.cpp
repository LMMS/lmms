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
#include "Lfo.h"
#include "lmms_math.h"
#include "plugin_export.h"
#include "StereoDelay.h"

namespace lmms
{


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT delay_plugin_descriptor =
{
	LMMS_STRINGIFY( PLUGIN_NAME ),
	"Delay",
	QT_TRANSLATE_NOOP( "PluginBrowser", "A native delay plugin" ),
	"Dave French <contact/dot/dave/dot/french3/at/googlemail/dot/com>",
	0x0100,
	Plugin::Type::Effect,
	new PluginPixmapLoader("logo"),
	{},
	nullptr,
} ;




DelayEffect::DelayEffect( Model* parent, const Plugin::Descriptor::SubPluginFeatures::Key* key ) :
	Effect( &delay_plugin_descriptor, parent, key ),
	m_delayControls( this )
{
	m_delay = 0;
	m_delay = new StereoDelay( 20, Engine::audioEngine()->outputSampleRate() );
	m_lfo = new Lfo( Engine::audioEngine()->outputSampleRate() );
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




Effect::ProcessStatus DelayEffect::processImpl(SampleFrame* buf, const fpp_t frames)
{
	const float sr = Engine::audioEngine()->outputSampleRate();
	const float d = dryLevel();
	const float w = wetLevel();

	SampleFrame peak;
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

	for (fpp_t f = 0; f < frames; ++f)
	{
		auto& currentFrame = buf[f];
		const auto dryS = currentFrame;

		// Prepare delay for current sample
		m_delay->setFeedback( *feedbackPtr );
		m_lfo->setFrequency( *lfoTimePtr );
		m_currentLength = static_cast<int>(*lengthPtr * Engine::audioEngine()->outputSampleRate());
		m_delay->setLength( m_currentLength + ( *amplitudePtr * ( float )m_lfo->tick() ) );

		// Process the wet signal
		m_delay->tick( currentFrame );
		currentFrame *= m_outGain;

		// Calculate peak of wet signal
		peak = peak.absMax(currentFrame);

		// Dry/wet mix
		currentFrame = dryS * d + currentFrame * w;

		lengthPtr += lengthInc;
		amplitudePtr += amplitudeInc;
		lfoTimePtr += lfoTimeInc;
		feedbackPtr += feedbackInc;
	}

	m_delayControls.m_outPeakL = peak.left();
	m_delayControls.m_outPeakR = peak.right();

	return ProcessStatus::ContinueIfNotQuiet;
}

void DelayEffect::changeSampleRate()
{
	m_lfo->setSampleRate( Engine::audioEngine()->outputSampleRate() );
	m_delay->setSampleRate( Engine::audioEngine()->outputSampleRate() );
}




extern "C"
{

//needed for getting plugin out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model* parent, void* data )
{
	return new DelayEffect( parent , static_cast<const Plugin::Descriptor::SubPluginFeatures::Key *>( data ) );
}

}}


} // namespace lmms
