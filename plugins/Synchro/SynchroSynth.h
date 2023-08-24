/*
 * SynchroSynth.h - 2-oscillator PM synth
 *
 * Copyright (c) 2023 rubiefawn <rubiefawn/at/gmail/dot/com>
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

#include "Graph.h"
#include "Instrument.h"
#include "InstrumentView.h"
#include "Knob.h"
#include "NotePlayHandle.h"
#include "hiir/Downsampler2xF64Fpu.h"

//===============//
// ~ constants ~ //
//===============//
namespace { // anonymous for hygienic constants

constexpr unsigned int SYNCHRO_WAVEFORM_GUI_SAMPLES = 64; // horizontal resolution for waveform preview
constexpr unsigned int SYNCHRO_OVERSAMPLING_FACTOR = 16;
static_assert( // no funny business
	SYNCHRO_OVERSAMPLING_FACTOR && !(SYNCHRO_OVERSAMPLING_FACTOR & (SYNCHRO_OVERSAMPLING_FACTOR - 1)),
	"oversampling factor must be a power of 2"
);
constexpr int SYNCHRO_OVERSAMPLING_FILTER_COEFFICIENT_COUNT = 12;

// these magic numbers have been selected because they make this synth sound ~roughly~ how i intended
// that does not mean they are scientific, ideal values. these values are not gospel
constexpr double SYNCHRO_CLIP_INHIBITOR = 0.5; // bad math -> too loud, this tames the volume to prevent clipping
constexpr double SYNCHRO_PM_BASE = 12.0; // fine-tuned magic number to dial in good modulation sounds
// named fields make the code that uses these way more readable, but this seven-line declaration feels sinful
constexpr struct Harmonic {
	unsigned int overtone;
	double amplitude;
} MAGIC_HARMONICS[] = { // list of {harmonic, amplitude} for phase modulation
	{32, 0.5},
	{38, 0.025}
};

} //namespace

namespace lmms {

//=========//
// ~ gui ~ //
//=========//
namespace gui {

class SynchroView : public InstrumentViewFixedSize {
	Q_OBJECT // see https://doc.qt.io/qt-5/qobject.html#Q_OBJECT

	public:
	SynchroView(Instrument* instrument, QWidget* parent);
	QSize sizeHint() const override { return QSize(448, 247); };

	protected slots:
	void modelChanged() override;

	private:
	// general
	Graph* m_resultingWaveform;
	Knob* m_harmonics;
	Knob* m_modRange;
	Knob* m_modAmount;

	// carrier oscillator
	Graph* m_carrierWaveform;
	Knob* m_carrierOctave;
	Knob* m_carrierDrive;
	Knob* m_carrierSync;
	Knob* m_carrierPulse;

	// modulator oscillator
	Graph* m_modulatorWaveform;
	Knob* m_modulatorOctave;
	Knob* m_modulatorDrive;
	Knob* m_modulatorSync;
	Knob* m_modulatorPulse;
}; //class SynchroView

} //namespace gui

//===========//
// ~ synth ~ //
//===========//

class SynchroSynth {
	MM_OPERATORS // see `MemoryManager.h`
	
	public:
	SynchroSynth(NotePlayHandle* notePlayHandle);
	double nextSample( // TODO: put these in the same order as they appear in member declarations
			const float modStrength,
			const float modRange,
			const float modulatorHarmonics,
			const float carrierOctave,
			const float modulatorOctave,
			const float carrierDrive,
			const float modulatorDrive,
			const float carrierSync,
			const float modulatorSync,
			const float carrierPulse,
			const float modulatorPulse
		);

	private:
	NotePlayHandle* m_nph;
	double m_carrierPhase = 0;
	double m_modulatorPhase = 0;
}; //class SynchroSynth

class SynchroInstrument : public Instrument {
	Q_OBJECT // see https://doc.qt.io/qt-5/qobject.html#Q_OBJECT

	public:
	SynchroInstrument(InstrumentTrack* instrument_track);
	void playNote(NotePlayHandle* nph, sampleFrame* working_buffer) override;
	f_cnt_t desiredReleaseFrames() const override { return static_cast<f_cnt_t>(128); };
	//Flags flags() const override { return IsSingleStreamed; };
	void saveSettings(QDomDocument& doc, QDomElement& thisElement) override;
	void loadSettings(const QDomElement& thiselement) override;
	QString nodeName() const override;
	gui::PluginView* instantiateView(QWidget* parent) override { return new gui::SynchroView(this, parent); };
	void deleteNotePluginData(NotePlayHandle* nph) override { delete static_cast<SynchroSynth*>(nph->m_pluginData); };

	protected slots: // protected so that gui::SynchroView can send signals to them (i assume)
	void sampleRateChanged();
	void generalChanged();
	void carrierChanged();
	void modulatorChanged();

	private:
	hiir::Downsampler2xF64Fpu<SYNCHRO_OVERSAMPLING_FILTER_COEFFICIENT_COUNT> m_downsamplingFilter;
	std::vector<double> m_downsamplingBuffer[2]; // these swap between downsampling iterations
	// general
	graphModel m_resultingWaveform; // ayo why is the class named "graphModel" and not "GraphModel"
	FloatModel m_harmonics;
	FloatModel m_modRange;
	FloatModel m_modAmount;

	// carrier oscillator
	graphModel m_carrierWaveform;
	FloatModel m_carrierOctave;
	FloatModel m_carrierDrive;
	FloatModel m_carrierSync;
	FloatModel m_carrierPulse;

	// modulator oscillator
	graphModel m_modulatorWaveform;
	FloatModel m_modulatorOctave;
	FloatModel m_modulatorDrive;
	FloatModel m_modulatorSync;
	FloatModel m_modulatorPulse;

	friend class gui::SynchroView;
}; //class SynchroInstrument

} //namespace lmms
#endif