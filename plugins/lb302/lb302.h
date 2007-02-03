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

#include "instrument.h"
#include "led_checkbox.h"
#include "effect_lib.h"
#include <iostream>


class knob;
class notePlayHandle;

class lb302FilterState
{
    public:
    float cutoff;
    float reso;
    float envmod;
    float envdecay;
    float dist;
};

class lb302Filter
{
    public:
    lb302Filter(lb302FilterState* p_fs);
    virtual ~lb302Filter() {};

    virtual void recalc();
    virtual void envRecalc();
    virtual float process(const float& samp)=0;
    virtual void playNote();

    protected:
    lb302FilterState *fs;  
    
    // Filter Decay
	float vcf_c0;           // c0=e1 on retrigger; c0*=ed every sample; cutoff=e0+c0
	float vcf_e0,           // e0 and e1 for interpolation
          vcf_e1;           
    float vcf_rescoeff;     // Resonance coefficient [0.30,9.54]
};

class lb302FilterIIR2 : public lb302Filter
{
    public:
    lb302FilterIIR2(lb302FilterState* p_fs);

    virtual void recalc();
    virtual void envRecalc();
    virtual float process(const float& samp);

    protected:
    float vcf_d1,           //   d1 and d2 are added back into the sample with 
          vcf_d2;           //   vcf_a and b as coefficients. IIR2 resonance
                            //   loop.

    // IIR2 Coefficients for mixing dry and delay.
    float vcf_a,            //   Mixing coefficients for the final sound.  
          vcf_b,            //  
          vcf_c;
    
	effectLib::monoToStereoAdaptor<effectLib::distortion<> > * m_dist_fx;
	effectLib::distortion<> * m_dist;
};

class lb302Filter3Pole : public lb302Filter
{
    public:
    lb302Filter3Pole(lb302FilterState* p_fs);

    //virtual void recalc();
    virtual void envRecalc();
    virtual void recalc();
    virtual float process(const float& samp);

    protected:
    float kfcn, 
          kp, 
          kp1, 
          kp1h, 
          kres;
    float ay1, 
          ay2, 
          aout, 
          lastin, 
          value;
};


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

    virtual bool isMonophonic(void) const {
        return true;
    }

private:


private:
	knob * vcf_cut_knob;
	knob * vcf_res_knob;
    knob * vcf_dec_knob;
	knob * vcf_mod_knob;

    knob * vco_fine_detune_knob;

    knob * dist_knob;
    knob * wave_knob;
    
    ledCheckBox * slideToggle;
    ledCheckBox * accentToggle;
    ledCheckBox * deadToggle;
    ledCheckBox * db24Toggle;

    knob * slide_dec_knob;

public slots:
    void filterChanged(float);
    void detuneChanged(float);
    void waveChanged(float);
    void db24Toggled( bool );

private:

    

private:
    // Oscillator
    float vco_inc,          // Sample increment for the frequency. Creates Sawtooth.
          vco_k,            // Raw oscillator sample [-0.5,0.5]
          vco_c;            // Raw oscillator sample [-0.5,0.5]

    float vco_slide,        //* Current value of slide exponential curve. Nonzero=sliding
          vco_slideinc,     //* Slide base to use in next node. Nonzero=slide next note
          vco_slidebase;    //* The base vco_inc while sliding.

    float vco_detune;

    enum  vco_shape_t { SAWTOOTH, INVERTED_SAWTOOTH, SQUARE, TRIANGLE, MOOG, ROUND_SQUARE };
    vco_shape_t vco_shape;

    // User settings
    lb302FilterState  fs;
    lb302Filter       *vcf;
    
  	
    float lastFramesPlayed;

	
    // More States
    int   vcf_envpos;       // Update counter. Updates when >= ENVINC

	float vca_attack,       // Amp attack 
          vca_decay,        // Amp decay
          vca_a0,           // Initial amplifier coefficient 
          vca_a;            // Amplifier coefficient.
    
    // Envelope State
	int   vca_mode;         // 0: attack, 1: decay, 2: idle

    // My hacks
    int   sample_cnt;

    void recalcFilter();

    int process(sampleFrame *outbuf, const Uint32 size);

} ;


#endif
