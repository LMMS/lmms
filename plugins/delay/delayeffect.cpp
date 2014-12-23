/*
 * delayeffect.cpp - definition of the DelayEffect class. The Delay Plugin
 *
 * Copyright (c) 2014 David French <dave/dot/french3/at/googlemail/dot/com>
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

#include "delayeffect.h"
#include "engine.h"
#include "embed.cpp"


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
	new PluginPixmapLoader( "logo" ),
	NULL,
	NULL
} ;




DelayEffect::DelayEffect( Model* parent, const Plugin::Descriptor::SubPluginFeatures::Key* key ) :
	Effect( &delay_plugin_descriptor, parent, key ),
	m_delayControls( this )
{
	m_delay = 0;
	m_delay = new StereoDelay( 20, engine::mixer()->processingSampleRate() );
	m_lfo = new Lfo( engine::mixer()->processingSampleRate() );
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
	const float d = dryLevel();
	const float w = wetLevel();
	const float length = m_delayControls.m_delayTimeModel.value() * engine::mixer()->processingSampleRate();
	const float amplitude = m_delayControls.m_lfoAmountModel.value() * engine::mixer()->processingSampleRate();
	m_lfo->setFrequency( 1.0 / m_delayControls.m_lfoTimeModel.value() );
	m_delay->setFeedback( m_delayControls.m_feedbackModel.value() );
	sample_t dryS[2];
	for( fpp_t f = 0; f < frames; ++f )
	{
		dryS[0] = buf[f][0];
		dryS[1] = buf[f][1];
		m_delay->setLength( ( float )length + ( amplitude * ( float )m_lfo->tick() ) );
		m_delay->tick( buf[f] );

		buf[f][0] = ( d * dryS[0] ) + ( w * buf[f][0] );
		buf[f][1] = ( d * dryS[1] ) + ( w * buf[f][1] );
		outSum += buf[f][0]*buf[f][0] + buf[f][1]*buf[f][1];
	}
	checkGate( outSum / frames );
	return isRunning();
}

void DelayEffect::changeSampleRate()
{
	m_lfo->setSampleRate( engine::mixer()->processingSampleRate() );
	m_delay->setSampleRate( engine::mixer()->processingSampleRate() );
}




extern "C"
{

//needed for getting plugin out of shared lib
Plugin * PLUGIN_EXPORT lmms_plugin_main( Model* parent, void* data )
{
	return new DelayEffect( parent , static_cast<const Plugin::Descriptor::SubPluginFeatures::Key *>( data ) );
}

}}

