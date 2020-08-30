/*
 * SynchroSynth.h - 2-oscillator PM synth
 *
 * Copyright (c) 2020 Ian Sannar <ian/dot/sannar/at/gmail/dot/com>
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

#include "Graph.h" //Required for the waveform view
#include "Instrument.h" //Required for the synth
#include "InstrumentView.h" //Required for instrument GUI
#include "Knob.h" //Required to use knobs
#include "NotePlayHandle.h" //Required for audio rendering

constexpr char SYNCHRO_VERSION [] = "0.6";
constexpr float SYNCHRO_VOLUME_CONST = 0.15f; //Prevents clipping
constexpr int SYNCHRO_OVERSAMPLE = 4; //Anti-aliasing samples
constexpr int SYNCHRO_PM_CONST = 20; //Strength of the phase modulation
constexpr int SYNCHRO_GRAPH_SAMPLES = 48; //Resolution of the waveform view
constexpr float MAGIC_HARMONICS[2][2] = {{32, 0.5}, {38, 0.025}}; //Yoioioi

//Used to pass the oscillator settings from the GUI to the synth itself
struct SynchroOscillatorSettings
{
	float Detune; //Detune in octaves
	float Drive; //Saturation amount
	float Sync; //Hard-sync frequency multiplier
	float Chop; //Per-period waveform damping
	float Attack; //Amplitude attack time (fade-in)
	float Decay; //Amplitude decay time (fade-out to sustain value)
	float Sustain; //Amplitude sustain value
	float Release; //Amplitude release time (fade out to silence)
};

//Renders the sound for each note or something
class SynchroNote
{
	MM_OPERATORS
public:
	//Constructor
	SynchroNote(NotePlayHandle * notePlayHandle);
	//Renders a single sample of audio
	void nextSample(sampleFrame &outputSample, sample_rate_t sample_rate,
		const float modulationStrength, const float modulationAmount, const float harmonics,
		const SynchroOscillatorSettings & carrier, const SynchroOscillatorSettings & modulator);
private:
	NotePlayHandle * m_nph;
	float m_CarrierSampleIndex = 0; //The index (or phase) of the carrier oscillator
	float m_ModulatorSampleIndex = 0; //The index (or phase) of the modulator oscillator
};

//Synth GUI
class SynchroSynthView : public InstrumentView
{
	Q_OBJECT
public:
	SynchroSynthView(Instrument * instrument, QWidget * parent);
	QSize sizeHint() const override { return QSize(448, 250); } //~3px wider than the artwork
protected slots:
private:
	void modelChanged() override;
	Knob * m_harmonicsKnob;
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
};

//Middleman between the GUI, LMMS, and each note (SynchroNote)
class SynchroSynth : public Instrument
{
	Q_OBJECT
public:
	SynchroSynth(InstrumentTrack * instrument_track);
	void playNote(NotePlayHandle * n, sampleFrame * working_buffer) override;
	f_cnt_t desiredReleaseFrames() const override;
	void saveSettings(QDomDocument & doc, QDomElement & parent) override;
	void loadSettings(const QDomElement & thisElement) override;
	QString nodeName() const override;
	PluginView * instantiateView(QWidget * parent) override { return new SynchroSynthView(this, parent); };
	void deleteNotePluginData(NotePlayHandle * n) override { delete static_cast<SynchroNote *>(n->m_pluginData); };
protected slots:
	void carrierChanged();
	void modulatorChanged();
	void generalChanged();
private:
	FloatModel m_modulation;
	FloatModel m_harmonics;
	FloatModel m_modulationStrength;

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
	friend class SynchroSynthView;
};

#endif
