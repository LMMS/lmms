/*
 * bass_booster.cpp - bass-booster-effect-plugin
 *
 * Copyright (c) 2006-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "bass_booster.h"


#undef SINGLE_SOURCE_COMPILE
#include "embed.cpp"


extern "C"
{

plugin::descriptor PLUGIN_EXPORT bassbooster_plugin_descriptor =
{
	STRINGIFY_PLUGIN_NAME( PLUGIN_NAME ),
	"BassBooster Effect",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"plugin for boosting bass" ),
	"Tobias Doerffel <tobydox/at/users.sf.net>",
	0x0100,
	plugin::Effect,
	new pluginPixmapLoader( "logo" ),
	NULL,
	NULL
} ;

}



bassBoosterEffect::bassBoosterEffect( model * _parent,
			const descriptor::subPluginFeatures::key * _key ) :
	effect( &bassbooster_plugin_descriptor, _parent, _key ),
	m_bbFX( effectLib::fastBassBoost( 70.0f, 1.0f, 2.8f ) ),
	m_bbControls( this )
{
}




bassBoosterEffect::~bassBoosterEffect()
{
}




bool bassBoosterEffect::processAudioBuffer( sampleFrame * _buf,
							const fpp_t _frames )
{
	if( !isEnabled() || !isRunning () )
	{
		return( FALSE );
	}

	double out_sum = 0.0;
	const float d = getDryLevel();
	const float w = getWetLevel();
	for( fpp_t f = 0; f < _frames; ++f )
	{
		sample_t s[2] = { _buf[f][0], _buf[f][1] };
		m_bbFX.nextSample( s[0], s[1] );

		_buf[f][0] = d * _buf[f][0] + w * s[0];
		_buf[f][1] = d * _buf[f][1] + w * s[1];

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
	return( new bassBoosterEffect( _parent,
		static_cast<const plugin::descriptor::subPluginFeatures::key *>(
								_data ) ) );
}

}

