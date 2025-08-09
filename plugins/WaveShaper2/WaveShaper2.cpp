/*
 * waveshaper.cpp - waveshaper effect-plugin
 *
 * Copyright (c) 2018 Joshua Wade (https://github.com/SecondFlight)
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

#include "plugin_export.h"

#include "WaveShaper2.h"
#include "lmms_math.h"
#include "embed.h"
#include "interpolation.h"



namespace lmms
{

extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT waveshaper2_plugin_descriptor =
{
	LMMS_STRINGIFY( PLUGIN_NAME ),
	"Waveshaper 2",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"plugin for waveshaping" ),
	"Joshua Wade <lastname/firstinitial/at/southern/dot/edu>",
	0x0100,
	Plugin::Type::Effect,
	new PluginPixmapLoader("logo"),
	nullptr,
	nullptr,
} ;

}


WaveShaper2Effect::WaveShaper2Effect( Model * _parent,
			const Descriptor::SubPluginFeatures::Key * _key ) :
	Effect( &waveshaper2_plugin_descriptor, _parent, _key ),
	m_wsControls( this )
{
}




WaveShaper2Effect::~WaveShaper2Effect()
{
}




Effect::ProcessStatus WaveShaper2Effect::processImpl(SampleFrame* buf, const fpp_t frames)
{
// variables for effect
	int i = 0;

	const float d = dryLevel();
	const float w = wetLevel();
	float input = m_wsControls.m_inputModel.value();
	float output = m_wsControls.m_outputModel.value();

	ValueBuffer *inputBuffer = m_wsControls.m_inputModel.valueBuffer();
	ValueBuffer *outputBufer = m_wsControls.m_outputModel.valueBuffer();

	int inputInc = inputBuffer ? 1 : 0;
	int outputInc = outputBufer ? 1 : 0;

	const float *inputPtr = inputBuffer ? &( inputBuffer->values()[ 0 ] ) : &input;
	const float *outputPtr = outputBufer ? &( outputBufer->values()[ 0 ] ) : &output;

	for (fpp_t f = 0; f < frames; ++f)
	{
		float s[2] = {buf[f][0], buf[f][1]};

// apply input gain
		s[0] *= *inputPtr;
		s[1] *= *inputPtr;

// clip if clip enabled
		//if( clip )
		//{
			s[0] = qBound( -1.0f, s[0], 1.0f );
			s[1] = qBound( -1.0f, s[1], 1.0f );
		//}

// start effect

		for( i=0; i <= 1; ++i )
		{
			bool invert = false;
			if (s[i] < 0)
				invert = true;

			if (invert)
				s[i] *= -1;

			s[i] = m_wsControls.m_vectorGraphModel.calculateSample(s[i]);

			if (invert)
				s[i] *= -1;
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
	return( new WaveShaper2Effect( _parent,
		static_cast<const Plugin::Descriptor::SubPluginFeatures::Key *>(
								_data ) ) );
}

}

} // namespace lmms

