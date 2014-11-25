/*
 * lb303.h - declaration of class lb303 which is a bass synth attempting to
 *           emulate the Roland TB303 bass synth
 *
 * Copyright (c) 2006-2008 Paul Giblock <pgib/at/users.sourceforge.net>
 * 
 * This file is part of LMMS - http://lmms.io
 *
 * lb303FilterIIR2 is based on the gsyn filter code by Andy Sloane.
 * 
 * lb303Filter3Pole is based on the TB303 instrument written by 
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


#ifndef _LB303_H_
#define _LB303_H_

#include "effect_lib.h"
#include "Instrument.h"
#include "InstrumentView.h"
#include "led_checkbox.h"
#include "knob.h"
#include "Mixer.h"

class lb303SynthView;
class NotePlayHandle;

class lb303FilterKnobState
{
	public:
	float cutoff;
	float reso;
	float envmod;
	float envdecay;
	float dist;
};


class lb303Filter
{
	public:
	lb303Filter(lb303FilterKnobState* p_fs);
	virtual ~lb303Filter() {};

	virtual void recalc();
	virtual void envRecalc();
	virtual float process(const float& samp)=0;
	virtual void playNote();

	protected:
	lb303FilterKnobState *fs;  

	// Filter Decay
	float vcf_c0;           // c0=e1 on retrigger; c0*=ed every sample; cutoff=e0+c0
	float vcf_e0,           // e0 and e1 for interpolation
	      vcf_e1;           
	float vcf_rescoeff;     // Resonance coefficient [0.30,9.54]
};

class lb303FilterIIR2 : public lb303Filter
{
	public:
	lb303FilterIIR2(lb303FilterKnobState* p_fs);
	virtual ~lb303FilterIIR2();

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


class lb303Filter3Pole : public lb303Filter
{
	public:
	lb303Filter3Pole(lb303FilterKnobState* p_fs);

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



class lb303Note
{
public:
	float vco_inc;
	bool dead;
};


class lb303Synth : public Instrument
{
	Q_OBJECT
public:
	lb303Synth( InstrumentTrack * _instrument_track );
	virtual ~lb303Synth();

	virtual void play( sampleFrame * _working_buffer );
	virtual void playNote( NotePlayHandle * _n,
						sampleFrame * _working_buffer );
	virtual void deleteNotePluginData( NotePlayHandle * _n );


	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	virtual QString nodeName() const;

	virtual f_cnt_t desiredReleaseFrames() const
	{
		return 0; //4048;
	}

	virtual PluginView * instantiateView( QWidget * _parent );

private:

	void initNote(lb303Note *note);


private:
	FloatModel vcf_cut_knob;
	FloatModel vcf_res_knob;
	FloatModel vcf_mod_knob;
	FloatModel vcf_dec_knob;

	FloatModel vco_fine_detune_knob;

	FloatModel dist_knob;
	FloatModel wave_knob;
	FloatModel slide_dec_knob;
    
	BoolModel slideToggle;
	BoolModel accentToggle;
	BoolModel deadToggle;
	BoolModel db24Toggle;


public slots:
	void filterChanged();
	void db24Toggled();

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
	lb303FilterKnobState fs;
	lb303Filter *vcf;

	int release_frame;


	// More States
	int   vcf_envpos;       // Update counter. Updates when >= ENVINC

	float vca_attack,       // Amp attack 
	      vca_decay,        // Amp decay
	      vca_a0,           // Initial amplifier coefficient 
	      vca_a;            // Amplifier coefficient.

	// Envelope State
	int   vca_mode;         // 0: attack, 1: decay, 2: idle, 3: never played

	// My hacks
	int   sample_cnt;

	int   last_offset;

	int catch_frame;
	int catch_decay;

	float new_freq;
	float current_freq;
	float delete_freq;
	float true_freq;

	void recalcFilter();

	int process(sampleFrame *outbuf, const Uint32 size);

	friend class lb303SynthView;

} ;


class lb303SynthView : public InstrumentView
{
	Q_OBJECT
public:
	lb303SynthView( Instrument * _instrument,
	                QWidget * _parent );
	virtual ~lb303SynthView();

private:
	virtual void modelChanged();
	
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

} ;

#endif
