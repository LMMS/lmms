/*
 * SynchroSynth.h - 2-oscillator PM synth
 *
 * Copyright (c) 2019 Ian Sannar <ian/dot/sannar/at/gmail/dot/com>
 * Credits to @DouglasDGI "Lost Robot" for performance optimizations
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
 */

#ifndef SYNCHROSYNTH_H
#define SYNCHROSYNTH_H

#include "lmms_constants.h"
#include "lmms_math.h"
#include "Instrument.h"
#include "InstrumentView.h"
#include "MemoryManager.h"
#include "Knob.h"
#include "Graph.h"

static const float MAGIC_HARMONICS[2][2] = {{32, 0.5}, {38, 0.025}};

class SynchroSynthView; //Additional definition to prevent errors, since SynchroInstrument references it

typedef struct SynchroOscillatorSettings
{
	float Detune;
	float Drive;
	float Sync;
	float Chop;
} SynchroOscillatorSettings;

class SynchroNote
{
	MM_OPERATORS
public:
	SynchroNote(NotePlayHandle * nph);
	virtual ~SynchroNote();
	void nextStringSample(sampleFrame &outputSample, sample_rate_t sample_rate,
		float modulationStrength, float modulationAmount, float harmonics,
		SynchroOscillatorSettings carrier,
		SynchroOscillatorSettings modulator);
private:
	NotePlayHandle * nph;
	float sample_index[2] = {0, 0}; //Index 0 is carrier, index 1 is modulator
};

class SynchroSynth : public Instrument
{
	Q_OBJECT
public:
	SynchroSynth(InstrumentTrack * instrument_track);
	virtual ~SynchroSynth();
	virtual void playNote(NotePlayHandle * n, sampleFrame * working_buffer);
	virtual void deleteNotePluginData(NotePlayHandle * n);
	virtual void saveSettings(QDomDocument & doc, QDomElement & parent);
	virtual void loadSettings(const QDomElement & thisElement);
	virtual QString nodeName() const;
	virtual f_cnt_t desiredReleaseFrames() const { return(64); } //Not sure what this is used for
	virtual PluginView * instantiateView(QWidget * parent);
	virtual Flags flags() const {	return IsSingleStreamed; } //Disables default envelopes/LFOs
protected slots:
void carrierChanged();
void modulatorChanged();
void generalChanged();
private:
	FloatModel m_harmonics;
	BoolModel m_useHarmonics;
	FloatModel m_modulationStrength;
	FloatModel m_modulation;

	FloatModel m_carrierDetune;
	FloatModel m_carrierDrive;
	FloatModel m_carrierSync;
	FloatModel m_carrierChop;
	FloatModel m_carrierAttack;
	FloatModel m_carrierDecay;
	FloatModel m_carrierSustain;
	FloatModel m_carrierRelease;

	FloatModel m_modulatorDetune;
	FloatModel m_modulatorDrive;
	FloatModel m_modulatorSync;
	FloatModel m_modulatorChop;
	FloatModel m_modulatorAttack;
	FloatModel m_modulatorDecay;
	FloatModel m_modulatorSustain;
	FloatModel m_modulatorRelease;

	graphModel m_carrierGraph;
	graphModel m_modulatorGraph;
	graphModel m_resultGraph;
	float m_carrierNormalize; //for later
	float m_modulatorNormalize;
	friend class SynchroSynthView;
};

class SynchroSynthView : public InstrumentView
{
	Q_OBJECT
public:
	SynchroSynthView(Instrument * instrument, QWidget * parent);
	QSize sizeHint() const override { return QSize(448, 250); }
	virtual ~SynchroSynthView() {};
protected slots:
private:
	virtual void modelChanged();
	Knob * m_harmonicsKnob;
	//led button
	Knob * m_modulationStrengthKnob;
	Knob * m_modulationKnob;

	Knob * m_carrierDetuneKnob;
	Knob * m_carrierDriveKnob;
	Knob * m_carrierSyncKnob;
	Knob * m_carrierChopKnob;
	Knob * m_carrierAttackKnob;
	Knob * m_carrierDecayKnob;
	Knob * m_carrierSustainKnob;
	Knob * m_carrierReleaseKnob;

	Knob * m_modulatorDetuneKnob;
	Knob * m_modulatorDriveKnob;
	Knob * m_modulatorSyncKnob;
	Knob * m_modulatorChopKnob;
	Knob * m_modulatorAttackKnob;
	Knob * m_modulatorDecayKnob;
	Knob * m_modulatorSustainKnob;
	Knob * m_modulatorReleaseKnob;

	Graph * m_carrierGraph;
	Graph * m_modulatorGraph;
	Graph * m_resultGraph;
	static QPixmap * s_artwork;
};

//Triangle waveform generator
static inline float tri(float x)
{
	return 2.0 * fabs(2.0 * ((x / F_2PI) - floorf((x / F_2PI) + 0.5))) - 1.0;
}

//Triangle waveform with harmonic generator
static inline float trih(float x, float harmonic)
{
	return tri(x) + ((tri(MAGIC_HARMONICS[0][0] * x) * MAGIC_HARMONICS[0][1]
		+ tri(MAGIC_HARMONICS[1][0] * x) * MAGIC_HARMONICS[1][1])) * harmonic;
}

//Waveform function for the Synchro synthesizer.
//x: input phase in radians (please keep it between 0 and 2PI)
//drive: how much atan distortion is applied to the waveform
//sync: hard sync with harmonic multiple
//chop: how strong the amplitude falloff is per waveform period
//harmonic: how strong the magic harmonics are
static inline float SynchroWaveform(float x, float drive, float sync,
	float chop, float harmonic)
{
	return tanhf(trih(F_PI_2 + x * sync, harmonic) * drive)
		/ powf(F_2PI / (F_2PI - x), chop);
}

static inline float DetuneCents(float pitch, float detune)
{
	return pitch * exp2(detune / 1200.0);
}

static inline float DetuneSemitones(float pitch, float detune)
{
	return pitch * exp2(detune / 12.0);
}

static inline float DetuneOctaves(float pitch, float detune)
{
	return pitch * exp2(detune);
}

#endif
