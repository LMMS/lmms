/*
 * dynamics_processor.cpp - dynamics_processor effect-plugin
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


#include "dynamics_processor.h"
#include "lmms_math.h"
#include "interpolation.h"

#include "embed.cpp"

extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT dynamicsprocessor_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"Dynamics Processor",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"plugin for processing dynamics in a flexible way" ),
	"Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>",
	0x0100,
	Plugin::Effect,
	new PluginPixmapLoader( "logo" ),
	NULL,
	NULL
} ;

}



dynProcEffect::dynProcEffect( Model * _parent,
			const Descriptor::SubPluginFeatures::Key * _key ) :
	Effect( &dynamicsprocessor_plugin_descriptor, _parent, _key ),
	m_dpControls( this )
{
	currentPeak[0] = 0.0f;
	currentPeak[1] = 0.0f;
}




dynProcEffect::~dynProcEffect()
{
}




bool dynProcEffect::processAudioBuffer( sampleFrame * _buf,
							const fpp_t _frames )
{
	if( !isEnabled() || !isRunning () )
	{
//apparently we can't keep running after the decay value runs out so we'll just set the peaks to zero
		currentPeak[0] = 0.0f;
		currentPeak[1] = 0.0f;
		return( false );
		
/*		if( currentPeak[0] == 0.0f && currentPeak[1] == 0.0f ) return( false );
		else 
		{
			if( currentPeak[0] != 0.0f ) 
			{ 
				currentPeak[0] = qMax ( currentPeak[0] - 
				(( 1.0f / ( m_dpControls.m_releaseModel.value() / 1000.0f ) ) / engine::mixer()->processingSampleRate()), 0.0f );
			}
			if( currentPeak[1] != 0.0f ) 
			{ 
				currentPeak[1] = qMax ( currentPeak[1] - 
				(( 1.0f / ( m_dpControls.m_releaseModel.value() / 1000.0f ) ) / engine::mixer()->processingSampleRate()), 0.0f );
			}
			
			return( true );
		}	*/
	}

// variables for effect
	int i = 0;

	float sm_peak[2] = { 0.0f, 0.0f };
	float gain;

	double out_sum = 0.0;
	const float d = dryLevel();
	const float w = wetLevel();

// debug code	
//	qDebug( "peaks %f %f", currentPeak[0], currentPeak[1] );
	
	float att_tmp = ( 1.0f / ( m_dpControls.m_attackModel.value() / 1000.0f ) ) / engine::mixer()->processingSampleRate();
	float rel_tmp = ( 1.0f / ( m_dpControls.m_releaseModel.value() / 1000.0f ) ) / engine::mixer()->processingSampleRate();
	
	for( fpp_t f = 0; f < _frames; ++f )
	{
		sample_t s[2] = { _buf[f][0], _buf[f][1] };

// check for nan/inf because they may cause errors?
		if( isnanf( s[0] ) ) s[0] = 0.0f;
		if( isnanf( s[1] ) ) s[1] = 0.0f;
		if( isinff( s[0] ) ) s[0] = 0.0f;
		if( isinff( s[1] ) ) s[1] = 0.0f;
		

// update peak values
		for ( i=0; i <= 1; i++ )
		{
			if( qAbs( s[i] ) > currentPeak[i] )
			{
				currentPeak[i] = qMin ( currentPeak[i] + att_tmp, qAbs( s[i] ) );
			}
			else 
			if( qAbs( s[i] ) < currentPeak[i] )
			{
				currentPeak[i] = qMax ( currentPeak[i] - rel_tmp, qAbs( s[i] ) );
			}
			
			currentPeak[i] = qBound( 0.0f, currentPeak[i], 1.0f );
			
		}

// account for stereo mode
		switch( m_dpControls.m_stereomodeModel.value() )
		{
			case dynProcControls::SM_Maximum:
			{
				sm_peak[0] = qMax( currentPeak[0], currentPeak[1] );
				sm_peak[1] = qMax( currentPeak[0], currentPeak[1] );
				break;
			}
			case dynProcControls::SM_Average:
			{
				sm_peak[0] = ( currentPeak[0] + currentPeak[1] ) / 2;
				sm_peak[1] = ( currentPeak[0] + currentPeak[1] ) / 2;
				break;
			}
			case dynProcControls::SM_Unlinked:
			{
				sm_peak[0] = currentPeak[0];
				sm_peak[1] = currentPeak[1];
				break;
			}
		}

// apply input gain
		s[0] *= m_dpControls.m_inputModel.value();
		s[1] *= m_dpControls.m_inputModel.value();


// start effect

		for ( i=0; i <= 1; i++ )
		{
			const int lookup = static_cast<int>( sm_peak[i] * 200.0f );
			const float frac = fraction( sm_peak[i] * 200.0f ); 
			
			if( sm_peak[i] > 0 ) 
			{ 
				if ( lookup < 1 )
				{
					gain = frac * m_dpControls.m_wavegraphModel.samples()[0];
				}
				else
				if ( lookup < 200 )
				{
					gain = linearInterpolate( m_dpControls.m_wavegraphModel.samples()[ lookup - 1 ],
							m_dpControls.m_wavegraphModel.samples()[ lookup ], frac );
				}
				else
				{
					gain = m_dpControls.m_wavegraphModel.samples()[199];
				};
				
				s[i] *= ( gain / sm_peak[i] ); 
			}
		}

// apply output gain
		s[0] *= m_dpControls.m_outputModel.value();
		s[1] *= m_dpControls.m_outputModel.value();

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
	return( new dynProcEffect( _parent,
		static_cast<const Plugin::Descriptor::SubPluginFeatures::Key *>(
								_data ) ) );
}

}

