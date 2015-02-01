/*
 * waveshaper.cpp - waveshaper effect-plugin
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2006-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "waveshaper.h"
#include "lmms_math.h"
#include "embed.cpp"
#include "interpolation.h"


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT waveshaper_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"Waveshaper Effect",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"plugin for waveshaping" ),
	"Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>",
	0x0100,
	Plugin::Effect,
	new PluginPixmapLoader( "logo" ),
	NULL,
	NULL
} ;

}



waveShaperEffect::waveShaperEffect( Model * _parent,
			const Descriptor::SubPluginFeatures::Key * _key ) :
	Effect( &waveshaper_plugin_descriptor, _parent, _key ),
	m_wsControls( this )
{
}




waveShaperEffect::~waveShaperEffect()
{
}




bool waveShaperEffect::processAudioBuffer( sampleFrame * _buf,
							const fpp_t _frames )
{
	if( !isEnabled() || !isRunning () )
	{
		return( false );
	}

// variables for effect
	int i = 0;

	double out_sum = 0.0;
	const float d = dryLevel();
	const float w = wetLevel();
	const float input = m_wsControls.m_inputModel.value();
	const float output = m_wsControls.m_outputModel.value();
	const float * samples = m_wsControls.m_wavegraphModel.samples();
	const bool clip = m_wsControls.m_clipModel.value();

	for( fpp_t f = 0; f < _frames; ++f )
	{
		float s[2] = { _buf[f][0], _buf[f][1] };

// apply input gain
		s[0] *= input;
		s[1] *= input;

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
				s[i] = linearInterpolate( samples[ lookup - 1 ], 
						samples[ lookup ], frac )
						* posneg;
			}
			else
			{
				s[i] *= samples[199];
			}
		}

// apply output gain
		s[0] *= output;
		s[1] *= output;

		out_sum += _buf[f][0]*_buf[f][0] + _buf[f][1]*_buf[f][1];
// mix wet/dry signals
		_buf[f][0] = d * _buf[f][0] + w * s[0];
		_buf[f][1] = d * _buf[f][1] + w * s[1];
	}

	checkGate( out_sum / _frames );

	return( isRunning() );
}





extern "C"
{

// necessary for getting instance out of shared lib
Plugin * PLUGIN_EXPORT lmms_plugin_main( Model * _parent, void * _data )
{
	return( new waveShaperEffect( _parent,
		static_cast<const Plugin::Descriptor::SubPluginFeatures::Key *>(
								_data ) ) );
}

}

