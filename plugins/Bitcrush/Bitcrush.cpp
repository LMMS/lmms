/*
 * Bitcrush.cpp - A native bitcrusher
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "Bitcrush.h"
#include "lmms_math.h"
#include "embed.h"
#include "plugin_export.h"

namespace lmms
{


const int OS_RATE = 5;
const float OS_RATIO = 1.0f / OS_RATE;
const float CUTOFF_RATIO = 0.353553391f;
const int SILENCEFRAMES = 10;
const auto OS_RESAMPLE = std::array{0.0001490062883964112f, 0.1645978376763992f, 0.6705063120704088f,
		0.1645978376763992f, 0.0001490062883964112f };

extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT bitcrush_plugin_descriptor =
{
	LMMS_STRINGIFY( PLUGIN_NAME ),
	"Bitcrush",
	QT_TRANSLATE_NOOP( "PluginBrowser", "An oversampling bitcrusher" ),
	"Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>",
	0x0100,
	Plugin::Type::Effect,
	new PluginPixmapLoader( "logo" ),
	{},
	nullptr,
};

}

BitcrushEffect::BitcrushEffect( Model * parent, const Descriptor::SubPluginFeatures::Key * key ) :
	Effect( &bitcrush_plugin_descriptor, parent, key ),
	m_controls( this ),
	m_sampleRate( Engine::audioEngine()->outputSampleRate() ),
	m_filter( m_sampleRate )
{
	m_buffer = new SampleFrame[Engine::audioEngine()->framesPerPeriod() * OS_RATE];
	m_filter.setLowpass( m_sampleRate * ( CUTOFF_RATIO * OS_RATIO ) );
	m_needsUpdate = true;

	m_bitCounterL = 0.0f;
	m_bitCounterR = 0.0f;

	m_left = 0.0f;
	m_right = 0.0f;

	m_silenceCounter = 0;
}

BitcrushEffect::~BitcrushEffect()
{
	delete[] m_buffer;
}


void BitcrushEffect::sampleRateChanged()
{
	m_sampleRate = Engine::audioEngine()->outputSampleRate();
	m_filter.setSampleRate( m_sampleRate );
	m_filter.setLowpass( m_sampleRate * ( CUTOFF_RATIO * OS_RATIO ) );
	m_needsUpdate = true;
}


inline float BitcrushEffect::depthCrush( float in )
{
	return roundf( in * (float) m_levels ) * m_levelsRatio;
}

inline float BitcrushEffect::noise( float amt )
{
	return fastRand(-amt, +amt);
}

Effect::ProcessStatus BitcrushEffect::processImpl(SampleFrame* buf, const fpp_t frames)
{
	// update values
	if( m_needsUpdate || m_controls.m_rateEnabled.isValueChanged() )
	{
		m_rateEnabled = m_controls.m_rateEnabled.value();
		m_bitCounterL = 0.0f;
		m_bitCounterR = 0.0f;
	}
	if( m_needsUpdate || m_controls.m_depthEnabled.isValueChanged() )
	{
		m_depthEnabled = m_controls.m_depthEnabled.value();
	}
	if( m_needsUpdate || m_controls.m_rate.isValueChanged() || m_controls.m_stereoDiff.isValueChanged() )
	{
		const float rate = m_controls.m_rate.value();
		const float diff = m_controls.m_stereoDiff.value() * 0.005 * rate;

		m_rateCoeffL = ( m_sampleRate * OS_RATE ) / ( rate - diff );
		m_rateCoeffR = ( m_sampleRate * OS_RATE ) / ( rate + diff );

		m_bitCounterL = 0.0f;
		m_bitCounterR = 0.0f;
	}
	if( m_needsUpdate || m_controls.m_levels.isValueChanged() )
	{
		m_levels = m_controls.m_levels.value();
		m_levelsRatio = 1.0f / (float) m_levels;
	}
	if( m_needsUpdate || m_controls.m_inGain.isValueChanged() )
	{
		m_inGain = dbfsToAmp( m_controls.m_inGain.value() );
	}
	if( m_needsUpdate || m_controls.m_outGain.isValueChanged() )
	{
		m_outGain = dbfsToAmp( m_controls.m_outGain.value() );
	}
	if( m_needsUpdate || m_controls.m_outClip.isValueChanged() )
	{
		m_outClip = dbfsToAmp( m_controls.m_outClip.value() );
	}
	m_needsUpdate = false;

	const float noiseAmt = m_controls.m_inNoise.value() * 0.01f;

	// read input buffer and write it to oversampled buffer
	if( m_rateEnabled ) // rate crushing enabled so do that
	{
		for (auto f = std::size_t{0}; f < frames; ++f)
		{
			for( int o = 0; o < OS_RATE; ++o )
			{
				m_buffer[f * OS_RATE + o][0] = m_left;
				m_buffer[f * OS_RATE + o][1] = m_right;
				m_bitCounterL += 1.0f;
				m_bitCounterR += 1.0f;
				if( m_bitCounterL > m_rateCoeffL )
				{
					m_bitCounterL -= m_rateCoeffL;
					m_left = m_depthEnabled
						? depthCrush( buf[f][0] * m_inGain + noise( buf[f][0] * noiseAmt ) )
						: buf[f][0] * m_inGain + noise( buf[f][0] * noiseAmt );
				}
				if( m_bitCounterR > m_rateCoeffR )
				{
					m_bitCounterR -= m_rateCoeffR;
					m_right = m_depthEnabled
						? depthCrush( buf[f][1] * m_inGain + noise( buf[f][1] * noiseAmt ) )
						: buf[f][1] * m_inGain + noise( buf[f][1] * noiseAmt );
				}
			}
		}
	}
	else // rate crushing disabled: simply oversample with zero-order hold
	{
		for (auto f = std::size_t{0}; f < frames; ++f)
		{
			for( int o = 0; o < OS_RATE; ++o )
			{
				m_buffer[f * OS_RATE + o][0] = m_depthEnabled
					? depthCrush( buf[f][0] * m_inGain + noise( buf[f][0] * noiseAmt ) )
					: buf[f][0] * m_inGain + noise( buf[f][0] * noiseAmt );
				m_buffer[f * OS_RATE + o][1] = m_depthEnabled
					? depthCrush( buf[f][1] * m_inGain + noise( buf[f][1] * noiseAmt ) )
					: buf[f][1] * m_inGain + noise( buf[f][1] * noiseAmt );
			}
		}
	}

	// the oversampled buffer is now written, so filter it to reduce aliasing

	for (auto f = std::size_t{0}; f < frames * OS_RATE; ++f)
	{
		if( qMax( qAbs( m_buffer[f][0] ), qAbs( m_buffer[f][1] ) ) >= 1.0e-10f )
		{
			m_silenceCounter = 0;
			m_buffer[f][0] = m_filter.update( m_buffer[f][0], 0 );
			m_buffer[f][1] = m_filter.update( m_buffer[f][1], 1 );
		}
		else
		{
			if( m_silenceCounter > SILENCEFRAMES )
			{
				m_buffer[f][0] = m_buffer[f][1] = 0.0f;
			}
			else
			{
				++m_silenceCounter;
				m_buffer[f][0] = m_filter.update( m_buffer[f][0], 0 );
				m_buffer[f][1] = m_filter.update( m_buffer[f][1], 1 );
			}
		}
	}


	// now downsample and write it back to main buffer

	const float d = dryLevel();
	const float w = wetLevel();
	for (auto f = std::size_t{0}; f < frames; ++f)
	{
		float lsum = 0.0f;
		float rsum = 0.0f;
		for( int o = 0; o < OS_RATE; ++o )
		{
			lsum += m_buffer[f * OS_RATE + o][0] * OS_RESAMPLE[o];
			rsum += m_buffer[f * OS_RATE + o][1] * OS_RESAMPLE[o];
		}
		buf[f][0] = d * buf[f][0] + w * qBound( -m_outClip, lsum, m_outClip ) * m_outGain;
		buf[f][1] = d * buf[f][1] + w * qBound( -m_outClip, rsum, m_outClip ) * m_outGain;
	}

	return ProcessStatus::ContinueIfNotQuiet;
}


extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model* parent, void* data )
{
	return new BitcrushEffect( parent, static_cast<const Plugin::Descriptor::SubPluginFeatures::Key *>( data ) );
}

}


} // namespace lmms
