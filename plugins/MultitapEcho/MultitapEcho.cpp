/*
 * MultitapEcho.cpp - a multitap echo delay plugin
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "MultitapEcho.h"
#include "embed.h"
#include "LmmsTypes.h"
#include "lmms_math.h"
#include "plugin_export.h"

namespace lmms
{


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT multitapecho_plugin_descriptor =
{
	LMMS_STRINGIFY( PLUGIN_NAME ),
	"Multitap Echo",
	QT_TRANSLATE_NOOP( "PluginBrowser", "A multitap echo delay plugin" ),
	"Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>",
	0x0100,
	Plugin::Type::Effect,
	new PixmapLoader("lmms-plugin-logo"),
	nullptr,
	nullptr,
} ;

}


MultitapEchoEffect::MultitapEchoEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key)
	: Effect(&multitapecho_plugin_descriptor, parent, key)
	, m_stages(1)
	, m_controls(this)
	, m_buffer(16100.0f)
	, m_sampleRate(Engine::audioEngine()->outputSampleRate())
	, m_sampleRatio(1.0f / m_sampleRate)
	, m_work(Engine::audioEngine()->framesPerPeriod())
{
	m_buffer.reset();
	m_stages = static_cast<int>( m_controls.m_stages.value() );
	updateFilters( 0, 19 );
}


void MultitapEchoEffect::updateFilters( int begin, int end )
{
	for( int i = begin; i <= end; ++i )
	{
		for( int s = 0; s < m_stages; ++s )
		{
			setFilterFreq( m_lpFreq[i] * m_sampleRatio, m_filter[i][s] );
		}
	}
}


void MultitapEchoEffect::runFilter(PlanarBufferView<float> dst, PlanarBufferView<const float> src, StereoOnePole& filter)
{
	assert(dst.frames() >= src.frames());
	for (f_cnt_t f = 0; f < src.frames(); ++f)
	{
		dst[0][f] = filter.update(src[0][f], 0);
		dst[1][f] = filter.update(src[1][f], 1);
	}
}


Effect::ProcessStatus MultitapEchoEffect::processImpl(PlanarBufferView<float> inOut)
{
	const float d = dryLevel();
	const float w = wetLevel();

	// get processing vars
	const int steps = m_controls.m_steps.value();
	const float stepLength = m_controls.m_stepLength.value();
	const float dryGain = dbfsToAmp(m_controls.m_dryGain.value());
	const bool swapInputs = m_controls.m_swapInputs.value();

	// check if number of stages has changed
	if (m_controls.m_stages.isValueChanged())
	{
		m_stages = static_cast<int>(m_controls.m_stages.value());
		updateFilters(0, steps - 1);
	}

	// add dry buffer - never swap inputs for dry
	m_buffer.write(inOut, [dryGain](PlanarBufferSpan<float> dst, PlanarBufferSpan<const float> src) {
		MixHelpers::addMultiplied(dst, src, dryGain);
	});

	auto workBuffers = m_work.allBuffers();

	// swapped inputs?
	if (swapInputs)
	{
		float offset = stepLength; // milliseconds
		for (int i = 0; i < steps; ++i) // add all steps swapped
		{
			for (int s = 0; s < m_stages; ++s)
			{
				runFilter(workBuffers, inOut, m_filter[i][s]);
			}
			m_buffer.write(workBuffers, offset,
				[amp = m_amp[i]](PlanarBufferSpan<float> dst, PlanarBufferSpan<const float> src) {
					MixHelpers::addSwappedMultiplied(dst, src, amp);
				}
			);
			offset += stepLength;
		}
	}
	else
	{
		float offset = stepLength; // milliseconds
		for (int i = 0; i < steps; ++i) // add all steps
		{
			for (int s = 0; s < m_stages; ++s)
			{
				runFilter(workBuffers, inOut, m_filter[i][s]);
			}
			m_buffer.write(workBuffers, offset,
				[amp = m_amp[i]](PlanarBufferSpan<float> dst, PlanarBufferSpan<const float> src) {
					MixHelpers::addMultiplied(dst, src, amp);
				}
			);
			offset += stepLength;
		}
	}

	// pop the buffer and mix it into output
	m_buffer.pop(workBuffers);

	// TODO: Add a RingBuffer::pop which accepts a lambda function, then perform the following work there
	for (f_cnt_t f = 0; f < inOut.frames(); ++f)
	{
		inOut[0][f] = d * inOut[0][f] + w * workBuffers[0][f];
		inOut[1][f] = d * inOut[1][f] + w * workBuffers[1][f];
	}

	return ProcessStatus::ContinueIfNotQuiet;
}


extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model* parent, void* data )
{
	return new MultitapEchoEffect( parent, static_cast<const Plugin::Descriptor::SubPluginFeatures::Key *>( data ) );
}

}


} // namespace lmms
