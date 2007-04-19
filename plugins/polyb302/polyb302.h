/*
 * polyb302.h - declaration of instrument polyb302, an attempt to emulate the
 *              Roland TB303 bass synth
 *
 * Copyright (c) 2006-2007 Paul Giblock <pgib/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
 *
 * lb302FilterIIR2 is based on the gsyn filter code by Andy Sloane.
 * 
 * lb302Filter3Pole is based on the TB303 instrument written by 
 *   Josep M Comajuncosas for the CSounds library
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


#ifndef _POLYB302_H_
#define _POLYB302_H_

#include "instrument.h"
#include "effect_lib.h"


class knob;
class ledCheckBox;
class notePlayHandle;


typedef struct
{
	float cutoff;
	float reso;
	float envmod;
	float envdecay;
	float dist;
} lb302FilterState;




class lb302Filter
{
public:
	lb302Filter( lb302FilterState * _p_fs );
	virtual ~lb302Filter() {};

	virtual void recalc( void );
	virtual void envRecalc( void );
	virtual float process( const float & _samp ) = 0;
	virtual void playNote( void );


protected:
	lb302FilterState * m_fs;  
    
	// Filter Decay
	float m_vcf_c0;	// c0=e1 on retrigger; c0*=ed every sample; cutoff=e0+c0
	float m_vcf_e0;	// e0 and e1 for interpolation
	float m_vcf_e1;
	float m_vcf_rescoeff;	// Resonance coefficient [0.30,9.54]

};




class lb302FilterIIR2 : public lb302Filter
{
public:
	lb302FilterIIR2( lb302FilterState * _p_fs );
	virtual ~lb302FilterIIR2();

	virtual void recalc( void );
	virtual void envRecalc( void );
	virtual float process( const float & _samp );


protected:
	float m_vcf_d1;	// d1 and d2 are added back into the sample with 
        float m_vcf_d2;	// vcf_a and b as coefficients. IIR2 resonance
			// loop.

	// IIR2 Coefficients for mixing dry and delay.
	float m_vcf_a;	// Mixing coefficients for the final sound.
	float m_vcf_b;
	float m_vcf_c;

	effectLib::distortion<> * m_dist;

};




class lb302Filter3Pole : public lb302Filter
{
public:
	lb302Filter3Pole( lb302FilterState * _p_fs );

	virtual void envRecalc( void );
	virtual void recalc( void );
	virtual float process( const float & _samp );


protected:
	float m_kfcn,
		m_kp,
		m_kp1,
		m_kp1h,
		m_kres;
	float m_ay1,
		m_ay2,
		m_aout,
		m_lastin,
		m_value;
};




class polyb302Synth : public instrument
{
	Q_OBJECT
public:
	polyb302Synth( instrumentTrack * _track );
	virtual ~polyb302Synth();

	virtual void FASTCALL playNote( notePlayHandle * _n,
						bool _try_parallelizing );
	virtual void FASTCALL deleteNotePluginData( notePlayHandle * _n );


	virtual void FASTCALL saveSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void FASTCALL loadSettings( const QDomElement & _this );

	virtual QString nodeName( void ) const;


public slots:
	void db24Toggled( bool );
	void detuneChanged( float );
	void filterChanged( float );
	void waveChanged( float );


private:
	class handleState
	{
	public:
		handleState( polyb302Synth * _synth );
		virtual ~handleState();

		enum vco_shape_t {
			SAWTOOTH, INVERTED_SAWTOOTH, SQUARE, TRIANGLE, MOOG,
			ROUND_SQUARE
		};

		// Oscillator
		// Sample increment for the frequency. Creates Sawtooth.
		float m_vco_inc;
		// Raw oscillator sample [-0.5,0.5]
		float m_vco_k;
		// Raw oscillator sample [-0.5,0.5]
		float m_vco_c;

		// Current value of slide exponential curve. Nonzero=sliding
		float m_vco_slide;
		// Slide base to use in next node. Nonzero=slide next note
		float m_vco_slideinc;
		// The base vco_inc while sliding.
		float m_vco_slidebase;

		float m_vco_detune;

		vco_shape_t m_vco_shape;

		// User settings
		lb302FilterState  m_fs;
		lb302Filter * m_vcf;

		float m_lastFramesPlayed;

		// More States
		// Update counter. Updates when >= ENVINC
		int m_vcf_envpos;

		float m_vca_attack;	// Amp attack 
		float m_vca_decay;	// Amp decay
		float m_vca_a0;		// Initial amplifier coefficient 
		float m_vca_a;		// Amplifier coefficient.

		// Envelope State
		int m_vca_mode;		// 0: attack, 1: decay, 2: idle

		// My hacks
		int m_sample_cnt;

		// TODO: split synth slots
		polyb302Synth * m_synth;

		void recalcFilter( void );

		void process( sampleFrame * _outbuf, const Uint32 _size,
								float _freq );

		void db24Toggled( void );
		void detuneChanged( void );
		void filterChanged( void );

	} ;


	knob * m_vcf_cut_knob;
	knob * m_vcf_res_knob;
	knob * m_vcf_dec_knob;
	knob * m_vcf_mod_knob;

	knob * m_vco_fine_detune_knob;

	knob * m_dist_knob;
	knob * m_wave_knob;
    
	ledCheckBox * m_slideToggle;
//	ledCheckBox * m_accentToggle;
//	ledCheckBox * m_deadToggle;
	ledCheckBox * m_db24Toggle;

	knob * m_slide_dec_knob;

	vlist<handleState *> m_handleStates;

} ;


#endif
