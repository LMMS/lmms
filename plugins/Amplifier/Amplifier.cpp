/*
 * Amplifier.cpp - A native amplifier effect plugin with sample-exact amplification
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

#include "Amplifier.h"

#include "embed.cpp"


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT amplifier_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"Amplifier",
	QT_TRANSLATE_NOOP( "pluginBrowser", "A native amplifier plugin" ),
	"Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>",
	0x0100,
	Plugin::Effect,
	new PluginPixmapLoader( "logo" ),
	NULL,
	NULL
} ;

}



AmplifierEffect::AmplifierEffect( Model* parent, const Descriptor::SubPluginFeatures::Key* key ) :
	Effect( &amplifier_plugin_descriptor, parent, key ),
	m_ampControls( this )
{
}




AmplifierEffect::~AmplifierEffect()
{
}




bool AmplifierEffect::processAudioBuffer( sampleFrame* buf, const fpp_t frames )
{
	if( !isEnabled() || !isRunning () )
	{
		return( false );
	}

	double outSum = 0.0;
	const float d = dryLevel();
	const float w = wetLevel();
	for( fpp_t f = 0; f < frames; ++f )
	{
//		qDebug( "offset %d, value %f", f, m_ampControls.m_volumeModel.value( f ) );
		outSum += buf[f][0]*buf[f][0] + buf[f][1]*buf[f][1];
	
		sample_t s[2] = { buf[f][0], buf[f][1] };

		// convert vol/pan values to left/right values
		const float left1 = m_ampControls.m_volumeModel.value() *
			( m_ampControls.m_panModel.value() <= 0
			? 1.0
			: 1.0 - m_ampControls.m_panModel.value( f ) / 100.0 );
		const float right1 = m_ampControls.m_volumeModel.value() *
			( m_ampControls.m_panModel.value() >= 0
			? 1.0
			: 1.0 + m_ampControls.m_panModel.value() / 100.0 );

		// first stage amplification
		s[0] *= ( left1 / 100.0 );
		s[1] *= ( right1 / 100.0 );

		// second stage amplification
		s[0] *= ( m_ampControls.m_leftModel.value() / 100.0 );
		s[1] *= ( m_ampControls.m_rightModel.value() / 100.0 );

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
	return new AmplifierEffect( parent, static_cast<const Plugin::Descriptor::SubPluginFeatures::Key *>( data ) );
}

}

