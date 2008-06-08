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


#include "controller.h"
#include "song.h"
#include "peak_controller.h"
#include "peak_controller_effect.h"


#undef SINGLE_SOURCE_COMPILE
#include "embed.cpp"


extern "C"
{

plugin::descriptor PLUGIN_EXPORT peakcontroller_effect_plugin_descriptor =
{
	STRINGIFY_PLUGIN_NAME( PLUGIN_NAME ),
	"Peak Controller",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"Plugin for controlling knobs with sound peaks" ),
	"Paul Giblock <drfaygo/at/gmail.com>",
	0x0100,
	plugin::Effect,
	new pluginPixmapLoader( "logo" ),
	NULL
} ;

}



peakControllerEffect::peakControllerEffect(
			model * _parent,
			const descriptor::subPluginFeatures::key * _key ) :
	effect( &peakcontroller_effect_plugin_descriptor, _parent, _key ),
	m_peakControls( this )
{
	engine::getSong()->addController( new peakController( engine::getSong(), this ) );
}




peakControllerEffect::~peakControllerEffect()
{
}



bool peakControllerEffect::processAudioBuffer( sampleFrame * _buf,
							const fpp_t _frames )
{
	peakControllerEffectControls & c = m_peakControls;
	
	// This appears to be used for determining whether or not to continue processing
	// audio with this effect	
	if( !isEnabled() || !isRunning() )
	{
		return( FALSE );
	}


	// RMS:
	float sum = 0;
	if( c.m_muteModel.value() )
	{
		for( int i = 0; i < _frames; ++i )
		{
			sum += (_buf[i][0]+_buf[i][0]) * (_buf[i][1]+_buf[i][1]);
			_buf[i][0] = _buf[i][1] = 0.0f;
		}
	}
	else
	{
		for( int i = 0; i < _frames; ++i )
		{
			sum += (_buf[i][0]+_buf[i][0]) * (_buf[i][1]+_buf[i][1]);
		}
	}

	m_lastSample = c.m_baseModel.value() + c.m_amountModel.value() * sqrtf( 0.5f * sum / _frames );

	//checkGate( out_sum / _frames );

	return( isRunning() );
}




extern "C"
{

// neccessary for getting instance out of shared lib
plugin * PLUGIN_EXPORT lmms_plugin_main( model * _parent, void * _data )
{
	return( new peakControllerEffect( _parent,
		static_cast<const plugin::descriptor::subPluginFeatures::key *>(
								_data ) ) );
}

}

