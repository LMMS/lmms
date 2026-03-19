/*
 * OpulenZ.h - AdLib OPL2 FM synth based instrument
 *
 * Copyright (c) 2013 Raine M. Ekman <raine/at/iki/fi>
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

#ifndef OPULENZ_H
#define OPULENZ_H


#include "AutomatableModel.h"
#include "Instrument.h"
#include "InstrumentView.h"

class Copl;


namespace lmms
{

namespace gui
{
class Knob;
class LcdSpinBox;
class PixmapButton;
class AutomatableButtonGroup;
}

// This one is a flag, MIDI notes take 7 low bits
#define OPL2_VOICE_FREE 128
#define OPL2_NO_VOICE 255
#define OPL2_VOICES 9

// The "normal" range for LMMS pitchbends
#define DEFAULT_BEND_CENTS 100

class OpulenzInstrument;

//! Stores parameters unique to both operators, to avoid duplication.
struct OpulenzOperatorModels
{
	//! `num` is the number of the operator (1 or 2 on an OPL2)
	OpulenzOperatorModels(OpulenzInstrument* ins, int num);
	~OpulenzOperatorModels() = default;

	FloatModel attack;
	FloatModel decay;
	FloatModel sustain;
	FloatModel release;
	FloatModel level;
	FloatModel scale;
	FloatModel multiplier;
	BoolModel ksr;
	BoolModel perc;
	BoolModel tremolo;
	BoolModel vibrato;
	IntModel waveform;
};

class OpulenzInstrument : public Instrument
{
	Q_OBJECT
public:
	OpulenzInstrument(InstrumentTrack* insTrack);
	~OpulenzInstrument() override;

	QString nodeName() const override;
	gui::PluginView* instantiateView(QWidget* _parent) override;

	bool handleMidiEvent(const MidiEvent& event, const TimePos& time, f_cnt_t offset = 0) override;
	void play(SampleFrame* _working_buffer) override;

	void saveSettings(QDomDocument& _doc, QDomElement& _this) override;
	void loadSettings(const QDomElement& _this) override;

	void loadDefaultPatch(); //!< Load default patch
	void loadPatch(const unsigned char inst[14]); //!< Load a patch into the emulator

	void tuneEqual(int center, float Hz);
	void loadFile(const QString& file) override; //!< Load an SBI file into the knob models

	IntModel m_patchModel;
	FloatModel m_feedbackModel;
	BoolModel m_fmModel;
	BoolModel m_vibDepthModel;
	BoolModel m_tremDepthModel;
	OpulenzOperatorModels m_op1;
	OpulenzOperatorModels m_op2;

private slots:
	void updatePatch(); //!< Update patch from the models to the chip emulation
	void reloadEmulator(); //!< Samplerate changes when choosing oversampling, so this is more or less mandatory
	void loadGMPatch(); //!< Load one of the default patches

private:
	Copl* theEmulator;
	QString storedname;
	f_cnt_t frameCount;
	short* renderbuffer;
	int voiceNote[OPL2_VOICES];

	int voiceLRU[OPL2_VOICES]; //!< Least recently used voices

	// 0 - no note, >0 - note on velocity
	int velocities[128];

	// These include both octave and Fnumber
	int fnums[128];

	// in cents, range defaults to +/-100 cents (should this be changeable?)
	int pitchbend;
	int pitchBendRange;

	int popVoice(); //!< Pop least recently used voice
	int pushVoice(int v); //!< Push voice into first free slot
	int Hz2fnum(float Hz); //!< Find suitable F number in lowest possible block

	/**
		Mutex for the emulator code. "I'd much rather do without a mutex, but it looks like the emulator code isn't
		really ready for threads"
	*/
	static QMutex s_emulatorMutex;

	//! (Weird) offsets for voice parameters
	static constexpr auto OpAdd = std::array<unsigned int, OPL2_VOICES>{
		0x00, 0x01, 0x02, 0x08, 0x09, 0x0A, 0x10, 0x11, 0x12
	};

	/**
		Write data to a specific voice's register, according to the `OpAdd` table. Can only be called by code
		protected by the holy mutex!
	*/
	void writeVoice(int voice, int reg, int val);

	//! Can only be called by code protected by the holy mutex!
	void setVoiceVelocity(int voice, int vel);

	// Pitch bend range comes through RPNs.
	int RPNcoarse, RPNfine;
};

namespace gui
{

//! Aggregates controls for models available in OpulenzOperatorModels. All pointers are non-owning.
struct OpulenzOperatorControls
{
	Knob* attack;
	Knob* decay;
	Knob* sustain;
	Knob* release;
	Knob* level;
	Knob* scale;
	Knob* multiplier;
	PixmapButton* ksr;
	PixmapButton* perc;
	PixmapButton* tremolo;
	PixmapButton* vibrato;
	AutomatableButtonGroup* waveform;
	PixmapButton* w0;
	PixmapButton* w1;
	PixmapButton* w2;
	PixmapButton* w3;
};

class OpulenzInstrumentView : public InstrumentViewFixedSize
{
	Q_OBJECT

public:
	OpulenzInstrumentView(Instrument* _instrument, QWidget* _parent);
	~OpulenzInstrumentView() override;
	LcdSpinBox* m_patch;
	void modelChanged() override;

	OpulenzOperatorControls op1View;
	OpulenzOperatorControls op2View;
	Knob* feedbackKnob;
	PixmapButton* fmButton;
	PixmapButton* vibDepthButton;
	PixmapButton* tremDepthButton;

private slots:
	//! Update hints to have user-friendly formatting and units.
	void updateKnobHints();

private:
	//! Formats time nicely for knob hints
	QString timeKnobHint(float n);
};

} // namespace gui

} // namespace lmms

#endif
