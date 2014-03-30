/*
 * Monstro.cpp - a monstrous semi-modular 3-osc synth with modulation matrix
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
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


#include <QtXml/QDomElement>

#include "Monstro.h"
#include "engine.h"
#include "InstrumentTrack.h"
#include "templates.h"
#include "gui_templates.h"
#include "tooltip.h"
#include "song.h"
#include "lmms_math.h"

#include "embed.cpp"


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT monstro_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"Monstro",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"Monstrous 3-oscillator synth with modulation matrix" ),
	"Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>",
	0x0100,
	Plugin::Instrument,
	new PluginPixmapLoader( "logo" ),
	NULL,
	NULL
} ;

}




MonstroSynth::MonstroSynth( MonstroInstrument * _i, NotePlayHandle * _nph,
					const sample_rate_t _samplerate, fpp_t _frames ) :
					m_parent( _i ),
					m_nph( _nph ),
					m_samplerate( _samplerate ),
					m_fpp( _frames )
{
	m_env1_buf = new sample_t[_frames];
	m_env2_buf = new sample_t[_frames];
	m_lfo1_buf = new sample_t[_frames];
	m_lfo2_buf = new sample_t[_frames];

	m_osc1l_phase = 0.0;
	m_osc1r_phase = 0.0;
	m_osc2l_phase = 0.0;
	m_osc2r_phase = 0.0;
	m_osc3l_phase = 0.0;
	m_osc3r_phase = 0.0;

	m_env1_phase = 0.0;
	m_env2_phase = 0.0;
}


MonstroSynth::~MonstroSynth()
{
	delete[] m_env1_buf;
	delete[] m_env2_buf;
	delete[] m_lfo1_buf;
	delete[] m_lfo2_buf;

}


void MonstroSynth::renderOutput( fpp_t _frames, sampleFrame * _buf  )
{
// macros for modulating with env/lfos
#define modulatefreq( car, mod ) \
		if( mod##_e1 != 0.0 ) car = qBound( MIN_FREQ, car * static_cast<float>( fastPow( 10.0, m_env1_buf[f] * mod##_e1 ) ), MAX_FREQ );	\
		if( mod##_e2 != 0.0 ) car = qBound( MIN_FREQ, car * static_cast<float>( fastPow( 10.0, m_env2_buf[f] * mod##_e2 ) ), MAX_FREQ );	\
		if( mod##_l1 != 0.0 ) car = qBound( MIN_FREQ, car * static_cast<float>( fastPow( 2.0, m_lfo1_buf[f] * mod##_l1 ) ), MAX_FREQ );	\
		if( mod##_l2 != 0.0 ) car = qBound( MIN_FREQ, car * static_cast<float>( fastPow( 2.0, m_lfo2_buf[f] * mod##_l2 ) ), MAX_FREQ );	

#define modulateabs( car, mod ) \
		if( mod##_e1 != 0.0 ) car = qBound( 0.0f, car + mod##_e1 * m_env1_buf[f], 1.0f );	\
		if( mod##_e2 != 0.0 ) car = qBound( 0.0f, car + mod##_e2 * m_env2_buf[f], 1.0f );	\
		if( mod##_l1 != 0.0 ) car = qBound( 0.0f, car + mod##_l1 / 2 * m_lfo1_buf[f], 1.0f );	\
		if( mod##_l2 != 0.0 ) car = qBound( 0.0f, car + mod##_l2 / 2 * m_lfo2_buf[f], 1.0f );
		
#define modulatephs( car, mod ) \
		if( mod##_e1 != 0.0 ) car = fraction( car + mod##_e1 * m_env1_buf[f] );	\
		if( mod##_e2 != 0.0 ) car = fraction( car + mod##_e2 * m_env2_buf[f] );	\
		if( mod##_l1 != 0.0 ) car = fraction( car + mod##_l1 / 2 * m_lfo1_buf[f] );	\
		if( mod##_l2 != 0.0 ) car = fraction( car + mod##_l2 / 2 * m_lfo2_buf[f] );
		
#define modulatevol( car, mod ) \
		if( mod##_e1 > 0.0 ) car = qBound( 0.0f, car * ( 1.0f - mod##_e1 + mod##_e1 * m_env1_buf[f] ), MODCLIP );	\
		if( mod##_e1 < 0.0 ) car = qBound( 0.0f, car * ( 1.0f + mod##_e1 * m_env1_buf[f] ), MODCLIP );	\
		if( mod##_e2 > 0.0 ) car = qBound( 0.0f, car * ( 1.0f - mod##_e2 + mod##_e2 * m_env2_buf[f] ), MODCLIP );	\
		if( mod##_e2 < 0.0 ) car = qBound( 0.0f, car * ( 1.0f + mod##_e2 * m_env2_buf[f] ), MODCLIP );	\
		if( mod##_l1 != 0.0 ) car = qBound( 0.0f, car * ( 1.0f + mod##_l1 * m_lfo1_buf[f] ), MODCLIP );	\
		if( mod##_l2 != 0.0 ) car = qBound( 0.0f, car * ( 1.0f + mod##_l2 * m_lfo2_buf[f] ), MODCLIP );
		
	// pre-render env's and lfo's
	renderModulators( _frames );

	// get updated osc1 values
	// get pulse width
	const float pw = ( m_parent->m_osc1Pw.value() / 100.0 );
	const float o1pw_e1 = ( m_parent->m_pw1env1.value() );
	const float o1pw_e2 = ( m_parent->m_pw1env2.value() );
	const float o1pw_l1 = ( m_parent->m_pw1lfo1.value() );
	const float o1pw_l2 = ( m_parent->m_pw1lfo2.value() );
	
	// get phases
	const float o1lpo = m_parent->m_osc1l_po;
	const float o1rpo = m_parent->m_osc1r_po;
	const float o1p_e1 = ( m_parent->m_phs1env1.value() );
	const float o1p_e2 = ( m_parent->m_phs1env2.value() );
	const float o1p_l1 = ( m_parent->m_phs1lfo1.value() );
	const float o1p_l2 = ( m_parent->m_phs1lfo2.value() );
	
	// get pitch 
	const float o1lfb = ( m_parent->m_osc1l_freq * m_nph->frequency() );
	const float o1rfb = ( m_parent->m_osc1r_freq * m_nph->frequency() );
	const float o1f_e1 = ( m_parent->m_pit1env1.value() );
	const float o1f_e2 = ( m_parent->m_pit1env2.value() );
	const float o1f_l1 = ( m_parent->m_pit1lfo1.value() );
	const float o1f_l2 = ( m_parent->m_pit1lfo2.value() );

	// get volumes
	const float o1lv = m_parent->m_osc1l_vol;
	const float o1rv = m_parent->m_osc1r_vol;
	const float o1v_e1 = ( m_parent->m_vol1env1.value() );
	const float o1v_e2 = ( m_parent->m_vol1env2.value() );
	const float o1v_l1 = ( m_parent->m_vol1lfo1.value() );
	const float o1v_l2 = ( m_parent->m_vol1lfo2.value() );

	
	// update osc2
	// get waveform
	const int o2w = m_parent->m_osc2Wave.value();
	
	// get phases
	const float o2lpo = m_parent->m_osc2l_po;
	const float o2rpo = m_parent->m_osc2r_po;
	const float o2p_e1 = ( m_parent->m_phs2env1.value() );
	const float o2p_e2 = ( m_parent->m_phs2env2.value() );
	const float o2p_l1 = ( m_parent->m_phs2lfo1.value() );
	const float o2p_l2 = ( m_parent->m_phs2lfo2.value() );

	// get pitch 
	const float o2lfb = ( m_parent->m_osc2l_freq * m_nph->frequency() );
	const float o2rfb = ( m_parent->m_osc2r_freq * m_nph->frequency() );	
	const float o2f_e1 = ( m_parent->m_pit2env1.value() );
	const float o2f_e2 = ( m_parent->m_pit2env2.value() );
	const float o2f_l1 = ( m_parent->m_pit2lfo1.value() );
	const float o2f_l2 = ( m_parent->m_pit2lfo2.value() );

	// get volumes
	const float o2lv = m_parent->m_osc2l_vol;
	const float o2rv = m_parent->m_osc2r_vol;
	const float o2v_e1 = ( m_parent->m_vol2env1.value() );
	const float o2v_e2 = ( m_parent->m_vol2env2.value() );
	const float o2v_l1 = ( m_parent->m_vol2lfo2.value() );
	const float o2v_l2 = ( m_parent->m_vol2lfo2.value() );
	
	

	// update osc3
	// get waveforms
	const int o3w1 = m_parent->m_osc3Wave1.value();
	const int o3w2 = m_parent->m_osc3Wave2.value();

	// get phases
	const float o3lpo = m_parent->m_osc3l_po;
	const float o3rpo = m_parent->m_osc3r_po;
	const float o3p_e1 = ( m_parent->m_phs3env1.value() );
	const float o3p_e2 = ( m_parent->m_phs3env2.value() );
	const float o3p_l1 = ( m_parent->m_phs3lfo1.value() );
	const float o3p_l2 = ( m_parent->m_phs3lfo2.value() );

	// get pitch modulators
	const float o3fb = ( m_parent->m_osc3_freq * m_nph->frequency() );
	const float o3f_e1 = ( m_parent->m_pit3env1.value() );
	const float o3f_e2 = ( m_parent->m_pit3env2.value() );
	const float o3f_l1 = ( m_parent->m_pit3lfo1.value() );
	const float o3f_l2 = ( m_parent->m_pit3lfo2.value() );

	// get volumes
	const float o3lv = m_parent->m_osc3l_vol;
	const float o3rv = m_parent->m_osc3r_vol;
	const float o3v_e1 = ( m_parent->m_vol3env1.value() );
	const float o3v_e2 = ( m_parent->m_vol3env2.value() );
	const float o3v_l1 = ( m_parent->m_vol3lfo1.value() );
	const float o3v_l2 = ( m_parent->m_vol3lfo2.value() );

	// get sub
	const float o3sub = ( m_parent->m_osc3Sub.value() + 100.0 ) / 200.0;
	const float o3s_e1 = ( m_parent->m_sub3env1.value() );
	const float o3s_e2 = ( m_parent->m_sub3env2.value() );
	const float o3s_l1 = ( m_parent->m_sub3lfo1.value() );
	const float o3s_l2 = ( m_parent->m_sub3lfo2.value() );
	
	
	//o2-o3 modulation
	
	const int omod = m_parent->m_o23Mod.value();
	
	
	///////////////////////////
	//                       //
	// 	 start buffer loop   //
	//                       //
	///////////////////////////
	
	
	
	for( f_cnt_t f = 0; f < _frames; f++ )
	{	

/*
	// debug code
		if( f % 10 == 0 ) { 
			qDebug( "env1 %f -- env1 phase %f", m_env1_buf[f], m_env1_phase );
			qDebug( "env1 pre %f att %f dec %f rel %f ", m_parent->m_env1_pre, m_parent->m_env1_att,
				m_parent->m_env1_dec, m_parent->m_env1_rel );
		}
*/

		/////////////////////////////
		//				           //
		//          OSC 1          //
		//				           //
		/////////////////////////////

		// calc and mod frequencies
		float o1l_f = o1lfb;
		float o1r_f = o1rfb;
		modulatefreq( o1l_f, o1f )
		modulatefreq( o1r_f, o1f )
		
		// calc and modulate phase
		float o1l_p = m_osc1l_phase + o1lpo;
		float o1r_p = m_osc1r_phase + o1rpo;
		modulatephs( o1l_p, o1p )
		modulatephs( o1r_p, o1p )
		
		// calc and modulate pulse
		float o1_pw = pw;
		modulateabs( o1_pw, o1pw )
		
		// bounds check for phase
		if( o1l_p < 0 ) o1l_p += 1.0f;
		if( o1r_p < 0 ) o1r_p += 1.0f;
		
		// pulse wave osc
		sample_t O1L = ( o1l_p < o1_pw ) ? 1.0f : -1.0f;
		sample_t O1R = ( o1r_p < o1_pw ) ? 1.0f : -1.0f;
		
		// modulate volume
		O1L *= o1lv;
		O1R *= o1rv;
		modulatevol( O1L, o1v )
		modulatevol( O1R, o1v )
		
		// update osc1 phases	
		m_osc1l_phase = fraction( m_osc1l_phase + 1.0f / ( static_cast<float>( m_samplerate ) / o1l_f ) );
		m_osc1r_phase = fraction( m_osc1r_phase + 1.0f / ( static_cast<float>( m_samplerate ) / o1r_f ) );
		
		/////////////////////////////
		//				           //
		//          OSC 2          //
		//				           //
		/////////////////////////////
		
		// calc and mod frequencies
		float o2l_f = o2lfb;
		float o2r_f = o2rfb;
		modulatefreq( o2l_f, o2f )
		modulatefreq( o2r_f, o2f )
		
		// calc and modulate phase
		float o2l_p = m_osc2l_phase + o2lpo;
		float o2r_p = m_osc2r_phase + o2rpo;
		modulatephs( o2l_p, o2p )
		modulatephs( o2r_p, o2p )

		// bounds check for phase
		if( o2l_p < 0 ) o2l_p += 1.0f;
		if( o2r_p < 0 ) o2r_p += 1.0f;
		
		// multi-wave DC Oscillator
		sample_t O2L = oscillate( o2w, o2l_p );
		sample_t O2R = oscillate( o2w, o2r_p );
		
		// modulate volume
		O2L *= o2lv;
		O2R *= o2rv;
		modulatevol( O2L, o2v )
		modulatevol( O2R, o2v )
		
		// update osc2 phases	
		m_osc2l_phase = fraction( m_osc2l_phase + 1.0f / ( static_cast<float>( m_samplerate ) / o2l_f ) );
		m_osc2r_phase = fraction( m_osc2r_phase + 1.0f / ( static_cast<float>( m_samplerate ) / o2r_f ) );
		
		/////////////////////////////
		//				           //
		//          OSC 3          //
		//				           //
		/////////////////////////////

		// calc and mod frequencies
		float o3l_f = o3fb;
		float o3r_f = o3fb;
		modulatefreq( o3l_f, o3f )
		modulatefreq( o3r_f, o3f )
		// o2 modulation?
		if( omod == MOD_FM )
		{
			o3l_f = qBound( MIN_FREQ, o3l_f * powf( 4.0f, O2L ), MAX_FREQ );
			o3r_f = qBound( MIN_FREQ, o3r_f * powf( 4.0f, O2R ), MAX_FREQ );
		}
		
		// calc and modulate phase
		float o3l_p = m_osc3l_phase + o3lpo;
		float o3r_p = m_osc3r_phase + o3rpo;
		modulatephs( o3l_p, o3p )
		modulatephs( o3r_p, o3p )
		// o2 modulation?
		if( omod == MOD_PM )
		{
			o3l_p = fraction( o3l_p + O2L/2 );
			o3r_p = fraction( o3r_p + O2R/2 );
		}
		
		// bounds check for phase
		if( o3l_p < 0 ) o3l_p += 1.0f;
		if( o3r_p < 0 ) o3r_p += 1.0f;

		// multi-wave DC Oscillator, sub-osc 1
		sample_t O3AL = oscillate( o3w1, o3l_p );
		sample_t O3AR = oscillate( o3w1, o3r_p );

		// multi-wave DC Oscillator, sub-osc 2
		sample_t O3BL = oscillate( o3w2, o3l_p );
		sample_t O3BR = oscillate( o3w2, o3r_p );
		
		// calc and modulate sub
		float sub = o3sub;
		modulateabs( sub, o3s )
		
		sample_t O3L = interpolate( O3AL, O3BL, sub );
		sample_t O3R = interpolate( O3AR, O3BR, sub );
		
		// modulate volume
		O3L *= o3lv;
		O3R *= o3rv;
		modulatevol( O3L, o3v )
		modulatevol( O3R, o3v )
		// o2 modulation?
		if( omod == MOD_AM )
		{
			O3L = qBound( -MODCLIP, O3L * qMax( 0.0f, 1.0f + O2L ), MODCLIP );
			O3R = qBound( -MODCLIP, O3R * qMax( 0.0f, 1.0f + O2R ), MODCLIP );
		}

		// update osc3 phases	
		m_osc3l_phase = fraction( m_osc3l_phase + 1.0f / ( static_cast<float>( m_samplerate ) / o3l_f ) );
		m_osc3r_phase = fraction( m_osc3r_phase + 1.0f / ( static_cast<float>( m_samplerate ) / o3r_f ) );
		
		_buf[f][0] = O1L + O3L + ( omod == MOD_MIX ? O2L : 0.0f );
		_buf[f][1] = O1R + O3R + ( omod == MOD_MIX ? O2R : 0.0f );
	}
}


void MonstroSynth::renderModulators( fpp_t _frames )
{
	// LFO phase offsets
	const float lfo1_p = m_parent->m_lfo1Phs.value() / 360.0f;
	const float lfo2_p = m_parent->m_lfo2Phs.value() / 360.0f;

	// LFO rates
	const float lfo1_r = m_parent->m_lfo1Rate.value() / 1000.0f * m_samplerate;
	const float lfo2_r = m_parent->m_lfo2Rate.value() / 1000.0f * m_samplerate;

	// LFOs

	sample_t lfo1_s;
	sample_t lfo2_s;

	// LFO 1

	switch( m_parent->m_lfo1Wave.value() )
	{
		case WAVE_SINE:
				for( f_cnt_t f = 0; f < _frames; f++ )
				{
					const f_cnt_t t = f + m_nph->totalFramesPlayed();
					lfo1_s = Oscillator::sinSample( lfo1_p + static_cast<float>( t ) / lfo1_r );
					if( t < m_parent->m_lfo1_att ) lfo1_s *= ( static_cast<sample_t>( t ) / m_parent-> m_lfo1_att );
					m_lfo1_buf[f] = lfo1_s;
				}
				break;
		case WAVE_TRI:
				for( f_cnt_t f = 0; f < _frames; f++ )
				{
					const f_cnt_t t = f + m_nph->totalFramesPlayed();
					lfo1_s = Oscillator::triangleSample( lfo1_p + static_cast<float>( t ) / lfo1_r );
					if( t < m_parent->m_lfo1_att ) lfo1_s *= ( static_cast<sample_t>( t ) / m_parent->m_lfo1_att );
					m_lfo1_buf[f] = lfo1_s;
				}
				break;
		case WAVE_SAW:
				for( f_cnt_t f = 0; f < _frames; f++ )
				{
					const f_cnt_t t = f + m_nph->totalFramesPlayed();
					lfo1_s = Oscillator::sawSample( lfo1_p + static_cast<float>( t ) / lfo1_r );
					if( t < m_parent->m_lfo1_att ) lfo1_s *= ( static_cast<sample_t>( t ) / m_parent->m_lfo1_att );
					m_lfo1_buf[f] = lfo1_s;
				}
				break;
		case WAVE_RAMP:
				for( f_cnt_t f = 0; f < _frames; f++ )
				{
					const f_cnt_t t = f + m_nph->totalFramesPlayed();
					lfo1_s = Oscillator::sawSample( lfo1_p + static_cast<float>( t ) / lfo1_r ) * -1.0f;
					if( t < m_parent->m_lfo1_att ) lfo1_s *= ( static_cast<sample_t>( t ) / m_parent->m_lfo1_att );
					m_lfo1_buf[f] = lfo1_s;
				}
				break;
		case WAVE_SQR:
				for( f_cnt_t f = 0; f < _frames; f++ )
				{
					const f_cnt_t t = f + m_nph->totalFramesPlayed();
					lfo1_s = Oscillator::squareSample( lfo1_p + static_cast<float>( t ) / lfo1_r );
					if( t < m_parent->m_lfo1_att ) lfo1_s *= ( static_cast<sample_t>( t ) / m_parent->m_lfo1_att );
					m_lfo1_buf[f] = lfo1_s;
				}
				break;
		case WAVE_SQRSOFT:
				for( f_cnt_t f = 0; f < _frames; f++ )
				{
					const f_cnt_t t = f + m_nph->totalFramesPlayed();
					lfo1_s = oscillate( WAVE_SQRSOFT, lfo1_p + static_cast<float>( t ) / lfo1_r );
					if( t < m_parent->m_lfo1_att ) lfo1_s *= ( static_cast<sample_t>( t ) / m_parent->m_lfo1_att );
					m_lfo1_buf[f] = lfo1_s;
				}
				break;
		case WAVE_MOOG:
				for( f_cnt_t f = 0; f < _frames; f++ )
				{
					const f_cnt_t t = f + m_nph->totalFramesPlayed();
					lfo1_s = Oscillator::moogSawSample( lfo1_p + static_cast<float>( t ) / lfo1_r );
					if( t < m_parent->m_lfo1_att ) lfo1_s *= ( static_cast<sample_t>( t ) / m_parent->m_lfo1_att );
					m_lfo1_buf[f] = lfo1_s;
				}
				break;
		case WAVE_SINABS:
				for( f_cnt_t f = 0; f < _frames; f++ )
				{
					const f_cnt_t t = f + m_nph->totalFramesPlayed();
					lfo1_s = oscillate( WAVE_SINABS, lfo1_p + static_cast<float>( t ) / lfo1_r );
					if( t < m_parent->m_lfo1_att ) lfo1_s *= ( static_cast<sample_t>( t ) / m_parent->m_lfo1_att );
					m_lfo1_buf[f] = lfo1_s;
				}
				break;
		case WAVE_EXP:
				for( f_cnt_t f = 0; f < _frames; f++ )
				{
					const f_cnt_t t = f + m_nph->totalFramesPlayed();
					lfo1_s = Oscillator::expSample( lfo1_p + static_cast<float>( t ) / lfo1_r );
					if( t < m_parent->m_lfo1_att ) lfo1_s *= ( static_cast<sample_t>( t ) / m_parent-> m_lfo1_att );
					m_lfo1_buf[f] = lfo1_s;
				}
				break;
		case WAVE_NOISE:
		default:
				for( f_cnt_t f = 0; f < _frames; f++ )
				{
					const f_cnt_t t = f + m_nph->totalFramesPlayed();
					if( t % static_cast<int>( lfo1_r ) == 0 ) m_lfo1_last = Oscillator::noiseSample( 0.0f );
					lfo1_s = m_lfo1_last;
					if( t < m_parent->m_lfo1_att ) lfo1_s *= ( static_cast<sample_t>( t ) / m_parent->m_lfo1_att );
					m_lfo1_buf[f] = lfo1_s;
				}
				break;
	}

	// LFO 2

	switch( m_parent->m_lfo2Wave.value() )
	{
		case WAVE_SINE:
				for( f_cnt_t f = 0; f < _frames; f++ )
				{
					const f_cnt_t t = f + m_nph->totalFramesPlayed();
					lfo2_s = Oscillator::sinSample( lfo2_p + static_cast<float>( t ) / lfo2_r );
					if( t < m_parent->m_lfo2_att ) lfo2_s *= ( static_cast<sample_t>( t ) / m_parent->m_lfo2_att );
					m_lfo2_buf[f] = lfo2_s;
				}
				break;
		case WAVE_TRI:
				for( f_cnt_t f = 0; f < _frames; f++ )
				{
					const f_cnt_t t = f + m_nph->totalFramesPlayed();
					lfo2_s = Oscillator::triangleSample( lfo2_p + static_cast<float>( t ) / lfo2_r );
					if( t < m_parent->m_lfo2_att ) lfo2_s *= ( static_cast<sample_t>( t ) / m_parent->m_lfo2_att );
					m_lfo2_buf[f] = lfo2_s;
				}
				break;
		case WAVE_SAW:
				for( f_cnt_t f = 0; f < _frames; f++ )
				{
					const f_cnt_t t = f + m_nph->totalFramesPlayed();
					lfo2_s = Oscillator::sawSample( lfo2_p + static_cast<float>( t ) / lfo2_r );
					if( t < m_parent->m_lfo2_att ) lfo2_s *= ( static_cast<sample_t>( t ) / m_parent->m_lfo2_att );
					m_lfo2_buf[f] = lfo2_s;
				}
				break;
		case WAVE_RAMP:
				for( f_cnt_t f = 0; f < _frames; f++ )
				{
					const f_cnt_t t = f + m_nph->totalFramesPlayed();
					lfo2_s = Oscillator::sawSample( lfo2_p + static_cast<float>( t ) / lfo2_r ) * -1.0f;
					if( t < m_parent->m_lfo2_att ) lfo2_s *= ( static_cast<sample_t>( t ) / m_parent->m_lfo2_att );
					m_lfo2_buf[f] = lfo2_s;
				}
				break;
		case WAVE_SQR:
				for( f_cnt_t f = 0; f < _frames; f++ )
				{
					const f_cnt_t t = f + m_nph->totalFramesPlayed();
					lfo2_s = Oscillator::squareSample( lfo2_p + static_cast<float>( t ) / lfo2_r );
					if( t < m_parent->m_lfo2_att ) lfo2_s *= ( static_cast<sample_t>( t ) / m_parent->m_lfo2_att );
					m_lfo2_buf[f] = lfo2_s;
				}
				break;
		case WAVE_SQRSOFT:
				for( f_cnt_t f = 0; f < _frames; f++ )
				{
					const f_cnt_t t = f + m_nph->totalFramesPlayed();
					lfo2_s = oscillate( WAVE_SQRSOFT, lfo2_p + static_cast<float>( t ) / lfo2_r );
					if( t < m_parent->m_lfo2_att ) lfo2_s *= ( static_cast<sample_t>( t ) / m_parent->m_lfo2_att );
					m_lfo2_buf[f] = lfo2_s;
				}
				break;
		case WAVE_MOOG:
				for( f_cnt_t f = 0; f < _frames; f++ )
				{
					const f_cnt_t t = f + m_nph->totalFramesPlayed();
					lfo2_s = Oscillator::moogSawSample( lfo2_p + static_cast<float>( t ) / lfo2_r );
					if( t < m_parent->m_lfo2_att ) lfo2_s *= ( static_cast<sample_t>( t ) / m_parent->m_lfo2_att );
					m_lfo2_buf[f] = lfo2_s;
				}
				break;
		case WAVE_SINABS:
				for( f_cnt_t f = 0; f < _frames; f++ )
				{
					const f_cnt_t t = f + m_nph->totalFramesPlayed();
					lfo2_s = oscillate( WAVE_SINABS, lfo2_p + static_cast<float>( t ) / lfo2_r );
					if( t < m_parent->m_lfo2_att ) lfo2_s *= ( static_cast<sample_t>( t ) / m_parent->m_lfo2_att );
					m_lfo2_buf[f] = lfo2_s;
				}
				break;
		case WAVE_EXP:
				for( f_cnt_t f = 0; f < _frames; f++ )
				{
					const f_cnt_t t = f + m_nph->totalFramesPlayed();
					lfo2_s = Oscillator::expSample( lfo2_p + static_cast<float>( t ) / lfo2_r );
					if( t < m_parent->m_lfo2_att ) lfo2_s *= ( static_cast<sample_t>( t ) / m_parent->m_lfo2_att );
					m_lfo2_buf[f] = lfo2_s;
				}
				break;
		case WAVE_NOISE:
		default:
				for( f_cnt_t f = 0; f < _frames; f++ )
				{
					const f_cnt_t t = f + m_nph->totalFramesPlayed();
					if( t % static_cast<int>( lfo2_r ) == 0 ) m_lfo2_last = Oscillator::noiseSample( 0.0f );
					lfo2_s = m_lfo2_last;
					if( t < m_parent->m_lfo2_att ) lfo2_s *= ( static_cast<sample_t>( t ) / m_parent->m_lfo2_att );
					m_lfo2_buf[f] = lfo2_s;
				}
				break;
	}

	/////////////////////////////////////////////
	//
	//
	// 					envelopes
	//
	//
	/////////////////////////////////////////////

	const float env1_s = m_parent-> m_env1Slope.value();
	const float env2_s = m_parent-> m_env2Slope.value();
	
	const float env1_sus = m_parent-> m_env1Sus.value();
	const float env2_sus = m_parent-> m_env2Sus.value();
	
	for( f_cnt_t f = 0; f < _frames; f++ )
	{
		// envelope 1

		// adjust phase for release
		if( m_nph->isReleased() && m_env1_phase < 4.0f )
		{
			if( m_env1_phase < 1.0f ) m_env1_phase = 5.0f;
			else if( m_env1_phase < 2.0f ) m_env1_phase = 5.0f - fraction( m_env1_phase );
			else if( m_env1_phase < 3.0f ) m_env1_phase = 4.0f;
			else m_env1_phase = 4.0f + fraction( m_env1_phase );
		}
		
		// process envelope
		if( m_env1_phase < 1.0f ) // pre-delay phase
		{
			m_env1_buf[f] = 0.0f;
			m_env1_phase = qMin( 1.0f, m_env1_phase + m_parent->m_env1_pre );
		}
		else if( m_env1_phase < 2.0f ) // attack phase
		{
			m_env1_buf[f] = calcSlope( fraction( m_env1_phase ), env1_s );
			m_env1_phase = qMin( 2.0f, m_env1_phase + m_parent->m_env1_att );
		}
		else if( m_env1_phase < 3.0f ) // hold phase
		{
			m_env1_buf[f] = 1.0f;
			m_env1_phase = qMin( 3.0f, m_env1_phase + m_parent->m_env1_hold );
		}
		else if( m_env1_phase < 4.0f ) // decay phase
		{
			const sample_t s = 1.0f - fraction( m_env1_phase );
			if( s <= env1_sus )	
			{
				m_env1_buf[f] = env1_sus;
			}
			else
			{
				m_env1_buf[f] = calcSlope( s, env1_s );
				m_env1_phase = qMin( 4.0f - env1_sus, m_env1_phase + m_parent->m_env1_dec );
			}
		}
		else if( m_env1_phase < 5.0f ) // release phase
		{
			m_env1_buf[f] = calcSlope( 1.0f - fraction( m_env1_phase ), env1_s );
			m_env1_phase += m_parent->m_env1_rel;
		}
		else m_env1_buf[f] = 0.0f;

//		qDebug( "env1 %f", m_env1_buf[f] );

		// envelope 2



		// adjust phase for release
		if( m_nph->isReleased() && m_env2_phase < 4.0f )
		{
			if( m_env2_phase < 1.0f ) m_env2_phase = 5.0f;
			else if( m_env2_phase < 2.0f ) m_env2_phase = 5.0f - fraction( m_env2_phase );
			else if( m_env2_phase < 3.0f ) m_env2_phase = 4.0f;
			else m_env2_phase = 4.0f + fraction( m_env2_phase );
		}
		
		// process envelope
		if( m_env2_phase < 1.0f ) // pre-delay phase
		{
			m_env2_buf[f] = 0.0f;
			m_env2_phase = qMin( 1.0f, m_env2_phase + m_parent->m_env2_pre );
		}
		else if( m_env2_phase < 2.0f ) // attack phase
		{
			m_env2_buf[f] = calcSlope( fraction( m_env2_phase ), env2_s );
			m_env2_phase = qMin( 2.0f, m_env2_phase + m_parent->m_env2_att );
		}
		else if( m_env2_phase < 3.0f ) // hold phase
		{
			m_env2_buf[f] = 1.0f;
			m_env2_phase = qMin( 3.0f, m_env2_phase + m_parent->m_env2_hold );
		}
		else if( m_env2_phase < 4.0f ) // decay phase
		{
			const sample_t s = 1.0f - fraction( m_env2_phase );
			if( s <= env2_sus )	
			{
				m_env2_buf[f] = env2_sus;
			}
			else
			{
				m_env2_buf[f] = calcSlope( s, env2_s );
				m_env2_phase = qMin( 4.0f - env2_sus, m_env2_phase + m_parent->m_env2_dec );
			}
		}
		else if( m_env2_phase < 5.0f ) // release phase
		{
			m_env2_buf[f] = calcSlope( 1.0f - fraction( m_env2_phase), env2_s );
			m_env2_phase += m_parent->m_env2_rel;
		}
		else m_env2_buf[f] = 0.0f;

	}

}



MonstroInstrument::MonstroInstrument( InstrumentTrack * _instrument_track ) :
		Instrument( _instrument_track, &monstro_plugin_descriptor ),

		m_osc1Vol( 33.0, 0.0, 200.0, 0.1, this, tr( "Osc 1 Volume" ) ),
		m_osc1Pan( 0.0, -100.0, 100.0, 0.1, this, tr( "Osc 1 Panning" ) ),
		m_osc1Crs( 0.0, -24.0, 24.0, 1.0, this, tr( "Osc 1 Coarse detune" ) ),
		m_osc1Ftl( 0.0, -100.0, 100.0, 1.0, this, tr( "Osc 1 Fine detune left" ) ),
		m_osc1Ftr( 0.0, -100.0, 100.0, 1.0, this, tr( "Osc 1 Fine detune right" ) ),
		m_osc1Spo( 0.0, -180.0, 180.0, 0.1, this, tr( "Osc 1 Stereo phase offset" ) ),
		m_osc1Pw( 50.0, 0.0, 100.0, 0.01, this, tr( "Osc 1 Pulse width" ) ),

		m_osc2Vol( 33.0, 0.0, 200.0, 0.1, this, tr( "Osc 2 Volume" ) ),
		m_osc2Pan( 0.0, -100.0, 100.0, 0.1, this, tr( "Osc 2 Panning" ) ),
		m_osc2Crs( 0.0, -24.0, 24.0, 1.0, this, tr( "Osc 2 Coarse detune" ) ),
		m_osc2Ftl( 0.0, -100.0, 100.0, 1.0, this, tr( "Osc 2 Fine detune left" ) ),
		m_osc2Ftr( 0.0, -100.0, 100.0, 1.0, this, tr( "Osc 2 Fine detune right" ) ),
		m_osc2Spo( 0.0, -180.0, 180.0, 0.1, this, tr( "Osc 2 Stereo phase offset" ) ),
		m_osc2Wave( this, tr( "Osc 2 Waveform" ) ),

		m_osc3Vol( 33.0, 0.0, 200.0, 0.1, this, tr( "Osc 3 Volume" ) ),
		m_osc3Pan( 0.0, -100.0, 100.0, 0.1, this, tr( "Osc 3 Panning" ) ),
		m_osc3Crs( 0.0, -24.0, 24.0, 1.0, this, tr( "Osc 3 Coarse detune" ) ),
		m_osc3Spo( 0.0, -180.0, 180.0, 0.1, this, tr( "Osc 3 Stereo phase offset" ) ),
		m_osc3Sub( 0.0, -100.0, 100.0, 0.1, this, tr( "Osc 3 Sub-oscillator mix" ) ),
		m_osc3Wave1( this, tr( "Osc 3 Waveform 1" ) ),
		m_osc3Wave2( this, tr( "Osc 3 Waveform 2" ) ),

		m_lfo1Wave( this, tr( "LFO 1 Waveform" ) ),
		m_lfo1Att( 0.0f, 0.0f, 2000.0f, 1.0f, 2000.0f, this, tr( "LFO 1 Attack" ) ),
		m_lfo1Rate( 1.0f, 0.1, 20000.0, 0.1, 20000.0f, this, tr( "LFO 1 Rate" ) ),
		m_lfo1Phs( 0.0, -180.0, 180.0, 0.1, this, tr( "LFO 1 Phase" ) ),

		m_lfo2Wave( this, tr( "LFO 2 Waveform" ) ),
		m_lfo2Att( 0.0f, 0.0f, 2000.0f, 1.0f, 2000.0f, this, tr( "LFO 2 Attack" ) ),
		m_lfo2Rate( 1.0f, 0.1, 20000.0, 0.1, 20000.0f, this, tr( "LFO 2 Rate" ) ),
		m_lfo2Phs( 0.0, -180.0, 180.0, 0.1, this, tr( "LFO 2 Phase" ) ),

		m_env1Pre( 0.0f, 0.0f, 2000.0f, 1.0f, 2000.0f, this, tr( "Env 1 Pre-delay" ) ),
		m_env1Att( 0.0f, 0.0f, 2000.0f, 1.0f, 2000.0f, this, tr( "Env 1 Attack" ) ),
		m_env1Hold( 0.0f, 0.0f, 2000.0f, 1.0f, 2000.0f, this, tr( "Env 1 Hold" ) ),
		m_env1Dec( 0.0f, 0.0f, 2000.0f, 1.0f, 2000.0f, this, tr( "Env 1 Decay" ) ),
		m_env1Sus( 1.0f, 0.0f, 1.0f, 0.001f, this, tr( "Env 1 Sustain" ) ),
		m_env1Rel( 0.0f, 0.0f, 2000.0f, 1.0f, 2000.0f, this, tr( "Env 1 Release" ) ),
		m_env1Slope( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Env 1 Slope" ) ),

		m_env2Pre( 0.0f, 0.0f, 2000.0f, 1.0f, 2000.0f, this, tr( "Env 2 Pre-delay" ) ),
		m_env2Att( 0.0f, 0.0f, 2000.0f, 1.0f, 2000.0f, this, tr( "Env 2 Attack" ) ),
		m_env2Hold( 0.0f, 0.0f, 2000.0f, 1.0f, 2000.0f, this, tr( "Env 2 Hold" ) ),
		m_env2Dec( 0.0f, 0.0f, 2000.0f, 1.0f, 2000.0f, this, tr( "Env 2 Decay" ) ),
		m_env2Sus( 1.0f, 0.0f, 1.0f, 0.001f, this, tr( "Env 2 Sustain" ) ),
		m_env2Rel( 0.0f, 0.0f, 2000.0f, 1.0f, 2000.0f, this, tr( "Env 2 Release" ) ),
		m_env2Slope( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Env 2 Slope" ) ),

		m_o23Mod( 0, 0, NUM_MODS - 1, this, tr( "Osc2-3 modulation" ) ),

		m_selectedView( 0, 0, 1, this, tr( "Selected view" ) ),

		m_vol1env1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Vol1-Env1" ) ),
		m_vol1env2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Vol1-Env2" ) ),
		m_vol1lfo1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Vol1-LFO1" ) ),
		m_vol1lfo2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Vol1-LFO2" ) ),

		m_vol2env1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Vol2-Env1" ) ),
		m_vol2env2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Vol2-Env2" ) ),
		m_vol2lfo1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Vol2-LFO1" ) ),
		m_vol2lfo2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Vol2-LFO2" ) ),

		m_vol3env1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Vol3-Env1" ) ),
		m_vol3env2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Vol3-Env2" ) ),
		m_vol3lfo1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Vol3-LFO1" ) ),
		m_vol3lfo2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Vol3-LFO2" ) ),

		m_phs1env1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Phs1-Env1" ) ),
		m_phs1env2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Phs1-Env2" ) ),
		m_phs1lfo1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Phs1-LFO1" ) ),
		m_phs1lfo2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Phs1-LFO2" ) ),

		m_phs2env1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Phs2-Env1" ) ),
		m_phs2env2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Phs2-Env2" ) ),
		m_phs2lfo1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Phs2-LFO1" ) ),
		m_phs2lfo2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Phs2-LFO2" ) ),

		m_phs3env1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Phs3-Env1" ) ),
		m_phs3env2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Phs3-Env2" ) ),
		m_phs3lfo1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Phs3-LFO1" ) ),
		m_phs3lfo2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Phs3-LFO2" ) ),

		m_pit1env1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Pit1-Env1" ) ),
		m_pit1env2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Pit1-Env2" ) ),
		m_pit1lfo1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Pit1-LFO1" ) ),
		m_pit1lfo2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Pit1-LFO2" ) ),

		m_pit2env1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Pit2-Env1" ) ),
		m_pit2env2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Pit2-Env2" ) ),
		m_pit2lfo1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Pit2-LFO1" ) ),
		m_pit2lfo2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Pit2-LFO2" ) ),

		m_pit3env1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Pit3-Env1" ) ),
		m_pit3env2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Pit3-Env2" ) ),
		m_pit3lfo1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Pit3-LFO1" ) ),
		m_pit3lfo2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Pit3-LFO2" ) ),

		m_pw1env1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "PW1-Env1" ) ),
		m_pw1env2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "PW1-Env2" ) ),
		m_pw1lfo1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "PW1-LFO1" ) ),
		m_pw1lfo2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "PW1-LFO2" ) ),

		m_sub3env1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Sub3-Env1" ) ),
		m_sub3env2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Sub3-Env2" ) ),
		m_sub3lfo1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Sub3-LFO1" ) ),
		m_sub3lfo2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Sub3-LFO2" ) )

{
// setup waveboxes
	setwavemodel( m_osc2Wave )
	setwavemodel( m_osc3Wave1 )
	setwavemodel( m_osc3Wave2 )
	setlfowavemodel( m_lfo1Wave )
	setlfowavemodel( m_lfo2Wave )

// make connections:

// updateVolumes

	connect( &m_osc1Vol, SIGNAL( dataChanged() ), this, SLOT( updateVolumes() ) );
	connect( &m_osc1Pan, SIGNAL( dataChanged() ), this, SLOT( updateVolumes() ) );
	connect( &m_osc2Vol, SIGNAL( dataChanged() ), this, SLOT( updateVolumes() ) );
	connect( &m_osc2Pan, SIGNAL( dataChanged() ), this, SLOT( updateVolumes() ) );
	connect( &m_osc3Vol, SIGNAL( dataChanged() ), this, SLOT( updateVolumes() ) );
	connect( &m_osc3Pan, SIGNAL( dataChanged() ), this, SLOT( updateVolumes() ) );

// updateFreq

	connect( &m_osc1Crs, SIGNAL( dataChanged() ), this, SLOT( updateFreq() ) );
	connect( &m_osc2Crs, SIGNAL( dataChanged() ), this, SLOT( updateFreq() ) );
	connect( &m_osc3Crs, SIGNAL( dataChanged() ), this, SLOT( updateFreq() ) );

	connect( &m_osc1Ftl, SIGNAL( dataChanged() ), this, SLOT( updateFreq() ) );
	connect( &m_osc2Ftl, SIGNAL( dataChanged() ), this, SLOT( updateFreq() ) );

	connect( &m_osc1Ftr, SIGNAL( dataChanged() ), this, SLOT( updateFreq() ) );
	connect( &m_osc2Ftr, SIGNAL( dataChanged() ), this, SLOT( updateFreq() ) );

// updatePO
	connect( &m_osc1Spo, SIGNAL( dataChanged() ), this, SLOT( updatePO() ) );
	connect( &m_osc2Spo, SIGNAL( dataChanged() ), this, SLOT( updatePO() ) );
	connect( &m_osc3Spo, SIGNAL( dataChanged() ), this, SLOT( updatePO() ) );
	
// updateEnvelope1

	connect( &m_env1Pre, SIGNAL( dataChanged() ), this, SLOT( updateEnvelope1() ) );
	connect( &m_env1Att, SIGNAL( dataChanged() ), this, SLOT( updateEnvelope1() ) );
	connect( &m_env1Hold, SIGNAL( dataChanged() ), this, SLOT( updateEnvelope1() ) );
	connect( &m_env1Dec, SIGNAL( dataChanged() ), this, SLOT( updateEnvelope1() ) );
	connect( &m_env1Rel, SIGNAL( dataChanged() ), this, SLOT( updateEnvelope1() ) );

// updateEnvelope2

	connect( &m_env2Pre, SIGNAL( dataChanged() ), this, SLOT( updateEnvelope2() ) );
	connect( &m_env2Att, SIGNAL( dataChanged() ), this, SLOT( updateEnvelope2() ) );
	connect( &m_env2Hold, SIGNAL( dataChanged() ), this, SLOT( updateEnvelope2() ) );
	connect( &m_env2Dec, SIGNAL( dataChanged() ), this, SLOT( updateEnvelope2() ) );
	connect( &m_env2Rel, SIGNAL( dataChanged() ), this, SLOT( updateEnvelope2() ) );

// updateLFOAtts

	connect( &m_lfo1Att, SIGNAL( dataChanged() ), this, SLOT( updateLFOAtts() ) );
	connect( &m_lfo2Att, SIGNAL( dataChanged() ), this, SLOT( updateLFOAtts() ) );

// updateSampleRate

	connect( engine::mixer(), SIGNAL( sampleRateChanged() ), this, SLOT( updateSamplerate() ) );

	m_fpp = engine::mixer()->framesPerPeriod();

	updateSamplerate();
	updateVolumes();
	updateFreq();
	updatePO();
}


MonstroInstrument::~MonstroInstrument()
{
}


void MonstroInstrument::playNote( NotePlayHandle * _n,
						sampleFrame * _working_buffer )
{
	if ( _n->totalFramesPlayed() == 0 || _n->m_pluginData == NULL )
	{
		const sample_rate_t samplerate = m_samplerate;
		_n->m_pluginData = new MonstroSynth( this, _n, samplerate, m_fpp );
	}

	const fpp_t frames = _n->framesLeftForCurrentPeriod();

	MonstroSynth * ms = static_cast<MonstroSynth *>( _n->m_pluginData );

	ms->renderOutput( frames, _working_buffer );

	applyRelease( _working_buffer, _n );

	instrumentTrack()->processAudioBuffer( _working_buffer, frames, _n );
}

void MonstroInstrument::deleteNotePluginData( NotePlayHandle * _n )
{
	delete static_cast<MonstroSynth *>( _n->m_pluginData );
}


void MonstroInstrument::saveSettings( QDomDocument & _doc,
							QDomElement & _this )
{
	m_osc1Vol.saveSettings( _doc, _this, "o1vol" );
	m_osc1Pan.saveSettings( _doc, _this, "o1pan" );
	m_osc1Crs.saveSettings( _doc, _this, "o1crs" );
	m_osc1Ftl.saveSettings( _doc, _this, "o1ftl" );
	m_osc1Ftr.saveSettings( _doc, _this, "o1ftr" );
	m_osc1Spo.saveSettings( _doc, _this, "o1spo" );
	m_osc1Pw.saveSettings( _doc, _this, "o1pw" );

	m_osc2Vol.saveSettings( _doc, _this, "o2vol" );
	m_osc2Pan.saveSettings( _doc, _this, "o2pan" );
	m_osc2Crs.saveSettings( _doc, _this, "o2crs" );
	m_osc2Ftl.saveSettings( _doc, _this, "o2ftl" );
	m_osc2Ftr.saveSettings( _doc, _this, "o2ftr" );
	m_osc2Spo.saveSettings( _doc, _this, "o2spo" );
	m_osc2Wave.saveSettings( _doc, _this, "o2wav" );

	m_osc3Vol.saveSettings( _doc, _this, "o3vol" );
	m_osc3Pan.saveSettings( _doc, _this, "o3pan" );
	m_osc3Crs.saveSettings( _doc, _this, "o3crs" );
	m_osc3Spo.saveSettings( _doc, _this, "o3spo" );
	m_osc3Sub.saveSettings( _doc, _this, "o3sub" );
	m_osc3Wave1.saveSettings( _doc, _this, "o3wav1" );
	m_osc3Wave2.saveSettings( _doc, _this, "o3wav2" );

	m_lfo1Wave.saveSettings( _doc, _this, "l1wav" );
	m_lfo1Att.saveSettings( _doc, _this, "l1att" );
	m_lfo1Rate.saveSettings( _doc, _this, "l1rat" );
	m_lfo1Phs.saveSettings( _doc, _this, "l1phs" );

	m_lfo2Wave.saveSettings( _doc, _this, "l2wav" );
	m_lfo2Att.saveSettings( _doc, _this, "l2att" );
	m_lfo2Rate.saveSettings( _doc, _this, "l2rat" );
	m_lfo2Phs.saveSettings( _doc, _this, "l2phs" );

	m_env1Pre.saveSettings( _doc, _this, "e1pre" );
	m_env1Att.saveSettings( _doc, _this, "e1att" );
	m_env1Hold.saveSettings( _doc, _this, "e1hol" );
	m_env1Dec.saveSettings( _doc, _this, "e1dec" );
	m_env1Sus.saveSettings( _doc, _this, "e1sus" );
	m_env1Rel.saveSettings( _doc, _this, "e1rel" );
	m_env1Slope.saveSettings( _doc, _this, "e1slo" );

	m_env2Pre.saveSettings( _doc, _this, "e2pre" );
	m_env2Att.saveSettings( _doc, _this, "e2att" );
	m_env2Hold.saveSettings( _doc, _this, "e2hol" );
	m_env2Dec.saveSettings( _doc, _this, "e2dec" );
	m_env2Sus.saveSettings( _doc, _this, "e2sus" );
	m_env2Rel.saveSettings( _doc, _this, "e2rel" );
	m_env2Slope.saveSettings( _doc, _this, "e2slo" );

	m_o23Mod.saveSettings( _doc, _this, "o23mo" );	

	m_vol1env1.saveSettings( _doc, _this, "v1e1" );
	m_vol1env2.saveSettings( _doc, _this, "v1e2" );
	m_vol1lfo1.saveSettings( _doc, _this, "v1l1" );
	m_vol1lfo2.saveSettings( _doc, _this, "v1l2" );

	m_vol2env1.saveSettings( _doc, _this, "v2e1" );
	m_vol2env2.saveSettings( _doc, _this, "v2e2" );
	m_vol2lfo1.saveSettings( _doc, _this, "v2l1" );
	m_vol2lfo2.saveSettings( _doc, _this, "v2l2" );

	m_vol3env1.saveSettings( _doc, _this, "v3e1" );
	m_vol3env2.saveSettings( _doc, _this, "v3e2" );
	m_vol3lfo1.saveSettings( _doc, _this, "v3l1" );
	m_vol3lfo2.saveSettings( _doc, _this, "v3l2" );

	m_phs1env1.saveSettings( _doc, _this, "p1e1" );
	m_phs1env2.saveSettings( _doc, _this, "p1e2" );
	m_phs1lfo1.saveSettings( _doc, _this, "p1l1" );
	m_phs1lfo2.saveSettings( _doc, _this, "p1l2" );

	m_phs2env1.saveSettings( _doc, _this, "p2e1" );
	m_phs2env2.saveSettings( _doc, _this, "p2e2" );
	m_phs2lfo1.saveSettings( _doc, _this, "p2l1" );
	m_phs2lfo2.saveSettings( _doc, _this, "p2l2" );

	m_phs3env1.saveSettings( _doc, _this, "p3e1" );
	m_phs3env2.saveSettings( _doc, _this, "p3e2" );
	m_phs3lfo1.saveSettings( _doc, _this, "p3l1" );
	m_phs3lfo2.saveSettings( _doc, _this, "p3l2" );

	m_pit1env1.saveSettings( _doc, _this, "f1e1" );
	m_pit1env2.saveSettings( _doc, _this, "f1e2" );
	m_pit1lfo1.saveSettings( _doc, _this, "f1l1" );
	m_pit1lfo2.saveSettings( _doc, _this, "f1l2" );

	m_pit2env1.saveSettings( _doc, _this, "f2e1" );
	m_pit2env2.saveSettings( _doc, _this, "f2e2" );
	m_pit2lfo1.saveSettings( _doc, _this, "f2l1" );
	m_pit2lfo2.saveSettings( _doc, _this, "f2l2" );

	m_pit3env1.saveSettings( _doc, _this, "f3e1" );
	m_pit3env2.saveSettings( _doc, _this, "f3e2" );
	m_pit3lfo1.saveSettings( _doc, _this, "f3l1" );
	m_pit3lfo2.saveSettings( _doc, _this, "f3l2" );

	m_pw1env1.saveSettings( _doc, _this, "w1e1" );
	m_pw1env2.saveSettings( _doc, _this, "w1e2" );
	m_pw1lfo1.saveSettings( _doc, _this, "w1l1" );
	m_pw1lfo2.saveSettings( _doc, _this, "w1l2" );

	m_sub3env1.saveSettings( _doc, _this, "s3e1" );
	m_sub3env2.saveSettings( _doc, _this, "s3e2" );
	m_sub3lfo1.saveSettings( _doc, _this, "s3l1" );
	m_sub3lfo2.saveSettings( _doc, _this, "s3l2" );
	
}						

void MonstroInstrument::loadSettings( const QDomElement & _this )
{
	m_osc1Vol.loadSettings( _this, "o1vol" );
	m_osc1Pan.loadSettings( _this, "o1pan" );
	m_osc1Crs.loadSettings( _this, "o1crs" );
	m_osc1Ftl.loadSettings( _this, "o1ftl" );
	m_osc1Ftr.loadSettings( _this, "o1ftr" );
	m_osc1Spo.loadSettings( _this, "o1spo" );
	m_osc1Pw.loadSettings( _this, "o1pw" );

	m_osc2Vol.loadSettings( _this, "o2vol" );
	m_osc2Pan.loadSettings( _this, "o2pan" );
	m_osc2Crs.loadSettings( _this, "o2crs" );
	m_osc2Ftl.loadSettings( _this, "o2ftl" );
	m_osc2Ftr.loadSettings( _this, "o2ftr" );
	m_osc2Spo.loadSettings( _this, "o2spo" );
	m_osc2Wave.loadSettings( _this, "o2wav" );

	m_osc3Vol.loadSettings( _this, "o3vol" );
	m_osc3Pan.loadSettings( _this, "o3pan" );
	m_osc3Crs.loadSettings( _this, "o3crs" );
	m_osc3Spo.loadSettings( _this, "o3spo" );
	m_osc3Sub.loadSettings( _this, "o3sub" );
	m_osc3Wave1.loadSettings( _this, "o3wav1" );
	m_osc3Wave2.loadSettings( _this, "o3wav2" );

	m_lfo1Wave.loadSettings( _this, "l1wav" );
	m_lfo1Att.loadSettings( _this, "l1att" );
	m_lfo1Rate.loadSettings( _this, "l1rat" );
	m_lfo1Phs.loadSettings( _this, "l1phs" );

	m_lfo2Wave.loadSettings( _this, "l2wav" );
	m_lfo2Att.loadSettings( _this, "l2att" );
	m_lfo2Rate.loadSettings( _this, "l2rat" );
	m_lfo2Phs.loadSettings( _this, "l2phs" );

	m_env1Pre.loadSettings( _this, "e1pre" );
	m_env1Att.loadSettings( _this, "e1att" );
	m_env1Hold.loadSettings( _this, "e1hol" );
	m_env1Dec.loadSettings( _this, "e1dec" );
	m_env1Sus.loadSettings( _this, "e1sus" );
	m_env1Rel.loadSettings( _this, "e1rel" );
	m_env1Slope.loadSettings( _this, "e1slo" );

	m_env2Pre.loadSettings( _this, "e2pre" );
	m_env2Att.loadSettings( _this, "e2att" );
	m_env2Hold.loadSettings( _this, "e2hol" );
	m_env2Dec.loadSettings( _this, "e2dec" );
	m_env2Sus.loadSettings( _this, "e2sus" );
	m_env2Rel.loadSettings( _this, "e2rel" );
	m_env2Slope.loadSettings( _this, "e2slo" );

	m_o23Mod.loadSettings( _this, "o23mo" );	

	m_vol1env1.loadSettings( _this, "v1e1" );
	m_vol1env2.loadSettings( _this, "v1e2" );
	m_vol1lfo1.loadSettings( _this, "v1l1" );
	m_vol1lfo2.loadSettings( _this, "v1l2" );

	m_vol2env1.loadSettings( _this, "v2e1" );
	m_vol2env2.loadSettings( _this, "v2e2" );
	m_vol2lfo1.loadSettings( _this, "v2l1" );
	m_vol2lfo2.loadSettings( _this, "v2l2" );

	m_vol3env1.loadSettings( _this, "v3e1" );
	m_vol3env2.loadSettings( _this, "v3e2" );
	m_vol3lfo1.loadSettings( _this, "v3l1" );
	m_vol3lfo2.loadSettings( _this, "v3l2" );

	m_phs1env1.loadSettings( _this, "p1e1" );
	m_phs1env2.loadSettings( _this, "p1e2" );
	m_phs1lfo1.loadSettings( _this, "p1l1" );
	m_phs1lfo2.loadSettings( _this, "p1l2" );

	m_phs2env1.loadSettings( _this, "p2e1" );
	m_phs2env2.loadSettings( _this, "p2e2" );
	m_phs2lfo1.loadSettings( _this, "p2l1" );
	m_phs2lfo2.loadSettings( _this, "p2l2" );

	m_phs3env1.loadSettings( _this, "p3e1" );
	m_phs3env2.loadSettings( _this, "p3e2" );
	m_phs3lfo1.loadSettings( _this, "p3l1" );
	m_phs3lfo2.loadSettings( _this, "p3l2" );

	m_pit1env1.loadSettings( _this, "f1e1" );
	m_pit1env2.loadSettings( _this, "f1e2" );
	m_pit1lfo1.loadSettings( _this, "f1l1" );
	m_pit1lfo2.loadSettings( _this, "f1l2" );

	m_pit2env1.loadSettings( _this, "f2e1" );
	m_pit2env2.loadSettings( _this, "f2e2" );
	m_pit2lfo1.loadSettings( _this, "f2l1" );
	m_pit2lfo2.loadSettings( _this, "f2l2" );

	m_pit3env1.loadSettings( _this, "f3e1" );
	m_pit3env2.loadSettings( _this, "f3e2" );
	m_pit3lfo1.loadSettings( _this, "f3l1" );
	m_pit3lfo2.loadSettings( _this, "f3l2" );

	m_pw1env1.loadSettings( _this, "w1e1" );
	m_pw1env2.loadSettings( _this, "w1e2" );
	m_pw1lfo1.loadSettings( _this, "w1l1" );
	m_pw1lfo2.loadSettings( _this, "w1l2" );

	m_sub3env1.loadSettings( _this, "s3e1" );
	m_sub3env2.loadSettings( _this, "s3e2" );
	m_sub3lfo1.loadSettings( _this, "s3l1" );
	m_sub3lfo2.loadSettings( _this, "s3l2" );
	
}


QString MonstroInstrument::nodeName() const
{
	return monstro_plugin_descriptor.name;
}


f_cnt_t MonstroInstrument::desiredReleaseFrames() const
{
	return qMax( 64, qMax( m_env1_relF, m_env2_relF ) );
}


PluginView * MonstroInstrument::instantiateView( QWidget * _parent )
{
	return( new MonstroView( this, _parent ) );
}


void MonstroInstrument::updateVolumes()
{
	m_osc1l_vol = leftCh( m_osc1Vol.value(), m_osc1Pan.value() );
	m_osc1r_vol = rightCh( m_osc1Vol.value(), m_osc1Pan.value() );
	
	m_osc2l_vol = leftCh( m_osc2Vol.value(), m_osc2Pan.value() );
	m_osc2r_vol = rightCh( m_osc2Vol.value(), m_osc2Pan.value() );
	
	m_osc3l_vol = leftCh( m_osc3Vol.value(), m_osc3Pan.value() );
	m_osc3r_vol = rightCh( m_osc3Vol.value(), m_osc3Pan.value() );	
}


void MonstroInstrument::updateFreq()
{
	m_osc1l_freq = powf( 2.0f, m_osc1Crs.value() / 12.0f ) *  
					powf( 2.0f, m_osc1Ftl.value() / 1200.0f );
	m_osc1r_freq = powf( 2.0f, m_osc1Crs.value() / 12.0f ) *  
					powf( 2.0f, m_osc1Ftr.value() / 1200.0f );
					
	m_osc2l_freq = powf( 2.0f, m_osc2Crs.value() / 12.0f ) *  
					powf( 2.0f, m_osc2Ftl.value() / 1200.0f );
	m_osc2r_freq = powf( 2.0f, m_osc2Crs.value() / 12.0f ) *  
					powf( 2.0f, m_osc2Ftr.value() / 1200.0f );
					
	m_osc3_freq = powf( 2.0f, m_osc3Crs.value() / 12.0f );
}


void MonstroInstrument::updatePO()
{
	m_osc1l_po = m_osc1Spo.value() / 360.0;
	m_osc1r_po = ( m_osc1Spo.value() * -1.0 ) / 360.0;
	
	m_osc2l_po = m_osc2Spo.value() / 360.0;
	m_osc2r_po = ( m_osc2Spo.value() * -1.0 ) / 360.0;
	
	m_osc3l_po = m_osc3Spo.value() / 360.0;
	m_osc3r_po = ( m_osc3Spo.value() * -1.0 ) / 360.0;
}

void MonstroInstrument::updateEnvelope1()
{
	if( m_env1Pre.value() == 0.0f ) m_env1_pre = 1.0;
		else m_env1_pre =  1.0 / ( m_env1Pre.value()  / 1000.0f ) / m_samplerate;
	if( m_env1Att.value() == 0.0f ) m_env1_att = 1.0;
		else m_env1_att =  1.0 / ( m_env1Att.value()  / 1000.0f ) / m_samplerate;
	if( m_env1Hold.value() == 0.0f ) m_env1_hold = 1.0;
		else m_env1_hold = 1.0 / ( m_env1Hold.value() / 1000.0f ) / m_samplerate;
	if( m_env1Dec.value() == 0.0f ) m_env1_dec = 1.0;
		else m_env1_dec =  1.0 / ( m_env1Dec.value()  / 1000.0f ) / m_samplerate;
	if( m_env1Rel.value() == 0.0f ) m_env1_rel = 1.0;
		else m_env1_rel =  1.0 / ( m_env1Rel.value()  / 1000.0f ) / m_samplerate;
	
	m_env1_len = ( m_env1Pre.value() + m_env1Att.value() + m_env1Hold.value() + m_env1Dec.value() ) * m_samplerate / 1000.0f;
	m_env1_relF = m_env1Rel.value() * m_samplerate / 1000.0f;
}
void MonstroInstrument::updateEnvelope2()
{
	if( m_env2Pre.value() == 0.0f ) m_env2_pre = 1.0;
		else m_env2_pre =  1.0 / ( m_env2Pre.value()  / 1000.0f ) / m_samplerate;
	if( m_env2Att.value() == 0.0f ) m_env2_att = 1.0;
		else m_env2_att =  1.0 / ( m_env2Att.value()  / 1000.0f ) / m_samplerate;
	if( m_env2Hold.value() == 0.0f ) m_env2_hold = 1.0;
		else m_env2_hold = 1.0 / ( m_env2Hold.value() / 1000.0f ) / m_samplerate;
	if( m_env2Dec.value() == 0.0f ) m_env2_dec = 1.0;
		else m_env2_dec =  1.0 / ( m_env2Dec.value()  / 1000.0f ) / m_samplerate;
	if( m_env2Rel.value() == 0.0f ) m_env2_rel = 1.0;
		else m_env2_rel =  1.0 / ( m_env2Rel.value()  / 1000.0f ) / m_samplerate;
	
	m_env2_len = ( m_env2Pre.value() + m_env2Att.value() + m_env2Hold.value() + m_env2Dec.value() ) * m_samplerate / 1000.0f;
	m_env2_relF = m_env2Rel.value() * m_samplerate / 1000.0f;
}


void MonstroInstrument::updateLFOAtts()
{
	m_lfo1_att = m_lfo1Att.value() * m_samplerate / 1000.0f;
	m_lfo2_att = m_lfo2Att.value() * m_samplerate / 1000.0f;
}


void MonstroInstrument::updateSamplerate()
{
	m_samplerate = engine::mixer()->processingSampleRate();
	updateEnvelope1();
	updateEnvelope2();
	updateLFOAtts();
}


MonstroView::MonstroView( Instrument * _instrument,
					QWidget * _parent ) :
					InstrumentView( _instrument, _parent )
{
	m_operatorsView = setupOperatorsView( this );
	setWidgetBackground( m_operatorsView, "artwork_op" );
	m_operatorsView->show();
	m_operatorsView->move( 0, 0 );
	
	m_matrixView = setupMatrixView( this );
	setWidgetBackground( m_matrixView, "artwork_mat" );
	m_matrixView->hide();
	m_matrixView->move( 0, 0 );
	
// "tab buttons"
	
	pixmapButton * m_opViewButton = new pixmapButton( this, NULL );
	m_opViewButton -> move( 0,0 );
	m_opViewButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "opview_active" ) );
	m_opViewButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "opview_inactive" ) );
	toolTip::add( m_opViewButton, tr( "Operators view" ) );
	
	pixmapButton * m_matViewButton = new pixmapButton( this, NULL );
	m_matViewButton -> move( 125,0 );
	m_matViewButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "matview_active" ) );
	m_matViewButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "matview_inactive" ) );
	toolTip::add( m_matViewButton, tr( "Matrix view" ) );
	
	m_selectedViewGroup = new automatableButtonGroup( this );
	m_selectedViewGroup -> addButton( m_opViewButton );
	m_selectedViewGroup -> addButton( m_matViewButton );
	
	connect( m_opViewButton, SIGNAL( clicked() ), this, SLOT( updateLayout() ) );
	connect( m_matViewButton, SIGNAL( clicked() ), this, SLOT( updateLayout() ) );
}


MonstroView::~MonstroView()
{
}


void MonstroView::updateLayout()
{
	switch( m_selectedViewGroup->model()->value() )
	{
		case OPVIEW:
			m_operatorsView->show();
			m_matrixView->hide();
			break;
		case MATVIEW:
			m_operatorsView->hide();
			m_matrixView->show();
			break;
	}
}


void MonstroView::modelChanged()
{
	MonstroInstrument * m = castModel<MonstroInstrument>();
	
	m_osc1VolKnob-> setModel( &m-> m_osc1Vol );
	m_osc1PanKnob-> setModel( &m-> m_osc1Pan );
	m_osc1CrsKnob-> setModel( &m-> m_osc1Crs );
	m_osc1FtlKnob-> setModel( &m-> m_osc1Ftl );
	m_osc1FtrKnob-> setModel( &m-> m_osc1Ftr );
	m_osc1SpoKnob-> setModel( &m-> m_osc1Spo );
	m_osc1PwKnob-> setModel( &m-> m_osc1Pw );

	m_osc2VolKnob-> setModel( &m-> m_osc2Vol );
	m_osc2PanKnob-> setModel( &m-> m_osc2Pan );
	m_osc2CrsKnob-> setModel( &m-> m_osc2Crs );
	m_osc2FtlKnob-> setModel( &m-> m_osc2Ftl );
	m_osc2FtrKnob-> setModel( &m-> m_osc2Ftr );
	m_osc2SpoKnob-> setModel( &m-> m_osc2Spo );
	m_osc2WaveBox-> setModel( &m-> m_osc2Wave );

	m_osc3VolKnob-> setModel( &m-> m_osc3Vol );
	m_osc3PanKnob-> setModel( &m-> m_osc3Pan );
	m_osc3CrsKnob-> setModel( &m-> m_osc3Crs );
	m_osc3SpoKnob-> setModel( &m-> m_osc3Spo );
	m_osc3SubKnob-> setModel( &m-> m_osc3Sub );	
	m_osc3Wave1Box-> setModel( &m-> m_osc3Wave1 );
	m_osc3Wave2Box-> setModel( &m-> m_osc3Wave2 );
	
	m_lfo1WaveBox-> setModel( &m-> m_lfo1Wave );
	m_lfo1AttKnob-> setModel( &m-> m_lfo1Att );
	m_lfo1RateKnob-> setModel( &m-> m_lfo1Rate );
	m_lfo1PhsKnob-> setModel( &m-> m_lfo1Phs );
	
	m_lfo2WaveBox-> setModel( &m-> m_lfo2Wave );
	m_lfo2AttKnob-> setModel( &m-> m_lfo2Att );
	m_lfo2RateKnob-> setModel( &m-> m_lfo2Rate );
	m_lfo2PhsKnob-> setModel( &m-> m_lfo2Phs );
	
	m_env1PreKnob-> setModel( &m->  m_env1Pre );
	m_env1AttKnob-> setModel( &m->  m_env1Att );
	m_env1HoldKnob-> setModel( &m->  m_env1Hold );
	m_env1DecKnob-> setModel( &m->  m_env1Dec );
	m_env1SusKnob-> setModel( &m->  m_env1Sus );
	m_env1RelKnob-> setModel( &m->  m_env1Rel  );
	m_env1SlopeKnob-> setModel( &m->  m_env1Slope );
	
	m_env2PreKnob-> setModel( &m->  m_env2Pre );
	m_env2AttKnob-> setModel( &m->  m_env2Att );
	m_env2HoldKnob-> setModel( &m->  m_env2Hold );
	m_env2DecKnob-> setModel( &m->  m_env2Dec );
	m_env2SusKnob-> setModel( &m->  m_env2Sus );
	m_env2RelKnob-> setModel( &m->  m_env2Rel  );
	m_env2SlopeKnob-> setModel( &m->  m_env2Slope );
	
	m_o23ModGroup-> setModel( &m-> m_o23Mod );
	m_selectedViewGroup-> setModel( &m-> m_selectedView );
	
	m_vol1env1Knob-> setModel( &m-> m_vol1env1 );
	m_vol1env2Knob-> setModel( &m-> m_vol1env2 );
	m_vol1lfo1Knob-> setModel( &m-> m_vol1lfo1 );
	m_vol1lfo2Knob-> setModel( &m-> m_vol1lfo2 );

	m_vol2env1Knob-> setModel( &m-> m_vol2env1 );
	m_vol2env2Knob-> setModel( &m-> m_vol2env2 );
	m_vol2lfo1Knob-> setModel( &m-> m_vol2lfo1 );
	m_vol2lfo2Knob-> setModel( &m-> m_vol2lfo2 );

	m_vol3env1Knob-> setModel( &m-> m_vol3env1 );
	m_vol3env2Knob-> setModel( &m-> m_vol3env2 );
	m_vol3lfo1Knob-> setModel( &m-> m_vol3lfo1 );
	m_vol3lfo2Knob-> setModel( &m-> m_vol3lfo2 );

 	m_phs1env1Knob-> setModel( &m-> m_phs1env1 );
	m_phs1env2Knob-> setModel( &m-> m_phs1env2 );
	m_phs1lfo1Knob-> setModel( &m-> m_phs1lfo1 );
	m_phs1lfo2Knob-> setModel( &m-> m_phs1lfo2 );

	m_phs2env1Knob-> setModel( &m-> m_phs2env1 );
	m_phs2env2Knob-> setModel( &m-> m_phs2env2 );
	m_phs2lfo1Knob-> setModel( &m-> m_phs2lfo1 );
	m_phs2lfo2Knob-> setModel( &m-> m_phs2lfo2 );

	m_phs3env1Knob-> setModel( &m-> m_phs3env1 );
	m_phs3env2Knob-> setModel( &m-> m_phs3env2 );
	m_phs3lfo1Knob-> setModel( &m-> m_phs3lfo1 );
	m_phs3lfo2Knob-> setModel( &m-> m_phs3lfo2 );

	m_pit1env1Knob-> setModel( &m-> m_pit1env1 );
	m_pit1env2Knob-> setModel( &m-> m_pit1env2 );
	m_pit1lfo1Knob-> setModel( &m-> m_pit1lfo1 );
	m_pit1lfo2Knob-> setModel( &m-> m_pit1lfo2 );

	m_pit2env1Knob-> setModel( &m-> m_pit2env1 );
	m_pit2env2Knob-> setModel( &m-> m_pit2env2 );
	m_pit2lfo1Knob-> setModel( &m-> m_pit2lfo1 );
	m_pit2lfo2Knob-> setModel( &m-> m_pit2lfo2 );

	m_pit3env1Knob-> setModel( &m-> m_pit3env1 );
	m_pit3env2Knob-> setModel( &m-> m_pit3env2 );
	m_pit3lfo1Knob-> setModel( &m-> m_pit3lfo1 );
	m_pit3lfo2Knob-> setModel( &m-> m_pit3lfo2 );

	m_pw1env1Knob-> setModel( &m-> m_pw1env1 );
	m_pw1env2Knob-> setModel( &m-> m_pw1env2 );
	m_pw1lfo1Knob-> setModel( &m-> m_pw1lfo1 );
	m_pw1lfo2Knob-> setModel( &m-> m_pw1lfo2 );

	m_sub3env1Knob-> setModel( &m-> m_sub3env1 );
	m_sub3env2Knob-> setModel( &m-> m_sub3env2 );
	m_sub3lfo1Knob-> setModel( &m-> m_sub3lfo1 );
	m_sub3lfo2Knob-> setModel( &m->	m_sub3lfo2 );
	
}


void MonstroView::setWidgetBackground( QWidget * _widget, const QString & _pic )
{
	_widget->setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( _widget->backgroundRole(),
		PLUGIN_NAME::getIconPixmap( _pic.toAscii().constData() ) );
	_widget->setPalette( pal );
}


QWidget * MonstroView::setupOperatorsView( QWidget * _parent )
{
	// operators view
	
	QWidget * view = new QWidget( _parent );
	view-> setFixedSize( 250, 250 );
	
	makeknob( m_osc1VolKnob, KNOBCOL1, O1ROW, "Volume", "%", "osc1Knob" )
	makeknob( m_osc1PanKnob, KNOBCOL2, O1ROW, "Panning", "", "osc1Knob" )
	makeknob( m_osc1CrsKnob, KNOBCOL3, O1ROW, "Coarse detune", " seminotes", "osc1Knob" )
	makeknob( m_osc1FtlKnob, KNOBCOL4, O1ROW, "Finetune left", " cents", "osc1Knob" )
	makeknob( m_osc1FtrKnob, KNOBCOL5, O1ROW, "Finetune right", " cents", "osc1Knob" )
	makeknob( m_osc1SpoKnob, KNOBCOL6, O1ROW, "Stereo phase offset", " deg", "osc1Knob" )
	makeknob( m_osc1PwKnob,  KNOBCOL7, O1ROW, "Pulse width", "%", "osc1Knob" )

	m_osc1VolKnob -> setVolumeKnob( true );

	makeknob( m_osc2VolKnob, KNOBCOL1, O2ROW, "Volume", "%", "osc2Knob" )
	makeknob( m_osc2PanKnob, KNOBCOL2, O2ROW, "Panning", "", "osc2Knob" )
	makeknob( m_osc2CrsKnob, KNOBCOL3, O2ROW, "Coarse detune", " seminotes", "osc2Knob" )
	makeknob( m_osc2FtlKnob, KNOBCOL4, O2ROW, "Finetune left", " cents", "osc2Knob" )
	makeknob( m_osc2FtrKnob, KNOBCOL5, O2ROW, "Finetune right", " cents", "osc2Knob" )
	makeknob( m_osc2SpoKnob, KNOBCOL6, O2ROW, "Stereo phase offset", " deg", "osc2Knob" )
	
	m_osc2VolKnob -> setVolumeKnob( true );
	
	m_osc2WaveBox = new comboBox( view );
	m_osc2WaveBox -> setGeometry( KNOBCOL7, O2ROW + 4, 42, 22 );
	m_osc2WaveBox->setFont( pointSize<8>( m_osc2WaveBox->font() ) );
	
	makeknob( m_osc3VolKnob, KNOBCOL1, O3ROW, "Volume", "%", "osc3Knob" )
	makeknob( m_osc3PanKnob, KNOBCOL2, O3ROW, "Panning", "", "osc3Knob" )
	makeknob( m_osc3CrsKnob, KNOBCOL3, O3ROW, "Coarse detune", " seminotes", "osc3Knob" )
	makeknob( m_osc3SpoKnob, KNOBCOL4, O3ROW, "Stereo phase offset", " deg", "osc3Knob" )
	makeknob( m_osc3SubKnob, KNOBCOL5, O3ROW, "Sub-osc mix", "", "osc3Knob" )

	m_osc3Wave1Box = new comboBox( view );
	m_osc3Wave1Box -> setGeometry( 160, O3ROW + 4, 42, 22 );
	m_osc3Wave1Box->setFont( pointSize<8>( m_osc3Wave1Box->font() ) );

	m_osc3Wave2Box = new comboBox( view );
	m_osc3Wave2Box -> setGeometry( 204, O3ROW + 4, 42, 22 );
	m_osc3Wave2Box->setFont( pointSize<8>( m_osc3Wave2Box->font() ) );
	
	m_lfo1WaveBox = new comboBox( view );
	m_lfo1WaveBox -> setGeometry( 2, LFOROW + 7, 42, 22 );
	m_lfo1WaveBox->setFont( pointSize<8>( m_lfo1WaveBox->font() ) );	

	maketsknob( m_lfo1AttKnob, LFOCOL1, LFOROW, "Attack", " ms", "lfoKnob" )
	maketsknob( m_lfo1RateKnob, LFOCOL2, LFOROW, "Rate", " ms", "lfoKnob" )
	makeknob( m_lfo1PhsKnob, LFOCOL3, LFOROW, "Phase", " deg", "lfoKnob" )
	
	m_lfo2WaveBox = new comboBox( view );
	m_lfo2WaveBox -> setGeometry( 127, LFOROW + 7, 42, 22 );
	m_lfo2WaveBox->setFont( pointSize<8>( m_lfo2WaveBox->font() ) );	

	maketsknob( m_lfo2AttKnob, LFOCOL4, LFOROW, "Attack", " ms", "lfoKnob" )
	maketsknob( m_lfo2RateKnob, LFOCOL5, LFOROW, "Rate", " ms", "lfoKnob" )
	makeknob( m_lfo2PhsKnob, LFOCOL6, LFOROW, "Phase", " deg", "lfoKnob" )
	
	maketsknob( m_env1PreKnob, KNOBCOL1, E1ROW, "Pre-delay", " ms", "envKnob" )
	maketsknob( m_env1AttKnob, KNOBCOL2, E1ROW, "Attack", " ms", "envKnob" )
	maketsknob( m_env1HoldKnob, KNOBCOL3, E1ROW, "Hold", " ms", "envKnob" )
	maketsknob( m_env1DecKnob, KNOBCOL4, E1ROW, "Decay", " ms", "envKnob" )
	makeknob( m_env1SusKnob, KNOBCOL5, E1ROW, "Sustain", "", "envKnob" )
	maketsknob( m_env1RelKnob, KNOBCOL6, E1ROW, "Release", " ms", "envKnob" )
	makeknob( m_env1SlopeKnob, KNOBCOL7, E1ROW, "Slope", "", "envKnob" )

	maketsknob( m_env2PreKnob, KNOBCOL1, E2ROW, "Pre-delay", " ms", "envKnob" )
	maketsknob( m_env2AttKnob, KNOBCOL2, E2ROW, "Attack", " ms", "envKnob" )
	maketsknob( m_env2HoldKnob, KNOBCOL3, E2ROW, "Hold", " ms", "envKnob" )
	maketsknob( m_env2DecKnob, KNOBCOL4, E2ROW, "Decay", " ms", "envKnob" )
	makeknob( m_env2SusKnob, KNOBCOL5, E2ROW, "Sustain", "", "envKnob" )
	maketsknob( m_env2RelKnob, KNOBCOL6, E2ROW, "Release", " ms", "envKnob" )
	makeknob( m_env2SlopeKnob, KNOBCOL7, E2ROW, "Slope", "", "envKnob" )

	// mod selector
	pixmapButton * m_mixButton = new pixmapButton( view, NULL );
	m_mixButton -> move( 225, 185 );
	m_mixButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "mix_active" ) );
	m_mixButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "mix_inactive" ) );
	toolTip::add( m_mixButton, tr( "Mix Osc2 with Osc3" ) );

	pixmapButton * m_amButton = new pixmapButton( view, NULL );
	m_amButton -> move( 225, 185 + 15 );
	m_amButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "am_active" ) );
	m_amButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "am_inactive" ) );
	toolTip::add( m_amButton, tr( "Modulate amplitude of Osc3 with Osc2" ) );
	
	pixmapButton * m_fmButton = new pixmapButton( view, NULL );
	m_fmButton -> move( 225, 185 + 15*2 );
	m_fmButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "fm_active" ) );
	m_fmButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "fm_inactive" ) );
	toolTip::add( m_fmButton, tr( "Modulate frequency of Osc3 with Osc2" ) );

	pixmapButton * m_pmButton = new pixmapButton( view, NULL );
	m_pmButton -> move( 225, 185 + 15*3 );
	m_pmButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "pm_active" ) );
	m_pmButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "pm_inactive" ) );
	toolTip::add( m_pmButton, tr( "Modulate phase of Osc3 with Osc2" ) );
	
	m_o23ModGroup = new automatableButtonGroup( view );
	m_o23ModGroup-> addButton( m_mixButton );
	m_o23ModGroup-> addButton( m_amButton );
	m_o23ModGroup-> addButton( m_fmButton );
	m_o23ModGroup-> addButton( m_pmButton );
	
	return( view );
}


QWidget * MonstroView::setupMatrixView( QWidget * _parent )
{
	// matrix view
	
	QWidget * view = new QWidget( _parent );
	view-> setFixedSize( 250, 250 );
	
	makeknob( m_vol1env1Knob, MATCOL1, MATROW1, "Modulation amount", "", "matrixKnob" )
	makeknob( m_vol1env2Knob, MATCOL2, MATROW1, "Modulation amount", "", "matrixKnob" )
	makeknob( m_vol1lfo1Knob, MATCOL3, MATROW1, "Modulation amount", "", "matrixKnob" )
	makeknob( m_vol1lfo2Knob, MATCOL4, MATROW1, "Modulation amount", "", "matrixKnob" )

	makeknob( m_vol2env1Knob, MATCOL1, MATROW3, "Modulation amount", "", "matrixKnob" )
	makeknob( m_vol2env2Knob, MATCOL2, MATROW3, "Modulation amount", "", "matrixKnob" )
	makeknob( m_vol2lfo1Knob, MATCOL3, MATROW3, "Modulation amount", "", "matrixKnob" )
	makeknob( m_vol2lfo2Knob, MATCOL4, MATROW3, "Modulation amount", "", "matrixKnob" )

	makeknob( m_vol3env1Knob, MATCOL1, MATROW5, "Modulation amount", "", "matrixKnob" )
	makeknob( m_vol3env2Knob, MATCOL2, MATROW5, "Modulation amount", "", "matrixKnob" )
	makeknob( m_vol3lfo1Knob, MATCOL3, MATROW5, "Modulation amount", "", "matrixKnob" )
	makeknob( m_vol3lfo2Knob, MATCOL4, MATROW5, "Modulation amount", "", "matrixKnob" )

	makeknob( m_phs1env1Knob, MATCOL1, MATROW2, "Modulation amount", "", "matrixKnob" )
	makeknob( m_phs1env2Knob, MATCOL2, MATROW2, "Modulation amount", "", "matrixKnob" )
	makeknob( m_phs1lfo1Knob, MATCOL3, MATROW2, "Modulation amount", "", "matrixKnob" )
	makeknob( m_phs1lfo2Knob, MATCOL4, MATROW2, "Modulation amount", "", "matrixKnob" )

	makeknob( m_phs2env1Knob, MATCOL1, MATROW4, "Modulation amount", "", "matrixKnob" )
	makeknob( m_phs2env2Knob, MATCOL2, MATROW4, "Modulation amount", "", "matrixKnob" )
	makeknob( m_phs2lfo1Knob, MATCOL3, MATROW4, "Modulation amount", "", "matrixKnob" )
	makeknob( m_phs2lfo2Knob, MATCOL4, MATROW4, "Modulation amount", "", "matrixKnob" )

	makeknob( m_phs3env1Knob, MATCOL1, MATROW6, "Modulation amount", "", "matrixKnob" )
	makeknob( m_phs3env2Knob, MATCOL2, MATROW6, "Modulation amount", "", "matrixKnob" )
	makeknob( m_phs3lfo1Knob, MATCOL3, MATROW6, "Modulation amount", "", "matrixKnob" )
	makeknob( m_phs3lfo2Knob, MATCOL4, MATROW6, "Modulation amount", "", "matrixKnob" )

	makeknob( m_pit1env1Knob, MATCOL5, MATROW1, "Modulation amount", "", "matrixKnob" )
	makeknob( m_pit1env2Knob, MATCOL6, MATROW1, "Modulation amount", "", "matrixKnob" )
	makeknob( m_pit1lfo1Knob, MATCOL7, MATROW1, "Modulation amount", "", "matrixKnob" )
	makeknob( m_pit1lfo2Knob, MATCOL8, MATROW1, "Modulation amount", "", "matrixKnob" )

	makeknob( m_pit2env1Knob, MATCOL5, MATROW3, "Modulation amount", "", "matrixKnob" )
	makeknob( m_pit2env2Knob, MATCOL6, MATROW3, "Modulation amount", "", "matrixKnob" )
	makeknob( m_pit2lfo1Knob, MATCOL7, MATROW3, "Modulation amount", "", "matrixKnob" )
	makeknob( m_pit2lfo2Knob, MATCOL8, MATROW3, "Modulation amount", "", "matrixKnob" )

	makeknob( m_pit3env1Knob, MATCOL5, MATROW5, "Modulation amount", "", "matrixKnob" )
	makeknob( m_pit3env2Knob, MATCOL6, MATROW5, "Modulation amount", "", "matrixKnob" )
	makeknob( m_pit3lfo1Knob, MATCOL7, MATROW5, "Modulation amount", "", "matrixKnob" )
	makeknob( m_pit3lfo2Knob, MATCOL8, MATROW5, "Modulation amount", "", "matrixKnob" )

	makeknob( m_pw1env1Knob, MATCOL5, MATROW2, "Modulation amount", "", "matrixKnob" )
	makeknob( m_pw1env2Knob, MATCOL6, MATROW2, "Modulation amount", "", "matrixKnob" )
	makeknob( m_pw1lfo1Knob, MATCOL7, MATROW2, "Modulation amount", "", "matrixKnob" )
	makeknob( m_pw1lfo2Knob, MATCOL8, MATROW2, "Modulation amount", "", "matrixKnob" )

	makeknob( m_sub3env1Knob, MATCOL5, MATROW6, "Modulation amount", "", "matrixKnob" )
	makeknob( m_sub3env2Knob, MATCOL6, MATROW6, "Modulation amount", "", "matrixKnob" )
	makeknob( m_sub3lfo1Knob, MATCOL7, MATROW6, "Modulation amount", "", "matrixKnob" )
	makeknob( m_sub3lfo2Knob, MATCOL8, MATROW6, "Modulation amount", "", "matrixKnob" )
	
	return( view );
}

extern "C"
{

// necessary for getting instance out of shared lib
Plugin * PLUGIN_EXPORT lmms_plugin_main( Model *, void * _data )
{
	return new MonstroInstrument( static_cast<InstrumentTrack *>( _data ) );
}


}



#include "moc_Monstro.cxx"
