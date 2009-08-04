/*
 * stereo_matrix.cpp - stereo-matrix-effect-plugin
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail/dot/com>
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


#include "stereo_matrix.h"

#include "embed.cpp"


extern "C"
{

plugin::descriptor PLUGIN_EXPORT stereomatrix_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"Stereophonic Matrix",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"Plugin for freely manipulating stereo output" ),
	"Paul Giblock <drfaygo/at/gmail.com>",
	0x0100,
	plugin::Effect,
	new pluginPixmapLoader( "logo" ),
	NULL,
	NULL
} ;

}



stereoMatrixEffect::stereoMatrixEffect(
			model * _parent,
			const descriptor::subPluginFeatures::key * _key ) :
	effect( &stereomatrix_plugin_descriptor, _parent, _key ),
	m_smControls( this )
{
}




stereoMatrixEffect::~stereoMatrixEffect()
{
}



bool stereoMatrixEffect::processAudioBuffer( sampleFrame * _buf,
							const fpp_t _frames )
{
	
	// This appears to be used for determining whether or not to continue processing
	// audio with this effect	
	if( !isEnabled() || !isRunning() )
	{
		return( FALSE );
	}

	double out_sum = 0.0;

	for( fpp_t f = 0; f < _frames; ++f )
	{	
		const float d = getDryLevel();
		const float w = getWetLevel();
		
		sample_t l = _buf[f][0];
		sample_t r = _buf[f][1];

		// Init with dry-mix
		_buf[f][0] = l * d;
		_buf[f][1] = r * d;

		// Add it wet
		_buf[f][0] += ( m_smControls.m_llModel.value( f ) * l  +
					m_smControls.m_rlModel.value( f ) * r ) * w;

		_buf[f][1] += ( m_smControls.m_lrModel.value( f ) * l  +
					m_smControls.m_rrModel.value( f ) * r ) * w;
		out_sum += _buf[f][0]*_buf[f][0] + _buf[f][1]*_buf[f][1];

	}

	checkGate( out_sum / _frames );

	return( isRunning() );
}




extern "C"
{

// neccessary for getting instance out of shared lib
plugin * PLUGIN_EXPORT lmms_plugin_main( model * _parent, void * _data )
{
	return( new stereoMatrixEffect( _parent,
		static_cast<const plugin::descriptor::subPluginFeatures::key *>(
								_data ) ) );
}

}

