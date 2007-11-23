/*
 * lb302.h - declaration of class lb302 which is a bass synth attempting to
 *           emulate the Roland TB303 bass synth
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


#ifndef _LB302_H_
#define _LB302_H_

#include "effect_lib.h"
#include "instrument.h"
#include "led_checkbox.h"
#include "mixer.h"


class knob;
class notePlayHandle;


// lb302FilterKnobState
// Bridges the lb302 plugin and it's active filter.
struct lb302FilterKnobState
{
public:
	float cutoff;
	float reso;
	float envmod;
	float envdecay;
	float dist;
};


// lb302FilterIIR2State
// State of the IIR2 filter, separated for achieving gapless playback
struct lb302FilterIIR2State
{
public:
	float c0;
	float a;
	float b;
	float c;
	float d1;
	float d2;
};


// lb302Filter3PoleState
// State of the 3-Pole filter, separated for achieving gapless playback
struct lb302Filter3PoleState
{
public:
	float c0;
	float kp;
	float kp1h;
	float kres;
	float ay1;
	float ay2;
	float lastin;
	float value;
	float aout;
};


// lb302FilterState
// Captured state of the active filter. Allows lb302State to generically
// hold on to one state or the other, without caring.
typedef union
{
	lb302FilterIIR2State  iir;
	lb302Filter3PoleState pole;
}
lb302FilterState;


// lb302Filter
// Used to filter the lb302.  Uses FilterKnobState pointer as input
class lb302Filter
{
public:
	lb302Filter( lb302FilterKnobState* _fs );
	virtual ~lb302Filter( void ) {};

	virtual void  recalc( void );
	virtual void  envRecalc( void );
	virtual float process( const float & _samp ) = 0;
	virtual void  playNote( void );

	virtual void  getState( lb302FilterState * _fs ) = 0;
	virtual void  setState( const lb302FilterState * _fs ) = 0;

protected:
	lb302FilterKnobState *m_fs;

	// Filter Decay
	float m_c0;         // c0=e1 on retrigger; c0*=ed every sample;
	float m_e0;         // e0 and e1 for interpolation
	float m_e1;
	float m_rescoeff;   // Resonance coefficient [0.30,9.54]
};


// lb302FilterIIR2
// The IIR2 filter implementation
class lb302FilterIIR2 : public lb302Filter
{
public:
	lb302FilterIIR2( lb302FilterKnobState * _fs );
	virtual ~lb302FilterIIR2( void );

	virtual void  recalc( void );
	virtual void  envRecalc( void );
	virtual float process( const float & _samp );

	virtual void  getState( lb302FilterState * _fs );
	virtual void  setState( const lb302FilterState * _fs );

protected:
	float m_d1;             // d1 and d2 are added back into the sample with
	float m_d2;             // vcf_a and b as coefficients. IIR2 resonance
	                        // loop.

	// IIR2 Coefficients for mixing dry and delay.
	float m_a;
	float m_b;
	float m_c;

	effectLib::monoToStereoAdaptor< effectLib::distortion<> > * m_dist_fx;
	effectLib::distortion<> * m_dist;
};


// lb302Filter3Pole
// The 3-pole filter implementation
class lb302Filter3Pole : public lb302Filter
{
public:
	lb302Filter3Pole(lb302FilterKnobState * _fs);

	virtual void  envRecalc( void );
	virtual void  recalc( void );
	virtual float process( const float & _samp );

	virtual void  getState( lb302FilterState * _fs );
	virtual void  setState( const lb302FilterState * _fs );

protected:
	float m_kfcn;
	float m_kp;
	float m_kp1;
	float m_kp1h;
	float m_kres;

	float m_ay1;
	float m_ay2;
	float m_aout;

	float m_lastin;
	float m_value;
};


// lb302State
// State of the VCA and pointer to VCF state.  Used with period states 
// in lb302Synth to provide gapless, smooth playback
struct lb302State
{
public:
	float vco_c;
	float vca_a;
	int   vca_mode;
	int   sampleCnt;

	lb302FilterState fs;
};


// lb302Note
// Description of a note used in lb302: frequency and deadness.  Struct 
// exists to allow reuse of code contained in initNote().
struct lb302Note
{
public:
	float vco_inc;
	bool  dead;
};


// lb302Synth
// Here it is, lb302Synth in all its glory.
class lb302Synth : public instrument
{
	Q_OBJECT

public:
	lb302Synth( instrumentTrack * _channel_track );
	virtual ~lb302Synth();

	virtual void FASTCALL playNote( notePlayHandle * _n,
	                                bool _try_parallelizing );
	virtual void FASTCALL deleteNotePluginData( notePlayHandle * _n );


	virtual void FASTCALL saveSettings( QDomDocument & _doc,
	                                    QDomElement & _parent );
	virtual void FASTCALL loadSettings( const QDomElement & _this );

	virtual QString nodeName( void ) const;

	virtual bool isMonophonic( void ) const {
		return true;
	}

	virtual f_cnt_t desiredReleaseFrames( void ) const
	{
		return 4048;
	}

public slots:
	void filterChanged( float );
	void detuneChanged( float );
	void waveChanged( float );
	void db24Toggled( bool );

private:

	void initNote( lb302Note * note );

	knob * m_vcfCutKnob;
	knob * m_vcfResKnob;
	knob * m_vcfDecKnob;
	knob * m_vcfModKnob;

	knob * m_vcoFineDetuneKnob;

	knob * m_distKnob;
	knob * m_waveKnob;

	knob * m_slideDecKnob;

	ledCheckBox * m_slideToggle;
	ledCheckBox * m_accentToggle;
	ledCheckBox * m_deadToggle;
	ledCheckBox * m_db24Toggle;

private:
	void recalcFilter( void );

	int process( sampleFrame *outbuf, const Uint32 size );

	// Oscillator
	enum  vcoShape { SAWTOOTH, INVERTED_SAWTOOTH, SQUARE, TRIANGLE, MOOG, ROUND_SQUARE };
	vcoShape m_vco_shape;

	float m_vco_slide;        //* Current value of slide exponential curve. Nonzero=sliding
	float m_vco_slideinc;     //* Slide base to use in next node. Nonzero=slide next note
	float m_vco_slidebase;    //* The base vco_inc while sliding.

	float m_vco_detune;
	float m_vco_inc;          // Sample increment for the frequency. Creates Sawtooth.
	float m_vco_k;            // Raw oscillator sample [-0.5,0.5]
	float m_vco_c;            // Raw oscillator sample [-0.5,0.5]

	// More States
	int   m_vcf_envpos;       // Update counter. Updates when >= ENVINC

	// Envelope State
	float m_vca_attack;       // Amp attack 
	float m_vca_decay;        // Amp decay
	float m_vca_a0;           // Initial amplifier coefficient 
	float m_vca_a;            // Amplifier coefficient.

	int   m_vca_mode;         // 0: attack, 1: decay, 2: idle

	// User settings
	lb302FilterKnobState m_fs;
	lb302Filter * m_vcf;
	lb302Note m_holdNote;

	bool m_useHoldNote;

	int m_sampleCnt;
	int m_releaseFrame;

	int m_catchFrame;
	int m_catchDecay;

	int m_lastFramesPlayed;
	int m_lastOffset;

	lb302State * m_periodStates;
	int m_periodStatesCnt;

};


#endif
