/*
 * DualFilter.cpp - A native dual filter effect plugin with two parallel filters
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

#include "DualFilter.h"

#include "embed.h"
#include "BasicFilters.h"
#include "plugin_export.h"

namespace lmms
{


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT dualfilter_plugin_descriptor =
{
	LMMS_STRINGIFY( PLUGIN_NAME ),
	"Dual Filter",
	QT_TRANSLATE_NOOP( "PluginBrowser", "A Dual filter plugin" ),
	"Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>",
	0x0100,
	Plugin::Type::Effect,
	new PluginPixmapLoader( "logo" ),
	nullptr,
	nullptr,
	nullptr,
} ;

}



DualFilterEffect::DualFilterEffect( Model* parent, const Descriptor::SubPluginFeatures::Key* key ) :
	Effect( &dualfilter_plugin_descriptor, parent, key ),
	m_dfControls( this )
{
	m_filter1 = new BasicFilters<2>( Engine::audioEngine()->outputSampleRate() );
	m_filter2 = new BasicFilters<2>( Engine::audioEngine()->outputSampleRate() );

	// ensure filters get updated
	m_filter1changed = true;
	m_filter2changed = true;
}




DualFilterEffect::~DualFilterEffect()
{
	delete m_filter1;
	delete m_filter2;
}




Effect::ProcessStatus DualFilterEffect::processImpl(SampleFrame* buf, const fpp_t frames)
{
	const float d = dryLevel();
	const float w = wetLevel();

	if (m_dfControls.m_filter1Model.isValueChanged() || m_filter1changed)
	{
		m_filter1->setFilterType( static_cast<BasicFilters<2>::FilterType>(m_dfControls.m_filter1Model.value()) );
		m_filter1changed = true;
	}
	if (m_dfControls.m_filter2Model.isValueChanged() || m_filter2changed)
	{
		m_filter2->setFilterType( static_cast<BasicFilters<2>::FilterType>(m_dfControls.m_filter2Model.value()) );
		m_filter2changed = true;
	}

	float cut1 = m_dfControls.m_cut1Model.value();
	float res1 = m_dfControls.m_res1Model.value();
	float gain1 = m_dfControls.m_gain1Model.value();
	float cut2 = m_dfControls.m_cut2Model.value();
	float res2 = m_dfControls.m_res2Model.value();
	float gain2 = m_dfControls.m_gain2Model.value();
	float mix = m_dfControls.m_mixModel.value();

	ValueBuffer *cut1Buffer = m_dfControls.m_cut1Model.valueBuffer();
	ValueBuffer *res1Buffer = m_dfControls.m_res1Model.valueBuffer();
	ValueBuffer *gain1Buffer = m_dfControls.m_gain1Model.valueBuffer();
	ValueBuffer *cut2Buffer = m_dfControls.m_cut2Model.valueBuffer();
	ValueBuffer *res2Buffer = m_dfControls.m_res2Model.valueBuffer();
	ValueBuffer *gain2Buffer = m_dfControls.m_gain2Model.valueBuffer();
	ValueBuffer *mixBuffer = m_dfControls.m_mixModel.valueBuffer();

	int cut1Inc = cut1Buffer ? 1 : 0;
	int res1Inc = res1Buffer ? 1 : 0;
	int gain1Inc = gain1Buffer ? 1 : 0;
	int cut2Inc = cut2Buffer ? 1 : 0;
	int res2Inc = res2Buffer ? 1 : 0;
	int gain2Inc = gain2Buffer ? 1 : 0;
	int mixInc = mixBuffer ? 1 : 0;

	float *cut1Ptr = cut1Buffer ? &( cut1Buffer->values()[ 0 ] ) : &cut1;
	float *res1Ptr = res1Buffer ? &( res1Buffer->values()[ 0 ] ) : &res1;
	float *gain1Ptr = gain1Buffer ? &( gain1Buffer->values()[ 0 ] ) : &gain1;
	float *cut2Ptr = cut2Buffer ? &( cut2Buffer->values()[ 0 ] ) : &cut2;
	float *res2Ptr = res2Buffer ? &( res2Buffer->values()[ 0 ] ) : &res2;
	float *gain2Ptr = gain2Buffer ? &( gain2Buffer->values()[ 0 ] ) : &gain2;
	float *mixPtr = mixBuffer ? &( mixBuffer->values()[ 0 ] ) : &mix;

	const bool enabled1 = m_dfControls.m_enabled1Model.value();
	const bool enabled2 = m_dfControls.m_enabled2Model.value();




	// buffer processing loop
	for( fpp_t f = 0; f < frames; ++f )
	{
		// get mix amounts for wet signals of both filters
		const float mix2 = ( ( *mixPtr + 1.0f ) * 0.5f );
		const float mix1 = 1.0f - mix2;
		const float gain1 = *gain1Ptr * 0.01f;
		const float gain2 = *gain2Ptr * 0.01f;
		auto s = std::array{0.0f, 0.0f};	// mix
		auto s1 = std::array{buf[f][0], buf[f][1]};	// filter 1
		auto s2 = std::array{buf[f][0], buf[f][1]};	// filter 2

		// update filter 1
		if( enabled1 )
		{
			//update filter 1 params here
			// recalculate only when necessary: either cut/res is changed, or the changed-flag is set (filter type or samplerate changed)
			if( ( ( *cut1Ptr != m_currentCut1 ||
				*res1Ptr != m_currentRes1 ) ) || m_filter1changed )
			{
				m_filter1->calcFilterCoeffs( *cut1Ptr, *res1Ptr );
				m_filter1changed = false;
				m_currentCut1 = *cut1Ptr;
				m_currentRes1 = *res1Ptr;
			}
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
			//update filter 2 params here
			if( ( ( *cut2Ptr != m_currentCut2 ||
								*res2Ptr != m_currentRes2 ) ) || m_filter2changed )
			{
				m_filter2->calcFilterCoeffs( *cut2Ptr, *res2Ptr );
				m_filter2changed = false;
				m_currentCut2 = *cut2Ptr;
				m_currentRes2 = *res2Ptr;
			}
			s2[0] = m_filter2->update( s2[0], 0 );
			s2[1] = m_filter2->update( s2[1], 1 );

			//apply gain
			s2[0] *= gain2;
			s2[1] *= gain2;

			// apply mix
			s[0] += ( s2[0] * mix2 );
			s[1] += ( s2[1] * mix2 );
		}

		// do another mix with dry signal
		buf[f][0] = d * buf[f][0] + w * s[0];
		buf[f][1] = d * buf[f][1] + w * s[1];

		//increment pointers
		cut1Ptr += cut1Inc;
		res1Ptr += res1Inc;
		gain1Ptr += gain1Inc;
		cut2Ptr += cut2Inc;
		res2Ptr += res2Inc;
		gain2Ptr += gain2Inc;
		mixPtr += mixInc;
	}

	return ProcessStatus::ContinueIfNotQuiet;
}

void DualFilterEffect::onEnabledChanged()
{
	m_filter1->clearHistory();
	m_filter2->clearHistory();
}




extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model* parent, void* data )
{
	return new DualFilterEffect( parent, static_cast<const Plugin::Descriptor::SubPluginFeatures::Key *>( data ) );
}

}


} // namespace lmms
