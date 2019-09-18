/*
 * SynchroSynth.cpp - 2-oscillator PM synth
 *
 * Copyright (c) 2019 Ian Sannar <ian/dot/sannar/at/gmail/dot/com>
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
 *
 */

#include "SynchroSynth.h"
//Standard headers
#include <math.h>
//QT headers
#include <QDomDocument>
//LMMS headers
#include "embed.h"
#include "Engine.h"
#include "InstrumentTrack.h"
#include "lmms_constants.h"
#include "Mixer.h"
#include "NotePlayHandle.h"
#include "plugin_export.h"

#define SYNCRHO_VERSION "0.4"
#define SYNCHRO_VOLUME_CONST 0.15f

static const int SYNCHRO_OVERSAMPLE = 4; //Anti-aliasing samples
static const int SYNCHRO_PM_CONST = 20; //Strength of the phase modulation
static const int SYNCHRO_GRAPH_SAMPLES = 48; //Resolution of the waveform view GUI elements

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
	// neccessary for getting instance out of shared lib
	PLUGIN_EXPORT Plugin * lmms_plugin_main(Model * m, void * data)
	{
		return new SynchroSynth(static_cast<InstrumentTrack *>(m));
	}
}

SynchroNote::SynchroNote(NotePlayHandle * nph) :
	nph(nph)
{
	//Empty constructor
}

SynchroNote::~SynchroNote()
{
	//Empty destructor
}

void SynchroNote::nextStringSample(sampleFrame &outputSample,
	sample_rate_t sample_rate, float modulationStrength,
	float modulationAmount, float harmonics, SynchroOscillatorSettings carrier,
	SynchroOscillatorSettings modulator)
{
	float sampleRatePi = F_2PI / sample_rate; //For oversampling
	//Find position in modulator waveform
	sample_index[1] += sampleRatePi * DetuneOctaves(nph->frequency(),
		modulator.Detune);
	while (sample_index[1] >= F_2PI) { sample_index[1] -= F_2PI; } //Make sure phase is always between 0 and 2PI
	//Get modulator waveform at position
	//Modulator is calculated first because it is used by the carrier
	float modulatorSample = SynchroWaveform(sample_index[1], modulator.Drive,
		modulator.Sync, modulator.Chop, harmonics)
		* (modulationStrength * modulationAmount);
	float pmSample = modulatorSample * SYNCHRO_PM_CONST;
	//Find position in carrier waveform
	sample_index[0] += sampleRatePi * DetuneOctaves(nph->frequency(),
		carrier.Detune);
	while (sample_index[0] >= F_2PI) { sample_index[0] -= F_2PI; } //Make sure phase is always between 0 and 2PI
	while (sample_index[0] < 0) { sample_index[0] += F_2PI; } //Phase modulation might push it below 0
	//Get carrier waveform at position, accounting for modulation
	//Index 0 is L, 1 is R. Synth is currently mono so they are the same value.
	outputSample[0] = SynchroWaveform(sample_index[0] + pmSample,
		carrier.Drive, carrier.Sync, carrier.Chop, 0)
		* (SYNCHRO_VOLUME_CONST);
	outputSample[1] = outputSample[0];
}

SynchroSynth::SynchroSynth(InstrumentTrack * instrument_track) :
	Instrument(instrument_track, &synchro_plugin_descriptor),
	m_harmonics(0, 0, 1.0f, 0.01f, this, tr("harmonics")),
	m_modulationStrength(0.5, 0, 1, 0.01f, this, tr("modulation maximum")),
	m_modulation(0, 0, 1, 0.001f, this, tr("modulation")),
	m_carrierDetune(0, -4, 0, 1, this, tr("voice octave")),
	m_carrierDrive(1, 1, 7, 0.01f, this, tr("voice drive")),
	m_carrierSync(1, 1, 16, 0.01f, this, tr("voice sync")),
	m_carrierChop(0, 0, 4, 0.01f, this, tr("voice chop")),
	m_modulatorDetune(0, -4, 0, 1, this, tr("modulator octave")),
	m_modulatorDrive(1, 1, 7, 0.01f, this, tr("modulator drive")),
	m_modulatorSync(1, 1, 16, 0.01f, this, tr("modulator sync")),
	m_modulatorChop(0, 0, 4, 0.01f, this, tr("modulator chop")),
	m_carrierGraph(-1.0f, 1.0f, SYNCHRO_GRAPH_SAMPLES, this),
	m_modulatorGraph(-1.0f, 1.0f, SYNCHRO_GRAPH_SAMPLES, this),
	m_resultGraph(-1.0f, 1.0f, SYNCHRO_GRAPH_SAMPLES, this)
{
	carrierChanged();
	modulatorChanged();
	generalChanged();

	connect(&m_carrierDetune, SIGNAL(dataChanged()), this,
		SLOT(carrierChanged()));
	connect(&m_carrierDrive, SIGNAL(dataChanged()), this,
		SLOT(carrierChanged()));
	connect(&m_carrierSync, SIGNAL(dataChanged()), this,
		SLOT(carrierChanged()));
	connect(&m_carrierChop, SIGNAL(dataChanged()), this,
		SLOT(carrierChanged()));

	connect(&m_modulatorDetune, SIGNAL(dataChanged()), this,
		SLOT(modulatorChanged()));
	connect(&m_modulatorDrive, SIGNAL(dataChanged()), this,
		SLOT(modulatorChanged()));
	connect(&m_modulatorSync, SIGNAL(dataChanged()), this,
		SLOT(modulatorChanged()));
	connect(&m_modulatorChop, SIGNAL(dataChanged()), this,
		SLOT(modulatorChanged()));
	connect(&m_harmonics, SIGNAL(dataChanged()), this,
		SLOT(modulatorChanged()));

	connect(&m_harmonics, SIGNAL(dataChanged()), this,
		SLOT(generalChanged()));
	connect(&m_modulation, SIGNAL(dataChanged()), this,
		SLOT(generalChanged()));
	connect(&m_modulationStrength, SIGNAL(dataChanged()), this,
		SLOT(generalChanged()));
}

SynchroSynth::~SynchroSynth()
{
	//Empty destructor
}

void SynchroSynth::saveSettings(QDomDocument & doc, QDomElement & thisElement)
{
	// Save plugin version (does this ever get used?)
	thisElement.setAttribute("version", SYNCRHO_VERSION);
	m_modulation.saveSettings(doc, thisElement, "modulation");
	m_harmonics.saveSettings(doc, thisElement, "harmonics");
	m_modulationStrength.saveSettings(doc, thisElement, "modulationStrength");
	m_carrierDetune.saveSettings(doc, thisElement, "carrierDetune");
	m_carrierDrive.saveSettings(doc, thisElement, "carrierDrive");
	m_carrierSync.saveSettings(doc, thisElement, "carrierSync");
	m_carrierChop.saveSettings(doc, thisElement, "carrierChop");
	m_modulatorDetune.saveSettings(doc, thisElement, "modulatorDetune");
	m_modulatorDrive.saveSettings(doc, thisElement, "modulatorDrive");
	m_modulatorSync.saveSettings(doc, thisElement, "modulatorSync");
	m_modulatorChop.saveSettings(doc, thisElement, "modulatorChop");
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
	m_modulatorDetune.loadSettings(thisElement, "modulatorDetune");
	m_modulatorDrive.loadSettings(thisElement, "modulatorDrive");
	m_modulatorSync.loadSettings(thisElement, "modulatorSync");
	m_modulatorChop.loadSettings(thisElement, "modulatorChop");
}

QString SynchroSynth::nodeName() const
{
	return synchro_plugin_descriptor.name;
}

void SynchroSynth::playNote(NotePlayHandle * n, sampleFrame * working_buffer)
{
	if (n->totalFramesPlayed() == 0 || n->m_pluginData == NULL)
	{
		n->m_pluginData = new SynchroNote(n);
	}

	const fpp_t frames = n->framesLeftForCurrentPeriod();
	const f_cnt_t offset = n->noteOffset();

	SynchroNote * ps = static_cast<SynchroNote *>(n->m_pluginData);
	for(fpp_t frame = offset; frame < frames + offset; ++frame)
	{
		sampleFrame outputSample = {0, 0};
		sampleFrame tempSample = {0, 0};
		SynchroOscillatorSettings carrier = { m_carrierDetune.value(),
			m_carrierDrive.value(), m_carrierSync.value(),
			m_carrierChop.value() };
		SynchroOscillatorSettings modulator = { m_modulatorDetune.value(),
			m_modulatorDrive.value(), m_modulatorSync.value(),
			m_modulatorChop.value() };

		//Oversampling using linear interpolation
		for (int i = 0; i < SYNCHRO_OVERSAMPLE; ++i)
		{
			ps->nextStringSample(tempSample,
				Engine::mixer()->processingSampleRate()
				* SYNCHRO_OVERSAMPLE, m_modulationStrength.value(),
				m_modulation.value(),m_harmonics.value(), carrier,
				modulator);
			outputSample[0] += tempSample[0];
			outputSample[1] += tempSample[1];
		}

		for(ch_cnt_t chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl)
		{
			working_buffer[frame][chnl] = outputSample[chnl]
				/ SYNCHRO_OVERSAMPLE;
		}
	}
	applyRelease(working_buffer, n);
	instrumentTrack()->processAudioBuffer(working_buffer, frames + offset, n);
}

void SynchroSynth::deleteNotePluginData(NotePlayHandle * n)
{
	delete static_cast<SynchroNote *>(n->m_pluginData);
}

PluginView * SynchroSynth::instantiateView(QWidget * parent)
{
	return new SynchroSynthView(this, parent);
}

void SynchroSynth::carrierChanged()
{
	for (int i = 0; i < SYNCHRO_GRAPH_SAMPLES; ++i)
	{
		float phase = (float)i / (float)SYNCHRO_GRAPH_SAMPLES;
		m_carrierGraph.setSampleAt(i, SynchroWaveform(phase * F_2PI,
			m_carrierDrive.value(), m_carrierSync.value(),
			m_carrierChop.value(), 0));
	}
	generalChanged();
}

void SynchroSynth::modulatorChanged()
{
	for (int i = 0; i < SYNCHRO_GRAPH_SAMPLES; ++i)
	{
		float phase = (float)i / (float)SYNCHRO_GRAPH_SAMPLES;
		m_modulatorGraph.setSampleAt(i, SynchroWaveform(phase * F_2PI,
			m_modulatorDrive.value(), m_modulatorSync.value(),
			m_modulatorChop.value(), m_harmonics.value()));
	}
	generalChanged();
}

void SynchroSynth::generalChanged()
{
	for (int i = 0; i < SYNCHRO_GRAPH_SAMPLES; ++i)
	{
		int octaveDiff = m_carrierDetune.value() - m_modulatorDetune.value();
		float pitchDifference = powf(2, octaveDiff);
		float phase = (float)i / (float)SYNCHRO_GRAPH_SAMPLES * F_2PI;
		float phaseMod = SynchroWaveform(phase, m_modulatorDrive.value(),
			m_modulatorSync.value(), m_modulatorChop.value(),
			m_harmonics.value()) * m_modulationStrength.value()
			* m_modulation.value() * SYNCHRO_PM_CONST;
		phase = phase + phaseMod * pitchDifference;
		while (phase >= F_2PI) { phase -= F_2PI; }
		m_resultGraph.setSampleAt(i, SynchroWaveform(phase, m_carrierDrive.value(), m_carrierSync.value(),
			m_carrierChop.value(), 0));
	}
}

SynchroSynthView::SynchroSynthView(Instrument * instrument, QWidget * parent) :
	InstrumentView(instrument, parent)
{
	setAutoFillBackground(true);
	QPalette pal;

	pal.setBrush(backgroundRole(), PLUGIN_NAME::getIconPixmap("artwork"));
	setPalette(pal);

	//General Controls
	m_harmonicsKnob = new Knob(knobDark_28, this);
	m_harmonicsKnob->move(136, 80);
	m_harmonicsKnob->setHintText(tr("harmonics"), "");

	m_modulationStrengthKnob = new Knob(knobDark_28, this);
	m_modulationStrengthKnob->move(200, 80);
	m_modulationStrengthKnob->setHintText(tr("modulation maximum"), "");

	m_modulationKnob = new Knob(knobDark_28, this);
	m_modulationKnob->move(72, 80);
	m_modulationKnob->setHintText(tr("modulation"), "");

	//Carrier Controls
	m_carrierDriveKnob = new Knob(knobDark_28, this);
	m_carrierDriveKnob->move(113, 144);
	m_carrierDriveKnob->setHintText(tr("voice drive"), "");

	m_carrierSyncKnob = new Knob(knobDark_28, this);
	m_carrierSyncKnob->move(161, 144);
	m_carrierSyncKnob->setHintText(tr("voice sync"), "");

	m_carrierChopKnob = new Knob(knobDark_28, this);
	m_carrierChopKnob->move(208, 144);
	m_carrierChopKnob->setHintText(tr("voice chop"), "");

	m_carrierDetuneKnob = new Knob(knobDark_28, this);
	m_carrierDetuneKnob->move(64, 144);
	m_carrierDetuneKnob->setHintText(tr("voice octave"), "");

	//Modulator Controls
	m_modulatorDriveKnob = new Knob(knobDark_28, this);
	m_modulatorDriveKnob->move(113, 210);
	m_modulatorDriveKnob->setHintText(tr("modulator drive"), "");

	m_modulatorSyncKnob = new Knob(knobDark_28, this);
	m_modulatorSyncKnob->move(161, 210);
	m_modulatorSyncKnob->setHintText(tr("modulator sync"), "");

	m_modulatorChopKnob = new Knob(knobDark_28, this);
	m_modulatorChopKnob->move(208, 210);
	m_modulatorChopKnob->setHintText(tr("modulator chop"), "");

	m_modulatorDetuneKnob = new Knob(knobDark_28, this);
	m_modulatorDetuneKnob->move(64, 210);
	m_modulatorDetuneKnob->setHintText(tr("modulator octave"), "");

	m_carrierGraph = new Graph(this, Graph::LinearStyle, 48, 42);
	m_carrierGraph->move(5, 138);
	m_carrierGraph->setAutoFillBackground(false);
	m_carrierGraph->setGraphColor(QColor(255, 255, 255));
	m_carrierGraph->setEnabled(false);

	m_modulatorGraph = new Graph(this, Graph::LinearStyle, 48, 42);
	m_modulatorGraph->move(5, 203);
	m_modulatorGraph->setAutoFillBackground(false);
	m_modulatorGraph->setGraphColor(QColor(255, 255, 255));
	m_modulatorGraph->setEnabled(false);

	m_resultGraph = new Graph(this, Graph::LinearStyle, 48, 42);
	m_resultGraph->move(5, 73);
	m_resultGraph->setAutoFillBackground(false);
	m_resultGraph->setGraphColor(QColor(255, 255, 255));
	m_resultGraph->setEnabled(false);
}

void SynchroSynthView::modelChanged()
{
	SynchroSynth * b = castModel<SynchroSynth>();
	m_harmonicsKnob->setModel(&b->m_harmonics);
	m_modulationStrengthKnob->setModel(&b->m_modulationStrength);
	m_modulationKnob->setModel(&b->m_modulation);
	m_carrierDriveKnob->setModel(&b->m_carrierDrive);
	m_carrierSyncKnob->setModel(&b->m_carrierSync);
	m_carrierChopKnob->setModel(&b->m_carrierChop);
	m_carrierDetuneKnob->setModel(&b->m_carrierDetune);
	m_modulatorDriveKnob->setModel(&b->m_modulatorDrive);
	m_modulatorSyncKnob->setModel(&b->m_modulatorSync);
	m_modulatorChopKnob->setModel(&b->m_modulatorChop);
	m_modulatorDetuneKnob->setModel(&b->m_modulatorDetune);
	m_carrierGraph->setModel(&b->m_carrierGraph);
	m_modulatorGraph->setModel(&b->m_modulatorGraph);
	m_resultGraph->setModel(&b->m_resultGraph);
}
