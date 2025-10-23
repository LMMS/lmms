/*
 * DynamicsProcessor.cpp - DynamicsProcessor effect-plugin
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


#include "DynamicsProcessor.h"

#include <cmath>

#include "lmms_math.h"
#include "RmsHelper.h"

#include "embed.h"
#include "plugin_export.h"

namespace lmms
{


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT dynamicsprocessor_plugin_descriptor =
{
	LMMS_STRINGIFY( PLUGIN_NAME ),
	"Dynamics Processor",
	QT_TRANSLATE_NOOP( "PluginBrowser",
				"plugin for processing dynamics in a flexible way" ),
	"Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>",
	0x0100,
	Plugin::Type::Effect,
	new PluginPixmapLoader("logo"),
	{},
	nullptr,
} ;

}

const float DYN_NOISE_FLOOR = 0.00001f; // -100dBFS noise floor
const double DNF_LOG = -1.0;

DynProcEffect::DynProcEffect( Model * _parent,
			const Descriptor::SubPluginFeatures::Key * _key ) :
	Effect( &dynamicsprocessor_plugin_descriptor, _parent, _key ),
	m_dpControls( this )
{
	m_currentPeak[0] = m_currentPeak[1] = DYN_NOISE_FLOOR;
	m_rms[0] = new RmsHelper( 64 * Engine::audioEngine()->outputSampleRate() / 44100 );
	m_rms[1] = new RmsHelper( 64 * Engine::audioEngine()->outputSampleRate() / 44100 );
	calcAttack();
	calcRelease();
}




DynProcEffect::~DynProcEffect()
{
	delete m_rms[0];
	delete m_rms[1];
}


inline void DynProcEffect::calcAttack()
{
	m_attCoeff = std::exp((DNF_LOG / (m_dpControls.m_attackModel.value() * 0.001)) / Engine::audioEngine()->outputSampleRate());
}

inline void DynProcEffect::calcRelease()
{
	m_relCoeff = std::exp((DNF_LOG / (m_dpControls.m_releaseModel.value() * 0.001)) / Engine::audioEngine()->outputSampleRate());
}


Effect::ProcessStatus DynProcEffect::processImpl(SampleFrame* buf, const fpp_t frames)
{
	//qDebug( "%f %f", m_currentPeak[0], m_currentPeak[1] );

// variables for effect
	int i = 0;

	auto sm_peak = std::array{0.0f, 0.0f};

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
		m_rms[0]->setSize( 64 * Engine::audioEngine()->outputSampleRate() / 44100 );
		m_rms[1]->setSize( 64 * Engine::audioEngine()->outputSampleRate() / 44100 );
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

	for (fpp_t f = 0; f < frames; ++f)
	{
		auto s = std::array{buf[f][0], buf[f][1]};

// apply input gain
		s[0] *= inputGain;
		s[1] *= inputGain;

// update peak values
		for ( i=0; i <= 1; i++ )
		{
			const double t = m_rms[i]->update( s[i] );
			if( t > m_currentPeak[i] )
			{
				m_currentPeak[i] = m_currentPeak[i] * m_attCoeff + (1 - m_attCoeff) * t;
			}
			else
			if( t < m_currentPeak[i] )
			{
				m_currentPeak[i] = m_currentPeak[i] * m_relCoeff + (1 - m_relCoeff) * t;
			}

			m_currentPeak[i] = std::max(DYN_NOISE_FLOOR, m_currentPeak[i]);
		}

// account for stereo mode
		switch( static_cast<DynProcControls::StereoMode>(stereoMode) )
		{
			case DynProcControls::StereoMode::Maximum:
			{
				sm_peak[0] = sm_peak[1] = qMax( m_currentPeak[0], m_currentPeak[1] );
				break;
			}
			case DynProcControls::StereoMode::Average:
			{
				sm_peak[0] = sm_peak[1] = ( m_currentPeak[0] + m_currentPeak[1] ) * 0.5;
				break;
			}
			case DynProcControls::StereoMode::Unlinked:
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
				float gain;
				if (lookup < 1) { gain = frac * samples[0]; }
				else if (lookup < 200) { gain = std::lerp(samples[lookup - 1], samples[lookup], frac); }
				else { gain = samples[199]; }

				s[i] *= gain;
				s[i] /= sm_peak[i];
			}
		}

// apply output gain
		s[0] *= outputGain;
		s[1] *= outputGain;

// mix wet/dry signals
		buf[f][0] = d * buf[f][0] + w * s[0];
		buf[f][1] = d * buf[f][1] + w * s[1];
	}

	return ProcessStatus::ContinueIfNotQuiet;
}

void DynProcEffect::processBypassedImpl()
{
	// Apparently we can't keep running after the decay value runs out so we'll just set the peaks to zero
	m_currentPeak[0] = m_currentPeak[1] = DYN_NOISE_FLOOR;
}



extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model * _parent, void * _data )
{
	return( new DynProcEffect( _parent,
		static_cast<const Plugin::Descriptor::SubPluginFeatures::Key *>(
								_data ) ) );
}

}


} // namespace lmms
