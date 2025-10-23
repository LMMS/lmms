/* Nes.cpp - A NES instrument plugin for LMMS
 *                        
 * Copyright (c) 2014 Vesa Kivimäki
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "Nes.h"

#include "AudioEngine.h"
#include "Engine.h"
#include "InstrumentTrack.h"
#include "Knob.h"
#include "Oscillator.h"

#include "embed.h"
#include "lmms_math.h"
#include "plugin_export.h"

namespace lmms
{


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT nes_plugin_descriptor =
{
	LMMS_STRINGIFY( PLUGIN_NAME ),
	"Nescaline",
	QT_TRANSLATE_NOOP( "PluginBrowser",
				"A NES-like synthesizer" ),
	"Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>",
	0x0100,
	Plugin::Type::Instrument,
	new PluginPixmapLoader( "logo" ),
	{},
	nullptr,
} ;

}


NesObject::NesObject( NesInstrument * nes, const sample_rate_t samplerate, NotePlayHandle * nph ) :
	m_parent( nes ),
	m_samplerate( samplerate ),
	m_nph( nph )
{
	m_pitchUpdateCounter = 0;
	m_pitchUpdateFreq = wavelength( 60.0f );	
	
	m_LFSR = LFSR_INIT;
	
	m_ch1Counter = 0;
	m_ch2Counter = 0;
	m_ch3Counter = 0;
	m_ch4Counter = 0;
	
	m_ch1EnvCounter = 0;
	m_ch2EnvCounter = 0;
	m_ch4EnvCounter = 0;
	
	m_ch1EnvValue = 15;
	m_ch2EnvValue = 15;
	m_ch4EnvValue = 15;
	
	m_ch1SweepCounter = 0;
	m_ch2SweepCounter = 0;
	m_ch4SweepCounter = 0;
	
	m_12Last = 0.0f;
	m_34Last = 0.0f;
	
	m_maxWlen = wavelength( MIN_FREQ );
	
	m_nsf = NES_SIMPLE_FILTER * ( m_samplerate / 44100.0 );
	
	m_lastNoteFreq = 0;
	m_lastNoiseFreq = -1.0f; // value that is always different than noisefreq so it gets updated at start
	
	m_vibratoPhase = 0;
	
	updatePitch();
}


void NesObject::renderOutput( SampleFrame* buf, fpp_t frames )
{
	////////////////////////////////
	//	                          //
	//  variables for processing  //
	//                            //
	////////////////////////////////
	
	bool ch1Enabled = m_parent->m_ch1Enabled.value();
	bool ch2Enabled = m_parent->m_ch2Enabled.value();
	bool ch3Enabled = m_parent->m_ch3Enabled.value();
	bool ch4Enabled = m_parent->m_ch4Enabled.value();
	
	float ch1DutyCycle = DUTY_CYCLE[ m_parent->m_ch1DutyCycle.value() ];
	int ch1EnvLen = wavelength( floorf( 240.0 / ( m_parent->m_ch1EnvLen.value() + 1 ) ) );
	bool ch1EnvLoop = m_parent->m_ch1EnvLooped.value();
	
	float ch2DutyCycle = DUTY_CYCLE[ m_parent->m_ch2DutyCycle.value() ];
	int ch2EnvLen = wavelength( floorf( 240.0 / ( m_parent->m_ch2EnvLen.value() + 1 ) ) );
	bool ch2EnvLoop = m_parent->m_ch2EnvLooped.value();
	
	int ch4EnvLen = wavelength( floorf( 240.0 / ( m_parent->m_ch4EnvLen.value() + 1 ) ) );
	bool ch4EnvLoop = m_parent->m_ch4EnvLooped.value();
	
	// processing variables for operators
	int ch1;
	int ch2;
	int ch3;
	int ch4;
	
	// levels for generators (used for dc offset compensation)
	int ch1Level;
	int ch2Level;
	int ch3Level;
	int ch4Level;
	
	int ch1SweepRate = wavelength( floorf( 120.0 / ( m_parent->m_ch1SweepRate.value() + 1 ) ) );
	int ch2SweepRate = wavelength( floorf( 120.0 / ( m_parent->m_ch2SweepRate.value() + 1 ) ) );
	int ch4SweepRate = wavelength( floorf( 60.0f / ( 8.0f - qAbs( m_parent->m_ch4Sweep.value() ) ) ) );

	int ch1Sweep = static_cast<int>( m_parent->m_ch1SweepAmt.value() * -1.0 );
	int ch2Sweep = static_cast<int>( m_parent->m_ch2SweepAmt.value() * -1.0 );

	int ch4Sweep = 0;
	if( m_parent->m_ch4Sweep.value() != 0.0f )
	{
		ch4Sweep = m_parent->m_ch4Sweep.value() > 0.0f
			? -1
			: 1;
	}

	// the amounts are inverted so we correct them here
	if( ch1Sweep > 0 )
	{
		ch1Sweep = 8 - ch1Sweep;
	}
	if( ch1Sweep < 0 )
	{
		ch1Sweep = -8 - ch1Sweep;
	}

	if( ch2Sweep > 0 )
	{
		ch2Sweep = 8 - ch2Sweep;
	}
	if( ch2Sweep < 0 )
	{
		ch2Sweep = -8 - ch2Sweep;
	}
	
		
	// start framebuffer loop
		
	for( f_cnt_t f = 0; f < frames; f++ )
	{
		////////////////////////////////
		//	                          //
		//        pitch update        //
		//                            //
		////////////////////////////////
		
		m_pitchUpdateCounter++;
		if( m_pitchUpdateCounter >= m_pitchUpdateFreq )
		{
			updatePitch();
			m_pitchUpdateCounter = 0;
		}


		////////////////////////////////
		//	                          //
		//        channel 1           //
		//                            //
		////////////////////////////////

		// render pulse wave
		if( m_wlen1 <= m_maxWlen && m_wlen1 >= MIN_WLEN && ch1Enabled )
		{
			ch1Level = m_parent->m_ch1EnvEnabled.value()
				? static_cast<int>( ( m_parent->m_ch1Volume.value() * m_ch1EnvValue ) / 15.0 )
				: static_cast<int>( m_parent->m_ch1Volume.value() );
			ch1 = m_ch1Counter > m_wlen1 * ch1DutyCycle 
				? 0
				: ch1Level;
		}
		else ch1 = ch1Level = 0;
		
		// update sweep
		m_ch1SweepCounter++;
		if( m_ch1SweepCounter >= ch1SweepRate )
		{
			m_ch1SweepCounter = 0;
			if( m_parent->m_ch1SweepEnabled.value() && m_wlen1 <= m_maxWlen && m_wlen1 >= MIN_WLEN )
			{
				// check if the sweep goes up or down
				if( ch1Sweep > 0 )
				{
					m_wlen1 += m_wlen1 >> qAbs( ch1Sweep );
				}
				if( ch1Sweep < 0 )
				{
					m_wlen1 -= m_wlen1 >> qAbs( ch1Sweep );
					m_wlen1--;  // additional minus 1 for ch1 only
				}
			}
		}
					
		// update framecounters
		m_ch1Counter++;
		m_ch1Counter = m_wlen1 ? m_ch1Counter % m_wlen1 : 0;

		m_ch1EnvCounter++;
		if( m_ch1EnvCounter >= ch1EnvLen )
		{
			m_ch1EnvCounter = 0;
			m_ch1EnvValue--;
			if( m_ch1EnvValue < 0 )
			{
				m_ch1EnvValue = ch1EnvLoop ? 15	: 0;
			}
		}
	

		////////////////////////////////
		//	                          //
		//        channel 2           //
		//                            //
		////////////////////////////////

		// render pulse wave
		if( m_wlen2 <= m_maxWlen && m_wlen2 >= MIN_WLEN && ch2Enabled )
		{
			ch2Level = m_parent->m_ch2EnvEnabled.value()
				? static_cast<int>( ( m_parent->m_ch2Volume.value() * m_ch2EnvValue ) / 15.0 )
				: static_cast<int>( m_parent->m_ch2Volume.value() );
			ch2 = m_ch2Counter > m_wlen2 * ch2DutyCycle 
				? 0
				: ch2Level;
		}
		else ch2 = ch2Level = 0;
		
		// update sweep
		m_ch2SweepCounter++;
		if( m_ch2SweepCounter >= ch2SweepRate )
		{
			m_ch2SweepCounter = 0;
			if( m_parent->m_ch2SweepEnabled.value() && m_wlen2 <= m_maxWlen && m_wlen2 >= MIN_WLEN )
			{				
				// check if the sweep goes up or down
				if( ch2Sweep > 0 )
				{
					m_wlen2 += m_wlen2 >> qAbs( ch2Sweep );
				}
				if( ch2Sweep < 0 )
				{
					m_wlen2 -= m_wlen2 >> qAbs( ch2Sweep );
				}
			}
		}
					
		// update framecounters
		m_ch2Counter++;
		m_ch2Counter = m_wlen2 ? m_ch2Counter % m_wlen2 : 0;
		
		m_ch2EnvCounter++;
		if( m_ch2EnvCounter >= ch2EnvLen )
		{
			m_ch2EnvCounter = 0;
			m_ch2EnvValue--;
			if( m_ch2EnvValue < 0 )
			{
				m_ch2EnvValue = ch2EnvLoop
					? 15
					: 0;
			}
		}
		
		
		////////////////////////////////
		//	                          //
		//        channel 3           //
		//                            //
		////////////////////////////////		
		
		// make sure we don't overflow
		m_ch3Counter = m_wlen3 ? m_ch3Counter % m_wlen3 : 0;
		
		// render triangle wave
		if( m_wlen3 <= m_maxWlen && ch3Enabled )
		{
			ch3Level = static_cast<int>( m_parent->m_ch3Volume.value() );
			ch3 = m_wlen3 ? TRIANGLE_WAVETABLE[ ( m_ch3Counter * 32 ) / m_wlen3 ] : 0;
			ch3 = ( ch3 * ch3Level ) / 15;
		}
		else ch3 = ch3Level = 0;
		
		m_ch3Counter++;
		
		
		////////////////////////////////
		//	                          //
		//        channel 4           //
		//                            //
		////////////////////////////////			
		
		// render pseudo noise 
		if( ch4Enabled )
		{
			ch4Level = m_parent->m_ch4EnvEnabled.value()
				? ( static_cast<int>( m_parent->m_ch4Volume.value() ) * m_ch4EnvValue ) / 15
				: static_cast<int>( m_parent->m_ch4Volume.value() );
			ch4 = LFSR()
				? ch4Level
				: 0;
		}
		else ch4 = ch4Level = 0;
		
		// update framecounters
		m_ch4Counter++;
		if( m_ch4Counter >= m_wlen4 )
		{
			m_ch4Counter = 0;
			updateLFSR( m_parent->m_ch4NoiseMode.value() );
		}
		m_ch4EnvCounter++;
		if( m_ch4EnvCounter >= ch4EnvLen )
		{
			m_ch4EnvCounter = 0;
			m_ch4EnvValue--;
			if( m_ch4EnvValue < 0 )
			{
				m_ch4EnvValue = ch4EnvLoop
					? 15
					: 0;
			}
		}
		
		m_ch4SweepCounter++;
		if( m_ch4SweepCounter >= ch4SweepRate )
		{
			m_ch4SweepCounter = 0;
			if( ch4Sweep != 0 )
			{
				int freqN = nearestNoiseFreq( static_cast<float>( m_samplerate ) / m_wlen4 );
				freqN = qBound( 0, freqN + ch4Sweep, 15 );
				m_wlen4 = wavelength( NOISE_FREQS[ freqN ] );

				if( m_wlen4 == 0 && ch4Sweep == 1 ) // a workaround for sweep getting stuck on 0 wavelength
				{
					while( m_wlen4 == 0 )
					{
						m_wlen4 = wavelength( NOISE_FREQS[ ++freqN ] );
					}
				}
			}
		}
		

		////////////////////////////////
		//	                          //
		//  final stage - mixing      //
		//                            //
		////////////////////////////////

		auto pin1 = static_cast<float>(ch1 + ch2);
		// add dithering noise
		pin1 *= 1.0 + ( Oscillator::noiseSample( 0.0f ) * DITHER_AMP );		
		pin1 = pin1 / 30.0f;
		
		pin1 = signedPowf(pin1, NES_DIST);
		
		pin1 = pin1 * 2.0f - 1.0f;
		
		// simple first order iir filter, to simulate the frequency response falloff in nes analog audio output
		pin1 = std::lerp(pin1, m_12Last, m_nsf);
		m_12Last = pin1;

		// compensate DC offset
		pin1 += 1.0f - signedPowf(static_cast<float>(ch1Level + ch2Level) / 30.0f, NES_DIST);
		
		pin1 *= NES_MIXING_12;

		auto pin2 = static_cast<float>(ch3 + ch4);
		// add dithering noise
		pin2 *= 1.0 + ( Oscillator::noiseSample( 0.0f ) * DITHER_AMP );		
		pin2 = pin2 / 30.0f;
		
		pin2 = signedPowf(pin2, NES_DIST);
		
		pin2 = pin2 * 2.0f - 1.0f;

		// simple first order iir filter, to simulate the frequency response falloff in nes analog audio output
		pin2 = std::lerp(pin2, m_34Last, m_nsf);
		m_34Last = pin2;
		
		// compensate DC offset
		pin2 += 1.0f - signedPowf(static_cast<float>(ch3Level + ch4Level) / 30.0f, NES_DIST);
		
		pin2 *= NES_MIXING_34;
		
		const float mixdown = ( pin1 + pin2 ) * NES_MIXING_ALL * m_parent->m_masterVol.value();

		buf[f][0] = mixdown;
		buf[f][1] = mixdown;
		
	} // end framebuffer loop

}


void NesObject::updateVibrato( float * freq )
{
	float vibratoAmt = floorf( m_parent->m_vibrato.value() ) / 15.0f;
	m_vibratoPhase++;
	m_vibratoPhase %= 32;
	float vibratoRatio = 1.0f + ( static_cast<float>( TRIANGLE_WAVETABLE[ m_vibratoPhase ] ) * 0.02f * vibratoAmt );
	*freq *= vibratoRatio;
}


void NesObject::updatePitch()
{
	float freq = m_nph->frequency();
	// if vibrato is active, update vibrato
	if( m_parent->m_vibrato.value() > 0 )
	{
		updateVibrato( &freq );
	}
	// check if frequency has changed, if so, update wavelengths of ch1-3
	if( freq != m_lastNoteFreq )
	{
		m_wlen1 = wavelength( freq * m_parent->m_freq1 );
		m_wlen2 = wavelength( freq * m_parent->m_freq2 );
		m_wlen3 = wavelength( freq * m_parent->m_freq3 );
	}
	// noise channel can use either note freq or preset freqs
	if( m_parent->m_ch4NoiseFreqMode.value() && freq != m_lastNoteFreq ) 
	{
		float f = freq * 2.0f;
		if( m_parent->m_ch4NoiseQuantize.value() ) // note freq can be quantized to the preset freqs
		{
			f = NOISE_FREQS[ nearestNoiseFreq( f ) ];
		}
		m_wlen4 = wavelength( f );
	}
	if( ! m_parent->m_ch4NoiseFreqMode.value() && m_lastNoiseFreq != m_parent->m_ch4NoiseFreq.value() )
	{
		m_wlen4 = wavelength( NOISE_FREQS[ 15 - static_cast<int>( m_parent->m_ch4NoiseFreq.value() ) ] );
		m_lastNoiseFreq = m_parent->m_ch4NoiseFreq.value();
	}
	
	m_lastNoteFreq = freq;
}




NesInstrument::NesInstrument( InstrumentTrack * instrumentTrack ) :
	Instrument( instrumentTrack, &nes_plugin_descriptor ),
	m_ch1Enabled(true, this, tr("Channel 1 enable")),
	m_ch1Crs( 0.f, -24.f, 24.f, 1.f, this, tr( "Channel 1 coarse detune" ) ),
	m_ch1Volume( 15.f, 0.f, 15.f, 1.f, this, tr( "Channel 1 volume" ) ),
	
	m_ch1EnvEnabled(false, this, tr("Channel 1 envelope enable")),
	m_ch1EnvLooped(false, this, tr("Channel 1 envelope loop")),
	m_ch1EnvLen( 0.f, 0.f, 15.f, 1.f, this, tr( "Channel 1 envelope length" ) ),
	
	m_ch1DutyCycle( 0, 0, 3, this, tr( "Channel 1 duty cycle" ) ),
	
	m_ch1SweepEnabled(false, this, tr("Channel 1 sweep enable")),
	m_ch1SweepAmt( 0.f, -7.f, 7.f, 1.f, this, tr( "Channel 1 sweep amount" ) ),
	m_ch1SweepRate( 0.f, 0.f, 7.f, 1.f, this, tr( "Channel 1 sweep rate" ) ),
	
	m_ch2Enabled(true, this, tr("Channel 2 enable")),
	m_ch2Crs( 0.f, -24.f, 24.f, 1.f, this, tr( "Channel 2 coarse detune" ) ),
	m_ch2Volume( 15.f, 0.f, 15.f, 1.f, this, tr( "Channel 2 volume" ) ),
	
	m_ch2EnvEnabled(false, this, tr("Channel 2 envelope enable")),
	m_ch2EnvLooped(false, this, tr("Channel 2 envelope loop")),
	m_ch2EnvLen( 0.f, 0.f, 15.f, 1.f, this, tr( "Channel 2 envelope length" ) ),
	
	m_ch2DutyCycle( 2, 0, 3, this, tr( "Channel 2 duty cycle" ) ),
	
	m_ch2SweepEnabled(false, this, tr("Channel 2 sweep enable")),
	m_ch2SweepAmt( 0.f, -7.f, 7.f, 1.f, this, tr( "Channel 2 sweep amount" ) ),
	m_ch2SweepRate( 0.f, 0.f, 7.f, 1.f, this, tr( "Channel 2 sweep rate" ) ),
	
	//channel 3
	m_ch3Enabled(true, this, tr("Channel 3 enable")),
	m_ch3Crs( 0.f, -24.f, 24.f, 1.f, this, tr( "Channel 3 coarse detune" ) ),
	m_ch3Volume( 15.f, 0.f, 15.f, 1.f, this, tr( "Channel 3 volume" ) ),

	//channel 4
	m_ch4Enabled(false, this, tr("Channel 4 enable")),
	m_ch4Volume( 15.f, 0.f, 15.f, 1.f, this, tr( "Channel 4 volume" ) ),
	
	m_ch4EnvEnabled(false, this, tr("Channel 4 envelope enable")),
	m_ch4EnvLooped(false, this, tr("Channel 4 envelope loop")),
	m_ch4EnvLen( 0.f, 0.f, 15.f, 1.f, this, tr( "Channel 4 envelope length" ) ),
	
	m_ch4NoiseMode(false, this, tr("Channel 4 noise mode")),
	m_ch4NoiseFreqMode(false, this, tr("Channel 4 frequency mode")),
	m_ch4NoiseFreq( 0.f, 0.f, 15.f, 1.f, this, tr( "Channel 4 noise frequency" ) ),
	
	m_ch4Sweep( 0.f, -7.f, 7.f, 1.f, this, tr( "Channel 4 noise frequency sweep" ) ),
	m_ch4NoiseQuantize(true, this, tr("Channel 4 quantize")),
	
	//master
	m_masterVol( 1.0f, 0.0f, 2.0f, 0.01f, this, tr( "Master volume" ) ),
	m_vibrato( 0.0f, 0.0f, 15.0f, 1.0f, this, tr( "Vibrato" ) )
{
	connect( &m_ch1Crs, SIGNAL( dataChanged() ), this, SLOT( updateFreq1() ), Qt::DirectConnection );
	connect( &m_ch2Crs, SIGNAL( dataChanged() ), this, SLOT( updateFreq2() ), Qt::DirectConnection );
	connect( &m_ch3Crs, SIGNAL( dataChanged() ), this, SLOT( updateFreq3() ), Qt::DirectConnection );
	
	updateFreq1();
	updateFreq2();
	updateFreq3();
}



void NesInstrument::playNote( NotePlayHandle * n, SampleFrame* workingBuffer )
{
	const fpp_t frames = n->framesLeftForCurrentPeriod();
	const f_cnt_t offset = n->noteOffset();
	
	if (!n->m_pluginData)
	{
		auto nes = new NesObject(this, Engine::audioEngine()->outputSampleRate(), n);
		n->m_pluginData = nes;
	}

	auto nes = static_cast<NesObject*>(n->m_pluginData);

	nes->renderOutput( workingBuffer + offset, frames );
	
	applyRelease( workingBuffer, n );
}


void NesInstrument::deleteNotePluginData( NotePlayHandle * n )
{
	delete static_cast<NesObject *>( n->m_pluginData );
}


void NesInstrument::saveSettings(  QDomDocument & doc, QDomElement & element )
{
		m_ch1Enabled.saveSettings( doc, element, "on1" );
		m_ch1Crs.saveSettings( doc, element, "crs1" );
		m_ch1Volume.saveSettings( doc, element, "vol1" );
	
		m_ch1EnvEnabled.saveSettings( doc, element, "envon1" );
		m_ch1EnvLooped.saveSettings( doc, element, "envloop1" );
		m_ch1EnvLen.saveSettings( doc, element, "envlen1" );
	
		m_ch1DutyCycle.saveSettings( doc, element, "dc1" );
	
		m_ch1SweepEnabled.saveSettings( doc, element, "sweep1" );
		m_ch1SweepAmt.saveSettings( doc, element, "swamt1" );
		m_ch1SweepRate.saveSettings( doc, element, "swrate1" );
	
	// channel 2
		m_ch2Enabled.saveSettings( doc, element, "on2" );
		m_ch2Crs.saveSettings( doc, element, "crs2" );
		m_ch2Volume.saveSettings( doc, element, "vol2" );
	
		m_ch2EnvEnabled.saveSettings( doc, element, "envon2" );
		m_ch2EnvLooped.saveSettings( doc, element, "envloop2" );
		m_ch2EnvLen.saveSettings( doc, element, "envlen2" );
	
		m_ch2DutyCycle.saveSettings( doc, element, "dc2" );
	
		m_ch2SweepEnabled.saveSettings( doc, element, "sweep2" );
		m_ch2SweepAmt.saveSettings( doc, element, "swamt2" );
		m_ch2SweepRate.saveSettings( doc, element, "swrate2" );	
	
	//channel 3
		m_ch3Enabled.saveSettings( doc, element, "on3" );
		m_ch3Crs.saveSettings( doc, element, "crs3" );
		m_ch3Volume.saveSettings( doc, element, "vol3" );

	//channel 4
		m_ch4Enabled.saveSettings( doc, element, "on4" );
		m_ch4Volume.saveSettings( doc, element, "vol4" );
	
		m_ch4EnvEnabled.saveSettings( doc, element, "envon4" );
		m_ch4EnvLooped.saveSettings( doc, element, "envloop4" );
		m_ch4EnvLen.saveSettings( doc, element, "envlen4" );
	
		m_ch4NoiseMode.saveSettings( doc, element, "nmode4" );
		m_ch4NoiseFreqMode.saveSettings( doc, element, "nfrqmode4" );
		m_ch4NoiseFreq.saveSettings( doc, element, "nfreq4" );
		
		m_ch4NoiseQuantize.saveSettings( doc, element, "nq4" );
		m_ch4Sweep.saveSettings( doc, element, "nswp4" );
	
	//master
		m_masterVol.saveSettings( doc, element, "vol" );
		m_vibrato.saveSettings( doc, element, "vibr" );	
}


void NesInstrument::loadSettings( const QDomElement & element )
{
		m_ch1Enabled.loadSettings(  element, "on1" );
		m_ch1Crs.loadSettings(  element, "crs1" );
		m_ch1Volume.loadSettings(  element, "vol1" );
	
		m_ch1EnvEnabled.loadSettings(  element, "envon1" );
		m_ch1EnvLooped.loadSettings(  element, "envloop1" );
		m_ch1EnvLen.loadSettings(  element, "envlen1" );
	
		m_ch1DutyCycle.loadSettings(  element, "dc1" );
	
		m_ch1SweepEnabled.loadSettings(  element, "sweep1" );
		m_ch1SweepAmt.loadSettings(  element, "swamt1" );
		m_ch1SweepRate.loadSettings(  element, "swrate1" );
	
	// channel 2
		m_ch2Enabled.loadSettings(  element, "on2" );
		m_ch2Crs.loadSettings(  element, "crs2" );
		m_ch2Volume.loadSettings(  element, "vol2" );
	
		m_ch2EnvEnabled.loadSettings(  element, "envon2" );
		m_ch2EnvLooped.loadSettings(  element, "envloop2" );
		m_ch2EnvLen.loadSettings(  element, "envlen2" );
	
		m_ch2DutyCycle.loadSettings(  element, "dc2" );
	
		m_ch2SweepEnabled.loadSettings(  element, "sweep2" );
		m_ch2SweepAmt.loadSettings(  element, "swamt2" );
		m_ch2SweepRate.loadSettings(  element, "swrate2" );	
	
	//channel 3
		m_ch3Enabled.loadSettings(  element, "on3" );
		m_ch3Crs.loadSettings(  element, "crs3" );
		m_ch3Volume.loadSettings(  element, "vol3" );

	//channel 4
		m_ch4Enabled.loadSettings(  element, "on4" );
		m_ch4Volume.loadSettings(  element, "vol4" );
	
		m_ch4EnvEnabled.loadSettings(  element, "envon4" );
		m_ch4EnvLooped.loadSettings(  element, "envloop4" );
		m_ch4EnvLen.loadSettings(  element, "envlen4" );
	
		m_ch4NoiseMode.loadSettings(  element, "nmode4" );
		m_ch4NoiseFreqMode.loadSettings(  element, "nfrqmode4" );
		m_ch4NoiseFreq.loadSettings(  element, "nfreq4" );
		
		m_ch4NoiseQuantize.loadSettings(  element, "nq4" );
		m_ch4Sweep.loadSettings(  element, "nswp4" );
	
	//master
		m_masterVol.loadSettings(  element, "vol" );
		m_vibrato.loadSettings(  element, "vibr" );	
}	


QString NesInstrument::nodeName() const
{
	return( nes_plugin_descriptor.name );
}


gui::PluginView* NesInstrument::instantiateView( QWidget * parent )
{
	return( new gui::NesInstrumentView( this, parent ) );
}



void NesInstrument::updateFreq1()
{
	m_freq1 = std::exp2(m_ch1Crs.value() / 12.0f);
}


void NesInstrument::updateFreq2()
{
	m_freq2 = std::exp2(m_ch2Crs.value() / 12.0f);
}


void NesInstrument::updateFreq3()
{
	m_freq3 = std::exp2(m_ch3Crs.value() / 12.0f);
}


namespace gui
{




NesInstrumentView::NesInstrumentView( Instrument * instrument,	QWidget * parent ) :
	InstrumentViewFixedSize( instrument, parent )
{
	setAutoFillBackground( true );
	QPalette pal;

	static auto s_artwork = PLUGIN_NAME::getIconPixmap("artwork");
	pal.setBrush(backgroundRole(), s_artwork);
	setPalette( pal );

	const int KNOB_Y1 = 24;
	const int KNOB_Y2 = 81;
	const int KNOB_Y3 = 138;
	const int KNOB_Y4 = 195;
	
	const int KNOB_X1 = 12;
	const int KNOB_X2 = 46;
	const int KNOB_X3 = 84;
	const int KNOB_X4 = 117;
	const int KNOB_X5 = 151;
	const int KNOB_X6 = 172;
	const int KNOB_X7 = 206;
	
	// channel 1
	
	makeknob( m_ch1VolumeKnob, KNOB_X1, KNOB_Y1, tr( "Volume" ), "", "" )
	makeknob( m_ch1CrsKnob, KNOB_X2, KNOB_Y1, tr( "Coarse detune" ), "", "" )
	makeknob( m_ch1EnvLenKnob, KNOB_X3, KNOB_Y1, tr( "Envelope length" ), "", "" )
	
	makenesled( m_ch1EnabledBtn, KNOB_X1, KNOB_Y1 - 12, tr( "Enable channel 1" ) )
	makenesled( m_ch1EnvEnabledBtn, KNOB_X3, KNOB_Y1 - 12, tr( "Enable envelope 1" ) )
	makenesled( m_ch1EnvLoopedBtn, 129, KNOB_Y1 - 12, tr( "Enable envelope 1 loop" ) )

	makenesled( m_ch1SweepEnabledBtn, KNOB_X6, KNOB_Y1 - 12, tr( "Enable sweep 1" ) )
	makeknob( m_ch1SweepAmtKnob, KNOB_X6, KNOB_Y1, tr( "Sweep amount" ), "", "" )
	makeknob( m_ch1SweepRateKnob, KNOB_X7, KNOB_Y1, tr( "Sweep rate" ), "", "" )

	int dcx = 117;
	makedcled( ch1_dc1, dcx, 42, tr( "12.5% Duty cycle" ), "nesdc1_on" )
	dcx += 13;
	makedcled( ch1_dc2, dcx, 42, tr( "25% Duty cycle" ), "nesdc2_on" )
	dcx += 13;
	makedcled( ch1_dc3, dcx, 42, tr( "50% Duty cycle" ), "nesdc3_on" )
	dcx += 13;
	makedcled( ch1_dc4, dcx, 42, tr( "75% Duty cycle" ), "nesdc4_on" )
		
	m_ch1DutyCycleGrp = new AutomatableButtonGroup( this );
	m_ch1DutyCycleGrp -> addButton( ch1_dc1 );
	m_ch1DutyCycleGrp -> addButton( ch1_dc2 );
	m_ch1DutyCycleGrp -> addButton( ch1_dc3 );
	m_ch1DutyCycleGrp -> addButton( ch1_dc4 );
	

	
	// channel 2
	
	makeknob( m_ch2VolumeKnob, KNOB_X1, KNOB_Y2, tr( "Volume" ), "", "" )
	makeknob( m_ch2CrsKnob, KNOB_X2, KNOB_Y2, tr( "Coarse detune" ), "", "" )
	makeknob( m_ch2EnvLenKnob, KNOB_X3, KNOB_Y2, tr( "Envelope length" ), "", "" )
	
	makenesled( m_ch2EnabledBtn, KNOB_X1, KNOB_Y2 - 12, tr( "Enable channel 2" ) )
	makenesled( m_ch2EnvEnabledBtn, KNOB_X3, KNOB_Y2 - 12, tr( "Enable envelope 2" ) )
	makenesled( m_ch2EnvLoopedBtn, 129, KNOB_Y2 - 12, tr( "Enable envelope 2 loop" ) )

	makenesled( m_ch2SweepEnabledBtn, KNOB_X6, KNOB_Y2 - 12, tr( "Enable sweep 2" ) )
	makeknob( m_ch2SweepAmtKnob, KNOB_X6, KNOB_Y2, tr( "Sweep amount" ), "", "" )
	makeknob( m_ch2SweepRateKnob, KNOB_X7, KNOB_Y2, tr( "Sweep rate" ), "", "" )

	dcx = 117;
	makedcled( ch2_dc1, dcx, 99, tr( "12.5% Duty cycle" ), "nesdc1_on" )
	dcx += 13;
	makedcled( ch2_dc2, dcx, 99, tr( "25% Duty cycle" ), "nesdc2_on" )
	dcx += 13;
	makedcled( ch2_dc3, dcx, 99, tr( "50% Duty cycle" ), "nesdc3_on" )
	dcx += 13;
	makedcled( ch2_dc4, dcx, 99, tr( "75% Duty cycle" ), "nesdc4_on" )
		
	m_ch2DutyCycleGrp = new AutomatableButtonGroup( this );
	m_ch2DutyCycleGrp -> addButton( ch2_dc1 );
	m_ch2DutyCycleGrp -> addButton( ch2_dc2 );
	m_ch2DutyCycleGrp -> addButton( ch2_dc3 );
	m_ch2DutyCycleGrp -> addButton( ch2_dc4 );


	
	//channel 3
	makenesled( m_ch3EnabledBtn, KNOB_X1, KNOB_Y3 - 12, tr( "Enable channel 3" ) )
	makeknob( m_ch3VolumeKnob, KNOB_X1, KNOB_Y3, tr( "Volume" ), "", "" )
	makeknob( m_ch3CrsKnob, KNOB_X2, KNOB_Y3, tr( "Coarse detune" ), "", "" )
	

	//channel 4
	makeknob( m_ch4VolumeKnob, KNOB_X1, KNOB_Y4, tr( "Volume" ), "", "" )
	makeknob( m_ch4NoiseFreqKnob, KNOB_X2, KNOB_Y4, tr( "Noise Frequency" ), "", "" )
	makeknob( m_ch4EnvLenKnob, KNOB_X3, KNOB_Y4, tr( "Envelope length" ), "", "" )
	makeknob( m_ch4SweepKnob, KNOB_X4, KNOB_Y4, tr( "Frequency sweep" ), "", "" )
	
	makenesled( m_ch4EnabledBtn, KNOB_X1, KNOB_Y4 - 12, tr( "Enable channel 4" ) )
	makenesled( m_ch4EnvEnabledBtn, KNOB_X3, KNOB_Y4 - 12, tr( "Enable envelope 4" ) )
	makenesled( m_ch4EnvLoopedBtn, 129, KNOB_Y4 - 12, tr( "Enable envelope 4 loop" ) )

	makenesled( m_ch4NoiseQuantizeBtn, 162, KNOB_Y4 - 12, tr( "Quantize noise frequency when using note frequency" ) )
	
	makenesled( m_ch4NoiseFreqModeBtn,  148, 203, tr( "Use note frequency for noise" ) )
	makenesled( m_ch4NoiseModeBtn, 148, 224, tr( "Noise mode" ) )

	
	//master
	makeknob( m_masterVolKnob, KNOB_X4, KNOB_Y3, tr( "Master volume" ), "", "" )
	makeknob( m_vibratoKnob, KNOB_X5, KNOB_Y3, tr( "Vibrato" ), "", "" )

}



void NesInstrumentView::modelChanged()
{
	auto nes = castModel<NesInstrument>();

	m_ch1EnabledBtn->setModel( &nes->m_ch1Enabled );
	m_ch1CrsKnob->setModel( &nes->m_ch1Crs );
	m_ch1VolumeKnob->setModel( &nes->m_ch1Volume );

	m_ch1EnvEnabledBtn->setModel( &nes->m_ch1EnvEnabled );
	m_ch1EnvLoopedBtn->setModel( &nes->m_ch1EnvLooped );
	m_ch1EnvLenKnob->setModel( &nes->m_ch1EnvLen );

	m_ch1DutyCycleGrp->setModel( &nes->m_ch1DutyCycle );

	m_ch1SweepEnabledBtn->setModel( &nes->m_ch1SweepEnabled );
	m_ch1SweepAmtKnob->setModel( &nes->m_ch1SweepAmt );
	m_ch1SweepRateKnob->setModel( &nes->m_ch1SweepRate );

	// channel 2
	m_ch2EnabledBtn->setModel( &nes->m_ch2Enabled );
	m_ch2CrsKnob->setModel( &nes->m_ch2Crs );
	m_ch2VolumeKnob->setModel( &nes->m_ch2Volume );

	m_ch2EnvEnabledBtn->setModel( &nes->m_ch2EnvEnabled );
	m_ch2EnvLoopedBtn->setModel( &nes->m_ch2EnvLooped );
	m_ch2EnvLenKnob->setModel( &nes->m_ch2EnvLen );

	m_ch2DutyCycleGrp->setModel( &nes->m_ch2DutyCycle );

	m_ch2SweepEnabledBtn->setModel( &nes->m_ch2SweepEnabled );
	m_ch2SweepAmtKnob->setModel( &nes->m_ch2SweepAmt );
	m_ch2SweepRateKnob->setModel( &nes->m_ch2SweepRate );

	//channel 3
	m_ch3EnabledBtn->setModel( &nes->m_ch3Enabled );
	m_ch3CrsKnob->setModel( &nes->m_ch3Crs );
	m_ch3VolumeKnob->setModel( &nes->m_ch3Volume );

	//channel 4
	m_ch4EnabledBtn->setModel( &nes->m_ch4Enabled );
	m_ch4VolumeKnob->setModel( &nes->m_ch4Volume );

	m_ch4EnvEnabledBtn->setModel( &nes->m_ch4EnvEnabled );
	m_ch4EnvLoopedBtn->setModel( &nes->m_ch4EnvLooped );
	m_ch4EnvLenKnob->setModel( &nes->m_ch4EnvLen );

	m_ch4NoiseModeBtn->setModel( &nes->m_ch4NoiseMode );
	m_ch4NoiseFreqModeBtn->setModel( &nes->m_ch4NoiseFreqMode );
	m_ch4NoiseFreqKnob->setModel( &nes->m_ch4NoiseFreq );

	m_ch4SweepKnob->setModel( &nes->m_ch4Sweep );
	m_ch4NoiseQuantizeBtn->setModel( &nes->m_ch4NoiseQuantize );

	//master
	m_masterVolKnob->setModel( &nes->m_masterVol );
	m_vibratoKnob->setModel( &nes->m_vibrato );		
}


} // namespace gui


extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model *m, void * _data )
{
	return( new NesInstrument( static_cast<InstrumentTrack *>( m ) ) );
}


}


} // namespace lmms
