/*
* growl.h - 2-oscillator PM synth
*
* Copyright (c) 2019 Ian Sannar <ian/dot/sannar/at/gmail/dot/com>
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
#define ONE_PERCENT 0.01f

#include "lmms_constants.h"
#include "lmms_math.h"
#include "Instrument.h"
#include "InstrumentView.h"
#include "MemoryManager.h"
#include "Knob.h"
#include "Graph.h"

static const double MAGIC_HARMONICS[2][2] = {{32, 0.5}, {38, 0.025}};

class SynchroSynthView; //Additional definition to prevent errors, since SynchroInstrument references it

typedef struct SynchroOscillatorSettings {
	double Detune;
	double Drive;
	double Sync;
	double Chop;
} SynchroOscillatorSettings;

class SynchroNote {
	MM_OPERATORS
public:
	SynchroNote(NotePlayHandle * nph, sample_rate_t sample_rate);
	virtual ~SynchroNote();
	void nextStringSample(sampleFrame &outputSample, float modulationStrength, float modulationAmount, float harmonics, SynchroOscillatorSettings carrier, SynchroOscillatorSettings modulator);
private:
	NotePlayHandle * nph;
	double sample_index[2] = {0, 0}; //Index 0 is carrier, index 1 is modulator
	const sample_rate_t sample_rate;
};

class SynchroSynth : public Instrument {
	Q_OBJECT
public:
	SynchroSynth(InstrumentTrack * instrument_track);
	virtual ~SynchroSynth();
	virtual void playNote(NotePlayHandle * n, sampleFrame * working_buffer);
	virtual void deleteNotePluginData(NotePlayHandle * n);
	virtual void saveSettings(QDomDocument & doc, QDomElement & parent);
	virtual void loadSettings(const QDomElement & thisElement);
	virtual QString nodeName() const;
	virtual f_cnt_t desiredReleaseFrames() const { return( 64 ); }
	virtual PluginView * instantiateView(QWidget * parent);
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
	FloatModel m_modulatorDetune;
	FloatModel m_modulatorDrive;
	FloatModel m_modulatorSync;
	FloatModel m_modulatorChop;
	graphModel m_carrierView;
	graphModel m_modulatorView;
	graphModel m_resultView;
	friend class SynchroSynthView;
};

class SynchroSynthView : public InstrumentView
{
	Q_OBJECT
public:
	SynchroSynthView(Instrument * instrument, QWidget * parent);
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
	Knob * m_modulatorDetuneKnob;
	Knob * m_modulatorDriveKnob;
	Knob * m_modulatorSyncKnob;
	Knob * m_modulatorChopKnob;
	Graph * m_carrierGraph;
	Graph * m_modulatorGraph;
	Graph * m_resultGraph;
	static QPixmap * s_artwork;
};

//Triangle waveform generator
static inline double tri(double x) {
	return 2.0 * fabs(2.0 * ((x / D_2PI) - floorf((x / D_2PI) + 0.5))) - 1.0;
}

//Triangle waveform with harmonic generator
static inline double trih(double x, double harmonic) {
	return tri(x) + (tri(MAGIC_HARMONICS[0][0] * x) * MAGIC_HARMONICS[0][1] + tri(MAGIC_HARMONICS[1][0] * x) * MAGIC_HARMONICS[1][1]) * harmonic;
}

//Waveform function for the Synchro synthesizer.
//x: input phase in radians
//drive: how much atan distortion is applied to the waveform
//sync: hard sync with harmonic multiple
//chop: how strong the amplitude falloff is per waveform period
//harmonic: how strong the magic harmonic is
static inline double SynchroWaveform(double x, double drive, double sync, double chop, double harmonic) {
	return (atan2(trih(D_PI_2 + x * sync, harmonic) * drive, 1) / atan2(drive, 1)) / powf(D_2PI / (D_2PI - x), chop);
}

static inline double DetuneCents(float pitch, float detune) {
	return pitch * exp2(detune / 1200.0);
}

static inline double DetuneSemitones(float pitch, float detune) {
	return pitch * exp2(detune / 12.0);
}

static inline double DetuneOctaves(float pitch, float detune) {
	return pitch * exp2(detune);
}

#endif
