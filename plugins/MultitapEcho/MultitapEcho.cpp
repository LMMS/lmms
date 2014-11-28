/*
 * MultitapEcho.cpp - a multitap echo delay plugin
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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

#include "MultitapEcho.h"
#include "embed.cpp"


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT multitapecho_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"Multitap Echo",
	QT_TRANSLATE_NOOP( "pluginBrowser", "A multitap echo delay plugin" ),
	"Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>",
	0x0100,
	Plugin::Effect,
	new PluginPixmapLoader( "logo" ),
	NULL,
	NULL
} ;

}


MultitapEchoEffect::MultitapEchoEffect( Model* parent, const Descriptor::SubPluginFeatures::Key* key ) :
	Effect( &multitapecho_plugin_descriptor, parent, key ),
	m_controls( this ),
	m_buffer( 20100.0f ),
	m_sampleRate( Engine::mixer()->processingSampleRate() ),
	m_sampleRatio( 1.0f / m_sampleRate )
{
	m_work = new sampleFrame[ Engine::mixer()->framesPerPeriod() ];
	m_buffer.reset();
	updateFilters( 0, 19 );
}


MultitapEchoEffect::~MultitapEchoEffect()
{
	delete m_work;
}


void MultitapEchoEffect::updateFilters( int begin, int end )
{
	for( int i = begin; i <= end; ++i )
	{ 
		m_filter[i][0].setFc( m_lpFreq[i] * m_sampleRatio );
		m_filter[i][1].setFc( m_lpFreq[i] * m_sampleRatio );
	}
}


void MultitapEchoEffect::runFilter( sampleFrame * dst, sampleFrame * src, StereoOnePole & filter, const fpp_t frames )
{
	for( int f = 0; f < frames; ++f )
	{
		dst[f][0] = filter[0].update( src[f][0] );
		dst[f][1] = filter[1].update( src[f][1] );
	}
}


bool MultitapEchoEffect::processAudioBuffer( sampleFrame * buf, const fpp_t frames )
{
	if( !isEnabled() || !isRunning () )
	{
		return( false );
	}
	
	double outSum = 0.0;
	const float d = dryLevel();
	const float w = wetLevel();

	// get processing vars
	const int steps = m_controls.m_steps.value();
	const float stepLength = m_controls.m_stepLength.value();
	const float dryGain = dbvToAmp( m_controls.m_dryGain.value() );
	const bool swapInputs = m_controls.m_swapInputs.value();
	
	// add dry buffer - never swap inputs for dry
	m_buffer.writeAddingMultiplied( buf, 0, frames, dryGain );
	
	// swapped inputs?
	if( swapInputs )
	{
		float offset = stepLength;
		for( int i = 0; i < steps; ++i ) // add all steps swapped
		{
			runFilter( m_work, buf, m_filter[i], frames );
			m_buffer.writeSwappedAddingMultiplied( m_work, offset, frames, m_amp[i] );
			offset += stepLength;
		}
	}
	else
	{
		float offset = stepLength;
		for( int i = 0; i < steps; ++i ) // add all steps swapped
		{
			runFilter( m_work, buf, m_filter[i], frames );
			m_buffer.writeAddingMultiplied( m_work, offset, frames, m_amp[i] );
			offset += stepLength;
		}
	}
	
	// pop the buffer and mix it into output
	m_buffer.pop( m_work );

	for( int f = 0; f < frames; ++f )
	{
		buf[f][0] = d * buf[f][0] + w * m_work[f][0];
		buf[f][1] = d * buf[f][1] + w * m_work[f][1];
		outSum += buf[f][0]*buf[f][0] + buf[f][1]*buf[f][1];
	}
	
	checkGate( outSum / frames );

	return isRunning();	
}


extern "C"
{

// necessary for getting instance out of shared lib
Plugin * PLUGIN_EXPORT lmms_plugin_main( Model* parent, void* data )
{
	return new MultitapEchoEffect( parent, static_cast<const Plugin::Descriptor::SubPluginFeatures::Key *>( data ) );
}

}
