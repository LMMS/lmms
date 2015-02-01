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

const float DYN_NOISE_FLOOR = 0.00001f; // -100dBV noise floor
const double DNF_LOG = 5.0;

dynProcEffect::dynProcEffect( Model * _parent,
			const Descriptor::SubPluginFeatures::Key * _key ) :
	Effect( &dynamicsprocessor_plugin_descriptor, _parent, _key ),
	m_dpControls( this )
{
	m_currentPeak[0] = m_currentPeak[1] = DYN_NOISE_FLOOR;
	m_rms[0] = new RmsHelper( 64 * engine::mixer()->processingSampleRate() / 44100 );
	m_rms[1] = new RmsHelper( 64 * engine::mixer()->processingSampleRate() / 44100 );
	calcAttack();
	calcRelease();
}




dynProcEffect::~dynProcEffect()
{
	delete m_rms[0];
	delete m_rms[1];
}


inline void dynProcEffect::calcAttack()
{
	m_attCoeff = exp10( ( DNF_LOG / ( m_dpControls.m_attackModel.value() * 0.001 ) ) / engine::mixer()->processingSampleRate() );
}

inline void dynProcEffect::calcRelease()
{
	m_relCoeff = exp10( ( -DNF_LOG / ( m_dpControls.m_releaseModel.value() * 0.001 ) ) / engine::mixer()->processingSampleRate() );
}


bool dynProcEffect::processAudioBuffer( sampleFrame * _buf,
							const fpp_t _frames )
{
	if( !isEnabled() || !isRunning () )
	{
//apparently we can't keep running after the decay value runs out so we'll just set the peaks to zero
		m_currentPeak[0] = m_currentPeak[1] = DYN_NOISE_FLOOR;
		return( false );
	}
	//qDebug( "%f %f", m_currentPeak[0], m_currentPeak[1] );

// variables for effect
	int i = 0;

	float sm_peak[2] = { 0.0f, 0.0f };
	float gain;

	double out_sum = 0.0;
	const float d = dryLevel();
	const float w = wetLevel();
	
	const int stereoMode = m_dpControls.m_stereomodeModel.value();
	const float inputGain = m_dpControls.m_inputModel.value();
	const float outputGain = m_dpControls.m_outputModel.value();
	
	const float * samples = m_dpControls.m_wavegraphModel.samples();

// debug code
//	qDebug( "peaks %f %f", m_currentPeak[0], m_currentPeak[1] );

	if( m_needsUpdate )
	{
		m_rms[0]->setSize( 64 * engine::mixer()->processingSampleRate() / 44100 );
		m_rms[1]->setSize( 64 * engine::mixer()->processingSampleRate() / 44100 );
		calcAttack();
		calcRelease();
		m_needsUpdate = false;
	}
	else
	{
		if( m_dpControls.m_attackModel.isValueChanged() )
		{
			calcAttack();
		}
		if( m_dpControls.m_releaseModel.isValueChanged() )
		{
			calcRelease();
		}
	}

	for( fpp_t f = 0; f < _frames; ++f )
	{
		double s[2] = { _buf[f][0], _buf[f][1] };

// apply input gain
		s[0] *= inputGain;
		s[1] *= inputGain;

// update peak values
		for ( i=0; i <= 1; i++ )
		{
			const double t = m_rms[i]->update( s[i] );
			if( t > m_currentPeak[i] )
			{
				m_currentPeak[i] = qMin( m_currentPeak[i] * m_attCoeff, t );
			}
			else
			if( t < m_currentPeak[i] )
			{
				m_currentPeak[i] = qMax( m_currentPeak[i] * m_relCoeff, t );
			}

			m_currentPeak[i] = qBound( DYN_NOISE_FLOOR, m_currentPeak[i], 10.0f );
		}

// account for stereo mode
		switch( stereoMode )
		{
			case dynProcControls::SM_Maximum:
			{
				sm_peak[0] = sm_peak[1] = qMax( m_currentPeak[0], m_currentPeak[1] );
				break;
			}
			case dynProcControls::SM_Average:
			{
				sm_peak[0] = sm_peak[1] = ( m_currentPeak[0] + m_currentPeak[1] ) * 0.5;
				break;
			}
			case dynProcControls::SM_Unlinked:
			{
				sm_peak[0] = m_currentPeak[0];
				sm_peak[1] = m_currentPeak[1];
				break;
			}
		}

// start effect

		for ( i=0; i <= 1; i++ )
		{
			const int lookup = static_cast<int>( sm_peak[i] * 200.0f );
			const float frac = fraction( sm_peak[i] * 200.0f );

			if( sm_peak[i] > DYN_NOISE_FLOOR )
			{
				if ( lookup < 1 )
				{
					gain = frac * samples[0];
				}
				else
				if ( lookup < 200 )
				{
					gain = linearInterpolate( samples[ lookup - 1 ],
							samples[ lookup ], frac );
				}
				else
				{
					gain = samples[199];
				};

				s[i] *= gain; 
				s[i] /= sm_peak[i];
			}
		}

// apply output gain
		s[0] *= outputGain;
		s[1] *= outputGain;

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
	return( new dynProcEffect( _parent,
		static_cast<const Plugin::Descriptor::SubPluginFeatures::Key *>(
								_data ) ) );
}

}

