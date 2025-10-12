/*
 * Lb302.h - declaration of class Lb302 which is a bass synth attempting to
 *           emulate the Roland TB-303 bass synth
 *
 * Copyright (c) 2006-2008 Paul Giblock <pgib/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
 *
 * Lb302FilterIIR2 is based on the gsyn filter code by Andy Sloane.
 *
 * Lb302Filter3Pole is based on the TB-303 instrument written by
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


#ifndef LB302_H
#define LB302_H

#include "Instrument.h"
#include "InstrumentView.h"
#include "NotePlayHandle.h"
#include <QMutex>

namespace lmms
{


static const int NUM_FILTERS = 2;


namespace DspEffectLibrary
{
class Distortion;
}

namespace gui
{
class AutomatableButtonGroup;
class Knob;
class Lb302SynthView;
class LedCheckBox;
}


class Lb302FilterKnobState
{
	public:
	float cutoff;
	float reso;
	float envmod;
	float envdecay;
	float dist;
};


class Lb302Filter
{
	public:
	Lb302Filter(Lb302FilterKnobState* p_fs);
	virtual ~Lb302Filter() = default;

	virtual void recalc();
	virtual void envRecalc();
	virtual float process(const float& samp)=0;
	virtual void playNote();

	protected:
	Lb302FilterKnobState *fs;

	// Filter Decay
	float vcf_c0;           // c0=e1 on retrigger; c0*=ed every sample; cutoff=e0+c0
	float vcf_e0,           // e0 and e1 for interpolation
	      vcf_e1;
	float vcf_rescoeff;     // Resonance coefficient [0.30,9.54]
};

class Lb302FilterIIR2 : public Lb302Filter
{
	public:
	Lb302FilterIIR2(Lb302FilterKnobState* p_fs);
	~Lb302FilterIIR2() override;

	void recalc() override;
	void envRecalc() override;
	float process(const float& samp) override;

	protected:
	float vcf_d1,           //   d1 and d2 are added back into the sample with
	      vcf_d2;           //   vcf_a and b as coefficients. IIR2 resonance
	                        //   loop.

	                        // IIR2 Coefficients for mixing dry and delay.
	float vcf_a,            //   Mixing coefficients for the final sound.
	      vcf_b,            //
	      vcf_c;

	DspEffectLibrary::Distortion * m_dist;
};


class Lb302Filter3Pole : public Lb302Filter
{
	public:
	Lb302Filter3Pole(Lb302FilterKnobState* p_fs);

	//virtual void recalc();
	void envRecalc() override;
	void recalc() override;
	float process(const float& samp) override;

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



class Lb302Note
{
public:
	float vco_inc;
	bool dead;
};


class Lb302Synth : public Instrument
{
	Q_OBJECT
public:
	Lb302Synth( InstrumentTrack * _instrument_track );
	~Lb302Synth() override;

	void play( SampleFrame* _working_buffer ) override;
	void playNote( NotePlayHandle * _n,
						SampleFrame* _working_buffer ) override;
	void deleteNotePluginData( NotePlayHandle * _n ) override;


	void saveSettings( QDomDocument & _doc, QDomElement & _parent ) override;
	void loadSettings( const QDomElement & _this ) override;

	QString nodeName() const override;

	gui::PluginView* instantiateView( QWidget * _parent ) override;

private:
	void processNote( NotePlayHandle * n );

	void initNote(Lb302Note *Note);
	void initSlide();

private:
	FloatModel vcf_cut_knob;
	FloatModel vcf_res_knob;
	FloatModel vcf_mod_knob;
	FloatModel vcf_dec_knob;

	FloatModel vco_fine_detune_knob;

	FloatModel dist_knob;
	IntModel wave_shape;
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

	enum class VcoShape { Sawtooth, Square, Triangle, Moog, RoundSquare, Sine, Exponential, WhiteNoise,
							BLSawtooth, BLSquare, BLTriangle, BLMoog };
	VcoShape vco_shape;

	// Filters (just keep both loaded and switch)
	Lb302Filter* vcfs[NUM_FILTERS];

	// User settings
	Lb302FilterKnobState fs;
	QAtomicPointer<Lb302Filter> vcf;

	size_t release_frame;

	// More States
	int   vcf_envpos;       // Update counter. Updates when >= ENVINC

	float vca_attack,       // Amp attack
	      vca_a0,           // Initial amplifier coefficient
	      vca_a;            // Amplifier coefficient.

	// Envelope State
	enum class VcaMode
	{
		Attack = 0,
		Decay = 1,
		Idle = 2,
		NeverPlayed = 3
	};
	VcaMode vca_mode;

	// My hacks
	int   sample_cnt;

	int   last_offset;

	int catch_frame;
	int catch_decay;

	bool new_freq;
	float true_freq;

	void recalcFilter();

	int process(SampleFrame* outbuf, const std::size_t size);

	friend class gui::Lb302SynthView;

	NotePlayHandle * m_playingNote;
	NotePlayHandleList m_notes;
	QMutex m_notesMutex;
} ;


namespace gui
{


class Lb302SynthView : public InstrumentViewFixedSize
{
	Q_OBJECT
public:
	Lb302SynthView( Instrument * _instrument,
	                QWidget * _parent );
	~Lb302SynthView() override = default;

private:
	void modelChanged() override;

	Knob * m_vcfCutKnob;
	Knob * m_vcfResKnob;
	Knob * m_vcfDecKnob;
	Knob * m_vcfModKnob;

	Knob * m_distKnob;
	Knob * m_slideDecKnob;
	AutomatableButtonGroup * m_waveBtnGrp;

	LedCheckBox * m_slideToggle;
	/*LedCheckBox * m_accentToggle;*/ // removed pending accent implementation
	LedCheckBox * m_deadToggle;
	LedCheckBox * m_db24Toggle;

} ;


} // namespace gui

} // namespace lmms

#endif // LB302_H
