/*
 * waveshaper.cpp - waveshaper effect-plugin
 *
 * * Copyright  * (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2006-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "waveshaper.h"
#include <math.h>
#include "embed.cpp"


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
	float lookup;
	float frac;
	float posneg;

	double out_sum = 0.0;
	const float d = dryLevel();
	const float w = wetLevel();
	for( fpp_t f = 0; f < _frames; ++f )
	{
		sample_t s[2] = { _buf[f][0], _buf[f][1] };

// apply input gain
		s[0] *= m_wsControls.m_inputModel.value();
		s[1] *= m_wsControls.m_inputModel.value();
		
// start effect

		for ( i=0; i <= 1; ++i )
		{
			lookup = fabsf( s[i] ) * 100.0f;
			posneg = s[i] < 0 ? -1.0f : 1.0f;
			
			if ( lookup < 1 ) 
			{
				frac = lookup - truncf(lookup);				
				s[i] = frac * m_wsControls.m_wavegraphModel.samples()[0] * posneg;
			}
			else
			if ( lookup < 100 )
			{
				frac = lookup - truncf(lookup);
				s[i] = 
						(( (1.0f-frac) * m_wsControls.m_wavegraphModel.samples()[ (int)truncf(lookup) - 1 ] ) +
						( frac * m_wsControls.m_wavegraphModel.samples()[ (int)truncf(lookup) ] )) 
						* posneg;
			}
			else 
			{
				s[i] *= m_wsControls.m_wavegraphModel.samples()[99];
			}
		}

// apply output gain
		s[0] *= m_wsControls.m_outputModel.value();
		s[1] *= m_wsControls.m_outputModel.value();
		
// mix wet/dry signals
		_buf[f][0] = d * _buf[f][0] + w * s[0];
		_buf[f][1] = d * _buf[f][1] + w * s[1];

		out_sum += _buf[f][0]*_buf[f][0] + _buf[f][1]*_buf[f][1];
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

