/*
 * WaveShaper.cpp - waveshaper effect-plugin
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2006-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "WaveShaper.h"
#include "lmms_math.h"
#include "embed.h"

#include "plugin_export.h"

namespace lmms
{


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT waveshaper_plugin_descriptor =
{
	LMMS_STRINGIFY( PLUGIN_NAME ),
	"Waveshaper Effect",
	QT_TRANSLATE_NOOP( "PluginBrowser",
				"plugin for waveshaping" ),
	"Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>",
	0x0100,
	Plugin::Type::Effect,
	new PluginPixmapLoader("logo"),
	{},
	nullptr,
} ;

}



WaveShaperEffect::WaveShaperEffect( Model * _parent,
			const Descriptor::SubPluginFeatures::Key * _key ) :
	Effect( &waveshaper_plugin_descriptor, _parent, _key ),
	m_wsControls( this )
{
}




Effect::ProcessStatus WaveShaperEffect::processImpl(SampleFrame* buf, const fpp_t frames)
{
// variables for effect
	int i = 0;

	const float d = dryLevel();
	const float w = wetLevel();
	float input = m_wsControls.m_inputModel.value();
	float output = m_wsControls.m_outputModel.value();
	const float * samples = m_wsControls.m_wavegraphModel.samples();
	const bool clip = m_wsControls.m_clipModel.value();

	ValueBuffer *inputBuffer = m_wsControls.m_inputModel.valueBuffer();
	ValueBuffer *outputBufer = m_wsControls.m_outputModel.valueBuffer();

	int inputInc = inputBuffer ? 1 : 0;
	int outputInc = outputBufer ? 1 : 0;

	const float *inputPtr = inputBuffer ? &( inputBuffer->values()[ 0 ] ) : &input;
	const float *outputPtr = outputBufer ? &( outputBufer->values()[ 0 ] ) : &output;

	for (fpp_t f = 0; f < frames; ++f)
	{
		auto s = std::array{buf[f][0], buf[f][1]};

// apply input gain
		s[0] *= *inputPtr;
		s[1] *= *inputPtr;

// clip if clip enabled
		if( clip )
		{
			s[0] = qBound( -1.0f, s[0], 1.0f );
			s[1] = qBound( -1.0f, s[1], 1.0f );
		}

// start effect

		for( i=0; i <= 1; ++i )
		{
			const int lookup = static_cast<int>( qAbs( s[i] ) * 200.0f );
			const float frac = fraction( qAbs( s[i] ) * 200.0f );
			const float posneg = s[i] < 0 ? -1.0f : 1.0f;

			if( lookup < 1 )
			{
				s[i] = frac * samples[0] * posneg;
			}
			else if( lookup < 200 )
			{
				s[i] = std::lerp(samples[lookup - 1], samples[lookup], frac) * posneg;
			}
			else
			{
				s[i] *= samples[199];
			}
		}

// apply output gain
		s[0] *= *outputPtr;
		s[1] *= *outputPtr;

// mix wet/dry signals
		buf[f][0] = d * buf[f][0] + w * s[0];
		buf[f][1] = d * buf[f][1] + w * s[1];

		outputPtr += outputInc;
		inputPtr += inputInc;
	}

	return ProcessStatus::ContinueIfNotQuiet;
}





extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model * _parent, void * _data )
{
	return( new WaveShaperEffect( _parent,
		static_cast<const Plugin::Descriptor::SubPluginFeatures::Key *>(
								_data ) ) );
}

}


} // namespace lmms
