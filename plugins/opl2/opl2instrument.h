/*
 * OPL2 FM synth
 *
 * Copyright (c) 2013 Raine M. Ekman <raine/at/iki/fi>
 *
 * This file is part of LMMS - http://lmms.io
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

#ifndef _OPL2_H
#define _OPL2_H

#include "Instrument.h"
#include "InstrumentView.h"
#include "opl.h"

#include "LcdSpinBox.h"
#include "knob.h"
#include "pixmap_button.h"

#define OPL2_VOICE_FREE 255
#define OPL2_NO_VOICE 255
// The "normal" range for LMMS pitchbends
#define DEFAULT_BEND_CENTS 100

class opl2instrument : public Instrument
{
	Q_OBJECT
public:
	opl2instrument( InstrumentTrack * _instrument_track );
	virtual ~opl2instrument();

	virtual QString nodeName() const;
	virtual PluginView * instantiateView( QWidget * _parent );

	virtual Flags flags() const
	{
		return IsSingleStreamed | IsMidiBased;
	}

	virtual bool handleMidiEvent( const MidiEvent& event, const MidiTime& time, f_cnt_t offset = 0 );
	virtual void play( sampleFrame * _working_buffer );

	void saveSettings( QDomDocument & _doc, QDomElement & _this );
	void loadSettings( const QDomElement & _this );
	void loadPatch(unsigned char inst[14]);
	void tuneEqual(int center, float Hz);

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
	fpp_t frameCount;
	short *renderbuffer;
	int voiceNote[9];
	// Least recently used voices
	int voiceLRU[9];
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



class opl2instrumentView : public InstrumentView
{
	Q_OBJECT
public:
        opl2instrumentView( Instrument * _instrument, QWidget * _parent );
	virtual ~opl2instrumentView();
	LcdSpinBox *m_patch;
	void modelChanged();

	knob *op1_a_kn;
	knob *op1_d_kn;
	knob *op1_s_kn;
	knob *op1_r_kn;
	knob *op1_lvl_kn;
	knob *op1_scale_kn;
	knob *op1_mul_kn;
	knob *feedback_kn;
	pixmapButton *op1_ksr_btn;
	pixmapButton *op1_perc_btn;
	pixmapButton *op1_trem_btn;
	pixmapButton *op1_vib_btn;
	pixmapButton *op1_w0_btn;
	pixmapButton *op1_w1_btn;
	pixmapButton *op1_w2_btn;
	pixmapButton *op1_w3_btn;
	automatableButtonGroup *op1_waveform;


	knob *op2_a_kn;
	knob *op2_d_kn;
	knob *op2_s_kn;
	knob *op2_r_kn;
	knob *op2_lvl_kn;
	knob *op2_scale_kn;
	knob *op2_mul_kn;
	pixmapButton *op2_ksr_btn;
	pixmapButton *op2_perc_btn;
	pixmapButton *op2_trem_btn;
	pixmapButton *op2_vib_btn;
	pixmapButton *op2_w0_btn;
	pixmapButton *op2_w1_btn;
	pixmapButton *op2_w2_btn;
	pixmapButton *op2_w3_btn;
	automatableButtonGroup *op2_waveform;


	pixmapButton *fm_btn;
	pixmapButton *vib_depth_btn;
	pixmapButton *trem_depth_btn;




};

#endif
