/*
 * SynchroSynth.cpp - 2-oscillator PM synth
 *
 * Copyright (c) 2020 Ian Sannar <ian/dot/sannar/at/gmail/dot/com>
 * Credit to @DouglasDGI "Lost Robot" for performance optimizations
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

#include "SynchroSynth.h"
//Standard headers
#include <math.h>
//QT headers
#include <QDomDocument>
//LMMS headers
#include "Engine.h" //Required to access sample rate
#include "InstrumentTrack.h" //Required for the plugin to appear in LMMS
#include "lmms_constants.h" //Greater precision than math.h constants?
#include "Mixer.h" //Required to access sample rate
#include "NotePlayHandle.h" //Required for audio rendering
#include "plugin_export.h" //Required for the plugin to appear in LMMS

//Plugin metadata
extern "C"
{
	Plugin::Descriptor PLUGIN_EXPORT synchro_plugin_descriptor =
	{
		STRINGIFY(PLUGIN_NAME),
		"Synchro",
		QT_TRANSLATE_NOOP("pluginBrowser", "2-oscillator PM synth"),
		"Ian Sannar <ian/dot/sannar/at/gmail/dot/com>",
		0x0100, //What does this do?
		Plugin::Instrument,
		new PluginPixmapLoader("logo"),
		NULL,
		NULL
	};

	PLUGIN_EXPORT Plugin * lmms_plugin_main(Model * m, void * data)
	{
		return new SynchroSynth(static_cast<InstrumentTrack *>(m));
	}
}

//Triangle waveform generator
//x: the phase of the waveform
static inline float tri(float x)
{
	return 2.0 * fabs(2.0 * ((x / F_2PI) - floorf((x / F_2PI) + 0.5))) - 1.0;
}

//Triangle waveform generator with magic harmonics that make things dubsteppy
//x: the phase of the waveform
static inline float trih(float x, float harmonic)
{
	return tri(x) + ((tri(MAGIC_HARMONICS[0][0] * x) * MAGIC_HARMONICS[0][1]
		+ tri(MAGIC_HARMONICS[1][0] * x) * MAGIC_HARMONICS[1][1])) * harmonic;
}

//Linear interpolation for envelopes
//from: the initial value at x = 0
//to: the final value at x = 1
//x: the progress between the two values between 0 and 1
static inline float lerp(float from, float to, float x)
{
	return from + x * (to - from);
}

//Exponential interpolation for envelopes
//from: the initial value at x = 0
//to: the final value at x = 1
//x: the progress between the two values between 0 and 1
static inline float expInterpol(float from, float to, float x)
{
	return from + sqrt(x) * (to - from);
}

//Detunes by octaves
//pitch: the input pitch
//detune: the amount of octaves to detune by
static inline float DetuneOctaves(float pitch, float detune)
{
	return pitch * exp2(detune);
}

//Waveform function for the Synchro synthesizer.
//x: input phase in radians (please keep it between 0 and 2PI)
//drive: how much atan distortion is applied to the waveform
//sync: hard sync with harmonic multiple
//chop: how strong the amplitude falloff is per waveform period
//harmonic: how strong the magic harmonics are
static inline float SynchroWaveform(float x, float drive, float sync, float chop, float harmonic)
{
	return tanhf(trih(F_PI_2 + x * sync, harmonic) * drive / powf(F_2PI / (F_2PI - x), chop));
}

//SynchroNote empty constructor
SynchroNote::SynchroNote(NotePlayHandle * nph) :
	nph(nph)
{

}

void SynchroNote::nextStringSample(sampleFrame &outputSample, sample_rate_t sample_rate,
	const float & modulationStrength, const float & modulationAmount, const float & harmonics,
	const SynchroOscillatorSettings & carrier, const SynchroOscillatorSettings & modulator)
{
	float freqToSampleStep = F_2PI / sample_rate;
	//How long (in frames) this note has been playing
	float NoteTime = nph->totalFramesPlayed();

	//Modulator is calculated first because it is used by the carrier
	//Find position in modulator waveform
	modulatorSampleIndex += freqToSampleStep * DetuneOctaves(nph->frequency(), modulator.Detune);
	//Make sure phase is always between 0 and 2PI
	while (modulatorSampleIndex >= F_2PI) { modulatorSampleIndex -= F_2PI; }

	//Get modulator waveform at position
	float modulatorSample = SynchroWaveform(modulatorSampleIndex,
		modulator.Drive, modulator.Sync, modulator.Chop, harmonics) * (modulationStrength * modulationAmount);

	//Modulator envelope
	//Using sample_rate here gives an incorrect envelope length for some reason
	float modulatorEnvelope;
	float modulatorAtk = modulator.Attack * 0.001 * Engine::mixer()->processingSampleRate();
	float modulatorDec = modulator.Decay * 0.001 * Engine::mixer()->processingSampleRate();
	float modulatorRel = modulator.Release * 0.001 * Engine::mixer()->processingSampleRate();
	if (NoteTime < modulatorAtk) //Attack
	{
		//Linear attack sounds better
		modulatorEnvelope = lerp(0, 1, NoteTime / modulatorAtk);
	}
	else if (NoteTime <= modulatorAtk + modulatorDec) //Decay
	{
		//But decay needs to be exponential
		modulatorEnvelope = expInterpol(1, modulator.Sustain, NoteTime / (modulatorAtk + modulatorDec));
	}
	else //Sustain
	{
		modulatorEnvelope = modulator.Sustain;
	}
	if (nph->isReleased()) //Release is done separately so the volume doesn't jump when released prematurely
	{
		float releaseProgress = (nph->releaseFramesDone() < modulatorRel) ? nph->releaseFramesDone() : modulatorRel;
		modulatorEnvelope *= expInterpol(1, 0, releaseProgress / modulatorRel);
	}

	//The final sample Synchro uses for phase modulation
	float pmSample = modulatorSample * modulatorEnvelope * SYNCHRO_PM_CONST;

	//Find position in carrier waveform
	carrierSampleIndex += freqToSampleStep * DetuneOctaves(nph->frequency(), carrier.Detune);
	//Make sure phase is always between 0 and 2PI
	while (carrierSampleIndex >= F_2PI) { carrierSampleIndex -= F_2PI; }
	//Extra check since phase modulation can push it below 0
	while (carrierSampleIndex < 0) { carrierSampleIndex += F_2PI; }
	//Carrier envelope
	float carrierEnvelope;
	float carrierAtk = carrier.Attack * 0.001 * Engine::mixer()->processingSampleRate();
	float carrierDec = carrier.Decay * 0.001 * Engine::mixer()->processingSampleRate();
	float carrierRel = carrier.Release * 0.001 * Engine::mixer()->processingSampleRate();
	if (NoteTime < carrierAtk) //Attack
	{
		carrierEnvelope = lerp(0, 1, NoteTime / carrierAtk);
	}
	else if (NoteTime <= carrierAtk + carrierDec) //Decay
	{
		carrierEnvelope = expInterpol(1, carrier.Sustain, NoteTime / (carrierAtk + carrierDec));
	}
	else //Sustain
	{
		carrierEnvelope = carrier.Sustain;
	}
	if (nph->isReleased()) { //Release
		float releaseProgress = (nph->releaseFramesDone() < carrierRel) ? nph->releaseFramesDone() : carrierRel;
		carrierEnvelope *= expInterpol(1, 0, releaseProgress / carrierRel);
	}

	//Get carrier waveform at position, accounting for modulation
	outputSample[0] = SynchroWaveform(carrierSampleIndex + pmSample, carrier.Drive, carrier.Sync, carrier.Chop, 0)
		* carrierEnvelope * SYNCHRO_VOLUME_CONST;

	//Output result
	//Index 0 is L, 1 is R. Synth is currently mono so they are the same value.
	outputSample[1] = outputSample[0];
}

//GUI layout & style
SynchroSynthView::SynchroSynthView(Instrument * instrument, QWidget * parent) :
	InstrumentView(instrument, parent)
{
	setAutoFillBackground(true);
	QPalette pal;

	pal.setBrush(backgroundRole(), PLUGIN_NAME::getIconPixmap("artwork"));
	setPalette(pal);

	//General Controls
	m_modulationKnob = new Knob(knobDark_28, this);
	m_modulationKnob->move(89, 80);
	m_modulationKnob->setHintText(tr("modulation"), "");

	m_harmonicsKnob = new Knob(knobDark_28, this);
	m_harmonicsKnob->move(185, 80);
	m_harmonicsKnob->setHintText(tr("harmonics"), "");

	m_modulationStrengthKnob = new Knob(knobDark_28, this);
	m_modulationStrengthKnob->move(281, 80);
	m_modulationStrengthKnob->setHintText(tr("modulation maximum"), "");

	//Carrier Controls
	m_carrierDetuneKnob = new Knob(knobDark_28, this);
	m_carrierDetuneKnob->move(64, 144);
	m_carrierDetuneKnob->setHintText(tr("voice octave"), "");

	m_carrierDriveKnob = new Knob(knobDark_28, this);
	m_carrierDriveKnob->move(113, 144);
	m_carrierDriveKnob->setHintText(tr("voice drive"), "");

	m_carrierSyncKnob = new Knob(knobDark_28, this);
	m_carrierSyncKnob->move(161, 144);
	m_carrierSyncKnob->setHintText(tr("voice sync"), "");

	m_carrierChopKnob = new Knob(knobDark_28, this);
	m_carrierChopKnob->move(208, 144);
	m_carrierChopKnob->setHintText(tr("voice chop"), "");

	m_carrierAttackKnob = new Knob(knobDark_28, this);
	m_carrierAttackKnob->move(257, 144);
	m_carrierAttackKnob->setHintText(tr("voice attack (ms)"), "");

	m_carrierDecayKnob = new Knob(knobDark_28, this);
	m_carrierDecayKnob->move(305, 144);
	m_carrierDecayKnob->setHintText(tr("voice decay (ms)"), "");

	m_carrierSustainKnob = new Knob(knobDark_28, this);
	m_carrierSustainKnob->move(353, 144);
	m_carrierSustainKnob->setHintText(tr("voice sustain"), "");

	m_carrierReleaseKnob = new Knob(knobDark_28, this);
	m_carrierReleaseKnob->move(401, 144);
	m_carrierReleaseKnob->setHintText(tr("voice release (ms)"), "");

	//Modulator Controls
	m_modulatorDetuneKnob = new Knob(knobDark_28, this);
	m_modulatorDetuneKnob->move(64, 210);
	m_modulatorDetuneKnob->setHintText(tr("modulator octave"), "");

	m_modulatorDriveKnob = new Knob(knobDark_28, this);
	m_modulatorDriveKnob->move(113, 210);
	m_modulatorDriveKnob->setHintText(tr("modulator drive"), "");

	m_modulatorSyncKnob = new Knob(knobDark_28, this);
	m_modulatorSyncKnob->move(161, 210);
	m_modulatorSyncKnob->setHintText(tr("modulator sync"), "");

	m_modulatorChopKnob = new Knob(knobDark_28, this);
	m_modulatorChopKnob->move(208, 210);
	m_modulatorChopKnob->setHintText(tr("modulator chop"), "");

	m_modulatorAttackKnob = new Knob(knobDark_28, this);
	m_modulatorAttackKnob->move(257, 210);
	m_modulatorAttackKnob->setHintText(tr("modulator attack (ms)"), "");

	m_modulatorDecayKnob = new Knob(knobDark_28, this);
	m_modulatorDecayKnob->move(305, 210);
	m_modulatorDecayKnob->setHintText(tr("modulator decay (ms)"), "");

	m_modulatorSustainKnob = new Knob(knobDark_28, this);
	m_modulatorSustainKnob->move(353, 210);
	m_modulatorSustainKnob->setHintText(tr("modulator sustain"), "");

	m_modulatorReleaseKnob = new Knob(knobDark_28, this);
	m_modulatorReleaseKnob->move(401, 210);
	m_modulatorReleaseKnob->setHintText(tr("modulator release (ms)"), "");

	//General waveform view
	m_resultGraph = new Graph(this, Graph::LinearStyle, 48, 42);
	m_resultGraph->move(5, 73);
	m_resultGraph->setAutoFillBackground(false);
	m_resultGraph->setGraphColor(QColor(255, 255, 255));
	m_resultGraph->setEnabled(false);

	//Carrier waveform view
	m_carrierGraph = new Graph(this, Graph::LinearStyle, 48, 42);
	m_carrierGraph->move(5, 138);
	m_carrierGraph->setAutoFillBackground(false);
	m_carrierGraph->setGraphColor(QColor(255, 255, 255));
	m_carrierGraph->setEnabled(false);

	//Modulator waveform view
	m_modulatorGraph = new Graph(this, Graph::LinearStyle, 48, 42);
	m_modulatorGraph->move(5, 203);
	m_modulatorGraph->setAutoFillBackground(false);
	m_modulatorGraph->setGraphColor(QColor(255, 255, 255));
	m_modulatorGraph->setEnabled(false);
}

//Hooks up the GUI to the synth
void SynchroSynthView::modelChanged()
{
	SynchroSynth * b = castModel<SynchroSynth>();

	m_harmonicsKnob->setModel(&b->m_harmonics);
	m_modulationStrengthKnob->setModel(&b->m_modulationStrength);
	m_modulationKnob->setModel(&b->m_modulation);

	m_carrierDetuneKnob->setModel(&b->m_carrierDetune);
	m_carrierDriveKnob->setModel(&b->m_carrierDrive);
	m_carrierSyncKnob->setModel(&b->m_carrierSync);
	m_carrierChopKnob->setModel(&b->m_carrierChop);
	m_carrierAttackKnob->setModel(&b->m_carrierAttack);
	m_carrierDecayKnob->setModel(&b->m_carrierDecay);
	m_carrierSustainKnob->setModel(&b->m_carrierSustain);
	m_carrierReleaseKnob->setModel(&b->m_carrierRelease);

	m_modulatorDetuneKnob->setModel(&b->m_modulatorDetune);
	m_modulatorDriveKnob->setModel(&b->m_modulatorDrive);
	m_modulatorSyncKnob->setModel(&b->m_modulatorSync);
	m_modulatorChopKnob->setModel(&b->m_modulatorChop);
	m_modulatorAttackKnob->setModel(&b->m_modulatorAttack);
	m_modulatorDecayKnob->setModel(&b->m_modulatorDecay);
	m_modulatorSustainKnob->setModel(&b->m_modulatorSustain);
	m_modulatorReleaseKnob->setModel(&b->m_modulatorRelease);

	m_resultGraph->setModel(&b->m_resultGraph);
	m_carrierGraph->setModel(&b->m_carrierGraph);
	m_modulatorGraph->setModel(&b->m_modulatorGraph);
}

//Synth constructor
SynchroSynth::SynchroSynth(InstrumentTrack * instrument_track) :
	Instrument(instrument_track, &synchro_plugin_descriptor),

	m_modulation(0, 0, 1, 0.001f, this, tr("modulation")),
	m_harmonics(0, 0, 1.0f, 0.01f, this, tr("harmonics")),
	m_modulationStrength(0.5, 0, 1, 0.01f, this, tr("modulation maximum")),

	m_carrierDetune(0, -4, 0, 1, this, tr("voice octave")),
	m_carrierDrive(1, 1, 7, 0.01f, this, tr("voice drive")),
	m_carrierSync(1, 1, 16, 0.01f, this, tr("voice sync")),
	m_carrierChop(0, 0, 4, 0.01f, this, tr("voice chop")),
	m_carrierAttack(10, 10, 1000, 1, this, tr("voice attack (ms)")),
	m_carrierDecay(50, 10, 4000, 1, this, tr("voice decay (ms)")),
	m_carrierSustain(1.0f, 0, 1.0f, 0.01f, this, tr("voice sustain")),
	m_carrierRelease(20, 20, 4000, 1, this, tr("voice release (ms)")),

	m_modulatorDetune(0, -4, 0, 1, this, tr("modulator octave")),
	m_modulatorDrive(1, 1, 7, 0.01f, this, tr("modulator drive")),
	m_modulatorSync(1, 1, 16, 0.01f, this, tr("modulator sync")),
	m_modulatorChop(0, 0, 4, 0.01f, this, tr("modulator chop")),
	m_modulatorAttack(0, 0, 1000, 1, this, tr("modulator attack (ms)")),
	m_modulatorDecay(50, 10, 4000, 1, this, tr("modulator decay (ms)")),
	m_modulatorSustain(1.0f, 0, 1.0f, 0.01f, this, tr("modulator sustain")),
	m_modulatorRelease(4000, 10, 4000, 1, this, tr("modulator release (ms)")),

	m_carrierGraph(-1.0f, 1.0f, SYNCHRO_GRAPH_SAMPLES, this),
	m_modulatorGraph(-1.0f, 1.0f, SYNCHRO_GRAPH_SAMPLES, this),
	m_resultGraph(-1.0f, 1.0f, SYNCHRO_GRAPH_SAMPLES, this)
{
	//Update GUI on load
	//generalChanged() is called at the end of each of these
	carrierChanged();
	modulatorChanged();

	//Update carrier GUI when its waveform is modified by these four controls
	connect(&m_carrierDetune, SIGNAL(dataChanged()), this, SLOT(carrierChanged()));
	connect(&m_carrierDrive, SIGNAL(dataChanged()), this, SLOT(carrierChanged()));
	connect(&m_carrierSync, SIGNAL(dataChanged()), this, SLOT(carrierChanged()));
	connect(&m_carrierChop, SIGNAL(dataChanged()), this, SLOT(carrierChanged()));

	//Update carrier GUI when its waveform is modified by these four controls
	connect(&m_modulatorDetune, SIGNAL(dataChanged()), this, SLOT(modulatorChanged()));
	connect(&m_modulatorDrive, SIGNAL(dataChanged()), this, SLOT(modulatorChanged()));
	connect(&m_modulatorSync, SIGNAL(dataChanged()), this, SLOT(modulatorChanged()));
	connect(&m_modulatorChop, SIGNAL(dataChanged()), this, SLOT(modulatorChanged()));

	//Update the general GUI when modulation controls are changed
	connect(&m_modulation, SIGNAL(dataChanged()), this, SLOT(generalChanged()));
	connect(&m_harmonics, SIGNAL(dataChanged()), this, SLOT(generalChanged()));
	connect(&m_modulationStrength, SIGNAL(dataChanged()), this, SLOT(generalChanged()));
}

//Sends notes to be rendered and returns the result to LMMS
void SynchroSynth::playNote(NotePlayHandle * n, sampleFrame * working_buffer)
{
	if (n->m_pluginData == NULL) { n->m_pluginData = new SynchroNote(n); }

	const fpp_t frames = n->framesLeftForCurrentPeriod();
	const f_cnt_t offset = n->noteOffset();

	SynchroNote * ps = static_cast<SynchroNote *>(n->m_pluginData);

	//Get pointers for sample-exact buffers
	const ValueBuffer * modBuf = m_modulation.valueBuffer();
	const ValueBuffer * modStrBuf = m_modulationStrength.valueBuffer();

	for(fpp_t frame = offset; frame < frames + offset; ++frame)
	{
		sampleFrame outputSample = {0, 0};
		sampleFrame tempSample = {0, 0};
		SynchroOscillatorSettings carrier = { m_carrierDetune.value(), m_carrierDrive.value(),
			m_carrierSync.value(), m_carrierChop.value(),
			m_carrierAttack.value(), m_carrierDecay.value(), m_carrierSustain.value(), m_carrierRelease.value() };
		SynchroOscillatorSettings modulator = { m_modulatorDetune.value(), m_modulatorDrive.value(),
			m_modulatorSync.value(), m_modulatorChop.value(),
			m_modulatorAttack.value(), m_modulatorDecay.value(), m_modulatorSustain.value(), m_modulatorRelease.value() };

		//Oversampling using linear interpolation
		for (int i = 0; i < SYNCHRO_OVERSAMPLE; ++i)
		{
			//Modulation strength and amount are sample-exact when Synchro can get the value from the buffer
			float strength = modStrBuf ? modStrBuf->value(frame) : m_modulationStrength.value();
			float amount = modBuf ? modBuf->value(frame) : m_modulation.value();

			ps->nextStringSample(tempSample, Engine::mixer()->processingSampleRate() * SYNCHRO_OVERSAMPLE,
				strength, amount, m_harmonics.value(), carrier, modulator);
			outputSample[0] += tempSample[0];
			outputSample[1] += tempSample[1];
		}

		for(ch_cnt_t chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl)
		{
			working_buffer[frame][chnl] = outputSample[chnl] / SYNCHRO_OVERSAMPLE;
		}
	}
	//applyRelease(working_buffer, n);
	instrumentTrack()->processAudioBuffer(working_buffer, frames + offset, n);
}

//Tells LMMS how much extra time Synchro needs at the end of a note to finish our envelopes
f_cnt_t SynchroSynth::desiredReleaseFrames() const
{
	return (f_cnt_t)(m_carrierRelease.value() * 0.001 * Engine::mixer()->processingSampleRate());
}

void SynchroSynth::saveSettings(QDomDocument & doc, QDomElement & thisElement)
{
	// Save plugin version (does this ever get used?)
	thisElement.setAttribute("version", SYNCHRO_VERSION);

	m_modulation.saveSettings(doc, thisElement, "modulation");
	m_harmonics.saveSettings(doc, thisElement, "harmonics");
	m_modulationStrength.saveSettings(doc, thisElement, "modulationStrength");

	m_carrierDetune.saveSettings(doc, thisElement, "carrierDetune");
	m_carrierDrive.saveSettings(doc, thisElement, "carrierDrive");
	m_carrierSync.saveSettings(doc, thisElement, "carrierSync");
	m_carrierChop.saveSettings(doc, thisElement, "carrierChop");
	m_carrierAttack.saveSettings(doc, thisElement, "carrierAttack");
	m_carrierDecay.saveSettings(doc, thisElement, "carrierDecay");
	m_carrierSustain.saveSettings(doc, thisElement, "carrierSustain");
	m_carrierRelease.saveSettings(doc, thisElement, "carrierRelease");

	m_modulatorDetune.saveSettings(doc, thisElement, "modulatorDetune");
	m_modulatorDrive.saveSettings(doc, thisElement, "modulatorDrive");
	m_modulatorSync.saveSettings(doc, thisElement, "modulatorSync");
	m_modulatorChop.saveSettings(doc, thisElement, "modulatorChop");
	m_modulatorAttack.saveSettings(doc, thisElement, "modulatorAttack");
	m_modulatorDecay.saveSettings(doc, thisElement, "modulatorDecay");
	m_modulatorSustain.saveSettings(doc, thisElement, "modulatorSustain");
	m_modulatorRelease.saveSettings(doc, thisElement, "modulatorRelease");
}

void SynchroSynth::loadSettings(const QDomElement & thisElement)
{
	m_modulation.loadSettings(thisElement, "modulation");
	m_harmonics.loadSettings(thisElement, "harmonics");
	m_modulationStrength.loadSettings(thisElement, "modulationStrength");

	m_carrierDetune.loadSettings(thisElement, "carrierDetune");
	m_carrierDrive.loadSettings(thisElement, "carrierDrive");
	m_carrierSync.loadSettings(thisElement, "carrierSync");
	m_carrierChop.loadSettings(thisElement, "carrierChop");
	m_carrierAttack.loadSettings(thisElement, "carrierAttack");
	m_carrierDecay.loadSettings(thisElement, "carrierDecay");
	m_carrierSustain.loadSettings(thisElement, "carrierSustain");
	m_carrierRelease.loadSettings(thisElement, "carrierRelease");

	m_modulatorDetune.loadSettings(thisElement, "modulatorDetune");
	m_modulatorDrive.loadSettings(thisElement, "modulatorDrive");
	m_modulatorSync.loadSettings(thisElement, "modulatorSync");
	m_modulatorChop.loadSettings(thisElement, "modulatorChop");
	m_modulatorAttack.loadSettings(thisElement, "modulatorAttack");
	m_modulatorDecay.loadSettings(thisElement, "modulatorDecay");
	m_modulatorSustain.loadSettings(thisElement, "modulatorSustain");
	m_modulatorRelease.loadSettings(thisElement, "modulatorRelease");
}

QString SynchroSynth::nodeName() const { return synchro_plugin_descriptor.name; };

//Update carrier waveform view
void SynchroSynth::carrierChanged()
{
	for (int i = 0; i < SYNCHRO_GRAPH_SAMPLES; ++i)
	{
		float phase = (float)i / (float)SYNCHRO_GRAPH_SAMPLES * F_2PI;
		m_carrierGraph.setSampleAt(i, SynchroWaveform(phase, m_carrierDrive.value(), m_carrierSync.value(),
			m_carrierChop.value(), 0));
	}
	//Update the general waveform view as well
	generalChanged();
}

//Update modulator waveform view
void SynchroSynth::modulatorChanged()
{
	for (int i = 0; i < SYNCHRO_GRAPH_SAMPLES; ++i)
	{
		float phase = (float)i / (float)SYNCHRO_GRAPH_SAMPLES * F_2PI;
		m_modulatorGraph.setSampleAt(i, SynchroWaveform(phase, m_modulatorDrive.value(), m_modulatorSync.value(),
			m_modulatorChop.value(), m_harmonics.value()));
	}
	//Update the general waveform view as well
	generalChanged();
}

void SynchroSynth::generalChanged()
{
	for (int i = 0; i < SYNCHRO_GRAPH_SAMPLES; ++i)
	{
		//Difference between the octaves of the two oscillators determines the optimal period for the waveform view
		int octaveDiff = m_carrierDetune.value() - m_modulatorDetune.value();
		float pitchDifference = powf(2, octaveDiff);

		float phase = (float)i / (float)SYNCHRO_GRAPH_SAMPLES * F_2PI;
		while (phase >= F_2PI) { phase -= F_2PI; }

		float phaseMod = SynchroWaveform(phase, m_modulatorDrive.value(), m_modulatorSync.value(),
			m_modulatorChop.value(), m_harmonics.value()) * m_modulationStrength.value() * m_modulation.value()
			* SYNCHRO_PM_CONST;

		phase = phase + phaseMod * pitchDifference;
		while (phase >= F_2PI) { phase -= F_2PI; }

		m_resultGraph.setSampleAt(i, SynchroWaveform(phase, m_carrierDrive.value(), m_carrierSync.value(),
			m_carrierChop.value(), 0));
	}
}
