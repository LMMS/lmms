/*
 * DualFilter.cpp - A native dual filter effect plugin with two parallel filters
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
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

#include "DualFilter.h"

#include "embed.cpp"
#include "basic_filters.h"


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT dualfilter_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"Dual Filter",
	QT_TRANSLATE_NOOP( "pluginBrowser", "A native amplifier plugin" ),
	"Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>",
	0x0100,
	Plugin::Effect,
	new PluginPixmapLoader( "logo" ),
	NULL,
	NULL
} ;

}



DualFilterEffect::DualFilterEffect( Model* parent, const Descriptor::SubPluginFeatures::Key* key ) :
	Effect( &dualfilter_plugin_descriptor, parent, key ),
	m_dfControls( this )
{
	m_filter1 = new basicFilters<2>( engine::mixer()->processingSampleRate() );
	m_filter2 = new basicFilters<2>( engine::mixer()->processingSampleRate() );

	// ensure filters get updated
	m_filter1changed = true;
	m_filter2changed = true;
}




DualFilterEffect::~DualFilterEffect()
{
	delete m_filter1;
	delete m_filter2;
}




bool DualFilterEffect::processAudioBuffer( sampleFrame* buf, const fpp_t frames )
{
	if( !isEnabled() || !isRunning () )
	{
		return( false );
	}

	double outSum = 0.0;
	const float d = dryLevel();
	const float w = wetLevel();

    if( m_dfControls.m_filter1Model.isValueChanged() || m_filter1changed )
	{
		m_filter1->setFilterType( m_dfControls.m_filter1Model.value() );
		m_filter1changed = true;
	}
    if( m_dfControls.m_filter2Model.isValueChanged() || m_filter2changed )
	{
		m_filter2->setFilterType( m_dfControls.m_filter2Model.value() );
		m_filter2changed = true;
	}

	const bool enabled1 = m_dfControls.m_enabled1Model.value();
	const bool enabled2 = m_dfControls.m_enabled2Model.value();

	// recalculate only when necessary: either cut/res is changed, or the changed-flag is set (filter type or samplerate changed)
	if( ( enabled1 && ( m_dfControls.m_cut1Model.isValueChanged() ||
		m_dfControls.m_res1Model.isValueChanged() ) ) || m_filter1changed )
	{
		m_filter1->calcFilterCoeffs( m_dfControls.m_cut1Model.value(), m_dfControls.m_res1Model.value() );
		m_filter1changed = false;
	}
	if( ( enabled2 && ( m_dfControls.m_cut2Model.isValueChanged() ||
		m_dfControls.m_res2Model.isValueChanged() ) ) || m_filter2changed )
	{
		m_filter2->calcFilterCoeffs( m_dfControls.m_cut2Model.value(), m_dfControls.m_res2Model.value() );
		m_filter2changed = false;
	}
	
	// get mix amounts for wet signals of both filters
	const float mix2 = ( ( m_dfControls.m_mixModel.value() + 1.0f ) * 0.5f );
	const float mix1 = 1.0f - mix2;
	
	const float gain1 = m_dfControls.m_gain1Model.value() * 0.01f;
	const float gain2 = m_dfControls.m_gain2Model.value() * 0.01f;

	// buffer processing loop
	for( fpp_t f = 0; f < frames; ++f )
	{
		sample_t s[2] = { 0.0f, 0.0f };	// mix
		sample_t s1[2] = { buf[f][0], buf[f][1] };	// filter 1
		sample_t s2[2] = { buf[f][0], buf[f][1] };	// filter 2

		// update filter 1
		if( enabled1 )
		{
			s1[0] = m_filter1->update( s1[0], 0 );
			s1[1] = m_filter1->update( s1[1], 1 );

			// apply gain
			s1[0] *= gain1;
			s1[1] *= gain1;

			// apply mix
			s[0] += ( s1[0] * mix1 );
			s[1] += ( s1[1] * mix1 );
		}

		// update filter 2
		if( enabled2 )
		{
			s2[0] = m_filter2->update( s2[0], 0 );
			s2[1] = m_filter2->update( s2[1], 1 );

			//apply gain
			s2[0] *= gain2;
			s2[1] *= gain2;

			// apply mix
			s[0] += ( s2[0] * mix2 );
			s[1] += ( s2[1] * mix2 );
		}
		outSum += buf[f][0]*buf[f][0] + buf[f][1]*buf[f][1];

		// do another mix with dry signal
		buf[f][0] = d * buf[f][0] + w * s[0];
		buf[f][1] = d * buf[f][1] + w * s[1];
	}

	checkGate( outSum / frames );

	return isRunning();
}





extern "C"
{

// necessary for getting instance out of shared lib
Plugin * PLUGIN_EXPORT lmms_plugin_main( Model* parent, void* data )
{
	return new DualFilterEffect( parent, static_cast<const Plugin::Descriptor::SubPluginFeatures::Key *>( data ) );
}

}

