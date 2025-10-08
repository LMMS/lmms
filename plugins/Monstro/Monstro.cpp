/*
 * Monstro.cpp - a monstrous semi-modular 3-osc synth with modulation matrix
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

#include "Monstro.h"


#include "ComboBox.h"
#include "Engine.h"
#include "InstrumentTrack.h"
#include "lmms_math.h"
#include "interpolation.h"

#include "embed.h"
#include "plugin_export.h"

namespace lmms
{


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT monstro_plugin_descriptor =
{
	LMMS_STRINGIFY( PLUGIN_NAME ),
	"Monstro",
	QT_TRANSLATE_NOOP( "PluginBrowser",
				"Monstrous 3-oscillator synth with modulation matrix" ),
	"Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>",
	0x0100,
	Plugin::Type::Instrument,
	new PluginPixmapLoader( "logo" ),
	{},
	nullptr,
} ;

}




MonstroSynth::MonstroSynth( MonstroInstrument * _i, NotePlayHandle * _nph ) :
					m_parent( _i ),
					m_nph( _nph )
{
	m_osc1l_phase = 0.0f;
	m_osc1r_phase = 0.0f;
	m_osc2l_phase = 0.0f;
	m_osc2r_phase = 0.0f;
	m_osc3l_phase = 0.0f;
	m_osc3r_phase = 0.0f;

	m_ph2l_last = 0.0f;
	m_ph2r_last = 0.0f;
	m_ph3l_last = 0.0f;
	m_ph3r_last = 0.0f;

	m_env_phase[0] = 0.0f;
	m_env_phase[1] = 0.0f;
	m_lfo_phase[0] = 0.0f;
	m_lfo_phase[1] = 0.0f;

	m_lfo_next[0] = Oscillator::noiseSample( 0.0f );
	m_lfo_next[1] = Oscillator::noiseSample( 0.0f );

	m_osc1l_last = 0.0f;
	m_osc1r_last = 0.0f;

	m_l_last = 0.0f;
	m_r_last = 0.0f;

	m_invert2l = false;
	m_invert2r = false;
	m_invert3l = false;
	m_invert3r = false;

	m_counter2l = 0;
	m_counter2r = 0;
	m_counter3l = 0;
	m_counter3r = 0;

	m_lfo[0].resize( m_parent->m_fpp );
	m_lfo[1].resize( m_parent->m_fpp );
	m_env[0].resize( m_parent->m_fpp );
	m_env[1].resize( m_parent->m_fpp );
}


void MonstroSynth::renderOutput( fpp_t _frames, SampleFrame* _buf  )
{
	float modtmp; // temp variable for freq modulation
// macros for modulating with env/lfos
#define modulatefreq( car, mod ) \
		modtmp = 0.0f; \
		if( mod##_e1 != 0.0f ) modtmp += m_env[0][f] * mod##_e1; \
		if( mod##_e2 != 0.0f ) modtmp += m_env[1][f] * mod##_e2; \
		if( mod##_l1 != 0.0f ) modtmp += m_lfo[0][f] * mod##_l1; \
		if( mod##_l2 != 0.0f ) modtmp += m_lfo[1][f] * mod##_l2; \
		(car) = qBound( MIN_FREQ, (car) * std::exp2(modtmp), MAX_FREQ);

#define modulateabs( car, mod ) \
		if( mod##_e1 != 0.0f ) car += m_env[0][f] * mod##_e1; \
		if( mod##_e2 != 0.0f ) car += m_env[1][f] * mod##_e2; \
		if( mod##_l1 != 0.0f ) car += m_lfo[0][f] * mod##_l1; \
		if( mod##_l2 != 0.0f ) car += m_lfo[1][f] * mod##_l2;

#define modulatephs( car, mod ) \
		if( mod##_e1 != 0.0f ) car += m_env[0][f] * mod##_e1; \
		if( mod##_e2 != 0.0f ) car += m_env[1][f] * mod##_e2; \
		if( mod##_l1 != 0.0f ) car += m_lfo[0][f] * mod##_l1; \
		if( mod##_l2 != 0.0f ) car += m_lfo[1][f] * mod##_l2;

#define modulatevol( car, mod ) \
		if( mod##_e1 > 0.0f ) car *= ( 1.0f - mod##_e1 + mod##_e1 * m_env[0][f] ); \
		if( mod##_e1 < 0.0f ) car *= ( 1.0f + mod##_e1 * m_env[0][f] );	\
		if( mod##_e2 > 0.0f ) car *= ( 1.0f - mod##_e2 + mod##_e2 * m_env[1][f] );	\
		if( mod##_e2 < 0.0f ) car *= ( 1.0f + mod##_e2 * m_env[1][f] );	\
		if( mod##_l1 != 0.0f ) car *= ( 1.0f + mod##_l1 * m_lfo[0][f] ); \
		if( mod##_l2 != 0.0f ) car *= ( 1.0f + mod##_l2 * m_lfo[1][f] ); \
		car = qBound( -MODCLIP, car, MODCLIP );



	////////////////////
	//                //
	//   MODULATORS   //
	//                //
	////////////////////

	// LFO phase offsets
	const float lfo1_po = m_parent->m_lfo1Phs.value() / 360.0f;
	const float lfo2_po = m_parent->m_lfo2Phs.value() / 360.0f;

	// remove cruft from phase counters to prevent overflow, add phase offset
	m_lfo_phase[0] = absFraction( m_lfo_phase[0] + lfo1_po );
	m_lfo_phase[1] = absFraction( m_lfo_phase[1] + lfo2_po );

	// LFO rates and increment
	m_lfo_rate[0] = ( m_parent->m_lfo1Rate.value() * 0.001f * m_parent->m_samplerate );
	m_lfo_rate[1] = ( m_parent->m_lfo2Rate.value() * 0.001f * m_parent->m_samplerate );
	m_lfo_inc[0] = 1.0f / m_lfo_rate[0];
	m_lfo_inc[1] = 1.0f / m_lfo_rate[1];

	m_env_sus[0] = m_parent-> m_env1Sus.value();
	m_env_sus[1] = m_parent-> m_env2Sus.value();

	m_lfovalue[0] = m_parent->m_lfo1Wave.value();
	m_lfovalue[1] = m_parent->m_lfo2Wave.value();
	m_lfoatt[0] = m_parent->m_lfo1_att;
	m_lfoatt[1] = m_parent->m_lfo2_att;

	m_env_pre[0] = m_parent->m_env1_pre;
	m_env_att[0] = m_parent->m_env1_att;
	m_env_hold[0] = m_parent->m_env1_hold;
	m_env_dec[0] = m_parent->m_env1_dec;
	m_env_rel[0] = m_parent->m_env1_rel;
	m_env_pre[1] = m_parent->m_env2_pre;
	m_env_att[1] = m_parent->m_env2_att;
	m_env_hold[1] = m_parent->m_env2_hold;
	m_env_dec[1] = m_parent->m_env2_dec;
	m_env_rel[1] = m_parent->m_env2_rel;


	// get updated osc1 values
	// get pulse width
	const float pw = ( m_parent->m_osc1Pw.value() * 0.01f );
	const float o1pw_e1 = ( m_parent->m_pw1env1.value() );
	const float o1pw_e2 = ( m_parent->m_pw1env2.value() );
	const float o1pw_l1 = ( m_parent->m_pw1lfo1.value() * 0.5f );
	const float o1pw_l2 = ( m_parent->m_pw1lfo2.value() * 0.5f );
	const bool o1pw_mod = o1pw_e1 != 0.0f || o1pw_e2 != 0.0f || o1pw_l1 != 0.0f || o1pw_l2 != 0.0f;

	// get phases
	const float o1lpo = m_parent->m_osc1l_po;
	const float o1rpo = m_parent->m_osc1r_po;
	const float o1p_e1 = ( m_parent->m_phs1env1.value() );
	const float o1p_e2 = ( m_parent->m_phs1env2.value() );
	const float o1p_l1 = ( m_parent->m_phs1lfo1.value() * 0.5f );
	const float o1p_l2 = ( m_parent->m_phs1lfo2.value() * 0.5f );
	const bool o1p_mod = o1p_e1 != 0.0f || o1p_e2 != 0.0f || o1p_l1 != 0.0f || o1p_l2 != 0.0f;

	// get pitch
	const float o1lfb = ( m_parent->m_osc1l_freq * m_nph->frequency() );
	const float o1rfb = ( m_parent->m_osc1r_freq * m_nph->frequency() );
	const float o1f_e1 = ( m_parent->m_pit1env1.value() * 2.0f );
	const float o1f_e2 = ( m_parent->m_pit1env2.value() * 2.0f );
	const float o1f_l1 = ( m_parent->m_pit1lfo1.value() );
	const float o1f_l2 = ( m_parent->m_pit1lfo2.value() );
	const bool o1f_mod = o1f_e1 != 0.0f || o1f_e2 != 0.0f || o1f_l1 != 0.0f || o1f_l2 != 0.0f;

	// get volumes
	const float o1lv = m_parent->m_osc1l_vol;
	const float o1rv = m_parent->m_osc1r_vol;
	const float o1v_e1 = ( m_parent->m_vol1env1.value() );
	const float o1v_e2 = ( m_parent->m_vol1env2.value() );
	const float o1v_l1 = ( m_parent->m_vol1lfo1.value() );
	const float o1v_l2 = ( m_parent->m_vol1lfo2.value() );
	const bool o1v_mod = o1v_e1 != 0.0f || o1v_e2 != 0.0f || o1v_l1 != 0.0f || o1v_l2 != 0.0f;

	// update osc2
	// get waveform
	const int o2w = m_parent->m_osc2Wave.value();

	// get phases
	const float o2lpo = m_parent->m_osc2l_po;
	const float o2rpo = m_parent->m_osc2r_po;
	const float o2p_e1 = ( m_parent->m_phs2env1.value() );
	const float o2p_e2 = ( m_parent->m_phs2env2.value() );
	const float o2p_l1 = ( m_parent->m_phs2lfo1.value() * 0.5f );
	const float o2p_l2 = ( m_parent->m_phs2lfo2.value() * 0.5f );
	const bool o2p_mod = o2p_e1 != 0.0f || o2p_e2 != 0.0f || o2p_l1 != 0.0f || o2p_l2 != 0.0f;

	// get pitch
	const float o2lfb = ( m_parent->m_osc2l_freq * m_nph->frequency() );
	const float o2rfb = ( m_parent->m_osc2r_freq * m_nph->frequency() );
	const float o2f_e1 = ( m_parent->m_pit2env1.value() * 2.0f );
	const float o2f_e2 = ( m_parent->m_pit2env2.value() * 2.0f );
	const float o2f_l1 = ( m_parent->m_pit2lfo1.value() );
	const float o2f_l2 = ( m_parent->m_pit2lfo2.value() );
	const bool o2f_mod = o2f_e1 != 0.0f || o2f_e2 != 0.0f || o2f_l1 != 0.0f || o2f_l2 != 0.0f;

	// get volumes
	const float o2lv = m_parent->m_osc2l_vol;
	const float o2rv = m_parent->m_osc2r_vol;
	const float o2v_e1 = ( m_parent->m_vol2env1.value() );
	const float o2v_e2 = ( m_parent->m_vol2env2.value() );
	const float o2v_l1 = ( m_parent->m_vol2lfo1.value() );
	const float o2v_l2 = ( m_parent->m_vol2lfo2.value() );
	const bool o2v_mod = o2v_e1 != 0.0f || o2v_e2 != 0.0f || o2v_l1 != 0.0f || o2v_l2 != 0.0f;


	// update osc3
	// get waveforms
	const int o3w1 = m_parent->m_osc3Wave1.value();
	const int o3w2 = m_parent->m_osc3Wave2.value();

	// get phases
	const float o3lpo = m_parent->m_osc3l_po;
	const float o3rpo = m_parent->m_osc3r_po;
	const float o3p_e1 = ( m_parent->m_phs3env1.value() );
	const float o3p_e2 = ( m_parent->m_phs3env2.value() );
	const float o3p_l1 = ( m_parent->m_phs3lfo1.value() * 0.5f );
	const float o3p_l2 = ( m_parent->m_phs3lfo2.value() * 0.5f );
	const bool o3p_mod = o3p_e1 != 0.0f || o3p_e2 != 0.0f || o3p_l1 != 0.0f || o3p_l2 != 0.0f;

	// get pitch modulators
	const float o3fb = ( m_parent->m_osc3_freq * m_nph->frequency() );
	const float o3f_e1 = ( m_parent->m_pit3env1.value() * 2.0f );
	const float o3f_e2 = ( m_parent->m_pit3env2.value() * 2.0f );
	const float o3f_l1 = ( m_parent->m_pit3lfo1.value() );
	const float o3f_l2 = ( m_parent->m_pit3lfo2.value() );
	const bool o3f_mod = o3f_e1 != 0.0f || o3f_e2 != 0.0f || o3f_l1 != 0.0f || o3f_l2 != 0.0f;

	// get volumes
	const float o3lv = m_parent->m_osc3l_vol;
	const float o3rv = m_parent->m_osc3r_vol;
	const float o3v_e1 = ( m_parent->m_vol3env1.value() );
	const float o3v_e2 = ( m_parent->m_vol3env2.value() );
	const float o3v_l1 = ( m_parent->m_vol3lfo1.value() );
	const float o3v_l2 = ( m_parent->m_vol3lfo2.value() );
	const bool o3v_mod = o3v_e1 != 0.0f || o3v_e2 != 0.0f || o3v_l1 != 0.0f || o3v_l2 != 0.0f;

	// get sub
	const float o3sub = ( m_parent->m_osc3Sub.value() + 100.0f ) / 200.0f;
	const float o3s_e1 = ( m_parent->m_sub3env1.value() );
	const float o3s_e2 = ( m_parent->m_sub3env2.value() );
	const float o3s_l1 = ( m_parent->m_sub3lfo1.value() * 0.5f );
	const float o3s_l2 = ( m_parent->m_sub3lfo2.value() * 0.5f );
	const bool o3s_mod = o3s_e1 != 0.0f || o3s_e2 != 0.0f || o3s_l1 != 0.0f || o3s_l2 != 0.0f;


	//o2-o3 modulation

	const int omod = m_parent->m_o23Mod.value();

	// sync information

	const bool o1ssr = m_parent->m_osc1SSR.value();
	const bool o1ssf = m_parent->m_osc1SSF.value();
	const bool o2sync = m_parent->m_osc2SyncH.value();
	const bool o3sync = m_parent->m_osc3SyncH.value();
	const bool o2syncr = m_parent->m_osc2SyncR.value();
	const bool o3syncr = m_parent->m_osc3SyncR.value();

	///////////////////////////
	//                       //
	// 	 start buffer loop   //
	//                       //
	///////////////////////////

	// declare working variables for for loop

	// phase manipulation vars - these can be reused by all oscs
	float leftph;
	float rightph;
	float pd_l;
	float pd_r;
	float len_l(0.);
	float len_r(0.);

	// osc1 vars
	float o1l_f;
	float o1r_f;
	float o1l_p = m_osc1l_phase + o1lpo; // we add phase offset here so we don't have to do it every frame
	float o1r_p = m_osc1r_phase + o1rpo; // then subtract it again after loop...
	float o1_pw;

	// osc2 vars
	float o2l_f;
	float o2r_f;
	float o2l_p = m_osc2l_phase + o2lpo;
	float o2r_p = m_osc2r_phase + o2rpo;

	// osc3 vars
	float o3l_f;
	float o3r_f;
	float o3l_p = m_osc3l_phase + o3lpo;
	float o3r_p = m_osc3r_phase + o3rpo;
	float sub;

	// render modulators: envelopes, lfos
	updateModulators( m_env[0].data(), m_env[1].data(), m_lfo[0].data(), m_lfo[1].data(), _frames );

	// begin for loop
	for( f_cnt_t f = 0; f < _frames; ++f )
	{
/*	// debug code
		if( f % 10 == 0 ) {
			qDebug( "env1 %f -- env1 phase %f", m_env1_buf[f], m_env1_phase );
			qDebug( "env1 pre %f att %f dec %f rel %f ", m_parent->m_env1_pre, m_parent->m_env1_att,
				m_parent->m_env1_dec, m_parent->m_env1_rel );
		}*/


		/////////////////////////////
		//				           //
		//          OSC 1          //
		//				           //
		/////////////////////////////

		// calc and mod frequencies
		o1l_f = o1lfb;
		o1r_f = o1rfb;
		if( o1f_mod )
		{
			modulatefreq( o1l_f, o1f )
			modulatefreq( o1r_f, o1f )
		}
		// calc and modulate pulse
		o1_pw = pw;
		if( o1pw_mod )
		{
			modulateabs( o1_pw, o1pw )
			o1_pw = qBound( PW_MIN, o1_pw, PW_MAX );
		}

		// calc and modulate phase
		leftph = o1l_p;
		rightph = o1r_p;
		if( o1p_mod )
		{
			modulatephs( leftph, o1p )
			modulatephs( rightph, o1p )
		}

		// pulse wave osc
		sample_t O1L = ( absFraction( leftph ) < o1_pw ) ? 1.0f : -1.0f;
		sample_t O1R = ( absFraction( rightph ) < o1_pw ) ? 1.0f : -1.0f;

		// check for rise/fall, and sync if appropriate
		// sync on rise
		if( o1ssr )
		{
			// hard sync
			if( o2sync )
			{
				if( O1L > m_osc1l_last ) { o2l_p = o2lpo; m_counter2l = m_parent->m_counterMax; }
				if( O1R > m_osc1r_last ) { o2r_p = o2rpo; m_counter2r = m_parent->m_counterMax; }
			}
			if( o3sync )
			{
				if( O1L > m_osc1l_last ) { o3l_p = o3lpo; m_counter3l = m_parent->m_counterMax; }
				if( O1R > m_osc1r_last ) { o3r_p = o3rpo; m_counter3r = m_parent->m_counterMax; }
			}
			// reverse sync
			if( o2syncr )
			{
				if( O1L > m_osc1l_last ) { m_invert2l = !m_invert2l; m_counter2l = m_parent->m_counterMax; }
				if( O1R > m_osc1r_last ) { m_invert2r = !m_invert2r; m_counter2r = m_parent->m_counterMax; }
			}
			if( o3syncr )
			{
				if( O1L > m_osc1l_last ) { m_invert3l = !m_invert3l; m_counter3l = m_parent->m_counterMax; }
				if( O1R > m_osc1r_last ) { m_invert3r = !m_invert3r; m_counter3r = m_parent->m_counterMax; }
			}
		}
		// sync on fall
		if( o1ssf )
		{
			// hard sync
			if( o2sync )
			{
				if( O1L < m_osc1l_last ) { o2l_p = o2lpo; m_counter2l = m_parent->m_counterMax; }
				if( O1R < m_osc1r_last ) { o2r_p = o2rpo; m_counter2r = m_parent->m_counterMax; }
			}
			if( o3sync )
			{
				if( O1L < m_osc1l_last ) { o3l_p = o3lpo; m_counter3l = m_parent->m_counterMax; }
				if( O1R < m_osc1r_last ) { o3r_p = o3rpo; m_counter3r = m_parent->m_counterMax; }
			}
			// reverse sync
			if( o2syncr )
			{
				if( O1L < m_osc1l_last ) { m_invert2l = !m_invert2l; m_counter2l = m_parent->m_counterMax; }
				if( O1R < m_osc1r_last ) { m_invert2r = !m_invert2r; m_counter2r = m_parent->m_counterMax; }
			}
			if( o3syncr )
			{
				if( O1L < m_osc1l_last ) { m_invert3l = !m_invert3l; m_counter3l = m_parent->m_counterMax; }
				if( O1R < m_osc1r_last ) { m_invert3r = !m_invert3r; m_counter3r = m_parent->m_counterMax; }
			}
		}

		// update last before signal is touched
		// also do a very simple amp delta cap
		const sample_t tmpl = m_osc1l_last;
		const sample_t tmpr = m_osc1r_last;

		m_osc1l_last = O1L;
		m_osc1r_last = O1R;

		if( tmpl != O1L ) O1L = 0.0f;
		if( tmpr != O1R ) O1R = 0.0f;

		// modulate volume
		O1L *= o1lv;
		O1R *= o1rv;
		if( o1v_mod )
		{
			modulatevol( O1L, o1v )
			modulatevol( O1R, o1v )
		}

		// update osc1 phase working variable
		o1l_p += 1.0f / ( static_cast<float>( m_parent->m_samplerate ) / o1l_f );
		o1r_p += 1.0f / ( static_cast<float>( m_parent->m_samplerate ) / o1r_f );

		/////////////////////////////
		//				           //
		//          OSC 2          //
		//				           //
		/////////////////////////////

		// calc and mod frequencies
		o2l_f = o2lfb;
		o2r_f = o2rfb;
		if( o2f_mod )
		{
			modulatefreq( o2l_f, o2f )
			modulatefreq( o2r_f, o2f )
		}

		// calc and modulate phase
		leftph = o2l_p;
		rightph = o2r_p;
		if( o2p_mod )
		{
			modulatephs( leftph, o2p )
			modulatephs( rightph, o2p )
		}
		leftph = absFraction( leftph );
		rightph = absFraction( rightph );

		// phase delta
		pd_l = qAbs( leftph - m_ph2l_last );
		if( pd_l > 0.5 ) pd_l = 1.0 - pd_l;
		pd_r = qAbs( rightph - m_ph2r_last );
		if( pd_r > 0.5 ) pd_r = 1.0 - pd_r;

		// multi-wave DC Oscillator
		sample_t O2L = 0.;
		if (pd_l != 0.)
		{
			len_l = BandLimitedWave::pdToLen(pd_l);
			if (m_counter2l > 0)
			{
				len_l /= m_counter2l; m_counter2l--;
			}
			O2L = oscillate(o2w, leftph, len_l);
		}
		
		sample_t O2R = 0.;
		if (pd_r != 0.)
		{
			len_r = BandLimitedWave::pdToLen(pd_r);
			if (m_counter2r > 0)
			{
				len_r /= m_counter2r; m_counter2r--;
			}
			O2R = oscillate(o2w, rightph, len_r);
		}

		// modulate volume
		O2L *= o2lv;
		O2R *= o2rv;
		if( o2v_mod )
		{
			modulatevol( O2L, o2v )
			modulatevol( O2R, o2v )
		}

		// reverse sync - invert waveforms when needed
		if( m_invert2l ) O2L *= -1.0;
		if( m_invert2r ) O2R *= -1.0;

		// update osc2 phases
		m_ph2l_last = leftph;
		m_ph2r_last = rightph;
		o2l_p += 1.0f / ( static_cast<float>( m_parent->m_samplerate ) / o2l_f );
		o2r_p += 1.0f / ( static_cast<float>( m_parent->m_samplerate ) / o2r_f );

		/////////////////////////////
		//				           //
		//          OSC 3          //
		//				           //
		/////////////////////////////

		// calc and mod frequencies
		o3l_f = o3fb;
		o3r_f = o3fb;
		if( o3f_mod )
		{
			modulatefreq( o3l_f, o3f )
			modulatefreq( o3r_f, o3f )
		}
		// calc and modulate phase
		leftph = o3l_p;
		rightph = o3r_p;
		if( o3p_mod )
		{
			modulatephs( leftph, o3p )
			modulatephs( rightph, o3p )
		}

		// o2 modulation?
		if( omod == MOD_PM )
		{
			leftph += O2L * 0.5f;
			rightph += O2R * 0.5f;
		}
		leftph = absFraction( leftph );
		rightph = absFraction( rightph );

		// phase delta
		pd_l = qAbs( leftph - m_ph3l_last );
		if( pd_l > 0.5 ) pd_l = 1.0 - pd_l;
		pd_r = qAbs( rightph - m_ph3r_last );
		if( pd_r > 0.5 ) pd_r = 1.0 - pd_r;

		// multi-wave DC Oscillator
		sample_t O3AL = 0.;
		sample_t O3AR = 0.;

		// multi-wave DC Oscillator, sub-osc 2
		sample_t O3BL = 0.;
		sample_t O3BR = 0.;

		if (pd_l != 0.)
		{
			len_l = BandLimitedWave::pdToLen(pd_l);
			if (m_counter3l > 0)
			{
				len_l /= m_counter3l; m_counter3l--;
			}
			//  sub-osc 1
			O3AL = oscillate(o3w1, leftph, len_l);

			// multi-wave DC Oscillator, sub-osc 2
			O3BL = oscillate(o3w2, leftph, len_l);
		}

		if (pd_r != 0.)
		{
			len_r = BandLimitedWave::pdToLen(pd_r);
			if (m_counter3r > 0)
			{
				len_r /= m_counter3r; m_counter3r--;
			}
			//  sub-osc 1
			O3AR = oscillate(o3w1, rightph, len_r);

			// multi-wave DC Oscillator, sub-osc 2
			O3BR = oscillate(o3w2, rightph, len_r);
		}

		// calc and modulate sub
		sub = o3sub;
		if( o3s_mod )
		{
			modulateabs( sub, o3s )
			sub = qBound( 0.0f, sub, 1.0f );
		}

		sample_t O3L = std::lerp(O3AL, O3BL, sub);
		sample_t O3R = std::lerp(O3AR, O3BR, sub);

		// modulate volume
		O3L *= o3lv;
		O3R *= o3rv;
		if( o3v_mod )
		{
			modulatevol( O3L, o3v )
			modulatevol( O3R, o3v )
		}
		// o2 modulation?
		if( omod == MOD_AM )
		{
			O3L = qBound( -MODCLIP, O3L * qMax( 0.0f, 1.0f + O2L ), MODCLIP );
			O3R = qBound( -MODCLIP, O3R * qMax( 0.0f, 1.0f + O2R ), MODCLIP );
		}

		// reverse sync - invert waveforms when needed
		if( m_invert3l ) O3L *= -1.0;
		if( m_invert3r ) O3R *= -1.0;

		// update osc3 phases
		m_ph3l_last = leftph;
		m_ph3r_last = rightph;
		len_l = 1.0f / ( static_cast<float>( m_parent->m_samplerate ) / o3l_f );
		len_r = 1.0f / ( static_cast<float>( m_parent->m_samplerate ) / o3r_f );
		// handle FM as PM
		if( omod == MOD_FM )
		{
			len_l += O2L * m_parent->m_fmCorrection;
			len_r += O2R * m_parent->m_fmCorrection;
		}
		o3l_p += len_l;
		o3r_p += len_r;

		// integrator - very simple filter
		sample_t L = O1L + O3L + ( omod == MOD_MIX ? O2L : 0.0f );
		sample_t R = O1R + O3R + ( omod == MOD_MIX ? O2R : 0.0f );

		_buf[f][0] = std::lerp(L, m_l_last, m_parent->m_integrator);
		_buf[f][1] = std::lerp(R, m_r_last, m_parent->m_integrator);

		m_l_last = L;
		m_r_last = R;
	}

	// update phases
	m_osc1l_phase = absFraction( o1l_p - o1lpo );
	m_osc1r_phase = absFraction( o1r_p - o1rpo );
	m_osc2l_phase = absFraction( o2l_p - o2lpo );
	m_osc2r_phase = absFraction( o2r_p - o2rpo );
	m_osc3l_phase = absFraction( o3l_p - o3lpo );
	m_osc3r_phase = absFraction( o3r_p - o3rpo );

	m_lfo_phase[0] = absFraction( m_lfo_phase[0] - lfo1_po );
	m_lfo_phase[1] = absFraction( m_lfo_phase[1] - lfo2_po );
}


inline void MonstroSynth::updateModulators(float * env1, float * env2, float * lfo1, float * lfo2, f_cnt_t frames)
{
	// frames played before
	const f_cnt_t tfp = m_nph->totalFramesPlayed();

	auto lfo = std::array<float*, 2>{};
	auto env = std::array<float*, 2>{};
	lfo[0] = lfo1;
	lfo[1] = lfo2;
	env[0] = env1;
	env[1] = env2;

	for( int i = 0; i < 2; ++i )
	{
		switch( m_lfovalue[i] )
		{
			case WAVE_SINE:
				for( f_cnt_t f = 0; f < frames; ++f )
				{
					lfo[i][f] = Oscillator::sinSample( m_lfo_phase[i] );
					m_lfo_phase[i] += m_lfo_inc[i];
				}
			break;
			case WAVE_TRI:
				for( f_cnt_t f = 0; f < frames; ++f )
				{
					lfo[i][f] = Oscillator::triangleSample( m_lfo_phase[i] );
					m_lfo_phase[i] += m_lfo_inc[i];
				}
			break;
			case WAVE_SAW:
				for( f_cnt_t f = 0; f < frames; ++f )
				{
					lfo[i][f] = Oscillator::sawSample( m_lfo_phase[i] );
					m_lfo_phase[i] += m_lfo_inc[i];
				}
			break;
			case WAVE_RAMP:
				for( f_cnt_t f = 0; f < frames; ++f )
				{
					lfo[i][f] = Oscillator::sawSample( m_lfo_phase[i] ) * -1.0f;
					m_lfo_phase[i] += m_lfo_inc[i];
				}
			break;
			case WAVE_SQR:
				for( f_cnt_t f = 0; f < frames; ++f )
				{
					lfo[i][f] = Oscillator::squareSample( m_lfo_phase[i] );
					m_lfo_phase[i] += m_lfo_inc[i];
				}
			break;
			case WAVE_SQRSOFT:
				for( f_cnt_t f = 0; f < frames; ++f )
				{
					lfo[i][f] = oscillate( WAVE_SQRSOFT, m_lfo_phase[i], 0 );
					m_lfo_phase[i] += m_lfo_inc[i];
				}
			break;
			case WAVE_MOOG:
				for( f_cnt_t f = 0; f < frames; ++f )
				{
					lfo[i][f] = Oscillator::moogSawSample( m_lfo_phase[i] );
					m_lfo_phase[i] += m_lfo_inc[i];
				}
			break;
			case WAVE_SINABS:
				for( f_cnt_t f = 0; f < frames; ++f )
				{
					lfo[i][f] = oscillate( WAVE_SINABS, m_lfo_phase[i], 0 );
					m_lfo_phase[i] += m_lfo_inc[i];
				}
			break;
			case WAVE_EXP:
				for( f_cnt_t f = 0; f < frames; ++f )
				{
					lfo[i][f] = Oscillator::expSample( m_lfo_phase[i] );
					m_lfo_phase[i] += m_lfo_inc[i];
				}
			break;
			case WAVE_RANDOM:
				for( f_cnt_t f = 0; f < frames; ++f )
				{
					if( ( tfp + f ) % static_cast<int>( m_lfo_rate[i] ) == 0 ) m_lfo_last[i] = Oscillator::noiseSample( 0.0f );
					lfo[i][f] = m_lfo_last[i];
					m_lfo_phase[i] += m_lfo_inc[i];
				}
			break;
			case WAVE_RANDOM_SMOOTH:
				for( f_cnt_t f = 0; f < frames; ++f )
				{
					const f_cnt_t tm = ( tfp + f ) % static_cast<int>( m_lfo_rate[i] );
					if( tm == 0 )
					{
						m_lfo_last[i] = m_lfo_next[i];
						m_lfo_next[i] = Oscillator::noiseSample( 0.0f );
					}
					lfo[i][f] = cosinusInterpolate( m_lfo_last[i], m_lfo_next[i], static_cast<float>( tm ) / m_lfo_rate[i] );
					m_lfo_phase[i] += m_lfo_inc[i];
				}
			break;
		}

		// attack
		for( f_cnt_t f = 0; f < frames; ++f )
		{
			if (tfp + f < static_cast<f_cnt_t>(m_lfoatt[i])) { lfo[i][f] *= static_cast<sample_t>(tfp) / m_lfoatt[i]; }
		}



	/////////////////////////////////////////////
	//                                         //
	//                                         //
	// 					envelopes              //
	//                                         //
	//                                         //
	/////////////////////////////////////////////

		for( f_cnt_t f = 0; f < frames; ++f )
		{
			if( m_env_phase[i] < 4.0f && m_nph->isReleased() && f >= m_nph->framesBeforeRelease() )
			{
				if( m_env_phase[i] < 1.0f ) m_env_phase[i] = 5.0f;
				else if( m_env_phase[i] < 2.0f ) m_env_phase[i] = 5.0f - fraction( m_env_phase[i] );
				else if( m_env_phase[i] < 3.0f ) m_env_phase[i] = 4.0f;
				else m_env_phase[i] = 4.0f + fraction( m_env_phase[i] );
			}

			// process envelope
			if( m_env_phase[i] < 1.0f ) // pre-delay phase
			{
				env[i][f] = 0.0f;
				m_env_phase[i] = qMin( 1.0f, m_env_phase[i] + m_env_pre[i] );
			}
			else if( m_env_phase[i] < 2.0f ) // attack phase
			{
				env[i][f] = calcSlope( i, fraction( m_env_phase[i] ) );
				m_env_phase[i] = qMin( 2.0f, m_env_phase[i] + m_env_att[i] );
			}
			else if( m_env_phase[i] < 3.0f ) // hold phase
			{
				env[i][f] = 1.0f;
				m_env_phase[i] = qMin( 3.0f, m_env_phase[i] + m_env_hold[i] );
			}
			else if( m_env_phase[i] < 4.0f ) // decay phase
			{
				const sample_t s = calcSlope( i, 1.0f - fraction( m_env_phase[i] ) );
				if( s <= m_env_sus[i] )
				{
					env[i][f] = m_env_sus[i];
				}
				else
				{
					env[i][f] = s;
					m_env_phase[i] = qMin( 4.0f - m_env_sus[i], m_env_phase[i] + m_env_dec[i] );
					if( m_env_phase[i] == 4.0f ) m_env_phase[i] = 5.0f; // jump over release if sustain is zero - fix for clicking
				}
			}
			else if( m_env_phase[i] < 5.0f ) // release phase
			{
				env[i][f] = calcSlope( i, 1.0f - fraction( m_env_phase[i] ) );
				m_env_phase[i] += m_env_rel[i];
			}
			else env[i][f] = 0.0f;
		}
	}
}


inline sample_t MonstroSynth::calcSlope( int slope, sample_t s )
{
	if( m_parent->m_slope[slope] == 1.0f ) return s;
	if( s == 0.0f ) return s;
	return fastPow(s, m_parent->m_slope[slope]);
}


MonstroInstrument::MonstroInstrument( InstrumentTrack * _instrument_track ) :
		Instrument( _instrument_track, &monstro_plugin_descriptor ),

		m_osc1Vol(33.f, 0.f, 200.f, 0.1f, this, tr("Osc 1 volume")),
		m_osc1Pan(0.f, -100.f, 100.f, 0.1f, this, tr("Osc 1 panning")),
		m_osc1Crs( 0.0, -24.0, 24.0, 1.0, this, tr( "Osc 1 coarse detune" ) ),
		m_osc1Ftl( 0.0, -100.0, 100.0, 1.0, this, tr( "Osc 1 fine detune left" ) ),
		m_osc1Ftr( 0.0, -100.0, 100.0, 1.0, this, tr( "Osc 1 fine detune right" ) ),
		m_osc1Spo(0.f, -180.f, 180.f, 0.1f, this, tr("Osc 1 stereo phase offset")),
		m_osc1Pw(50.f, PW_MIN, PW_MAX, 0.01f, this, tr("Osc 1 pulse width")),
		m_osc1SSR( false, this, tr( "Osc 1 sync send on rise" ) ),
		m_osc1SSF( false, this, tr( "Osc 1 sync send on fall" ) ),

		m_osc2Vol(33.f, 0.f, 200.f, 0.1f, this, tr("Osc 2 volume")),
		m_osc2Pan(0.f, -100.f, 100.f, 0.1f, this, tr("Osc 2 panning")),
		m_osc2Crs( 0.0, -24.0, 24.0, 1.0, this, tr( "Osc 2 coarse detune" ) ),
		m_osc2Ftl( 0.0, -100.0, 100.0, 1.0, this, tr( "Osc 2 fine detune left" ) ),
		m_osc2Ftr( 0.0, -100.0, 100.0, 1.0, this, tr( "Osc 2 fine detune right" ) ),
		m_osc2Spo(0.f, -180.f, 180.f, 0.1f, this, tr("Osc 2 stereo phase offset")),
		m_osc2Wave( this, tr( "Osc 2 waveform" ) ),
		m_osc2SyncH( false, this, tr( "Osc 2 sync hard" ) ),
		m_osc2SyncR( false, this, tr( "Osc 2 sync reverse" ) ),

		m_osc3Vol(33.f, 0.f, 200.f, 0.1f, this, tr("Osc 3 volume")),
		m_osc3Pan(0.f, -100.f, 100.f, 0.1f, this, tr("Osc 3 panning")),
		m_osc3Crs( 0.0, -24.0, 24.0, 1.0, this, tr( "Osc 3 coarse detune" ) ),
		m_osc3Spo(0.f, -180.f, 180.f, 0.1f, this, tr("Osc 3 Stereo phase offset")),
		m_osc3Sub(0.f, -100.f, 100.f, 0.1f, this, tr("Osc 3 sub-oscillator mix")),
		m_osc3Wave1( this, tr( "Osc 3 waveform 1" ) ),
		m_osc3Wave2( this, tr( "Osc 3 waveform 2" ) ),
		m_osc3SyncH( false, this, tr( "Osc 3 sync hard" ) ),
		m_osc3SyncR( false, this, tr( "Osc 3 Sync reverse" ) ),

		m_lfo1Wave( this, tr( "LFO 1 waveform" ) ),
		m_lfo1Att( 0.0f, 0.0f, 2000.0f, 1.0f, 2000.0f, this, tr( "LFO 1 attack" ) ),
		m_lfo1Rate(1.0f, 0.1f, 10000.f, 0.1f, 10000.0f, this, tr("LFO 1 rate")),
		m_lfo1Phs(0.f, -180.f, 180.f, 0.1f, this, tr("LFO 1 phase")),

		m_lfo2Wave( this, tr( "LFO 2 waveform" ) ),
		m_lfo2Att( 0.0f, 0.0f, 2000.0f, 1.0f, 2000.0f, this, tr( "LFO 2 attack" ) ),
		m_lfo2Rate(1.0f, 0.1f, 10000.f, 0.1f, 10000.0f, this, tr("LFO 2 rate")),
		m_lfo2Phs(0.0, -180.f, 180.f, 0.1f, this, tr("LFO 2 phase")),

		m_env1Pre( 0.0f, 0.0f, 2000.0f, 1.0f, 2000.0f, this, tr( "Env 1 pre-delay" ) ),
		m_env1Att( 0.0f, 0.0f, 2000.0f, 1.0f, 2000.0f, this, tr( "Env 1 attack" ) ),
		m_env1Hold( 0.0f, 0.0f, 4000.0f, 1.0f, 4000.0f, this, tr( "Env 1 hold" ) ),
		m_env1Dec( 0.0f, 0.0f, 4000.0f, 1.0f, 4000.0f, this, tr( "Env 1 decay" ) ),
		m_env1Sus( 1.0f, 0.0f, 1.0f, 0.001f, this, tr( "Env 1 sustain" ) ),
		m_env1Rel( 0.0f, 0.0f, 4000.0f, 1.0f, 4000.0f, this, tr( "Env 1 release" ) ),
		m_env1Slope( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Env 1 slope" ) ),

		m_env2Pre( 0.0f, 0.0f, 2000.0f, 1.0f, 2000.0f, this, tr( "Env 2 pre-delay" ) ),
		m_env2Att( 0.0f, 0.0f, 2000.0f, 1.0f, 2000.0f, this, tr( "Env 2 attack" ) ),
		m_env2Hold( 0.0f, 0.0f, 4000.0f, 1.0f, 4000.0f, this, tr( "Env 2 hold" ) ),
		m_env2Dec( 0.0f, 0.0f, 4000.0f, 1.0f, 4000.0f, this, tr( "Env 2 decay" ) ),
		m_env2Sus( 1.0f, 0.0f, 1.0f, 0.001f, this, tr( "Env 2 sustain" ) ),
		m_env2Rel( 0.0f, 0.0f, 4000.0f, 1.0f, 4000.0f, this, tr( "Env 2 release" ) ),
		m_env2Slope( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Env 2 slope" ) ),

		m_o23Mod( 0, 0, NUM_MODS - 1, this, tr( "Osc 2+3 modulation" ) ),

		m_selectedView( 0, 0, 1, this, tr( "Selected view" ) ),

		m_vol1env1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 1 - Vol env 1" ) ),
		m_vol1env2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 1 - Vol env 2" ) ),
		m_vol1lfo1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 1 - Vol LFO 1" ) ),
		m_vol1lfo2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 1 - Vol LFO 2" ) ),

		m_vol2env1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 2 - Vol env 1" ) ),
		m_vol2env2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 2 - Vol env 2" ) ),
		m_vol2lfo1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 2 - Vol LFO 1" ) ),
		m_vol2lfo2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 2 - Vol LFO 2" ) ),

		m_vol3env1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 3 - Vol env 1" ) ),
		m_vol3env2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 3 - Vol env 2" ) ),
		m_vol3lfo1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 3 - Vol LFO 1" ) ),
		m_vol3lfo2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 3 - Vol LFO 2" ) ),

		m_phs1env1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 1 - Phs env 1" ) ),
		m_phs1env2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 1 - Phs env 2" ) ),
		m_phs1lfo1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 1 - Phs LFO 1" ) ),
		m_phs1lfo2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 1 - Phs LFO 2" ) ),

		m_phs2env1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 2 - Phs env 1" ) ),
		m_phs2env2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 2 - Phs env 2" ) ),
		m_phs2lfo1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 2 - Phs LFO 1" ) ),
		m_phs2lfo2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 2 - Phs LFO 2" ) ),

		m_phs3env1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 3 - Phs env 1" ) ),
		m_phs3env2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 3 - Phs env 2" ) ),
		m_phs3lfo1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 3 - Phs LFO 1" ) ),
		m_phs3lfo2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 3 - Phs LFO 2" ) ),

		m_pit1env1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 1 - Pit env 1" ) ),
		m_pit1env2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 1 - Pit env 2" ) ),
		m_pit1lfo1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 1 - Pit LFO 1" ) ),
		m_pit1lfo2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 1 - Pit LFO 2" ) ),

		m_pit2env1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 2 - Pit env 1" ) ),
		m_pit2env2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 2 - Pit env 2" ) ),
		m_pit2lfo1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 2 - Pit LFO 1" ) ),
		m_pit2lfo2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 2 - Pit LFO 2" ) ),

		m_pit3env1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 3 - Pit env 1" ) ),
		m_pit3env2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 3 - Pit env 2" ) ),
		m_pit3lfo1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 3 - Pit LFO 1" ) ),
		m_pit3lfo2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 3 - Pit LFO 2" ) ),

		m_pw1env1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 1 - PW env 1" ) ),
		m_pw1env2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 1 - PW env 2" ) ),
		m_pw1lfo1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 1 - PW LFO 1" ) ),
		m_pw1lfo2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 1 - PW LFO 2" ) ),

		m_sub3env1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 3 - Sub env 1" ) ),
		m_sub3env2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 3 - Sub env 2" ) ),
		m_sub3lfo1( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 3 - Sub LFO 1" ) ),
		m_sub3lfo2( 0.0f, -1.0f, 1.0f, 0.001f, this, tr( "Osc 3 - Sub LFO 2" ) )

{

// setup waveboxes
	setwavemodel( m_osc2Wave )
	setwavemodel( m_osc3Wave1 )
	setwavemodel( m_osc3Wave2 )
	setlfowavemodel( m_lfo1Wave )
	setlfowavemodel( m_lfo2Wave )

// make connections:

// updateVolumes

	connect( &m_osc1Vol, SIGNAL( dataChanged() ), this, SLOT( updateVolume1() ), Qt::DirectConnection );
	connect( &m_osc1Pan, SIGNAL( dataChanged() ), this, SLOT( updateVolume1() ), Qt::DirectConnection );
	connect( &m_osc2Vol, SIGNAL( dataChanged() ), this, SLOT( updateVolume2() ), Qt::DirectConnection );
	connect( &m_osc2Pan, SIGNAL( dataChanged() ), this, SLOT( updateVolume2() ), Qt::DirectConnection );
	connect( &m_osc3Vol, SIGNAL( dataChanged() ), this, SLOT( updateVolume3() ), Qt::DirectConnection );
	connect( &m_osc3Pan, SIGNAL( dataChanged() ), this, SLOT( updateVolume3() ), Qt::DirectConnection );

// updateFreq

	connect( &m_osc1Crs, SIGNAL( dataChanged() ), this, SLOT( updateFreq1() ), Qt::DirectConnection );
	connect( &m_osc2Crs, SIGNAL( dataChanged() ), this, SLOT( updateFreq2() ), Qt::DirectConnection );
	connect( &m_osc3Crs, SIGNAL( dataChanged() ), this, SLOT( updateFreq3() ), Qt::DirectConnection );

	connect( &m_osc1Ftl, SIGNAL( dataChanged() ), this, SLOT( updateFreq1() ), Qt::DirectConnection );
	connect( &m_osc2Ftl, SIGNAL( dataChanged() ), this, SLOT( updateFreq2() ), Qt::DirectConnection );

	connect( &m_osc1Ftr, SIGNAL( dataChanged() ), this, SLOT( updateFreq1() ), Qt::DirectConnection );
	connect( &m_osc2Ftr, SIGNAL( dataChanged() ), this, SLOT( updateFreq2() ), Qt::DirectConnection );

// updatePO
	connect( &m_osc1Spo, SIGNAL( dataChanged() ), this, SLOT( updatePO1() ), Qt::DirectConnection );
	connect( &m_osc2Spo, SIGNAL( dataChanged() ), this, SLOT( updatePO2() ), Qt::DirectConnection );
	connect( &m_osc3Spo, SIGNAL( dataChanged() ), this, SLOT( updatePO3() ), Qt::DirectConnection );

// updateEnvelope1

	connect( &m_env1Pre, SIGNAL( dataChanged() ), this, SLOT( updateEnvelope1() ), Qt::DirectConnection );
	connect( &m_env1Att, SIGNAL( dataChanged() ), this, SLOT( updateEnvelope1() ), Qt::DirectConnection );
	connect( &m_env1Hold, SIGNAL( dataChanged() ), this, SLOT( updateEnvelope1() ), Qt::DirectConnection );
	connect( &m_env1Dec, SIGNAL( dataChanged() ), this, SLOT( updateEnvelope1() ), Qt::DirectConnection );
	connect( &m_env1Rel, SIGNAL( dataChanged() ), this, SLOT( updateEnvelope1() ), Qt::DirectConnection );
	connect( &m_env1Slope, SIGNAL( dataChanged() ), this, SLOT( updateSlope1() ), Qt::DirectConnection );

// updateEnvelope2

	connect( &m_env2Pre, SIGNAL( dataChanged() ), this, SLOT( updateEnvelope2() ), Qt::DirectConnection );
	connect( &m_env2Att, SIGNAL( dataChanged() ), this, SLOT( updateEnvelope2() ), Qt::DirectConnection );
	connect( &m_env2Hold, SIGNAL( dataChanged() ), this, SLOT( updateEnvelope2() ), Qt::DirectConnection );
	connect( &m_env2Dec, SIGNAL( dataChanged() ), this, SLOT( updateEnvelope2() ), Qt::DirectConnection );
	connect( &m_env2Rel, SIGNAL( dataChanged() ), this, SLOT( updateEnvelope2() ), Qt::DirectConnection );
	connect( &m_env2Slope, SIGNAL( dataChanged() ), this, SLOT( updateSlope2() ), Qt::DirectConnection );

// updateLFOAtts

	connect( &m_lfo1Att, SIGNAL( dataChanged() ), this, SLOT( updateLFOAtts() ), Qt::DirectConnection );
	connect( &m_lfo2Att, SIGNAL( dataChanged() ), this, SLOT( updateLFOAtts() ), Qt::DirectConnection );

// updateSampleRate

	connect( Engine::audioEngine(), SIGNAL( sampleRateChanged() ), this, SLOT( updateSamplerate() ) );

	m_fpp = Engine::audioEngine()->framesPerPeriod();

	updateSamplerate();
	updateVolume1();
	updateVolume2();
	updateVolume3();
	updateFreq1();
	updateFreq2();
	updateFreq3();
	updatePO1();
	updatePO2();
	updatePO3();
	updateSlope1();
	updateSlope2();
}


void MonstroInstrument::playNote( NotePlayHandle * _n,
						SampleFrame* _working_buffer )
{
	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	const f_cnt_t offset = _n->noteOffset();

	if (!_n->m_pluginData)
	{
		_n->m_pluginData = new MonstroSynth( this, _n );
	}

	auto ms = static_cast<MonstroSynth*>(_n->m_pluginData);

	ms->renderOutput( frames, _working_buffer + offset );

	//applyRelease( _working_buffer, _n ); // we have our own release
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
	m_osc1SSR.saveSettings( _doc, _this, "o1ssr" );
	m_osc1SSF.saveSettings( _doc, _this, "o1ssf" );

	m_osc2Vol.saveSettings( _doc, _this, "o2vol" );
	m_osc2Pan.saveSettings( _doc, _this, "o2pan" );
	m_osc2Crs.saveSettings( _doc, _this, "o2crs" );
	m_osc2Ftl.saveSettings( _doc, _this, "o2ftl" );
	m_osc2Ftr.saveSettings( _doc, _this, "o2ftr" );
	m_osc2Spo.saveSettings( _doc, _this, "o2spo" );
	m_osc2Wave.saveSettings( _doc, _this, "o2wav" );
	m_osc2SyncH.saveSettings( _doc, _this, "o2syn" );
	m_osc2SyncR.saveSettings( _doc, _this, "o2synr" );

	m_osc3Vol.saveSettings( _doc, _this, "o3vol" );
	m_osc3Pan.saveSettings( _doc, _this, "o3pan" );
	m_osc3Crs.saveSettings( _doc, _this, "o3crs" );
	m_osc3Spo.saveSettings( _doc, _this, "o3spo" );
	m_osc3Sub.saveSettings( _doc, _this, "o3sub" );
	m_osc3Wave1.saveSettings( _doc, _this, "o3wav1" );
	m_osc3Wave2.saveSettings( _doc, _this, "o3wav2" );
	m_osc3SyncH.saveSettings( _doc, _this, "o3syn" );
	m_osc3SyncR.saveSettings( _doc, _this, "o3synr" );

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
	m_osc1SSR.loadSettings( _this, "o1ssr" );
	m_osc1SSF.loadSettings( _this, "o1ssf" );

	m_osc2Vol.loadSettings( _this, "o2vol" );
	m_osc2Pan.loadSettings( _this, "o2pan" );
	m_osc2Crs.loadSettings( _this, "o2crs" );
	m_osc2Ftl.loadSettings( _this, "o2ftl" );
	m_osc2Ftr.loadSettings( _this, "o2ftr" );
	m_osc2Spo.loadSettings( _this, "o2spo" );
	m_osc2Wave.loadSettings( _this, "o2wav" );
	m_osc2SyncH.loadSettings( _this, "o2syn" );
	m_osc2SyncR.loadSettings( _this, "o2synr" );

	m_osc3Vol.loadSettings( _this, "o3vol" );
	m_osc3Pan.loadSettings( _this, "o3pan" );
	m_osc3Crs.loadSettings( _this, "o3crs" );
	m_osc3Spo.loadSettings( _this, "o3spo" );
	m_osc3Sub.loadSettings( _this, "o3sub" );
	m_osc3Wave1.loadSettings( _this, "o3wav1" );
	m_osc3Wave2.loadSettings( _this, "o3wav2" );
	m_osc3SyncH.loadSettings( _this, "o3syn" );
	m_osc3SyncR.loadSettings( _this, "o3synr" );

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

float MonstroInstrument::desiredReleaseTimeMs() const
{
	const auto maxEnvelope = std::max(m_env1_rel, m_env2_rel);
	return std::max(1.5f, maxEnvelope);
}

gui::PluginView* MonstroInstrument::instantiateView( QWidget * _parent )
{
	return( new gui::MonstroView( this, _parent ) );
}


void MonstroInstrument::updateVolume1()
{
	m_osc1l_vol = leftCh( m_osc1Vol.value(), m_osc1Pan.value() );
	m_osc1r_vol = rightCh( m_osc1Vol.value(), m_osc1Pan.value() );
}


void MonstroInstrument::updateVolume2()
{
	m_osc2l_vol = leftCh( m_osc2Vol.value(), m_osc2Pan.value() );
	m_osc2r_vol = rightCh( m_osc2Vol.value(), m_osc2Pan.value() );
}


void MonstroInstrument::updateVolume3()
{
	m_osc3l_vol = leftCh( m_osc3Vol.value(), m_osc3Pan.value() );
	m_osc3r_vol = rightCh( m_osc3Vol.value(), m_osc3Pan.value() );
}


void MonstroInstrument::updateFreq1()
{
	m_osc1l_freq = std::exp2(m_osc1Crs.value() / 12.0f)
		* std::exp2(m_osc1Ftl.value() / 1200.0f);
	m_osc1r_freq = std::exp2(m_osc1Crs.value() / 12.0f)
		* std::exp2(m_osc1Ftr.value() / 1200.0f);
}


void MonstroInstrument::updateFreq2()
{
	m_osc2l_freq = std::exp2(m_osc2Crs.value() / 12.0f)
		* std::exp2(m_osc2Ftl.value() / 1200.0f);
	m_osc2r_freq = std::exp2(m_osc2Crs.value() / 12.0f)
		* std::exp2(m_osc2Ftr.value() / 1200.0f);
}


void MonstroInstrument::updateFreq3()
{
	m_osc3_freq = std::exp2(m_osc3Crs.value() / 12.0f);
}


void MonstroInstrument::updatePO1()
{
	m_osc1l_po = m_osc1Spo.value() / 720.0f;
	m_osc1r_po = ( m_osc1Spo.value() * -1.0 ) / 720.0f;
}


void MonstroInstrument::updatePO2()
{
	m_osc2l_po = m_osc2Spo.value() / 720.0f;
	m_osc2r_po = ( m_osc2Spo.value() * -1.0 ) / 720.0f;
}


void MonstroInstrument::updatePO3()
{
	m_osc3l_po = m_osc3Spo.value() / 720.0f;
	m_osc3r_po = ( m_osc3Spo.value() * -1.0 ) / 720.0f;
}


void MonstroInstrument::updateEnvelope1()
{
	if( m_env1Pre.value() == 0.0f ) m_env1_pre = 1.0;
		else m_env1_pre =  1.0f / ( m_env1Pre.value()  / 1000.0f ) / m_samplerate;
	if( m_env1Att.value() == 0.0f ) m_env1_att = 1.0;
		else m_env1_att =  1.0f / ( m_env1Att.value()  / 1000.0f ) / m_samplerate;
	if( m_env1Hold.value() == 0.0f ) m_env1_hold = 1.0;
		else m_env1_hold = 1.0f / ( m_env1Hold.value() / 1000.0f ) / m_samplerate;
	if( m_env1Dec.value() == 0.0f ) m_env1_dec = 1.0;
		else m_env1_dec =  1.0f / ( m_env1Dec.value()  / 1000.0f ) / m_samplerate;
	if( m_env1Rel.value() == 0.0f ) m_env1_rel = 1.0;
		else m_env1_rel =  1.0f / ( m_env1Rel.value()  / 1000.0f ) / m_samplerate;

	m_env1_len = ( m_env1Pre.value() + m_env1Att.value() + m_env1Hold.value() + m_env1Dec.value() ) * m_samplerate / 1000.0f;
	m_env1_relF = m_env1Rel.value() * m_samplerate / 1000.0f;
}
void MonstroInstrument::updateEnvelope2()
{
	if( m_env2Pre.value() == 0.0f ) m_env2_pre = 1.0;
		else m_env2_pre =  1.0f / ( m_env2Pre.value()  / 1000.0f ) / m_samplerate;
	if( m_env2Att.value() == 0.0f ) m_env2_att = 1.0;
		else m_env2_att =  1.0f / ( m_env2Att.value()  / 1000.0f ) / m_samplerate;
	if( m_env2Hold.value() == 0.0f ) m_env2_hold = 1.0;
		else m_env2_hold = 1.0f / ( m_env2Hold.value() / 1000.0f ) / m_samplerate;
	if( m_env2Dec.value() == 0.0f ) m_env2_dec = 1.0;
		else m_env2_dec =  1.0f / ( m_env2Dec.value()  / 1000.0f ) / m_samplerate;
	if( m_env2Rel.value() == 0.0f ) m_env2_rel = 1.0;
		else m_env2_rel =  1.0f / ( m_env2Rel.value()  / 1000.0f ) / m_samplerate;

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
	m_samplerate = Engine::audioEngine()->outputSampleRate();

	m_integrator = 0.5f - ( 0.5f - INTEGRATOR ) * 44100.0f / m_samplerate;
	m_fmCorrection = 44100.f / m_samplerate * FM_AMOUNT;
	m_counterMax = ( m_samplerate * 5 ) / 44100;

	updateEnvelope1();
	updateEnvelope2();
	updateLFOAtts();
}


void MonstroInstrument::updateSlope1()
{
	const float slope = m_env1Slope.value();
	m_slope[0] = fastPow10f(-slope);
}


void MonstroInstrument::updateSlope2()
{
	const float slope = m_env2Slope.value();
	m_slope[1] = fastPow10f(-slope);
}


namespace gui
{


MonstroView::MonstroView( Instrument * _instrument,
					QWidget * _parent ) :
					InstrumentViewFixedSize( _instrument, _parent )
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

	auto m_opViewButton = new PixmapButton(this, nullptr);
	m_opViewButton -> move( 0,0 );
	m_opViewButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "opview_active" ) );
	m_opViewButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "opview_inactive" ) );
	m_opViewButton->setToolTip(tr("Operators view"));

	auto m_matViewButton = new PixmapButton(this, nullptr);
	m_matViewButton -> move( 125,0 );
	m_matViewButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "matview_active" ) );
	m_matViewButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "matview_inactive" ) );
	m_matViewButton->setToolTip(tr("Matrix view"));

	m_selectedViewGroup = new AutomatableButtonGroup( this );
	m_selectedViewGroup -> addButton( m_opViewButton );
	m_selectedViewGroup -> addButton( m_matViewButton );

	connect( m_opViewButton, SIGNAL( clicked() ), this, SLOT( updateLayout() ) );
	connect( m_matViewButton, SIGNAL( clicked() ), this, SLOT( updateLayout() ) );
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
	auto m = castModel<MonstroInstrument>();

	m_osc1VolKnob-> setModel( &m-> m_osc1Vol );
	m_osc1PanKnob-> setModel( &m-> m_osc1Pan );
	m_osc1CrsKnob-> setModel( &m-> m_osc1Crs );
	m_osc1FtlKnob-> setModel( &m-> m_osc1Ftl );
	m_osc1FtrKnob-> setModel( &m-> m_osc1Ftr );
	m_osc1SpoKnob-> setModel( &m-> m_osc1Spo );
	m_osc1PwKnob-> setModel( &m-> m_osc1Pw );
	m_osc1SSRButton-> setModel( &m-> m_osc1SSR );
	m_osc1SSFButton-> setModel( &m-> m_osc1SSF );

	m_osc2VolKnob-> setModel( &m-> m_osc2Vol );
	m_osc2PanKnob-> setModel( &m-> m_osc2Pan );
	m_osc2CrsKnob-> setModel( &m-> m_osc2Crs );
	m_osc2FtlKnob-> setModel( &m-> m_osc2Ftl );
	m_osc2FtrKnob-> setModel( &m-> m_osc2Ftr );
	m_osc2SpoKnob-> setModel( &m-> m_osc2Spo );
	m_osc2WaveBox-> setModel( &m-> m_osc2Wave );
	m_osc2SyncHButton-> setModel( &m-> m_osc2SyncH );
	m_osc2SyncRButton-> setModel( &m-> m_osc2SyncR );

	m_osc3VolKnob-> setModel( &m-> m_osc3Vol );
	m_osc3PanKnob-> setModel( &m-> m_osc3Pan );
	m_osc3CrsKnob-> setModel( &m-> m_osc3Crs );
	m_osc3SpoKnob-> setModel( &m-> m_osc3Spo );
	m_osc3SubKnob-> setModel( &m-> m_osc3Sub );
	m_osc3Wave1Box-> setModel( &m-> m_osc3Wave1 );
	m_osc3Wave2Box-> setModel( &m-> m_osc3Wave2 );
	m_osc3SyncHButton-> setModel( &m-> m_osc3SyncH );
	m_osc3SyncRButton-> setModel( &m-> m_osc3SyncR );

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
		PLUGIN_NAME::getIconPixmap( _pic.toLatin1().constData() ) );
	_widget->setPalette( pal );
}


QWidget * MonstroView::setupOperatorsView( QWidget * _parent )
{
	// operators view

	auto view = new QWidget(_parent);
	view-> setFixedSize( 250, 250 );

	makeknob( m_osc1VolKnob, KNOBCOL1, O1ROW, tr( "Volume" ), "%", "osc1Knob" )
	makeknob( m_osc1PanKnob, KNOBCOL2, O1ROW, tr( "Panning" ), "", "osc1Knob" )
	makeknob( m_osc1CrsKnob, KNOBCOL3, O1ROW, tr( "Coarse detune" ), tr( " semitones" ), "osc1Knob" )
	makeknob( m_osc1FtlKnob, KNOBCOL4, O1ROW, tr( "Fine tune left" ), tr( " cents" ), "osc1Knob" )
	makeknob( m_osc1FtrKnob, KNOBCOL5, O1ROW, tr( "Fine tune right" ), tr( " cents" ), "osc1Knob" )
	makeknob( m_osc1SpoKnob, KNOBCOL6, O1ROW, tr( "Stereo phase offset" ), tr( " deg" ), "osc1Knob" )
	makeknob( m_osc1PwKnob,  KNOBCOL7, O1ROW, tr( "Pulse width" ), "%", "osc1Knob" )

	m_osc1VolKnob -> setVolumeKnob( true );

	maketinyled( m_osc1SSRButton, 230, 34, tr( "Send sync on pulse rise" ) )
	maketinyled( m_osc1SSFButton, 230, 44, tr( "Send sync on pulse fall" ) )

	makeknob( m_osc2VolKnob, KNOBCOL1, O2ROW, tr( "Volume" ), "%", "osc2Knob" )
	makeknob( m_osc2PanKnob, KNOBCOL2, O2ROW, tr( "Panning" ), "", "osc2Knob" )
	makeknob( m_osc2CrsKnob, KNOBCOL3, O2ROW, tr( "Coarse detune" ), tr( " semitones" ), "osc2Knob" )
	makeknob( m_osc2FtlKnob, KNOBCOL4, O2ROW, tr( "Fine tune left" ), tr( " cents" ), "osc2Knob" )
	makeknob( m_osc2FtrKnob, KNOBCOL5, O2ROW, tr( "Fine tune right" ), tr( " cents" ), "osc2Knob" )
	makeknob( m_osc2SpoKnob, KNOBCOL6, O2ROW, tr( "Stereo phase offset" ), tr( " deg" ), "osc2Knob" )

	m_osc2VolKnob -> setVolumeKnob( true );

	m_osc2WaveBox = new ComboBox( view );
	m_osc2WaveBox -> setGeometry( 204, O2ROW + 7, 42, ComboBox::DEFAULT_HEIGHT );

	maketinyled( m_osc2SyncHButton, 212, O2ROW - 3, tr( "Hard sync oscillator 2" ) )
	maketinyled( m_osc2SyncRButton, 191, O2ROW - 3, tr( "Reverse sync oscillator 2" ) )

	makeknob( m_osc3VolKnob, KNOBCOL1, O3ROW, tr( "Volume" ), "%", "osc3Knob" )
	makeknob( m_osc3PanKnob, KNOBCOL2, O3ROW, tr( "Panning" ), "", "osc3Knob" )
	makeknob( m_osc3CrsKnob, KNOBCOL3, O3ROW, tr( "Coarse detune" ), tr( " semitones" ), "osc3Knob" )
	makeknob( m_osc3SpoKnob, KNOBCOL4, O3ROW, tr( "Stereo phase offset" ), tr( " deg" ), "osc3Knob" )
	makeknob( m_osc3SubKnob, KNOBCOL5, O3ROW, tr( "Sub-osc mix" ), "", "osc3Knob" )

	m_osc3VolKnob -> setVolumeKnob( true );

	m_osc3Wave1Box = new ComboBox( view );
	m_osc3Wave1Box -> setGeometry( 160, O3ROW + 7, 42, ComboBox::DEFAULT_HEIGHT );

	m_osc3Wave2Box = new ComboBox( view );
	m_osc3Wave2Box -> setGeometry( 204, O3ROW + 7, 42, ComboBox::DEFAULT_HEIGHT );

	maketinyled( m_osc3SyncHButton, 212, O3ROW - 3, tr( "Hard sync oscillator 3" ) )
	maketinyled( m_osc3SyncRButton, 191, O3ROW - 3, tr( "Reverse sync oscillator 3" ) )

	m_lfo1WaveBox = new ComboBox( view );
	m_lfo1WaveBox -> setGeometry( 2, LFOROW + 7, 42, ComboBox::DEFAULT_HEIGHT );

	maketsknob( m_lfo1AttKnob, LFOCOL1, LFOROW, tr( "Attack" ), " ms", "lfoKnob" )
	maketsknob( m_lfo1RateKnob, LFOCOL2, LFOROW, tr( "Rate" ), " ms", "lfoKnob" )
	makeknob( m_lfo1PhsKnob, LFOCOL3, LFOROW, tr( "Phase" ), tr( " deg" ), "lfoKnob" )

	m_lfo2WaveBox = new ComboBox( view );
	m_lfo2WaveBox -> setGeometry( 127, LFOROW + 7, 42, ComboBox::DEFAULT_HEIGHT );

	maketsknob(m_lfo2AttKnob, LFOCOL4, LFOROW, tr("Attack"), " ms", "lfoKnob")
	maketsknob(m_lfo2RateKnob, LFOCOL5, LFOROW, tr("Rate"), " ms", "lfoKnob")
	makeknob(m_lfo2PhsKnob, LFOCOL6, LFOROW, tr("Phase"), tr(" deg"), "lfoKnob")

	maketsknob(m_env1PreKnob, KNOBCOL1, E1ROW, tr("Pre-delay"), " ms", "envKnob")
	maketsknob(m_env1AttKnob, KNOBCOL2, E1ROW, tr("Attack"), " ms", "envKnob")
	maketsknob(m_env1HoldKnob, KNOBCOL3, E1ROW, tr("Hold"), " ms", "envKnob")
	maketsknob(m_env1DecKnob, KNOBCOL4, E1ROW, tr("Decay"), " ms", "envKnob")

	makeknob(m_env1SusKnob, KNOBCOL5, E1ROW, tr("Sustain"), "", "envKnob")
	maketsknob(m_env1RelKnob, KNOBCOL6, E1ROW, tr("Release"), " ms", "envKnob")
	makeknob(m_env1SlopeKnob, KNOBCOL7, E1ROW, tr("Slope"), "", "envKnob")

	maketsknob(m_env2PreKnob, KNOBCOL1, E2ROW, tr("Pre-delay"), " ms", "envKnob")
	maketsknob(m_env2AttKnob, KNOBCOL2, E2ROW, tr("Attack"), " ms", "envKnob")
	maketsknob(m_env2HoldKnob, KNOBCOL3, E2ROW, tr("Hold"), " ms", "envKnob")
	maketsknob(m_env2DecKnob, KNOBCOL4, E2ROW, tr("Decay"), " ms", "envKnob")
	makeknob(m_env2SusKnob, KNOBCOL5, E2ROW, tr("Sustain"), "", "envKnob")
	maketsknob(m_env2RelKnob, KNOBCOL6, E2ROW, tr("Release"), " ms", "envKnob")
	makeknob(m_env2SlopeKnob, KNOBCOL7, E2ROW, tr("Slope"), "", "envKnob")

	// mod selector
	auto m_mixButton = new PixmapButton(view, nullptr);
	m_mixButton -> move( 225, 185 );
	m_mixButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "mix_active" ) );
	m_mixButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "mix_inactive" ) );
	m_mixButton->setToolTip(tr("Mix osc 2 with osc 3"));

	auto m_amButton = new PixmapButton(view, nullptr);
	m_amButton -> move( 225, 185 + 15 );
	m_amButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "am_active" ) );
	m_amButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "am_inactive" ) );
	m_amButton->setToolTip(tr("Modulate amplitude of osc 3 by osc 2"));

	auto m_fmButton = new PixmapButton(view, nullptr);
	m_fmButton -> move( 225, 185 + 15*2 );
	m_fmButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "fm_active" ) );
	m_fmButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "fm_inactive" ) );
	m_fmButton->setToolTip(tr("Modulate frequency of osc 3 by osc 2"));

	auto m_pmButton = new PixmapButton(view, nullptr);
	m_pmButton -> move( 225, 185 + 15*3 );
	m_pmButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "pm_active" ) );
	m_pmButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "pm_inactive" ) );
	m_pmButton->setToolTip(tr("Modulate phase of osc 3 by osc 2"));

	m_o23ModGroup = new AutomatableButtonGroup( view );
	m_o23ModGroup-> addButton( m_mixButton );
	m_o23ModGroup-> addButton( m_amButton );
	m_o23ModGroup-> addButton( m_fmButton );
	m_o23ModGroup-> addButton( m_pmButton );



	return( view );
}


QWidget * MonstroView::setupMatrixView( QWidget * _parent )
{
	// matrix view

	auto view = new QWidget(_parent);
	view-> setFixedSize( 250, 250 );

	makeknob( m_vol1env1Knob, MATCOL1, MATROW1, tr( "Modulation amount" ), "", "matrixKnob" )
	makeknob( m_vol1env2Knob, MATCOL2, MATROW1, tr( "Modulation amount" ), "", "matrixKnob" )
	makeknob( m_vol1lfo1Knob, MATCOL3, MATROW1, tr( "Modulation amount" ), "", "matrixKnob" )
	makeknob( m_vol1lfo2Knob, MATCOL4, MATROW1, tr( "Modulation amount" ), "", "matrixKnob" )

	makeknob( m_vol2env1Knob, MATCOL1, MATROW3, tr( "Modulation amount" ), "", "matrixKnob" )
	makeknob( m_vol2env2Knob, MATCOL2, MATROW3, tr( "Modulation amount" ), "", "matrixKnob" )
	makeknob( m_vol2lfo1Knob, MATCOL3, MATROW3, tr( "Modulation amount" ), "", "matrixKnob" )
	makeknob( m_vol2lfo2Knob, MATCOL4, MATROW3, tr( "Modulation amount" ), "", "matrixKnob" )

	makeknob( m_vol3env1Knob, MATCOL1, MATROW5, tr( "Modulation amount" ), "", "matrixKnob" )
	makeknob( m_vol3env2Knob, MATCOL2, MATROW5, tr( "Modulation amount" ), "", "matrixKnob" )
	makeknob( m_vol3lfo1Knob, MATCOL3, MATROW5, tr( "Modulation amount" ), "", "matrixKnob" )
	makeknob( m_vol3lfo2Knob, MATCOL4, MATROW5, tr( "Modulation amount" ), "", "matrixKnob" )

	makeknob( m_phs1env1Knob, MATCOL1, MATROW2, tr( "Modulation amount" ), "", "matrixKnob" )
	makeknob( m_phs1env2Knob, MATCOL2, MATROW2, tr( "Modulation amount" ), "", "matrixKnob" )
	makeknob( m_phs1lfo1Knob, MATCOL3, MATROW2, tr( "Modulation amount" ), "", "matrixKnob" )
	makeknob( m_phs1lfo2Knob, MATCOL4, MATROW2, tr( "Modulation amount" ), "", "matrixKnob" )

	makeknob( m_phs2env1Knob, MATCOL1, MATROW4, tr( "Modulation amount" ), "", "matrixKnob" )
	makeknob( m_phs2env2Knob, MATCOL2, MATROW4, tr( "Modulation amount" ), "", "matrixKnob" )
	makeknob( m_phs2lfo1Knob, MATCOL3, MATROW4, tr( "Modulation amount" ), "", "matrixKnob" )
	makeknob( m_phs2lfo2Knob, MATCOL4, MATROW4, tr( "Modulation amount" ), "", "matrixKnob" )

	makeknob( m_phs3env1Knob, MATCOL1, MATROW6, tr( "Modulation amount" ), "", "matrixKnob" )
	makeknob( m_phs3env2Knob, MATCOL2, MATROW6, tr( "Modulation amount" ), "", "matrixKnob" )
	makeknob( m_phs3lfo1Knob, MATCOL3, MATROW6, tr( "Modulation amount" ), "", "matrixKnob" )
	makeknob( m_phs3lfo2Knob, MATCOL4, MATROW6, tr( "Modulation amount" ), "", "matrixKnob" )

	makeknob( m_pit1env1Knob, MATCOL5, MATROW1, tr( "Modulation amount" ), "", "matrixKnob" )
	makeknob( m_pit1env2Knob, MATCOL6, MATROW1, tr( "Modulation amount" ), "", "matrixKnob" )
	makeknob( m_pit1lfo1Knob, MATCOL7, MATROW1, tr( "Modulation amount" ), "", "matrixKnob" )
	makeknob( m_pit1lfo2Knob, MATCOL8, MATROW1, tr( "Modulation amount" ), "", "matrixKnob" )

	makeknob( m_pit2env1Knob, MATCOL5, MATROW3, tr( "Modulation amount" ), "", "matrixKnob" )
	makeknob( m_pit2env2Knob, MATCOL6, MATROW3, tr( "Modulation amount" ), "", "matrixKnob" )
	makeknob( m_pit2lfo1Knob, MATCOL7, MATROW3, tr( "Modulation amount" ), "", "matrixKnob" )
	makeknob( m_pit2lfo2Knob, MATCOL8, MATROW3, tr( "Modulation amount" ), "", "matrixKnob" )

	makeknob( m_pit3env1Knob, MATCOL5, MATROW5, tr( "Modulation amount" ), "", "matrixKnob" )
	makeknob( m_pit3env2Knob, MATCOL6, MATROW5, tr( "Modulation amount" ), "", "matrixKnob" )
	makeknob( m_pit3lfo1Knob, MATCOL7, MATROW5, tr( "Modulation amount" ), "", "matrixKnob" )
	makeknob( m_pit3lfo2Knob, MATCOL8, MATROW5, tr( "Modulation amount" ), "", "matrixKnob" )

	makeknob( m_pw1env1Knob, MATCOL5, MATROW2, tr( "Modulation amount" ), "", "matrixKnob" )
	makeknob( m_pw1env2Knob, MATCOL6, MATROW2, tr( "Modulation amount" ), "", "matrixKnob" )
	makeknob( m_pw1lfo1Knob, MATCOL7, MATROW2, tr( "Modulation amount" ), "", "matrixKnob" )
	makeknob( m_pw1lfo2Knob, MATCOL8, MATROW2, tr( "Modulation amount" ), "", "matrixKnob" )

	makeknob( m_sub3env1Knob, MATCOL5, MATROW6, tr( "Modulation amount" ), "", "matrixKnob" )
	makeknob( m_sub3env2Knob, MATCOL6, MATROW6, tr( "Modulation amount" ), "", "matrixKnob" )
	makeknob( m_sub3lfo1Knob, MATCOL7, MATROW6, tr( "Modulation amount" ), "", "matrixKnob" )
	makeknob( m_sub3lfo2Knob, MATCOL8, MATROW6, tr( "Modulation amount" ), "", "matrixKnob" )

	return( view );
}


} // namespace gui


extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model *m, void * )
{
	return new MonstroInstrument( static_cast<InstrumentTrack *>( m ) );
}


}


} // namespace lmms
