/*
 * Watsyn.cpp - a 4-oscillator modulating wavetable synth
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
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

#include <QDomElement>

#include "Watsyn.h"
#include "base64.h"
#include "AudioEngine.h"
#include "Engine.h"
#include "InstrumentTrack.h"
#include "PixmapButton.h"
#include "Song.h"
#include "lmms_math.h"

#include "embed.h"
#include "plugin_export.h"

namespace lmms
{


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT watsyn_plugin_descriptor =
{
	LMMS_STRINGIFY( PLUGIN_NAME ),
	"Watsyn",
	QT_TRANSLATE_NOOP( "PluginBrowser",
				"4-oscillator modulatable wavetable synth" ),
	"Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>",
	0x0100,
	Plugin::Type::Instrument,
	new PluginPixmapLoader( "logo" ),
	nullptr,
	nullptr,
} ;

}




WatsynObject::WatsynObject( float * _A1wave, float * _A2wave,
					float * _B1wave, float * _B2wave,
					int _amod, int _bmod, const sample_rate_t _samplerate, NotePlayHandle * _nph, fpp_t _frames,
					WatsynInstrument * _w ) :
				m_amod( _amod ),
				m_bmod( _bmod ),
				m_samplerate( _samplerate ),
				m_nph( _nph ),
				m_fpp( _frames ),
				m_parent( _w )
{
	m_abuf = new SampleFrame[_frames];
	m_bbuf = new SampleFrame[_frames];

	m_lphase[A1_OSC] = 0.0f;
	m_lphase[A2_OSC] = 0.0f;
	m_lphase[B1_OSC] = 0.0f;
	m_lphase[B2_OSC] = 0.0f;

	m_rphase[A1_OSC] = 0.0f;
	m_rphase[A2_OSC] = 0.0f;
	m_rphase[B1_OSC] = 0.0f;
	m_rphase[B2_OSC] = 0.0f;

	// copy wavegraphs to the synth object to prevent race conditions

	memcpy( &m_A1wave, _A1wave, sizeof( m_A1wave ) );
	memcpy( &m_A2wave, _A2wave, sizeof( m_A2wave ) );
	memcpy( &m_B1wave, _B1wave, sizeof( m_B1wave ) );
	memcpy( &m_B2wave, _B2wave, sizeof( m_B2wave ) );
}



WatsynObject::~WatsynObject()
{
	delete[] m_abuf;
	delete[] m_bbuf;
}


void WatsynObject::renderOutput( fpp_t _frames )
{
	if( m_abuf == nullptr )
		m_abuf = new SampleFrame[m_fpp];
	if( m_bbuf == nullptr )
		m_bbuf = new SampleFrame[m_fpp];

	for( fpp_t frame = 0; frame < _frames; frame++ )
	{
		// put phases of 1-series oscs into variables because phase modulation might happen
		float A1_lphase = m_lphase[A1_OSC];
		float A1_rphase = m_rphase[A1_OSC];
		float B1_lphase = m_lphase[B1_OSC];
		float B1_rphase = m_rphase[B1_OSC];

		/////////////   A-series   /////////////////

		// A2
		sample_t A2_L = m_parent->m_lvol[A2_OSC] * std::lerp(
			m_A2wave[static_cast<int>(m_lphase[A2_OSC])],
			m_A2wave[static_cast<int>(m_lphase[A2_OSC] + 1) % WAVELEN],
			fraction(m_lphase[A2_OSC])
		);
		sample_t A2_R = m_parent->m_rvol[A2_OSC] * std::lerp(
			m_A2wave[static_cast<int>(m_rphase[A2_OSC])],
			m_A2wave[static_cast<int>(m_rphase[A2_OSC] + 1) % WAVELEN],
			fraction(m_rphase[A2_OSC])
		);

		// if phase mod, add to phases
		if( m_amod == MOD_PM )
		{
			A1_lphase = std::fmod(A1_lphase + A2_L * PMOD_AMT, WAVELEN);
			if( A1_lphase < 0 ) A1_lphase += WAVELEN;
			A1_rphase = std::fmod(A1_rphase + A2_R * PMOD_AMT, WAVELEN);
			if( A1_rphase < 0 ) A1_rphase += WAVELEN;
		}
		// A1
		sample_t A1_L = m_parent->m_lvol[A1_OSC] * std::lerp(
			m_A1wave[static_cast<int>(A1_lphase)],
			m_A1wave[static_cast<int>(A1_lphase + 1) % WAVELEN],
			fraction(A1_lphase)
		);
		sample_t A1_R = m_parent->m_rvol[A1_OSC] * std::lerp(
			m_A1wave[static_cast<int>(A1_rphase)],
			m_A1wave[static_cast<int>(A1_rphase + 1) % WAVELEN],
			fraction(A1_rphase)
		);

		/////////////   B-series   /////////////////

		// B2
		sample_t B2_L = m_parent->m_lvol[B2_OSC] * std::lerp(
			m_B2wave[static_cast<int>(m_lphase[B2_OSC])],
			m_B2wave[static_cast<int>(m_lphase[B2_OSC] + 1) % WAVELEN],
			fraction(m_lphase[B2_OSC])
		);
		sample_t B2_R = m_parent->m_rvol[B2_OSC] * std::lerp(
			m_B2wave[static_cast<int>(m_rphase[B2_OSC])],
			m_B2wave[static_cast<int>(m_rphase[B2_OSC] + 1) % WAVELEN],
			fraction(m_rphase[B2_OSC])
		);

		// if crosstalk active, add a1
		const float xt = m_parent->m_xtalk.value();
		if( xt > 0.0 )
		{
			B2_L += ( A1_L * xt ) * 0.01f;
			B2_R += ( A1_R * xt ) * 0.01f;
		}

		// if phase mod, add to phases
		if( m_bmod == MOD_PM )
		{
			B1_lphase = std::fmod(B1_lphase + B2_L * PMOD_AMT, WAVELEN);
			if( B1_lphase < 0 ) B1_lphase += WAVELEN;
			B1_rphase = std::fmod(B1_rphase + B2_R * PMOD_AMT, WAVELEN);
			if( B1_rphase < 0 ) B1_rphase += WAVELEN;
		}
		// B1
		sample_t B1_L = m_parent->m_lvol[B1_OSC] * std::lerp(
			m_B1wave[static_cast<int>(B1_lphase) % WAVELEN],
			m_B1wave[static_cast<int>(B1_lphase + 1) % WAVELEN],
			fraction(B1_lphase)
		);
		sample_t B1_R = m_parent->m_rvol[B1_OSC] * std::lerp(
			m_B1wave[static_cast<int>(B1_rphase) % WAVELEN],
			m_B1wave[static_cast<int>(B1_rphase + 1) % WAVELEN],
			fraction(B1_rphase)
		);


		// A-series modulation)
		switch( m_amod )
		{
			case MOD_MIX:
				A1_L = ( A1_L + A2_L ) / 2.0;
				A1_R = ( A1_R + A2_R ) / 2.0;
				break;
			case MOD_AM:
				A1_L *= qMax( 0.0f, A2_L + 1.0f );
				A1_R *= qMax( 0.0f, A2_R + 1.0f );
				break;
			case MOD_RM:
				A1_L *= A2_L;
				A1_R *= A2_R;
				break;
		}
		m_abuf[frame][0] = A1_L;
		m_abuf[frame][1] = A1_R;

		// B-series modulation (other than phase mod)
		switch( m_bmod )
		{
			case MOD_MIX:
				B1_L = ( B1_L + B2_L ) / 2.0;
				B1_R = ( B1_R + B2_R ) / 2.0;
				break;
			case MOD_AM:
				B1_L *= qMax( 0.0f, B2_L + 1.0f );
				B1_R *= qMax( 0.0f, B2_R + 1.0f );
				break;
			case MOD_RM:
				B1_L *= B2_L;
				B1_R *= B2_R;
				break;
		}
		m_bbuf[frame][0] = B1_L;
		m_bbuf[frame][1] = B1_R;

		// update phases
		for( int i = 0; i < NUM_OSCS; i++ )
		{
			m_lphase[i] += ( static_cast<float>( WAVELEN ) / ( m_samplerate / ( m_nph->frequency() * m_parent->m_lfreq[i] ) ) );
			m_lphase[i] = std::fmod(m_lphase[i], WAVELEN);
			m_rphase[i] += ( static_cast<float>( WAVELEN ) / ( m_samplerate / ( m_nph->frequency() * m_parent->m_rfreq[i] ) ) );
			m_rphase[i] = std::fmod(m_rphase[i], WAVELEN);
		}
	}

}



WatsynInstrument::WatsynInstrument( InstrumentTrack * _instrument_track ) :
		Instrument( _instrument_track, &watsyn_plugin_descriptor ),

		a1_vol( 100.0f, 0.0f, 200.0f, 0.1f, this, tr( "Volume A1" ) ),
		a2_vol( 100.0f, 0.0f, 200.0f, 0.1f, this, tr( "Volume A2" ) ),
		b1_vol( 100.0f, 0.0f, 200.0f, 0.1f, this, tr( "Volume B1" ) ),
		b2_vol( 100.0f, 0.0f, 200.0f, 0.1f, this, tr( "Volume B2" ) ),

		a1_pan( 0.0f, -100.0f, 100.0f, 0.1f, this, tr( "Panning A1" ) ),
		a2_pan( 0.0f, -100.0f, 100.0f, 0.1f, this, tr( "Panning A2" ) ),
		b1_pan( 0.0f, -100.0f, 100.0f, 0.1f, this, tr( "Panning B1" ) ),
		b2_pan( 0.0f, -100.0f, 100.0f, 0.1f, this, tr( "Panning B2" ) ),

		a1_mult( 8.0f, 1.0, 24.0, 1.0, this, tr( "Freq. multiplier A1" ) ),
		a2_mult( 8.0f, 1.0, 24.0, 1.0, this, tr( "Freq. multiplier A2" ) ),
		b1_mult( 8.0f, 1.0, 24.0, 1.0, this, tr( "Freq. multiplier B1" ) ),
		b2_mult( 8.0f, 1.0, 24.0, 1.0, this, tr( "Freq. multiplier B2" ) ),

		a1_ltune( 0.0f, -600.0f, 600.0f, 1.0f, this, tr( "Left detune A1" ) ),
		a2_ltune( 0.0f, -600.0f, 600.0f, 1.0f, this, tr( "Left detune A2" ) ),
		b1_ltune( 0.0f, -600.0f, 600.0f, 1.0f, this, tr( "Left detune B1" ) ),
		b2_ltune( 0.0f, -600.0f, 600.0f, 1.0f, this, tr( "Left detune B2" ) ),

		a1_rtune( 0.0f, -600.0f, 600.0f, 1.0f, this, tr( "Right detune A1" ) ),
		a2_rtune( 0.0f, -600.0f, 600.0f, 1.0f, this, tr( "Right detune A2" ) ),
		b1_rtune( 0.0f, -600.0f, 600.0f, 1.0f, this, tr( "Right detune B1" ) ),
		b2_rtune( 0.0f, -600.0f, 600.0f, 1.0f, this, tr( "Right detune B2" ) ),

		a1_graph( -1.0f, 1.0f, GRAPHLEN, this ),
		a2_graph( -1.0f, 1.0f, GRAPHLEN, this ),
		b1_graph( -1.0f, 1.0f, GRAPHLEN, this ),
		b2_graph( -1.0f, 1.0f, GRAPHLEN, this ),

		m_abmix( 0.0f, -100.0f, 100.0f, 0.1f, this, tr( "A-B Mix" ) ),
		m_envAmt( 0.0f, -200.0f, 200.0f, 1.0f, this, tr( "A-B Mix envelope amount" ) ),

		m_envAtt( 0.0f, 0.0f, 2000.0f, 1.0f, 2000.0f, this, tr( "A-B Mix envelope attack" ) ),
		m_envHold( 0.0f, 0.0f, 2000.0f, 1.0f, 2000.0f, this, tr( "A-B Mix envelope hold" ) ),
		m_envDec( 0.0f, 0.0f, 2000.0f, 1.0f, 2000.0f, this, tr( "A-B Mix envelope decay" ) ),

		m_xtalk( 0.0f, 0.0f, 100.0f, 0.1f, this, tr( "A1-B2 Crosstalk" ) ),

		m_amod( 0, 0, 3, this, tr( "A2-A1 modulation" ) ),
		m_bmod( 0, 0, 3, this, tr( "B2-B1 modulation" ) ),

		m_selectedGraph( 0, 0, 3, this, tr( "Selected graph" ) )
{
	connect( &a1_vol, SIGNAL( dataChanged() ), this, SLOT( updateVolumes() ) );
	connect( &a2_vol, SIGNAL( dataChanged() ), this, SLOT( updateVolumes() ) );
	connect( &b1_vol, SIGNAL( dataChanged() ), this, SLOT( updateVolumes() ) );
	connect( &b2_vol, SIGNAL( dataChanged() ), this, SLOT( updateVolumes() ) );

	connect( &a1_pan, SIGNAL( dataChanged() ), this, SLOT( updateVolumes() ) );
	connect( &a2_pan, SIGNAL( dataChanged() ), this, SLOT( updateVolumes() ) );
	connect( &b1_pan, SIGNAL( dataChanged() ), this, SLOT( updateVolumes() ) );
	connect( &b2_pan, SIGNAL( dataChanged() ), this, SLOT( updateVolumes() ) );

	connect( &a1_mult, SIGNAL( dataChanged() ), this, SLOT( updateFreqA1() ) );
	connect( &a2_mult, SIGNAL( dataChanged() ), this, SLOT( updateFreqA2() ) );
	connect( &b1_mult, SIGNAL( dataChanged() ), this, SLOT( updateFreqB1() ) );
	connect( &b2_mult, SIGNAL( dataChanged() ), this, SLOT( updateFreqB2() ) );

	connect( &a1_ltune, SIGNAL( dataChanged() ), this, SLOT( updateFreqA1() ) );
	connect( &a2_ltune, SIGNAL( dataChanged() ), this, SLOT( updateFreqA2() ) );
	connect( &b1_ltune, SIGNAL( dataChanged() ), this, SLOT( updateFreqB1() ) );
	connect( &b2_ltune, SIGNAL( dataChanged() ), this, SLOT( updateFreqB2() ) );

	connect( &a1_rtune, SIGNAL( dataChanged() ), this, SLOT( updateFreqA1() ) );
	connect( &a2_rtune, SIGNAL( dataChanged() ), this, SLOT( updateFreqA2() ) );
	connect( &b1_rtune, SIGNAL( dataChanged() ), this, SLOT( updateFreqB1() ) );
	connect( &b2_rtune, SIGNAL( dataChanged() ), this, SLOT( updateFreqB2() ) );
	
	connect( &a1_graph, SIGNAL( samplesChanged( int, int ) ), this, SLOT( updateWaveA1() ) );
	connect( &a2_graph, SIGNAL( samplesChanged( int, int ) ), this, SLOT( updateWaveA2() ) );
	connect( &b1_graph, SIGNAL( samplesChanged( int, int ) ), this, SLOT( updateWaveB1() ) );
	connect( &b2_graph, SIGNAL( samplesChanged( int, int ) ), this, SLOT( updateWaveB2() ) );

	a1_graph.setWaveToSine();
	a2_graph.setWaveToSine();
	b1_graph.setWaveToSine();
	b2_graph.setWaveToSine();

	updateVolumes();
	updateFreqA1();
	updateFreqA2();
	updateFreqB1();
	updateFreqB2();
	updateWaveA1();
	updateWaveA2();
	updateWaveB1();
	updateWaveB2();
}


void WatsynInstrument::playNote( NotePlayHandle * _n,
						SampleFrame* _working_buffer )
{
	if (!_n->m_pluginData)
	{
		auto w = new WatsynObject(&A1_wave[0], &A2_wave[0], &B1_wave[0], &B2_wave[0], m_amod.value(), m_bmod.value(),
			Engine::audioEngine()->outputSampleRate(), _n, Engine::audioEngine()->framesPerPeriod(), this);

		_n->m_pluginData = w;
	}

	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	const f_cnt_t offset = _n->noteOffset();
	SampleFrame* buffer = _working_buffer + offset;

	auto w = static_cast<WatsynObject*>(_n->m_pluginData);

	SampleFrame* abuf = w->abuf();
	SampleFrame* bbuf = w->bbuf();

	w-> renderOutput( frames );

	// envelope parameters
	const float envAmt = m_envAmt.value();
	const float envAtt = ( m_envAtt.value() * w->samplerate() ) / 1000.0f;
	const float envHold = ( m_envHold.value() * w->samplerate() ) / 1000.0f;
	const float envDec = ( m_envDec.value() * w->samplerate() ) / 1000.0f;
	const float envLen = envAtt + envDec + envHold;
	const auto tfp_ = static_cast<float>(_n->totalFramesPlayed());

	// if sample-exact is enabled, use sample-exact calculations...
	// disabled pending proper implementation of sample-exactness
/*	if( engine::audioEngine()->currentQualitySettings().sampleExactControllers )
	{
		for( fpp_t f=0; f < frames; f++ )
		{
			const float tfp = tfp_ + f;
			// handle mixing envelope
			float mixvalue = m_abmix.value( f );
			if( envAmt != 0.0f && tfp < envLen )
			{
				if( tfp < envAtt )
				{
					mixvalue = qBound( -100.0f, mixvalue + ( tfp / envAtt * envAmt ), 100.0f );
				}
				else if ( tfp >= envAtt && tfp < envAtt + envHold )
				{
					mixvalue = qBound( -100.0f, mixvalue + envAmt, 100.0f );
				}
				else
				{
					mixvalue = qBound( -100.0f, mixvalue + envAmt - ( ( tfp - ( envAtt + envHold ) ) / envDec * envAmt ), 100.0f );
				}
			}
			// get knob values in sample-exact way
			const float bmix = ( ( mixvalue + 100.0 ) / 200.0 );
			const float amix = 1.0 - bmix;

			// mix a/b streams according to mixing knob
			_working_buffer[f][0] = ( abuf[f][0] * amix ) +
									( bbuf[f][0] * bmix );
			_working_buffer[f][1] = ( abuf[f][1] * amix ) +
									( bbuf[f][1] * bmix );
		}
	}
	else*/ 
	
	// if sample-exact is not enabled, use simpler calculations:
	// if mix envelope is active, and we haven't gone past the envelope end, use envelope-aware calculation...
	if( envAmt != 0.0f && tfp_ < envLen )
	{
		const float mixvalue_ = m_abmix.value();
		for( fpp_t f=0; f < frames; f++ )
		{
			float mixvalue = mixvalue_;
			const float tfp = tfp_ + f;
			// handle mixing envelope
			if( tfp < envAtt )
			{
				mixvalue = qBound( -100.0f, mixvalue + ( tfp / envAtt * envAmt ), 100.0f );
			}
			else if ( tfp >= envAtt && tfp < envAtt + envHold )
			{
				mixvalue = qBound( -100.0f, mixvalue + envAmt, 100.0f );
			}
			else
			{
				mixvalue = qBound( -100.0f, mixvalue + envAmt - ( ( tfp - ( envAtt + envHold ) ) / envDec * envAmt ), 100.0f );
			}

			// get knob values
			const float bmix = ( ( mixvalue + 100.0 ) / 200.0 );
			const float amix = 1.0 - bmix;

			// mix a/b streams according to mixing knob
			buffer[f][0] = ( abuf[f][0] * amix ) +
									( bbuf[f][0] * bmix );
			buffer[f][1] = ( abuf[f][1] * amix ) +
									( bbuf[f][1] * bmix );
		}
	}

	// ... mix envelope is inactive or we've past the end of envelope, so use a faster calculation to save cpu
	else
	{
		// get knob values
		const float bmix = ( ( m_abmix.value() + 100.0 ) / 200.0 );
		const float amix = 1.0 - bmix;
		for( fpp_t f=0; f < frames; f++ )
		{
			// mix a/b streams according to mixing knob
			buffer[f][0] = ( abuf[f][0] * amix ) +
									( bbuf[f][0] * bmix );
			buffer[f][1] = ( abuf[f][1] * amix ) +
									( bbuf[f][1] * bmix );
		}
	}

	applyRelease( _working_buffer, _n );
}


void WatsynInstrument::deleteNotePluginData( NotePlayHandle * _n )
{
	delete static_cast<WatsynObject *>( _n->m_pluginData );
}


void WatsynInstrument::saveSettings( QDomDocument & _doc,
							QDomElement & _this )
{
	a1_vol.saveSettings( _doc, _this, "a1_vol" );
	a2_vol.saveSettings( _doc, _this, "a2_vol" );
	b1_vol.saveSettings( _doc, _this, "b1_vol" );
	b2_vol.saveSettings( _doc, _this, "b2_vol" );

	a1_pan.saveSettings( _doc, _this, "a1_pan" );
	a2_pan.saveSettings( _doc, _this, "a2_pan" );
	b1_pan.saveSettings( _doc, _this, "b1_pan" );
	b2_pan.saveSettings( _doc, _this, "b2_pan" );

	a1_mult.saveSettings( _doc, _this, "a1_mult" );
	a2_mult.saveSettings( _doc, _this, "a2_mult" );
	b1_mult.saveSettings( _doc, _this, "b1_mult" );
	b2_mult.saveSettings( _doc, _this, "b2_mult" );

	a1_ltune.saveSettings( _doc, _this, "a1_ltune" );
	a2_ltune.saveSettings( _doc, _this, "a2_ltune" );
	b1_ltune.saveSettings( _doc, _this, "b1_ltune" );
	b2_ltune.saveSettings( _doc, _this, "b2_ltune" );

	a1_rtune.saveSettings( _doc, _this, "a1_rtune" );
	a2_rtune.saveSettings( _doc, _this, "a2_rtune" );
	b1_rtune.saveSettings( _doc, _this, "b1_rtune" );
	b2_rtune.saveSettings( _doc, _this, "b2_rtune" );

	// save graphs
	QString sampleString;

	base64::encode( (const char *)a1_graph.samples(), a1_graph.length() * sizeof(float), sampleString );
	_this.setAttribute( "a1_wave", sampleString );
	base64::encode( (const char *)a2_graph.samples(), a2_graph.length() * sizeof(float), sampleString );
	_this.setAttribute( "a2_wave", sampleString );
	base64::encode( (const char *)b1_graph.samples(), b1_graph.length() * sizeof(float), sampleString );
	_this.setAttribute( "b1_wave", sampleString );
	base64::encode( (const char *)b2_graph.samples(), b2_graph.length() * sizeof(float), sampleString );
	_this.setAttribute( "b2_wave", sampleString );

	m_abmix.saveSettings( _doc, _this, "abmix" );
	m_envAmt.saveSettings( _doc, _this, "envAmt" );
	m_envAtt.saveSettings( _doc, _this, "envAtt" );
	m_envHold.saveSettings( _doc, _this, "envHold" );
	m_envDec.saveSettings( _doc, _this, "envDec" );

	m_xtalk.saveSettings( _doc, _this, "xtalk" );

	m_amod.saveSettings( _doc, _this, "amod" );
	m_bmod.saveSettings( _doc, _this, "bmod" );
/*	m_selectedGraph.saveSettings( _doc, _this, "selgraph" );*/
}


void WatsynInstrument::loadSettings( const QDomElement & _this )
{
	a1_vol.loadSettings( _this, "a1_vol" );
	a2_vol.loadSettings( _this, "a2_vol" );
	b1_vol.loadSettings( _this, "b1_vol" );
	b2_vol.loadSettings( _this, "b2_vol" );

	a1_pan.loadSettings( _this, "a1_pan" );
	a2_pan.loadSettings( _this, "a2_pan" );
	b1_pan.loadSettings( _this, "b1_pan" );
	b2_pan.loadSettings( _this, "b2_pan" );

	a1_mult.loadSettings( _this, "a1_mult" );
	a2_mult.loadSettings( _this, "a2_mult" );
	b1_mult.loadSettings( _this, "b1_mult" );
	b2_mult.loadSettings( _this, "b2_mult" );

	a1_ltune.loadSettings( _this, "a1_ltune" );
	a2_ltune.loadSettings( _this, "a2_ltune" );
	b1_ltune.loadSettings( _this, "b1_ltune" );
	b2_ltune.loadSettings( _this, "b2_ltune" );

	a1_rtune.loadSettings( _this, "a1_rtune" );
	a2_rtune.loadSettings( _this, "a2_rtune" );
	b1_rtune.loadSettings( _this, "b1_rtune" );
	b2_rtune.loadSettings( _this, "b2_rtune" );

	// load graphs
	int size = 0;
	char * dst = 0;

	base64::decode( _this.attribute( "a1_wave"), &dst, &size );
	a1_graph.setSamples( (float*) dst );
	base64::decode( _this.attribute( "a2_wave"), &dst, &size );
	a2_graph.setSamples( (float*) dst );
	base64::decode( _this.attribute( "b1_wave"), &dst, &size );
	b1_graph.setSamples( (float*) dst );
	base64::decode( _this.attribute( "b2_wave"), &dst, &size );
	b2_graph.setSamples( (float*) dst );

	delete[] dst;

	m_abmix.loadSettings( _this, "abmix" );

	m_envAmt.loadSettings( _this, "envAmt" );
	m_envAtt.loadSettings( _this, "envAtt" );
	m_envHold.loadSettings( _this, "envHold" );
	m_envDec.loadSettings( _this, "envDec" );

	m_xtalk.loadSettings( _this, "xtalk" );

	m_amod.loadSettings( _this, "amod" );
	m_bmod.loadSettings( _this, "bmod" );
/*	m_selectedGraph.loadSettings( _this, "selgraph" );*/
}


QString WatsynInstrument::nodeName() const
{
	return( watsyn_plugin_descriptor.name );
}


gui::PluginView* WatsynInstrument::instantiateView( QWidget * _parent )
{
	return( new gui::WatsynView( this, _parent ) );
}


void WatsynInstrument::updateVolumes()
{
	m_lvol[A1_OSC] = leftCh( a1_vol.value(), a1_pan.value() );
	m_rvol[A1_OSC] = rightCh( a1_vol.value(), a1_pan.value() );

	m_lvol[A2_OSC] = leftCh( a2_vol.value(), a2_pan.value() );
	m_rvol[A2_OSC] = rightCh( a2_vol.value(), a2_pan.value() );

	m_lvol[B1_OSC] = leftCh( b1_vol.value(), b1_pan.value() );
	m_rvol[B1_OSC] = rightCh( b1_vol.value(), b1_pan.value() );

	m_lvol[B2_OSC] = leftCh( b2_vol.value(), b2_pan.value() );
	m_rvol[B2_OSC] = rightCh( b2_vol.value(), b2_pan.value() );
}


void WatsynInstrument::updateFreqA1()
{
	// calculate frequencies
	m_lfreq[A1_OSC] = (a1_mult.value() / 8) * std::exp2(a1_ltune.value() / 1200);
	m_rfreq[A1_OSC] = (a1_mult.value() / 8) * std::exp2(a1_rtune.value() / 1200);
}


void WatsynInstrument::updateFreqA2()
{
	// calculate frequencies
	m_lfreq[A2_OSC] = (a2_mult.value() / 8) * std::exp2(a2_ltune.value() / 1200);
	m_rfreq[A2_OSC] = (a2_mult.value() / 8) * std::exp2(a2_rtune.value() / 1200);
}


void WatsynInstrument::updateFreqB1()
{
	// calculate frequencies
	m_lfreq[B1_OSC] = (b1_mult.value() / 8) * std::exp2(b1_ltune.value() / 1200);
	m_rfreq[B1_OSC] = (b1_mult.value() / 8) * std::exp2(b1_rtune.value() / 1200);
}


void WatsynInstrument::updateFreqB2()
{
	// calculate frequencies
	m_lfreq[B2_OSC] = (b2_mult.value() / 8) * std::exp2(b2_ltune.value() / 1200);
	m_rfreq[B2_OSC] = (b2_mult.value() / 8) * std::exp2(b2_rtune.value() / 1200);	
}


void WatsynInstrument::updateWaveA1()
{
	// do sinc+oversampling on the wavetables to improve quality
	srccpy( &A1_wave[0], const_cast<float*>( a1_graph.samples() ) );
}


void WatsynInstrument::updateWaveA2()
{
	// do sinc+oversampling on the wavetables to improve quality
	srccpy( &A2_wave[0], const_cast<float*>( a2_graph.samples() ) );
}


void WatsynInstrument::updateWaveB1()
{
	// do sinc+oversampling on the wavetables to improve quality
	srccpy( &B1_wave[0], const_cast<float*>( b1_graph.samples() ) );
}


void WatsynInstrument::updateWaveB2()
{
	// do sinc+oversampling on the wavetables to improve quality
	srccpy( &B2_wave[0], const_cast<float*>( b2_graph.samples() ) );
}


namespace gui
{


WatsynView::WatsynView( Instrument * _instrument,
					QWidget * _parent ) :
	InstrumentViewFixedSize( _instrument, _parent )
{
	setAutoFillBackground( true );
	QPalette pal;

	pal.setBrush( backgroundRole(),	PLUGIN_NAME::getIconPixmap(	"artwork" ) );
	setPalette( pal );

// knobs... lots of em

	makeknob( a1_volKnob, 130, A1ROW, tr( "Volume" ), "%", "aKnob" )
	makeknob( a2_volKnob, 130, A2ROW, tr( "Volume" ), "%", "aKnob" )
	makeknob( b1_volKnob, 130, B1ROW, tr( "Volume" ), "%", "bKnob" )
	makeknob( b2_volKnob, 130, B2ROW, tr( "Volume" ), "%", "bKnob"  )

	makeknob( a1_panKnob, 154, A1ROW, tr( "Panning" ), "", "aKnob" )
	makeknob( a2_panKnob, 154, A2ROW, tr( "Panning" ), "", "aKnob" )
	makeknob( b1_panKnob, 154, B1ROW, tr( "Panning" ), "", "bKnob"  )
	makeknob( b2_panKnob, 154, B2ROW, tr( "Panning" ), "", "bKnob"  )

	makeknob( a1_multKnob, 178, A1ROW, tr( "Freq. multiplier" ), "/8", "aKnob" )
	makeknob( a2_multKnob, 178, A2ROW, tr( "Freq. multiplier" ), "/8", "aKnob" )
	makeknob( b1_multKnob, 178, B1ROW, tr( "Freq. multiplier" ), "/8", "bKnob"  )
	makeknob( b2_multKnob, 178, B2ROW, tr( "Freq. multiplier" ), "/8", "bKnob"  )

	makeknob( a1_ltuneKnob, 202, A1ROW, tr( "Left detune" ), tr( " cents" ), "aKnob" )
	makeknob( a2_ltuneKnob, 202, A2ROW, tr( "Left detune" ), tr( " cents" ), "aKnob" )
	makeknob( b1_ltuneKnob, 202, B1ROW, tr( "Left detune" ), tr( " cents" ), "bKnob"  )
	makeknob( b2_ltuneKnob, 202, B2ROW, tr( "Left detune" ), tr( " cents" ), "bKnob"  )

	makeknob( a1_rtuneKnob, 226, A1ROW, tr( "Right detune" ), tr( " cents" ), "aKnob" )
	makeknob( a2_rtuneKnob, 226, A2ROW, tr( "Right detune" ), tr( " cents" ), "aKnob" )
	makeknob( b1_rtuneKnob, 226, B1ROW, tr( "Right detune" ), tr( " cents" ), "bKnob"  )
	makeknob( b2_rtuneKnob, 226, B2ROW, tr( "Right detune" ), tr( " cents" ), "bKnob"  )

	makeknob( m_abmixKnob, 4, 3, tr( "A-B Mix" ), "", "mixKnob" )

	makeknob( m_envAmtKnob, 88, 3, tr( "Mix envelope amount" ), "", "mixenvKnob" )

	maketsknob( m_envAttKnob, 88, A1ROW, tr( "Mix envelope attack" ), " ms", "mixenvKnob" )
	maketsknob( m_envHoldKnob, 88, A2ROW, tr( "Mix envelope hold" ), " ms", "mixenvKnob" )
	maketsknob( m_envDecKnob, 88, B1ROW, tr( "Mix envelope decay" ), " ms", "mixenvKnob" )

	makeknob( m_xtalkKnob, 88, B2ROW, tr( "Crosstalk" ), "", "xtalkKnob" )

// let's set volume knobs
	a1_volKnob -> setVolumeKnob( true );
	a2_volKnob -> setVolumeKnob( true );
	b1_volKnob -> setVolumeKnob( true );
	b2_volKnob -> setVolumeKnob( true );

	m_abmixKnob -> setFixedSize( 31, 31 );


// button groups next.
// graph select buttons
	auto a1_selectButton = new PixmapButton(this, nullptr);
	a1_selectButton -> move( 4, 121 );
	a1_selectButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "a1_active" ) );
	a1_selectButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "a1_inactive" ) );
	a1_selectButton->setToolTip(tr("Select oscillator A1"));

	auto a2_selectButton = new PixmapButton(this, nullptr);
	a2_selectButton -> move( 44, 121 );
	a2_selectButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "a2_active" ) );
	a2_selectButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "a2_inactive" ) );
	a2_selectButton->setToolTip(tr("Select oscillator A2"));

	auto b1_selectButton = new PixmapButton(this, nullptr);
	b1_selectButton -> move( 84, 121 );
	b1_selectButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "b1_active" ) );
	b1_selectButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "b1_inactive" ) );
	b1_selectButton->setToolTip(tr("Select oscillator B1"));

	auto b2_selectButton = new PixmapButton(this, nullptr);
	b2_selectButton -> move( 124, 121 );
	b2_selectButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "b2_active" ) );
	b2_selectButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "b2_inactive" ) );
	b2_selectButton->setToolTip(tr("Select oscillator B2"));

	m_selectedGraphGroup = new AutomatableButtonGroup( this );
	m_selectedGraphGroup -> addButton( a1_selectButton );
	m_selectedGraphGroup -> addButton( a2_selectButton );
	m_selectedGraphGroup -> addButton( b1_selectButton );
	m_selectedGraphGroup -> addButton( b2_selectButton );
	auto w = castModel<WatsynInstrument>();
	m_selectedGraphGroup -> setModel( &w -> m_selectedGraph);

// A-modulation button group
	auto amod_mixButton = new PixmapButton(this, nullptr);
	amod_mixButton -> move( 4, 50 );
	amod_mixButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "amix_active" ) );
	amod_mixButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "amix_inactive" ) );
	amod_mixButton->setToolTip(tr("Mix output of A2 to A1"));

	auto amod_amButton = new PixmapButton(this, nullptr);
	amod_amButton -> move( 4, 66 );
	amod_amButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "aam_active" ) );
	amod_amButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "aam_inactive" ) );
	amod_amButton->setToolTip(tr("Modulate amplitude of A1 by output of A2"));

	auto amod_rmButton = new PixmapButton(this, nullptr);
	amod_rmButton -> move( 4, 82 );
	amod_rmButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "arm_active" ) );
	amod_rmButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "arm_inactive" ) );
	amod_rmButton->setToolTip(tr("Ring modulate A1 and A2"));

	auto amod_pmButton = new PixmapButton(this, nullptr);
	amod_pmButton -> move( 4, 98 );
	amod_pmButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "apm_active" ) );
	amod_pmButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "apm_inactive" ) );
	amod_pmButton->setToolTip(tr("Modulate phase of A1 by output of A2"));

	m_aModGroup = new AutomatableButtonGroup( this );
	m_aModGroup -> addButton( amod_mixButton );
	m_aModGroup -> addButton( amod_amButton );
	m_aModGroup -> addButton( amod_rmButton );
	m_aModGroup -> addButton( amod_pmButton );

// B-modulation button group
	auto bmod_mixButton = new PixmapButton(this, nullptr);
	bmod_mixButton -> move( 44, 50 );
	bmod_mixButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "bmix_active" ) );
	bmod_mixButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "bmix_inactive" ) );
	bmod_mixButton->setToolTip(tr("Mix output of B2 to B1"));

	auto bmod_amButton = new PixmapButton(this, nullptr);
	bmod_amButton -> move( 44, 66 );
	bmod_amButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "bam_active" ) );
	bmod_amButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "bam_inactive" ) );
	bmod_amButton->setToolTip(tr("Modulate amplitude of B1 by output of B2"));

	auto bmod_rmButton = new PixmapButton(this, nullptr);
	bmod_rmButton -> move( 44, 82 );
	bmod_rmButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "brm_active" ) );
	bmod_rmButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "brm_inactive" ) );
	bmod_rmButton->setToolTip(tr("Ring modulate B1 and B2"));

	auto bmod_pmButton = new PixmapButton(this, nullptr);
	bmod_pmButton -> move( 44, 98 );
	bmod_pmButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "bpm_active" ) );
	bmod_pmButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "bpm_inactive" ) );
	bmod_pmButton->setToolTip(tr("Modulate phase of B1 by output of B2"));

	m_bModGroup = new AutomatableButtonGroup( this );
	m_bModGroup -> addButton( bmod_mixButton );
	m_bModGroup -> addButton( bmod_amButton );
	m_bModGroup -> addButton( bmod_rmButton );
	m_bModGroup -> addButton( bmod_pmButton );


// graph widgets
	pal = QPalette();
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap("wavegraph") );
// a1 graph
	a1_graph = new Graph( this, Graph::Style::Linear, 224, 105 );
	a1_graph->move( 4, 141 );
	a1_graph->setAutoFillBackground( true );
	a1_graph->setGraphColor( QColor( 0x43, 0xb2, 0xff ) );
	a1_graph->setToolTip(tr("Draw your own waveform here by dragging your mouse on this graph."));
	a1_graph->setPalette( pal );

// a2 graph
	a2_graph = new Graph( this, Graph::Style::Linear, 224, 105 );
	a2_graph->move( 4, 141 );
	a2_graph->setAutoFillBackground( true );
	a2_graph->setGraphColor( QColor( 0x43, 0xb2, 0xff ) );
	a2_graph->setToolTip(tr("Draw your own waveform here by dragging your mouse on this graph."));
	a2_graph->setPalette( pal );

// b1 graph
	b1_graph = new Graph( this, Graph::Style::Linear, 224, 105 );
	b1_graph->move( 4, 141 );
	b1_graph->setAutoFillBackground( true );
	b1_graph->setGraphColor( QColor( 0xfc, 0x54, 0x31 ) );
	b1_graph->setToolTip(tr("Draw your own waveform here by dragging your mouse on this graph."));
	b1_graph->setPalette( pal );

// b2 graph
	b2_graph = new Graph( this, Graph::Style::Linear, 224, 105 );
	b2_graph->move( 4, 141 );
	b2_graph->setAutoFillBackground( true );
	b2_graph->setGraphColor( QColor( 0xfc, 0x54, 0x31 ) );
	b2_graph->setToolTip(tr("Draw your own waveform here by dragging your mouse on this graph."));
	b2_graph->setPalette( pal );


// misc pushbuttons
// waveform modifications

	m_loadButton = new PixmapButton( this, tr( "Load waveform" ) );
	m_loadButton -> move ( 173, 121 );
	m_loadButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "load_active" ) );
	m_loadButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "load_inactive" ) );
	m_loadButton->setToolTip(tr("Load a waveform from a sample file"));

	m_phaseLeftButton = new PixmapButton( this, tr( "Phase left" ) );
	m_phaseLeftButton -> move ( 193, 121 );
	m_phaseLeftButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "phl_active" ) );
	m_phaseLeftButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "phl_inactive" ) );
	m_phaseLeftButton->setToolTip(tr("Shift phase by -15 degrees"));

	m_phaseRightButton = new PixmapButton( this, tr( "Phase right" ) );
	m_phaseRightButton -> move ( 210, 121 );
	m_phaseRightButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "phr_active" ) );
	m_phaseRightButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "phr_inactive" ) );
	m_phaseRightButton->setToolTip(tr("Shift phase by +15 degrees"));

	m_normalizeButton = new PixmapButton( this, tr( "Normalize" ) );
	m_normalizeButton -> move ( 230, 121 );
	m_normalizeButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "norm_active" ) );
	m_normalizeButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "norm_inactive" ) );
	m_normalizeButton->setToolTip(tr("Normalize"));


	m_invertButton = new PixmapButton( this, tr( "Invert" ) );
	m_invertButton -> move ( 230, 138 );
	m_invertButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "inv_active" ) );
	m_invertButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "inv_inactive" ) );
	m_invertButton->setToolTip(tr("Invert"));

	m_smoothButton = new PixmapButton( this, tr( "Smooth" ) );
	m_smoothButton -> move ( 230, 155 );
	m_smoothButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "smooth_active" ) );
	m_smoothButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "smooth_inactive" ) );
	m_smoothButton->setToolTip(tr("Smooth"));

// waveforms

	m_sinWaveButton = new PixmapButton( this, tr( "Sine wave" ) );
	m_sinWaveButton -> move ( 230, 176 );
	m_sinWaveButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "sin_active" ) );
	m_sinWaveButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "sin_inactive" ) );
	m_sinWaveButton->setToolTip(tr("Sine wave"));

	m_triWaveButton = new PixmapButton( this, tr( "Triangle wave" ) );
	m_triWaveButton -> move ( 230, 194 );
	m_triWaveButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "tri_active" ) );
	m_triWaveButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "tri_inactive" ) );
	m_triWaveButton->setToolTip(tr("Triangle wave"));

	m_sawWaveButton = new PixmapButton( this, tr( "Triangle wave" ) );
	m_sawWaveButton -> move ( 230, 212 );
	m_sawWaveButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "saw_active" ) );
	m_sawWaveButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "saw_inactive" ) );
	m_sawWaveButton->setToolTip(tr("Saw wave"));

	m_sqrWaveButton = new PixmapButton( this, tr( "Square wave" ) );
	m_sqrWaveButton -> move ( 230, 230 );
	m_sqrWaveButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "sqr_active" ) );
	m_sqrWaveButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "sqr_inactive" ) );
	m_sqrWaveButton->setToolTip(tr("Square wave"));



	connect( m_sinWaveButton, SIGNAL( clicked() ), this, SLOT( sinWaveClicked() ) );
	connect( m_triWaveButton, SIGNAL( clicked() ), this, SLOT( triWaveClicked() ) );
	connect( m_sawWaveButton, SIGNAL( clicked() ), this, SLOT( sawWaveClicked() ) );
	connect( m_sqrWaveButton, SIGNAL( clicked() ), this, SLOT( sqrWaveClicked() ) );
	connect( m_normalizeButton, SIGNAL( clicked() ), this, SLOT( normalizeClicked() ) );
	connect( m_invertButton, SIGNAL( clicked() ), this, SLOT( invertClicked() ) );
	connect( m_smoothButton, SIGNAL( clicked() ), this, SLOT( smoothClicked() ) );
	connect( m_phaseLeftButton, SIGNAL( clicked() ), this, SLOT( phaseLeftClicked() ) );
	connect( m_phaseRightButton, SIGNAL( clicked() ), this, SLOT( phaseRightClicked() ) );
	connect( m_loadButton, SIGNAL( clicked() ), this, SLOT( loadClicked() ) );

	connect( a1_selectButton, SIGNAL( clicked() ), this, SLOT( updateLayout() ) );
	connect( a2_selectButton, SIGNAL( clicked() ), this, SLOT( updateLayout() ) );
	connect( b1_selectButton, SIGNAL( clicked() ), this, SLOT( updateLayout() ) );
	connect( b2_selectButton, SIGNAL( clicked() ), this, SLOT( updateLayout() ) );

	updateLayout();
}


void WatsynView::updateLayout()
{
	switch( m_selectedGraphGroup->model()->value() )
	{
		case A1_OSC:
			a1_graph->show();
			a2_graph->hide();
			b1_graph->hide();
			b2_graph->hide();
			break;
		case A2_OSC:
			a1_graph->hide();
			a2_graph->show();
			b1_graph->hide();
			b2_graph->hide();
			break;
		case B1_OSC:
			a1_graph->hide();
			a2_graph->hide();
			b1_graph->show();
			b2_graph->hide();
			break;
		case B2_OSC:
			a1_graph->hide();
			a2_graph->hide();
			b1_graph->hide();
			b2_graph->show();
			break;
	}
}



void WatsynView::sinWaveClicked()
{
	switch( m_selectedGraphGroup->model()->value() )
	{
		case A1_OSC:
			a1_graph->model()->setWaveToSine();
			Engine::getSong()->setModified();
			break;
		case A2_OSC:
			a2_graph->model()->setWaveToSine();
			Engine::getSong()->setModified();
			break;
		case B1_OSC:
			b1_graph->model()->setWaveToSine();
			Engine::getSong()->setModified();
			break;
		case B2_OSC:
			b2_graph->model()->setWaveToSine();
			Engine::getSong()->setModified();
			break;
	}
}


void WatsynView::triWaveClicked()
{
	switch( m_selectedGraphGroup->model()->value() )
	{
		case A1_OSC:
			a1_graph->model()->setWaveToTriangle();
			Engine::getSong()->setModified();
			break;
		case A2_OSC:
			a2_graph->model()->setWaveToTriangle();
			Engine::getSong()->setModified();
			break;
		case B1_OSC:
			b1_graph->model()->setWaveToTriangle();
			Engine::getSong()->setModified();
			break;
		case B2_OSC:
			b2_graph->model()->setWaveToTriangle();
			Engine::getSong()->setModified();
			break;
	}
}


void WatsynView::sawWaveClicked()
{
	switch( m_selectedGraphGroup->model()->value() )
	{
		case A1_OSC:
			a1_graph->model()->setWaveToSaw();
			Engine::getSong()->setModified();
			break;
		case A2_OSC:
			a2_graph->model()->setWaveToSaw();
			Engine::getSong()->setModified();
			break;
		case B1_OSC:
			b1_graph->model()->setWaveToSaw();
			Engine::getSong()->setModified();
			break;
		case B2_OSC:
			b2_graph->model()->setWaveToSaw();
			Engine::getSong()->setModified();
			break;
	}
}


void WatsynView::sqrWaveClicked()
{
	switch( m_selectedGraphGroup->model()->value() )
	{
		case A1_OSC:
			a1_graph->model()->setWaveToSquare();
			Engine::getSong()->setModified();
			break;
		case A2_OSC:
			a2_graph->model()->setWaveToSquare();
			Engine::getSong()->setModified();
			break;
		case B1_OSC:
			b1_graph->model()->setWaveToSquare();
			Engine::getSong()->setModified();
			break;
		case B2_OSC:
			b2_graph->model()->setWaveToSquare();
			Engine::getSong()->setModified();
			break;
	}
}


void WatsynView::normalizeClicked()
{
	switch( m_selectedGraphGroup->model()->value() )
	{
		case A1_OSC:
			a1_graph->model()->normalize();
			Engine::getSong()->setModified();
			break;
		case A2_OSC:
			a2_graph->model()->normalize();
			Engine::getSong()->setModified();
			break;
		case B1_OSC:
			b1_graph->model()->normalize();
			Engine::getSong()->setModified();
			break;
		case B2_OSC:
			b2_graph->model()->normalize();
			Engine::getSong()->setModified();
			break;
	}
}


void WatsynView::invertClicked()
{
	switch( m_selectedGraphGroup->model()->value() )
	{
		case A1_OSC:
			a1_graph->model()->invert();
			Engine::getSong()->setModified();
			break;
		case A2_OSC:
			a2_graph->model()->invert();
			Engine::getSong()->setModified();
			break;
		case B1_OSC:
			b1_graph->model()->invert();
			Engine::getSong()->setModified();
			break;
		case B2_OSC:
			b2_graph->model()->invert();
			Engine::getSong()->setModified();
			break;
	}
}


void WatsynView::smoothClicked()
{
	switch( m_selectedGraphGroup->model()->value() )
	{
		case A1_OSC:
			a1_graph->model()->smooth();
			Engine::getSong()->setModified();
			break;
		case A2_OSC:
			a2_graph->model()->smooth();
			Engine::getSong()->setModified();
			break;
		case B1_OSC:
			b1_graph->model()->smooth();
			Engine::getSong()->setModified();
			break;
		case B2_OSC:
			b2_graph->model()->smooth();
			Engine::getSong()->setModified();
			break;
	}
}


void WatsynView::phaseLeftClicked()
{
	switch( m_selectedGraphGroup->model()->value() )
	{
		case A1_OSC:
			a1_graph->model()->shiftPhase( -15 );
			Engine::getSong()->setModified();
			break;
		case A2_OSC:
			a2_graph->model()->shiftPhase( -15 );
			Engine::getSong()->setModified();
			break;
		case B1_OSC:
			b1_graph->model()->shiftPhase( -15 );
			Engine::getSong()->setModified();
			break;
		case B2_OSC:
			b2_graph->model()->shiftPhase( -15 );
			Engine::getSong()->setModified();
			break;
	}
}


void WatsynView::phaseRightClicked()
{
	switch( m_selectedGraphGroup->model()->value() )
	{
		case A1_OSC:
			a1_graph->model()->shiftPhase( 15 );
			Engine::getSong()->setModified();
			break;
		case A2_OSC:
			a2_graph->model()->shiftPhase( 15 );
			Engine::getSong()->setModified();
			break;
		case B1_OSC:
			b1_graph->model()->shiftPhase( 15 );
			Engine::getSong()->setModified();
			break;
		case B2_OSC:
			b2_graph->model()->shiftPhase( 15 );
			Engine::getSong()->setModified();
			break;
	}
}


void WatsynView::loadClicked()
{
	QString fileName;
	switch( m_selectedGraphGroup->model()->value() )
	{
		case A1_OSC:
			a1_graph->model()->setWaveToUser();
			Engine::getSong()->setModified();
			break;
		case A2_OSC:
			a2_graph->model()->setWaveToUser();
			Engine::getSong()->setModified();
			break;
		case B1_OSC:
			b1_graph->model()->setWaveToUser();
			Engine::getSong()->setModified();
			break;
		case B2_OSC:
			b2_graph->model()->setWaveToUser();
			Engine::getSong()->setModified();
			break;
	}
}


void WatsynView::modelChanged()
{
	auto w = castModel<WatsynInstrument>();

	a1_volKnob -> setModel( &w -> a1_vol );
	a2_volKnob -> setModel( &w -> a2_vol );
	b1_volKnob -> setModel( &w -> b1_vol );
	b2_volKnob -> setModel( &w -> b2_vol );

	a1_panKnob -> setModel( &w -> a1_pan );
	a2_panKnob -> setModel( &w -> a2_pan );
	b1_panKnob -> setModel( &w -> b1_pan );
	b2_panKnob -> setModel( &w -> b2_pan );

	a1_multKnob -> setModel( &w -> a1_mult );
	a2_multKnob -> setModel( &w -> a2_mult );
	b1_multKnob -> setModel( &w -> b1_mult );
	b2_multKnob -> setModel( &w -> b2_mult );

	a1_ltuneKnob -> setModel( &w -> a1_ltune );
	a2_ltuneKnob -> setModel( &w -> a2_ltune );
	b1_ltuneKnob -> setModel( &w -> b1_ltune );
	b2_ltuneKnob -> setModel( &w -> b2_ltune );

	a1_rtuneKnob -> setModel( &w -> a1_rtune );
	a2_rtuneKnob -> setModel( &w -> a2_rtune );
	b1_rtuneKnob -> setModel( &w -> b1_rtune );
	b2_rtuneKnob -> setModel( &w -> b2_rtune );

	m_abmixKnob -> setModel( &w -> m_abmix );

	m_selectedGraphGroup -> setModel( &w -> m_selectedGraph );

	m_aModGroup -> setModel( &w -> m_amod );
	m_bModGroup -> setModel( &w -> m_bmod );

	a1_graph -> setModel( &w -> a1_graph );
	a2_graph -> setModel( &w -> a2_graph );
	b1_graph -> setModel( &w -> b1_graph );
	b2_graph -> setModel( &w -> b2_graph );

	m_envAmtKnob -> setModel( &w -> m_envAmt );
	m_envAttKnob -> setModel( &w -> m_envAtt );
	m_envHoldKnob -> setModel( &w -> m_envHold );
	m_envDecKnob -> setModel( &w -> m_envDec );

	m_xtalkKnob -> setModel( &w -> m_xtalk );
}


} // namespace gui



extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model *m, void * )
{
	return( new WatsynInstrument( static_cast<InstrumentTrack *>( m ) ) );
}


}


} // namespace lmms
