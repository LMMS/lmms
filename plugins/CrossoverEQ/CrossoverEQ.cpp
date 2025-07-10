/*
 * CrossoverEQ.cpp - A native 4-band Crossover Equalizer 
 * good for simulating tonestacks or simple peakless (flat-band) equalization
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 
#include "CrossoverEQ.h"
#include "lmms_math.h"
#include "embed.h"
#include "plugin_export.h"

namespace lmms
{


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT crossovereq_plugin_descriptor =
{
	LMMS_STRINGIFY( PLUGIN_NAME ),
	"Crossover Equalizer",
	QT_TRANSLATE_NOOP( "PluginBrowser", "A 4-band Crossover Equalizer" ),
	"Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>",
	0x0100,
	Plugin::Type::Effect,
	new PluginPixmapLoader( "logo" ),
	nullptr,
	nullptr,
	nullptr,
};

}


CrossoverEQEffect::CrossoverEQEffect( Model* parent, const Descriptor::SubPluginFeatures::Key* key ) :
	Effect( &crossovereq_plugin_descriptor, parent, key ),
	m_controls( this ),
	m_sampleRate( Engine::audioEngine()->outputSampleRate() ),
	m_lp1( m_sampleRate ),
	m_lp2( m_sampleRate ),
	m_lp3( m_sampleRate ),
	m_hp2( m_sampleRate ),
	m_hp3( m_sampleRate ),
	m_hp4( m_sampleRate ),
	m_needsUpdate( true )
{
	m_tmp2 = new SampleFrame[Engine::audioEngine()->framesPerPeriod()];
	m_tmp1 = new SampleFrame[Engine::audioEngine()->framesPerPeriod()];
	m_work = new SampleFrame[Engine::audioEngine()->framesPerPeriod()];
}

CrossoverEQEffect::~CrossoverEQEffect()
{
	delete[] m_tmp1;
	delete[] m_tmp2;
	delete[] m_work;
}

void CrossoverEQEffect::sampleRateChanged()
{
	m_sampleRate = Engine::audioEngine()->outputSampleRate();
	m_lp1.setSampleRate( m_sampleRate );
	m_lp2.setSampleRate( m_sampleRate );
	m_lp3.setSampleRate( m_sampleRate );
	m_hp2.setSampleRate( m_sampleRate );
	m_hp3.setSampleRate( m_sampleRate );
	m_hp4.setSampleRate( m_sampleRate );
	m_needsUpdate = true;
}


Effect::ProcessStatus CrossoverEQEffect::processImpl(SampleFrame* buf, const fpp_t frames)
{
	// filters update
	if( m_needsUpdate || m_controls.m_xover12.isValueChanged() )
	{
		m_lp1.setLowpass( m_controls.m_xover12.value() );
		m_hp2.setHighpass( m_controls.m_xover12.value() );
	}
	if( m_needsUpdate || m_controls.m_xover23.isValueChanged() )
	{
		m_lp2.setLowpass( m_controls.m_xover23.value() );
		m_hp3.setHighpass( m_controls.m_xover23.value() );
	}
	if( m_needsUpdate || m_controls.m_xover34.isValueChanged() )
	{
		m_lp3.setLowpass( m_controls.m_xover34.value() );
		m_hp4.setHighpass( m_controls.m_xover34.value() );
	}
	
	// gain values update
	if( m_needsUpdate || m_controls.m_gain1.isValueChanged() )
	{
		m_gain1 = dbfsToAmp( m_controls.m_gain1.value() );
	}
	if( m_needsUpdate || m_controls.m_gain2.isValueChanged() )
	{
		m_gain2 = dbfsToAmp( m_controls.m_gain2.value() );
	}
	if( m_needsUpdate || m_controls.m_gain3.isValueChanged() )
	{
		m_gain3 = dbfsToAmp( m_controls.m_gain3.value() );
	}
	if( m_needsUpdate || m_controls.m_gain4.isValueChanged() )
	{
		m_gain4 = dbfsToAmp( m_controls.m_gain4.value() );
	}
	
	// mute values update
	const bool mute1 = m_controls.m_mute1.value();
	const bool mute2 = m_controls.m_mute2.value();
	const bool mute3 = m_controls.m_mute3.value();
	const bool mute4 = m_controls.m_mute4.value();
	
	m_needsUpdate = false;
	
	zeroSampleFrames(m_work, frames);
	
	// run temp bands
	for (auto f = std::size_t{0}; f < frames; ++f)
	{
		m_tmp1[f][0] = m_lp2.update( buf[f][0], 0 );
		m_tmp1[f][1] = m_lp2.update( buf[f][1], 1 );
		m_tmp2[f][0] = m_hp3.update( buf[f][0], 0 );
		m_tmp2[f][1] = m_hp3.update( buf[f][1], 1 );
	}

	// run band 1
	if( mute1 )
	{
		for (auto f = std::size_t{0}; f < frames; ++f)
		{
			m_work[f][0] += m_lp1.update( m_tmp1[f][0], 0 ) * m_gain1;
			m_work[f][1] += m_lp1.update( m_tmp1[f][1], 1 ) * m_gain1;
		}
	}
	
	// run band 2
	if( mute2 )
	{
		for (auto f = std::size_t{0}; f < frames; ++f)
		{
			m_work[f][0] += m_hp2.update( m_tmp1[f][0], 0 ) * m_gain2;
			m_work[f][1] += m_hp2.update( m_tmp1[f][1], 1 ) * m_gain2;
		}
	}
	
	// run band 3
	if( mute3 )
	{
		for (auto f = std::size_t{0}; f < frames; ++f)
		{
			m_work[f][0] += m_lp3.update( m_tmp2[f][0], 0 ) * m_gain3;
			m_work[f][1] += m_lp3.update( m_tmp2[f][1], 1 ) * m_gain3;
		}
	}
	
	// run band 4
	if( mute4 )
	{
		for (auto f = std::size_t{0}; f < frames; ++f)
		{
			m_work[f][0] += m_hp4.update( m_tmp2[f][0], 0 ) * m_gain4;
			m_work[f][1] += m_hp4.update( m_tmp2[f][1], 1 ) * m_gain4;
		}
	}
	
	const float d = dryLevel();
	const float w = wetLevel();

	for (auto f = std::size_t{0}; f < frames; ++f)
	{
		buf[f][0] = d * buf[f][0] + w * m_work[f][0];
		buf[f][1] = d * buf[f][1] + w * m_work[f][1];
	}

	return ProcessStatus::ContinueIfNotQuiet;
}

void CrossoverEQEffect::clearFilterHistories()
{
	m_lp1.clearHistory();
	m_lp2.clearHistory();
	m_lp3.clearHistory();
	m_hp2.clearHistory();
	m_hp3.clearHistory();
	m_hp4.clearHistory();
}


extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model* parent, void* data )
{
	return new CrossoverEQEffect( parent, static_cast<const Plugin::Descriptor::SubPluginFeatures::Key *>( data ) );
}

}


} // namespace lmms