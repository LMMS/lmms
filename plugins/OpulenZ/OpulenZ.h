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

class OpulenzInstrument : public Instrument
{
	Q_OBJECT
public:
	OpulenzInstrument( InstrumentTrack * _instrument_track );
	~OpulenzInstrument() override;

	QString nodeName() const override;
	gui::PluginView* instantiateView( QWidget * _parent ) override;

	bool handleMidiEvent( const MidiEvent& event, const TimePos& time, f_cnt_t offset = 0 ) override;
	void play( SampleFrame* _working_buffer ) override;

	void saveSettings( QDomDocument & _doc, QDomElement & _this ) override;
	void loadSettings( const QDomElement & _this ) override;
	void loadPatch(const unsigned char inst[14]);
	void tuneEqual(int center, float Hz);
	void loadFile( const QString& file ) override;

	IntModel m_patchModel;

	FloatModel op1_a_mdl;
	FloatModel op1_d_mdl;
	FloatModel op1_s_mdl;
	FloatModel op1_r_mdl;
	FloatModel op1_lvl_mdl;
	FloatModel op1_scale_mdl;
	FloatModel op1_mul_mdl;
	FloatModel feedback_mdl;
	BoolModel op1_ksr_mdl;
	BoolModel op1_perc_mdl;
	BoolModel op1_trem_mdl;
	BoolModel op1_vib_mdl;
	BoolModel op1_w0_mdl;
	BoolModel op1_w1_mdl;
	BoolModel op1_w2_mdl;
	BoolModel op1_w3_mdl;
	IntModel op1_waveform_mdl;


	FloatModel op2_a_mdl;
	FloatModel op2_d_mdl;
	FloatModel op2_s_mdl;
	FloatModel op2_r_mdl;
	FloatModel op2_lvl_mdl;
	FloatModel op2_scale_mdl;
	FloatModel op2_mul_mdl;
	BoolModel op2_ksr_mdl;
	BoolModel op2_perc_mdl;
	BoolModel op2_trem_mdl;
	BoolModel op2_vib_mdl;
	BoolModel op2_w0_mdl;
	BoolModel op2_w1_mdl;
	BoolModel op2_w2_mdl;
	BoolModel op2_w3_mdl;
	IntModel op2_waveform_mdl;

	BoolModel fm_mdl;
	BoolModel vib_depth_mdl;
	BoolModel trem_depth_mdl;


private slots:
        void updatePatch();
	void reloadEmulator();
	void loadGMPatch();

private:
	Copl *theEmulator;
	QString storedname;
	fpp_t frameCount;
	short *renderbuffer;
	int voiceNote[OPL2_VOICES];
	// Least recently used voices
	int voiceLRU[OPL2_VOICES];
	// 0 - no note, >0 - note on velocity
	int velocities[128];
	// These include both octave and Fnumber
	int fnums[128];
	// in cents, range defaults to +/-100 cents (should this be changeable?)
	int pitchbend;
	int pitchBendRange;

	int popVoice();
	int pushVoice(int v);

	int Hz2fnum(float Hz);
	static QMutex emulatorMutex;
	void setVoiceVelocity(int voice, int vel);

	// Pitch bend range comes through RPNs.
	int RPNcoarse, RPNfine;
};


namespace gui
{


class OpulenzInstrumentView : public InstrumentViewFixedSize
{
	Q_OBJECT
public:
	OpulenzInstrumentView( Instrument * _instrument, QWidget * _parent );
	~OpulenzInstrumentView() override;
	LcdSpinBox *m_patch;
	void modelChanged() override;

	Knob *op1_a_kn;
	Knob *op1_d_kn;
	Knob *op1_s_kn;
	Knob *op1_r_kn;
	Knob *op1_lvl_kn;
	Knob *op1_scale_kn;
	Knob *op1_mul_kn;
	Knob *feedback_kn;
	PixmapButton *op1_ksr_btn;
	PixmapButton *op1_perc_btn;
	PixmapButton *op1_trem_btn;
	PixmapButton *op1_vib_btn;
	PixmapButton *op1_w0_btn;
	PixmapButton *op1_w1_btn;
	PixmapButton *op1_w2_btn;
	PixmapButton *op1_w3_btn;
	AutomatableButtonGroup *op1_waveform;


	Knob *op2_a_kn;
	Knob *op2_d_kn;
	Knob *op2_s_kn;
	Knob *op2_r_kn;
	Knob *op2_lvl_kn;
	Knob *op2_scale_kn;
	Knob *op2_mul_kn;
	PixmapButton *op2_ksr_btn;
	PixmapButton *op2_perc_btn;
	PixmapButton *op2_trem_btn;
	PixmapButton *op2_vib_btn;
	PixmapButton *op2_w0_btn;
	PixmapButton *op2_w1_btn;
	PixmapButton *op2_w2_btn;
	PixmapButton *op2_w3_btn;
	AutomatableButtonGroup *op2_waveform;


	PixmapButton *fm_btn;
	PixmapButton *vib_depth_btn;
	PixmapButton *trem_depth_btn;

	private slots:
	void updateKnobHints();

 private:
	QString knobHintHelper(float n);

};


} // namespace gui

} // namespace lmms

#endif
