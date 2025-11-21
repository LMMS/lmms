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

#include <array>
#include <memory>

#include <QMutex>

#include "Instrument.h"
#include "InstrumentView.h"
#include "NotePlayHandle.h"

namespace lmms
{


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


struct Lb302FilterKnobState
{
	float cutoff;
	float reso;
	float envmod;
	float envdecay;
	float dist;
};


class Lb302Filter
{
public:
	Lb302Filter(Lb302FilterKnobState* p_fs) : fs{p_fs} {};
	virtual ~Lb302Filter() = default;

	virtual void recalc();
	virtual void envRecalc();
	virtual sample_t process(const sample_t& samp) = 0;
	virtual void playNote();

protected:
	Lb302FilterKnobState *fs;

	// Filter Decay
	float vcf_c0 = 0.f; // c0=e1 on retrigger; c0*=ed every sample; cutoff=e0+c0
	float vcf_e0 = 0.f; // e0 and e1 for interpolation
	float vcf_e1 = 0.f;
	float vcf_rescoeff; // Resonance coefficient [0.30,9.54]
};

class Lb302FilterIIR2 : public Lb302Filter
{
public:
	Lb302FilterIIR2(Lb302FilterKnobState* p_fs);

	void recalc() override;
	void envRecalc() override;
	sample_t process(const sample_t& samp) override;

protected:
	// d1 and d2 are added back into the sample with vcf_a and b as
	// coefficients. IIR2 resonance loop.
	float vcf_d1 = 0.f;
	float vcf_d2 = 0.f;

	// IIR2 Coefficients for mixing dry and delay.
	// Mixing coefficients for the final sound.
	float vcf_a = 0.f;
	float vcf_b = 0.f;
	float vcf_c = 1.f;

	std::unique_ptr<DspEffectLibrary::Distortion> m_dist;
};


class Lb302Filter3Pole : public Lb302Filter
{
public:
	Lb302Filter3Pole(Lb302FilterKnobState* p_fs) : Lb302Filter(p_fs) {};

	//virtual void recalc();
	void envRecalc() override;
	void recalc() override;
	sample_t process(const sample_t& samp) override;

protected:
	static constexpr float VOL_ADJUST = 3.f;
	float kfcn;
	float kp;
	float kp1;
	float kp1h;
	float kres;
	float ay1 = 0.f;
	float ay2 = 0.f;
	float aout = 0.f;
	float lastin = 0.f;
	float value;
};




class Lb302Synth : public Instrument
{
	Q_OBJECT
	friend class gui::Lb302SynthView;

public:
	Lb302Synth(InstrumentTrack*);
	void play(SampleFrame* working_buffer) override;
	void playNote(NotePlayHandle* nph, SampleFrame* working_buffer) override;
	void deleteNotePluginData(NotePlayHandle* nph) override;
	void saveSettings(QDomDocument& doc, QDomElement& el) override;
	void loadSettings(const QDomElement& el) override;
	QString nodeName() const override;
	gui::PluginView* instantiateView(QWidget* parent) override;

public slots:
	void filterChanged();
	void db24Toggled();

private:
	void processNote(NotePlayHandle* nph);
	void process(SampleFrame* outbuf, const fpp_t size);
	void initNote(float noteVcoInc, bool noteIsDead);
	void initSlide();
	void recalcFilter();

	enum class VcoShape { Sawtooth, Square, Triangle, Moog, RoundSquare, Sine, Exponential, WhiteNoise,
		BLSawtooth, BLSquare, BLTriangle, BLMoog };
	enum class VcaMode { Attack, Decay, Idle, NeverPlayed };

	static constexpr float DIST_RATIO = 4.f;
	static constexpr fpp_t ENVINC = 64; //* Envelope Recalculation period
	static constexpr float vca_attack = 1.f - 0.96406088f; // Amp attack
	static constexpr float vca_a0 = 0.5f; // Initial amplifier coefficient

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

	// Oscillator
	float vco_inc = 0.f; // Sample increment for the frequency. Creates Sawtooth.
	float vco_k = 0.f;   // Raw oscillator sample [-0.5,0.5]
	float vco_c = 0.f;   // Raw oscillator sample [-0.5,0.5]

	float vco_slide = 0.f;     //* Current value of slide exponential curve. Nonzero=sliding
	float vco_slideinc = 0.f;  //* Slide base to use in next node. Nonzero=slide next note
	float vco_slidebase = 0.f; //* The base vco_inc while sliding.

	VcoShape vco_shape = VcoShape::BLSawtooth;

	// User settings
	Lb302FilterKnobState fs = {};

	// Filters (just keep both loaded and switch)
	std::array<std::unique_ptr<Lb302Filter>, 2> vcfs;
	inline Lb302Filter& vcf() { return *vcfs[db24Toggle.value()]; } // Helper to get current vcf
	f_cnt_t vcf_envpos = ENVINC; // Update counter. Updates when >= ENVINC
	f_cnt_t release_frame;

	// Envelope State
	float vca_a = 0.f; // Amplifier coefficient.
	VcaMode vca_mode = VcaMode::NeverPlayed;

	// My hacks
	f_cnt_t sample_cnt = 0;
	// f_cnt_t catch_decay = 0;

	bool new_freq = false;
	float true_freq;

	NotePlayHandle* m_playingNote;
	NotePlayHandleList m_notes;
	QMutex m_notesMutex;
};


namespace gui
{


class Lb302SynthView : public InstrumentViewFixedSize
{
	Q_OBJECT

public:
	Lb302SynthView(Instrument* instrument, QWidget* parent);
	~Lb302SynthView() override = default;

private:
	void modelChanged() override;

	Knob* m_vcfCutKnob;
	Knob* m_vcfResKnob;
	Knob* m_vcfDecKnob;
	Knob* m_vcfModKnob;

	Knob* m_distKnob;
	Knob* m_slideDecKnob;
	AutomatableButtonGroup* m_waveBtnGrp;

	LedCheckBox* m_slideToggle;
	// LedCheckBox* m_accentToggle; // TODO: implement accent notes
	LedCheckBox* m_deadToggle;
	LedCheckBox* m_db24Toggle;
};


} // namespace gui

} // namespace lmms

#endif // LB302_H
