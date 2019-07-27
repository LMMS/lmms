/*
 * Microwave.h - declaration of class Microwave (a wavetable synthesizer)
 *
 * Copyright (c) 2019 Robert Daniel Black AKA Lost Robot <r94231/at/gmail/dot/com>
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


#ifndef Microwave_H
#define Microwave_H

#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QScrollBar>

#include "ComboBox.h"
#include "Graph.h"
#include "Instrument.h"
#include "InstrumentView.h"
#include "Knob.h"
#include "LcdSpinBox.h"
#include "LedCheckbox.h"
#include "MemoryManager.h"
#include "NotePlayHandle.h"
#include "PixmapButton.h"
#include "SampleBuffer.h"
#include "samplerate.h"
#include "stdshims.h"


// Macros, mostly for comboboxes but also a few other things

#define setwavemodel(name)\
		name->clear();\
		name->addItem(tr("None"), make_unique<PluginPixmapLoader>("none"));\
		name->addItem(tr("Pulse Width"), make_unique<PluginPixmapLoader>("pulse_width"));\
		name->addItem(tr("Weird 1"), make_unique<PluginPixmapLoader>("weird1"));\
		name->addItem(tr("Weird 2"), make_unique<PluginPixmapLoader>("weird2"));\
		name->addItem(tr("Asym To Right"), make_unique<PluginPixmapLoader>("asym_r"));\
		name->addItem(tr("Asym To Left"), make_unique<PluginPixmapLoader>("asym_l"));\
		name->addItem(tr("Bidirectional Asym"), make_unique<PluginPixmapLoader>("asym_r"));\
		name->addItem(tr("Squish To Center"), make_unique<PluginPixmapLoader>("squish_to_center"));\
		name->addItem(tr("Stretch From Center"), make_unique<PluginPixmapLoader>("stretch_from_center"));\
		name->addItem(tr("Stretch And Squish"), make_unique<PluginPixmapLoader>("squish_to_center"));\
		name->addItem(tr("Cut Off Right"), make_unique<PluginPixmapLoader>("cut_off_r"));\
		name->addItem(tr("Cut Off Left"), make_unique<PluginPixmapLoader>("cut_off_l"));\
		name->addItem(tr("Squarify"), make_unique<PluginPixmapLoader>("sqr"));\
		name->addItem(tr("Pulsify"), make_unique<PluginPixmapLoader>("pulse"));\
		name->addItem(tr("Flip"), make_unique<PluginPixmapLoader>("flip"));\
		name->addItem(tr("Clip"), make_unique<PluginPixmapLoader>("clip"));\
		name->addItem(tr("Inverse Clip"), make_unique<PluginPixmapLoader>("clip_inverse"));\
		name->addItem(tr("Sine"), make_unique<PluginPixmapLoader>("sin"));\
		name->addItem(tr("Atan"), make_unique<PluginPixmapLoader>("sqrsoft"));\
		name->addItem(tr("Sync"), make_unique<PluginPixmapLoader>("sync"));\
		name->addItem(tr("Sync Half Interpolate"), make_unique<PluginPixmapLoader>("sync_half_inter"));\
		name->addItem(tr("Sync Interpolate"), make_unique<PluginPixmapLoader>("sync_inter"));\
		name->addItem(tr("Mirror"), make_unique<PluginPixmapLoader>("mirror"));\
		name->addItem(tr("Diagonal Morph"), make_unique<PluginPixmapLoader>("diagonal_morph"));\
		name->addItem(tr("Sideways Morph"), make_unique<PluginPixmapLoader>("sideways_morph"));

#define modinmodel(name)\
		name->clear();\
		name->addItem(tr("None"), make_unique<PluginPixmapLoader>("none"));\
		name->addItem(tr("Wavetable OSC"), make_unique<PluginPixmapLoader>("wavetable"));\
		name->addItem(tr("Sub OSC"), make_unique<PluginPixmapLoader>("sin"));\
		name->addItem(tr("Sample OSC"), make_unique<PluginPixmapLoader>("sample"));\
		name->addItem(tr("Filter Output"), make_unique<PluginPixmapLoader>("filter_lowpass"));\
		name->addItem(tr("Velocity"), make_unique<PluginPixmapLoader>("volume"));\
		name->addItem(tr("Panning"), make_unique<PluginPixmapLoader>("panning"));\
		name->addItem(tr("Humanizer"), make_unique<PluginPixmapLoader>("letter_h"));\
		name->addItem(tr("Macro"), make_unique<PluginPixmapLoader>("macro"));

#define modsectionsmodel(name)\
		name->clear();\
		name->addItem(tr("None"), make_unique<PluginPixmapLoader>("none"));\
		name->addItem(tr("Wavetable OSC"), make_unique<PluginPixmapLoader>("wavetable"));\
		name->addItem(tr("Sub OSC"), make_unique<PluginPixmapLoader>("sin"));\
		name->addItem(tr("Sample OSC"), make_unique<PluginPixmapLoader>("sample"));\
		name->addItem(tr("Matrix"), make_unique<PluginPixmapLoader>("matrix"));\
		name->addItem(tr("Filter Input"), make_unique<PluginPixmapLoader>("filter_lowpass"));\
		name->addItem(tr("Filter Parameters"), make_unique<PluginPixmapLoader>("filter_parameters"));\
		name->addItem(tr("Macro"), make_unique<PluginPixmapLoader>("macro"));

#define mainoscsignalsmodel(name)\
		name->clear();\
		name->addItem(tr("None"), make_unique<PluginPixmapLoader>("none"));\
		name->addItem(tr("Morph"), make_unique<PluginPixmapLoader>("morph"));\
		name->addItem(tr("Range"), make_unique<PluginPixmapLoader>("range"));\
		name->addItem(tr("Modify"), make_unique<PluginPixmapLoader>("pulse_width"));\
		name->addItem(tr("Detune"), make_unique<PluginPixmapLoader>("detune"));\
		name->addItem(tr("Phase"), make_unique<PluginPixmapLoader>("phase"));\
		name->addItem(tr("Volume"), make_unique<PluginPixmapLoader>("volume"));\
		name->addItem(tr("Panning"), make_unique<PluginPixmapLoader>("panning"));\
		name->addItem(tr("Unison Voice Number"), make_unique<PluginPixmapLoader>("unison_number"));\
		name->addItem(tr("Unison Detune"), make_unique<PluginPixmapLoader>("unison_detune"));\
		name->addItem(tr("Unison Morph"), make_unique<PluginPixmapLoader>("unison_morph"));\
		name->addItem(tr("Unison Modify"), make_unique<PluginPixmapLoader>("unison_modify"));

#define subsignalsmodel(name)\
		name->clear();\
		name->addItem(tr("None"), make_unique<PluginPixmapLoader>("none"));\
		name->addItem(tr("Detune"), make_unique<PluginPixmapLoader>("detune"));\
		name->addItem(tr("Phase"), make_unique<PluginPixmapLoader>("phase"));\
		name->addItem(tr("Volume"), make_unique<PluginPixmapLoader>("volume"));\
		name->addItem(tr("Panning"), make_unique<PluginPixmapLoader>("panning"));\
		name->addItem(tr("Length"), make_unique<PluginPixmapLoader>("letter_l"));\
		name->addItem(tr("Rate Limit"), make_unique<PluginPixmapLoader>("letter_r"));\
		name->addItem(tr("Unison Voice Number"), make_unique<PluginPixmapLoader>("unison_number"));\
		name->addItem(tr("Unison Detune"), make_unique<PluginPixmapLoader>("unison_detune"));

#define samplesignalsmodel(name)\
		name->clear();\
		name->addItem(tr("None"), make_unique<PluginPixmapLoader>("none"));\
		name->addItem(tr("Detune"), make_unique<PluginPixmapLoader>("detune"));\
		name->addItem(tr("Phase"), make_unique<PluginPixmapLoader>("phase"));\
		name->addItem(tr("Volume"), make_unique<PluginPixmapLoader>("volume"));\
		name->addItem(tr("Panning"), make_unique<PluginPixmapLoader>("panning"));

#define matrixsignalsmodel(name)\
		name->clear();\
		name->addItem(tr("None"), make_unique<PluginPixmapLoader>("none"));\
		name->addItem(tr("Amount"), make_unique<PluginPixmapLoader>("volume"));\
		name->addItem(tr("Curve"), make_unique<PluginPixmapLoader>("curve"));\
		name->addItem(tr("Secondary Amount"), make_unique<PluginPixmapLoader>("volume"));\
		name->addItem(tr("Secondary Curve"), make_unique<PluginPixmapLoader>("curve"));\
		name->addItem(tr("Input Section"), make_unique<PluginPixmapLoader>("letter_i"));\
		name->addItem(tr("Input Number"), make_unique<PluginPixmapLoader>("letter_i"));\
		name->addItem(tr("Secondary Input Section"), make_unique<PluginPixmapLoader>("letter_i"));\
		name->addItem(tr("Secondary Input Number"), make_unique<PluginPixmapLoader>("letter_i"));\
		name->addItem(tr("Output Section 1"), make_unique<PluginPixmapLoader>("letter_o"));\
		name->addItem(tr("Output Section 2"), make_unique<PluginPixmapLoader>("letter_o"));\
		name->addItem(tr("Output Section Number"), make_unique<PluginPixmapLoader>("letter_o"));\

#define filtersignalsmodel(name)\
		name->clear();\
		name->addItem(tr("None"), make_unique<PluginPixmapLoader>("none"));\
		name->addItem(tr("Cutoff Frequency"), make_unique<PluginPixmapLoader>("filter_lowpass"));\
		name->addItem(tr("Resonance"), make_unique<PluginPixmapLoader>("letter_r"));\
		name->addItem(tr("db Gain"), make_unique<PluginPixmapLoader>("volume"));\
		name->addItem(tr("Filter Type"), make_unique<PluginPixmapLoader>("letter_t"));\
		name->addItem(tr("Slope"), make_unique<PluginPixmapLoader>("letter_s"));\
		name->addItem(tr("Input Volume"), make_unique<PluginPixmapLoader>("volume_input"));\
		name->addItem(tr("Output Volume"), make_unique<PluginPixmapLoader>("volume_output"));\
		name->addItem(tr("Wet/Dry"), make_unique<PluginPixmapLoader>("ramp"));\
		name->addItem(tr("Balance/Panning"), make_unique<PluginPixmapLoader>("panning"));\
		name->addItem(tr("Saturation"), make_unique<PluginPixmapLoader>("sqrsoft"));\
		name->addItem(tr("Feedback"), make_unique<PluginPixmapLoader>("letter_f"));\
		name->addItem(tr("Detune"), make_unique<PluginPixmapLoader>("detune"));

#define mod8model(name)\
		name->clear();\
		name->addItem(tr("1"), make_unique<PluginPixmapLoader>("number_1"));\
		name->addItem(tr("2"), make_unique<PluginPixmapLoader>("number_2"));\
		name->addItem(tr("3"), make_unique<PluginPixmapLoader>("number_3"));\
		name->addItem(tr("4"), make_unique<PluginPixmapLoader>("number_4"));\
		name->addItem(tr("5"), make_unique<PluginPixmapLoader>("number_5"));\
		name->addItem(tr("6"), make_unique<PluginPixmapLoader>("number_6"));\
		name->addItem(tr("7"), make_unique<PluginPixmapLoader>("number_7"));\
		name->addItem(tr("8"), make_unique<PluginPixmapLoader>("number_8"));

#define matrixoutmodel(name)\
		name->clear();\
		name->addItem(tr("1"));\
		name->addItem(tr("2"));\
		name->addItem(tr("3"));\
		name->addItem(tr("4"));\
		name->addItem(tr("5"));\
		name->addItem(tr("6"));\
		name->addItem(tr("7"));\
		name->addItem(tr("8"));\
		name->addItem(tr("9"));\
		name->addItem(tr("10"));\
		name->addItem(tr("11"));\
		name->addItem(tr("12"));\
		name->addItem(tr("13"));\
		name->addItem(tr("14"));\
		name->addItem(tr("15"));\
		name->addItem(tr("16"));\
		name->addItem(tr("17"));\
		name->addItem(tr("18"));

#define filtertypesmodel(name)\
		name->clear();\
		name->addItem(tr("Lowpass"), make_unique<PluginPixmapLoader>("filter_lowpass"));\
		name->addItem(tr("Highpass"), make_unique<PluginPixmapLoader>("filter_highpass"));\
		name->addItem(tr("Bandpass"), make_unique<PluginPixmapLoader>("filter_bandpass"));\
		name->addItem(tr("Low Shelf"), make_unique<PluginPixmapLoader>("filter_lowshelf"));\
		name->addItem(tr("High Shelf"), make_unique<PluginPixmapLoader>("filter_highshelf"));\
		name->addItem(tr("Peak"), make_unique<PluginPixmapLoader>("filter_peak"));\
		name->addItem(tr("Notch"), make_unique<PluginPixmapLoader>("filter_notch"));\
		name->addItem(tr("Allpass"), make_unique<PluginPixmapLoader>("filter_allpass"));\
		name->addItem(tr("Moog Lowpass (2x Slope)"), make_unique<PluginPixmapLoader>("filter_moog"));

#define filterslopesmodel(name)\
		name->clear();\
		name->addItem(tr("12 db"), make_unique<PluginPixmapLoader>("number_1"));\
		name->addItem(tr("24 db"), make_unique<PluginPixmapLoader>("number_2"));\
		name->addItem(tr("36 db"), make_unique<PluginPixmapLoader>("number_3"));\
		name->addItem(tr("48 db"), make_unique<PluginPixmapLoader>("number_4"));\
		name->addItem(tr("60 db"), make_unique<PluginPixmapLoader>("number_5"));\
		name->addItem(tr("72 db"), make_unique<PluginPixmapLoader>("number_6"));\
		name->addItem(tr("84 db"), make_unique<PluginPixmapLoader>("number_7"));\
		name->addItem(tr("96 db"), make_unique<PluginPixmapLoader>("number_8"));

#define modcombinetypemodel(name)\
		name->clear();\
		name->addItem(tr("Add Bidirectional"), make_unique<PluginPixmapLoader>("bidirectional_add"));\
		name->addItem(tr("Multiply Bidirectional"), make_unique<PluginPixmapLoader>("bidirectional_multi"));\
		name->addItem(tr("Add Unidirectional"), make_unique<PluginPixmapLoader>("unidirectional_add"));\
		name->addItem(tr("Multiply Unidirectional"), make_unique<PluginPixmapLoader>("unidirectional_multi"));

#define oversamplemodel(name)\
		name.clear();\
		name.addItem(tr("1x"), make_unique<PluginPixmapLoader>("number_1"));\
		name.addItem(tr("2x"), make_unique<PluginPixmapLoader>("number_2"));\
		name.addItem(tr("3x"), make_unique<PluginPixmapLoader>("number_3"));\
		name.addItem(tr("4x"), make_unique<PluginPixmapLoader>("number_4"));\
		name.addItem(tr("5x"), make_unique<PluginPixmapLoader>("number_5"));\
		name.addItem(tr("6x"), make_unique<PluginPixmapLoader>("number_6"));\
		name.addItem(tr("7x"), make_unique<PluginPixmapLoader>("number_7"));\
		name.addItem(tr("8x"), make_unique<PluginPixmapLoader>("number_8"));

#define loadmodemodel(name)\
		name.clear();\
		name.addItem(tr("Lock waveform edges to zero crossings"));\
		name.addItem(tr("Load sample without changes"));\
		name.addItem(tr("Load wavetable file"));\
		name.addItem(tr("Autocorrelation (static pitch detection)"));

#define oversamplemodemodel(name)\
		name.clear();\
		name.addItem(tr("Decimate"), make_unique<PluginPixmapLoader>("number_1"));\
		name.addItem(tr("Average"), make_unique<PluginPixmapLoader>("number_2"));

#define visimove(name, x, y, isVisible)\
	name->move(x, y);\
	name->setVisible(isVisible);


// Create the knob, set its tooltip, set its default color
#define makeknob(name, knobtype, hint, tooltip)\
	name = new MicrowaveKnob(knobtype, this);\
	name->setHintText(hint, "");\
	ToolTip::add(name, tooltip);\
	name->m_knobView = this;\
	name->setarcColor(QColor(46,74,80));\
	name->setlineColor(QColor(102,198,199));\
	name->setInnerColor(QColor(64,92,97));\
	connect(name, &MicrowaveKnob::updateScroll, this, &MicrowaveView::updateScroll);


// Constant variables for use in many situations.  Strongly related to srccpy stuff.

const int STOREDSUBWAVELEN = 2048;
const int STOREDMAINWAVELEN = 2048;

// This number divided by 4 is the number of megabytes of RAM each wavetable will use up (I think?).
// I'd like to increase this, but the higher this number is, the more time it takes to load Microwave.
// 16 is the ideal value here.
const int WAVERATIO = 16;

const int SUBWAVELEN = STOREDSUBWAVELEN * WAVERATIO;
const int MAINWAVELEN = STOREDMAINWAVELEN * WAVERATIO;

const int STOREDMAINARRAYLEN = STOREDMAINWAVELEN * 256;
const int MAINARRAYLEN = MAINWAVELEN * 256;


class oscillator;
class MicrowaveView;
class Microwave;


class MicrowaveKnob: public Knob
{
	Q_OBJECT
	using Knob::Knob;
public:
	void setMatrixLocation(int loc1, int loc2, int loc3);

	void setWhichMacroKnob(int which);

	MicrowaveView * m_knobView;
signals:
	void sendToMatrixAsOutput();
	void switchToMatrixKnob();
	void updateScroll();
	void setMacroTooltip();
	void chooseMacroColor();
	void refreshMacroColor();
	void setMacroColortoDefault();
protected:
	virtual void contextMenuEvent(QContextMenuEvent * me);
	virtual void mousePressEvent(QMouseEvent * me);
	virtual void mouseReleaseEvent(QMouseEvent * me);
private:
	int m_matrixLocation[3] = {0};
	bool m_ignoreShift = false;
	int m_whichMacroKnob = -1;
};


class Microwave : public Instrument
{
	Q_OBJECT

public:
	Microwave(InstrumentTrack * instrument_track);
	virtual ~Microwave();
	virtual PluginView * instantiateView(QWidget * parent);
	virtual QString nodeName() const;
	virtual void saveSettings(QDomDocument & doc, QDomElement & parent);
	virtual void loadSettings(const QDomElement & thissave);
	virtual void playNote(NotePlayHandle * n, sampleFrame * working_buffer);
	virtual void deleteNotePluginData(NotePlayHandle * n);

	virtual f_cnt_t desiredReleaseFrames() const
	{
		return(64);
	}

	void switchMatrixSections(int source, int destination);

	inline void fillSubOsc(int which, bool doInterpolate = true);
	inline void fillMainOsc(int which, bool doInterpolate = true);

	float m_scroll = 0;
	bool m_viewOpen = false;

protected slots:
	void valueChanged(int, int);
	void morphMaxChanged(int);
	void sampLenChanged(int);
	void subSampLenChanged(int);
	void mainEnabledChanged(int);
	void subEnabledChanged(int);
	void modEnabledChanged(int);
	void filtEnabledChanged(int);
	void sampleEnabledChanged(int);
	void samplesChanged(int, int);
	void interpolateChanged(int);
	void subInterpolateChanged(int);

private:

	// Stolen from WatSyn, with a few changes
	// memcpy utilizing libsamplerate (src) for sinc interpolation
	inline void srccpy(float * dst, float * src, int wavelength)
	{
		int err;
		const int margin = 64;
		
		// copy to temp array
		std::unique_ptr<float[]> tmps(new float[wavelength + margin]);
		float * tmp = &tmps.get()[0];

		memcpy(tmp, src, sizeof(float) * wavelength);
		memcpy(tmp + wavelength, src, sizeof(float) * margin);
		SRC_STATE * src_state = src_new(SRC_SINC_FASTEST, 1, &err);
		SRC_DATA src_data;
		src_data.data_in = tmp;
		src_data.input_frames = wavelength + margin;
		src_data.data_out = dst;
		src_data.output_frames = wavelength * WAVERATIO;
		src_data.src_ratio = static_cast<double>(WAVERATIO);
		src_data.end_of_input = 0;
		err = src_process(src_state, &src_data); 
		if (err) { qDebug("Microwave SRC error: %s", src_strerror(err)); }
		src_delete(src_state);
	}

	FloatModel * m_morph[8];
	FloatModel * m_range[8];
	FloatModel * m_modify[8];
	ComboBoxModel * m_modifyMode[8];
	FloatModel * m_vol[8];
	FloatModel * m_pan[8];
	FloatModel * m_detune[8];
	FloatModel * m_phase[8];
	FloatModel * m_phaseRand[8];
	BoolModel * m_enabled[8];
	BoolModel * m_muted[8];
	FloatModel * m_sampLen[8];
	FloatModel * m_morphMax[8];
	FloatModel * m_unisonVoices[8];
	FloatModel * m_unisonDetune[8];
	FloatModel * m_unisonMorph[8];
	FloatModel * m_unisonModify[8];
	BoolModel * m_keytracking[8];
	FloatModel * m_tempo[8];
	BoolModel * m_interpolate[8];

	BoolModel * m_subEnabled[64];
	BoolModel * m_subMuted[64];
	BoolModel * m_subKeytrack[64];
	BoolModel * m_subNoise[64];
	FloatModel * m_subVol[64];
	FloatModel * m_subPanning[64];
	FloatModel * m_subDetune[64];
	FloatModel * m_subPhase[64];
	FloatModel * m_subPhaseRand[64];
	FloatModel * m_subSampLen[64];
	FloatModel * m_subTempo[64];
	FloatModel * m_subRateLimit[64];
	FloatModel * m_subUnisonNum[64];
	FloatModel * m_subUnisonDetune[64];
	BoolModel * m_subInterpolate[64];

	BoolModel * m_sampleEnabled[8];
	BoolModel * m_sampleGraphEnabled[8];
	BoolModel * m_sampleMuted[8];
	BoolModel * m_sampleKeytracking[8];
	BoolModel * m_sampleLoop[8];
	FloatModel * m_sampleVolume[8];
	FloatModel * m_samplePanning[8];
	FloatModel * m_sampleDetune[8];
	FloatModel * m_samplePhase[8];
	FloatModel * m_samplePhaseRand[8];
	FloatModel * m_sampleStart[8];
	FloatModel * m_sampleEnd[8];

	ComboBoxModel * m_modIn[64];
	IntModel * m_modInNum[64];
	FloatModel * m_modInAmnt[64];
	FloatModel * m_modInCurve[64];
	ComboBoxModel * m_modIn2[64];
	IntModel * m_modInNum2[64];
	FloatModel * m_modInAmnt2[64];
	FloatModel * m_modInCurve2[64];
	ComboBoxModel * m_modOutSec[64];
	ComboBoxModel * m_modOutSig[64];
	IntModel * m_modOutSecNum[64];
	BoolModel * m_modEnabled[64];
	ComboBoxModel * m_modCombineType[64];
	BoolModel * m_modType[64];
	BoolModel * m_modType2[64];

	FloatModel * m_filtCutoff[8];
	FloatModel * m_filtReso[8];
	FloatModel * m_filtGain[8];
	ComboBoxModel * m_filtType[8];
	ComboBoxModel * m_filtSlope[8];
	FloatModel * m_filtInVol[8];
	FloatModel * m_filtOutVol[8];
	FloatModel * m_filtWetDry[8];
	FloatModel * m_filtBal[8];
	FloatModel * m_filtSatu[8];
	FloatModel * m_filtFeedback[8];
	FloatModel * m_filtDetune[8];
	BoolModel * m_filtEnabled[8];
	BoolModel * m_filtMuted[8];
	BoolModel * m_filtKeytracking[8];

	FloatModel * m_macro[18];
	QString m_macroTooltips[18] = {};
	int m_macroColors[18][3] = {{0}};

	FloatModel m_visvol;

	FloatModel m_loadAlg;
	FloatModel m_loadChnl;

	IntModel m_mainNum;
	IntModel m_subNum;
	IntModel m_sampNum;
	ComboBoxModel m_oversample;
	ComboBoxModel m_oversampleMode;
	ComboBoxModel m_loadMode;

	graphModel m_graph;
	
	BoolModel m_visualize;

	std::vector<float> m_storedwaveforms[8];
	std::vector<float> m_waveforms[8];
	bool m_mainFilled[8] = {false};
	std::vector<float> m_storedsubs[64];
	std::vector<float> m_subs[64];
	bool m_subFilled[64] = {false};
	float m_sampGraphs[1024] = {0};
	std::vector<float> m_samples[8][2];

	BoolModel m_mainFlipped;
	BoolModel m_subFlipped;

	BoolModel m_removeDC;

	SampleBuffer m_sampleBuffer;

	//Below is for passing to mSynth initialization
	float m_morphArr[8] = {0};
	float m_rangeArr[8] = {0};
	float m_modifyArr[8] = {0};
	int m_modifyModeArr[8] = {0};
	float m_volArr[8] = {0};
	float m_panArr[8] = {0};
	float m_detuneArr[8] = {0};
	float m_phaseArr[8] = {0};
	float m_phaseRandArr[8] = {0};
	bool m_enabledArr[8] = {false};
	bool m_mutedArr[8] = {false};
	float m_sampLenArr[8] = {0};
	float m_morphMaxArr[8] = {0};
	float m_unisonVoicesArr[8] = {0};
	float m_unisonDetuneArr[8] = {0};
	float m_unisonMorphArr[8] = {0};
	float m_unisonModifyArr[8] = {0};
	bool m_keytrackingArr[8] = {true};
	float m_tempoArr[8] = {0};
	bool m_interpolateArr[8] = {true};

	int m_modInArr[64] = {0};
	int m_modInNumArr[64] = {0};
	float m_modInAmntArr[64] = {0};
	float m_modInCurveArr[64] = {0};
	int m_modIn2Arr[64] = {0};
	int m_modInNum2Arr[64] = {0};
	float m_modInAmnt2Arr[64] = {0};
	float m_modInCurve2Arr[64] = {0};
	int m_modOutSecArr[64] = {0};
	int m_modOutSigArr[64] = {0};
	int m_modOutSecNumArr[64] = {0};
	bool m_modEnabledArr[64] = {false};
	int m_modCombineTypeArr[64] = {0};
	bool m_modTypeArr[64] = {0};
	bool m_modType2Arr[64] = {0};
	
	bool m_subEnabledArr[64] = {false};
	float m_subVolArr[64] = {0};
	float m_subPhaseArr[64] = {0};
	float m_subPhaseRandArr[64] = {0};
	float m_subDetuneArr[64] = {0};
	bool m_subMutedArr[64] = {false};
	bool m_subKeytrackArr[64] = {false};
	float m_subSampLenArr[64] = {0};
	bool m_subNoiseArr[64] = {false};
	float m_subPanningArr[64] = {0};
	float m_subTempoArr[64] = {0};
	float m_subRateLimitArr[64] = {0};
	float m_subUnisonNumArr[64] = {0};
	float m_subUnisonDetuneArr[64] = {0};
	bool m_subInterpolateArr[64] = {true};
	
	float m_filtInVolArr[8] = {0};
	int m_filtTypeArr[8] = {0};
	int m_filtSlopeArr[8] = {0};
	float m_filtCutoffArr[8] = {0};
	float m_filtResoArr[8] = {0};
	float m_filtGainArr[8] = {0};
	float m_filtSatuArr[8] = {0};
	float m_filtWetDryArr[8] = {0};
	float m_filtBalArr[8] = {0};
	float m_filtOutVolArr[8] = {0};
	bool m_filtEnabledArr[8] = {false};
	float m_filtFeedbackArr[8] = {0};
	float m_filtDetuneArr[8] = {0};
	bool m_filtKeytrackingArr[8] = {false};
	bool m_filtMutedArr[8] = {false};

	bool m_sampleEnabledArr[8] = {false};
	bool m_sampleGraphEnabledArr[8] = {false};
	bool m_sampleMutedArr[8] = {false};
	bool m_sampleKeytrackingArr[8] = {false};
	bool m_sampleLoopArr[8] = {false};
	float m_sampleVolumeArr[8] = {0};
	float m_samplePanningArr[8] = {0};
	float m_sampleDetuneArr[8] = {0};
	float m_samplePhaseArr[8] = {0};
	float m_samplePhaseRandArr[8] = {0};
	float m_sampleStartArr[8] = {0};
	float m_sampleEndArr[8] = {1, 1, 1, 1, 1, 1, 1, 1};

	float m_macroArr[18] = {0};
	//Above is for passing to mSynth initialization

	int m_maxMainEnabled = 0;// The highest number of wavetable oscillator sections that must be looped through
	int m_maxModEnabled = 0;// The highest number of matrix sections that must be looped through
	int m_maxSubEnabled = 0;// The highest number of sub oscillator sections that must be looped through
	int m_maxSampleEnabled = 0;// The highest number of sample sections that must be looped through
	int m_maxFiltEnabled = 0;// The highest number of filter sections that must be looped through

	float m_visualizerValues[204] = {0};

	QString m_wavetableSaveStrings[8] = {""};
	bool m_updateWavetable[8] = {true,true,true,true,true,true,true,true};

	ConstNotePlayHandleList m_nphList;

	InstrumentTrack * m_microwaveTrack = this->instrumentTrack();

	Microwave * m_mwc;
	
	friend class MicrowaveView;
	friend class mSynth;
	friend class MicrowaveKnob;
};



class MicrowaveView : public InstrumentView
{
	Q_OBJECT
public:
	MicrowaveView(Instrument * instrument,
					QWidget * parent);

	virtual ~MicrowaveView();

	void sendToMatrixAsOutput(int loc1, int loc2, int loc3);
	void switchToMatrixKnob(MicrowaveKnob * m_theKnob, int loc1, int loc2, int loc3);
	void setMacroTooltip(MicrowaveKnob * m_theKnob, int which);
	void chooseMacroColor(MicrowaveKnob * m_theKnob, int which);
	void refreshMacroColor(MicrowaveKnob * m_theKnob, int which);
	void setMacroColortoDefault(MicrowaveKnob * m_theKnob, int which);
	void setGraphEnabledColor(bool isEnabled);

protected slots:
	void updateScroll();
	void mainNumChanged();
	void subNumChanged();
	void sampNumChanged();
	void modOutSecChanged(int i);
	void modInChanged(int i);
	void modIn2Changed(int i);
	void tabChanged(int tabnum);
	void visualizeToggled(bool value);
	void sinWaveClicked();
	void triangleWaveClicked();
	void sqrWaveClicked();
	void sawWaveClicked();
	void noiseWaveClicked();
	void usrWaveClicked();
	void smoothClicked(void );
	void chooseWavetableFile();
	void openWavetableFile(QString fileName = "");
	void openWavetableFileBtnClicked();
	void openSampleFile();
	void openSampleFileBtnClicked();

	void modUpClicked(int);
	void modDownClicked(int);
	void i1Clicked(int);
	void i2Clicked(int);

	void confirmWavetableLoadClicked();

	void tabBtnClicked(int);

	void manualBtnClicked();

	void updateBackground();

	void flipperClicked();

	void XBtnClicked();
	void MatrixXBtnClicked();

	void normalizeClicked();
	void desawClicked();

	void modEnabledChanged();

private:
	void wheelEvent(QWheelEvent * me);
	virtual void dropEvent(QDropEvent * de);
	virtual void dragEnterEvent(QDragEnterEvent * dee);

	PixmapButton * m_sinWaveBtn;
	PixmapButton * m_triangleWaveBtn;
	PixmapButton * m_sqrWaveBtn;
	PixmapButton * m_sawWaveBtn;
	PixmapButton * m_whiteNoiseWaveBtn;
	PixmapButton * m_smoothBtn;
	PixmapButton * m_usrWaveBtn;

	PixmapButton * m_sinWave2Btn;
	PixmapButton * m_triangleWave2Btn;
	PixmapButton * m_sqrWave2Btn;
	PixmapButton * m_sawWave2Btn;
	PixmapButton * m_whiteNoiseWave2Btn;
	PixmapButton * m_smooth2Btn;
	PixmapButton * m_usrWave2Btn;

	PixmapButton * m_XBtn;// For leaving wavetable loading section
	PixmapButton * m_MatrixXBtn;// For when you send something to the Matrix via right click

	PixmapButton * m_normalizeBtn;
	PixmapButton * m_desawBtn;

	PixmapButton * m_openWavetableButton;
	PixmapButton * m_confirmLoadButton;
	PixmapButton * m_openSampleButton;

	PixmapButton * m_modUpArrow[8];
	PixmapButton * m_modDownArrow[8];
	PixmapButton * m_i1Button[8];
	PixmapButton * m_i2Button[8];

	PixmapButton * m_tab1Btn;
	PixmapButton * m_tab2Btn;
	PixmapButton * m_tab3Btn;
	PixmapButton * m_tab4Btn;
	PixmapButton * m_tab5Btn;
	PixmapButton * m_tab6Btn;

	PixmapButton * m_mainFlipBtn;
	PixmapButton * m_subFlipBtn;

	PixmapButton * m_manualBtn;

	PixmapButton * m_removeDCBtn;
	ComboBox * m_oversampleModeBox;

	MicrowaveKnob * m_morphKnob;
	MicrowaveKnob * m_rangeKnob;
	MicrowaveKnob * m_modifyKnob;
	ComboBox * m_modifyModeBox;
	MicrowaveKnob * m_volKnob;
	MicrowaveKnob * m_panKnob;
	MicrowaveKnob * m_detuneKnob;
	MicrowaveKnob * m_phaseKnob;
	MicrowaveKnob * m_phaseRandKnob;
	LedCheckBox * m_enabledToggle;
	LedCheckBox * m_mutedToggle;
	MicrowaveKnob * m_sampLenKnob;
	MicrowaveKnob * m_morphMaxKnob;
	MicrowaveKnob * m_unisonVoicesKnob;
	MicrowaveKnob * m_unisonDetuneKnob;
	MicrowaveKnob * m_unisonMorphKnob;
	MicrowaveKnob * m_unisonModifyKnob;
	LedCheckBox * m_keytrackingToggle;
	MicrowaveKnob * m_tempoKnob;
	LedCheckBox * m_interpolateToggle;

	LedCheckBox * m_subEnabledToggle;
	LedCheckBox * m_subMutedToggle;
	LedCheckBox * m_subKeytrackToggle;
	LedCheckBox * m_subNoiseToggle;
	MicrowaveKnob * m_subVolKnob;
	MicrowaveKnob * m_subPanningKnob;
	MicrowaveKnob * m_subDetuneKnob;
	MicrowaveKnob * m_subPhaseKnob;
	MicrowaveKnob * m_subPhaseRandKnob;
	MicrowaveKnob * m_subSampLenKnob;
	MicrowaveKnob * m_subTempoKnob;
	MicrowaveKnob * m_subRateLimitKnob;
	MicrowaveKnob * m_subUnisonNumKnob;
	MicrowaveKnob * m_subUnisonDetuneKnob;
	LedCheckBox * m_subInterpolateToggle;

	LedCheckBox * m_sampleEnabledToggle;
	LedCheckBox * m_sampleGraphEnabledToggle;
	LedCheckBox * m_sampleMutedToggle;
	LedCheckBox * m_sampleKeytrackingToggle;
	LedCheckBox * m_sampleLoopToggle;
	MicrowaveKnob * m_sampleVolumeKnob;
	MicrowaveKnob * m_samplePanningKnob;
	MicrowaveKnob * m_sampleDetuneKnob;
	MicrowaveKnob * m_samplePhaseKnob;
	MicrowaveKnob * m_samplePhaseRandKnob;
	MicrowaveKnob * m_sampleStartKnob;
	MicrowaveKnob * m_sampleEndKnob;

	ComboBox * m_modInBox[8];
	LcdSpinBox * m_modInNumBox[8];
	MicrowaveKnob * m_modInAmntKnob[8];
	MicrowaveKnob * m_modInCurveKnob[8];
	ComboBox * m_modInBox2[8];
	LcdSpinBox * m_modInNumBox2[8];
	MicrowaveKnob * m_modInAmntKnob2[8];
	MicrowaveKnob * m_modInCurveKnob2[8];
	ComboBox * m_modOutSecBox[8];
	ComboBox * m_modOutSigBox[8];
	LcdSpinBox * m_modOutSecNumBox[8];
	LedCheckBox * m_modEnabledToggle[8];
	ComboBox * m_modCombineTypeBox[8];
	LedCheckBox * m_modTypeToggle[8];
	LedCheckBox * m_modType2Toggle[8];

	MicrowaveKnob * m_filtCutoffKnob[8];
	MicrowaveKnob * m_filtResoKnob[8];
	MicrowaveKnob * m_filtGainKnob[8];
	ComboBox * m_filtTypeBox[8];
	ComboBox * m_filtSlopeBox[8];
	MicrowaveKnob * m_filtInVolKnob[8];
	MicrowaveKnob * m_filtOutVolKnob[8];
	MicrowaveKnob * m_filtWetDryKnob[8];
	MicrowaveKnob * m_filtBalKnob[8];
	MicrowaveKnob * m_filtSatuKnob[8];
	MicrowaveKnob * m_filtFeedbackKnob[8];
	MicrowaveKnob * m_filtDetuneKnob[8];
	LedCheckBox * m_filtEnabledToggle[8];
	LedCheckBox * m_filtMutedToggle[8];
	LedCheckBox * m_filtKeytrackingToggle[8];

	MicrowaveKnob * m_macroKnob[18];

	LcdSpinBox * m_subNumBox;
	LcdSpinBox * m_sampNumBox;
	LcdSpinBox * m_mainNumBox;

	ComboBox * m_oversampleBox;
	ComboBox * m_loadModeBox;

	QLineEdit * m_modNumText[8];

	MicrowaveKnob * m_loadAlgKnob;
	MicrowaveKnob * m_loadChnlKnob;

	MicrowaveKnob * m_visvolKnob;

	Graph * m_graph;
	LedCheckBox * m_visualizeToggle;

	QScrollBar * m_effectScrollBar;
	QScrollBar * m_matrixScrollBar;

	QLabel * m_filtForegroundLabel;
	QLabel * m_filtBoxesLabel;
	QLabel * m_matrixForegroundLabel;
	QLabel * m_matrixBoxesLabel;

	QPalette m_pal = QPalette();
	QPixmap m_tab1ArtworkImg = PLUGIN_NAME::getIconPixmap("tab1_artwork");
	QPixmap m_tab1FlippedArtworkImg = PLUGIN_NAME::getIconPixmap("tab1_artwork_flipped");
	QPixmap m_tab2ArtworkImg = PLUGIN_NAME::getIconPixmap("tab2_artwork");
	QPixmap m_tab2FlippedArtworkImg = PLUGIN_NAME::getIconPixmap("tab2_artwork_flipped");
	QPixmap m_tab3ArtworkImg = PLUGIN_NAME::getIconPixmap("tab3_artwork");
	QPixmap m_tab4ArtworkImg = PLUGIN_NAME::getIconPixmap("tab4_artwork");
	QPixmap m_tab5ArtworkImg = PLUGIN_NAME::getIconPixmap("tab5_artwork");
	QPixmap m_tab6ArtworkImg = PLUGIN_NAME::getIconPixmap("tab6_artwork");
	QPixmap m_tab7ArtworkImg = PLUGIN_NAME::getIconPixmap("tab7_artwork");

	QString m_wavetableFileName = "";

	Microwave * m_microwave;

	float m_temp1;
	float m_temp2;

	int m_tabWhenSendingToMatrix;

	Microwave * m_b = castModel<Microwave>();

	friend class mSynth;
};


class mSynth
{
	MM_OPERATORS
public:
	mSynth(NotePlayHandle * nph,
		float * morph, float * range, float * modify, int * modifyMode, float * vol, float * pan, float * detune, float * phase, float * phaseRand, bool * enabled, bool * muted,
		float * sampLen, float * morphMax, float * unisonVoices, float * unisonDetune, float * unisonMorph, float * unisonModify, bool * keytracking, float * tempo, bool * interpolate,
		bool * subEnabled, bool * subMuted, bool * subKeytrack, bool * subNoise, float * subVol, float * subPanning, float * subDetune, float * subPhase, float * subPhaseRand,
		float * subSampLen, float * subTempo, float * subRateLimit, float * subUnisonNum, float * subUnisonDetune, bool * subInterpolate,
		bool * sampleEnabled, bool * sampleMuted, bool * sampleKeytracking, bool * sampleGraphEnabled, bool * sampleLoop, float * sampleVolume, float * samplePanning,
		float * sampleDetune, float * samplePhase, float * samplePhaseRand, float * sampleStart, float * sampleEnd,
		int * modIn, int * modInNum, float * modInAmnt, float * modInCurve, int * modIn2, int * modInNum2, float * modInAmnt2, float * modInCurve2,
		int * modOutSec, int * modOutSig, int * modOutSecNum, bool * modEnabled, int * modCombineType, bool * modType, bool * modType2,
		float * filtCutoff, float * filtReso, float * filtGain, int * filtType, int * filtSlope, float * filtInVol, float * filtOutVol, float * filtWetDry, float * filtBal,
		float * filtSatu, float * filtFeedback, float * filtDetune, bool * filtEnabled, bool * filtMuted, bool * filtKeytracking,
		float * macro,
		std::vector<float> (&samples)[8][2]);
		
	virtual ~mSynth();
	
	void nextStringSample(sampleFrame &outputSample, std::vector<float> (&m_waveforms)[8], std::vector<float> (&subs)[64], std::vector<float> (&m_samples)[8][2], float * sampGraphs, int maxMainEnabled, int maxSubEnabled, int maxSampleEnabled, int maxFiltEnabled, int maxModEnabled, int sample_rate, Microwave * mwc, bool removeDC, std::vector<float> (&m_storedsubs)[64]);

	inline float detuneWithCents(float pitchValue, float detuneValue);

	inline void refreshValue(int which, int num, Microwave * mwc);

	inline float realfmod(float k, float n);

private:
	double m_sample_realindex[8][32] = {{0}};
	double m_sample_subindex[64][32] = {{0}};
	double m_sample_sampleindex[8] = {0};
	NotePlayHandle* m_nph;

	int m_noteDuration;
	float m_noteFreq;

	float m_lastMainOscVal[8][2] = {{0}};
	float m_lastSubVal[64][2] = {{0}};
	float m_lastSubNoiseVal[64][32] = {{0}};
	double m_sample_step_sub = 0;
	float m_noiseSampRand = 0;
	float m_lastSampleVal[8][2] = {{0}};
	double m_sample_step_sample = 0;

	float m_lastMainOscEnvVal[8][2] = {{0}};
	float m_lastSubEnvVal[64][2] = {{0}};
	float m_lastSampleEnvVal[8][2] = {{0}};

	bool m_lastMainOscEnvDone[8] = {false};
	bool m_lastSubEnvDone[64] = {false};
	bool m_lastSampleEnvDone[8] = {false};

	int m_subNoiseDirection[64][32] = {{0}};

	int m_loopStart = 0;
	int m_loopEnd = 0;
	float m_currentRangeValInvert = 0;
	int m_currentSampLen = 0;
	int m_currentIndex = 0;

	float m_filtInputs[8][2] = {{0}};// [filter number][channel]
	float m_filtOutputs[8][2] = {{0}};// [filter number][channel]
	float m_filtPrevSampIn[8][8][5][2] = {{{{0}}}};// [filter number][slope][samples back in time][channel]
	float m_filtPrevSampOut[8][8][5][2] = {{{{0}}}};// [filter number][slope][samples back in time][channel]
	float m_filtModOutputs[8][2] = {{0}};// [filter number][channel]

	std::vector<float> m_filtDelayBuf[8][2];// [filter number][channel]

	float m_filty1[2] = {0};
	float m_filty2[2] = {0};
	float m_filty3[2] = {0};
	float m_filty4[2] = {0};
	float m_filtoldx[2] = {0};
	float m_filtoldy1[2] = {0};
	float m_filtoldy2[2] = {0};
	float m_filtoldy3[2] = {0};
	float m_filtx[2] = {0};

	int m_modValType[128] = {0};
	int m_modValNum[128] = {0};

	int m_numberToReset = 0;


	float m_morph[8];
	float m_range[8];
	float m_modify[8];
	int m_modifyMode[8];
	float m_vol[8];
	float m_pan[8];
	float m_detune[8];
	float m_phase[8];
	float m_phaseRand[8];
	bool m_enabled[8];
	bool m_muted[8];
	float m_sampLen[8];
	float m_morphMax[8];
	float m_unisonVoices[8];
	float m_unisonDetune[8];
	float m_unisonMorph[8];
	float m_unisonModify[8];
	bool m_keytracking[8];
	float m_tempo[8];
	bool m_interpolate[64];

	bool m_subEnabled[64];
	bool m_subMuted[64];
	bool m_subKeytrack[64];
	bool m_subNoise[64];
	float m_subVol[64];
	float m_subPanning[64];
	float m_subDetune[64];
	float m_subPhase[64];
	float m_subPhaseRand[64];
	float m_subSampLen[64];
	float m_subTempo[64];
	float m_subRateLimit[64];
	float m_subUnisonNum[64];
	float m_subUnisonDetune[64];
	bool m_subInterpolate[64];

	bool m_sampleEnabled[8];
	bool m_sampleMuted[8];
	bool m_sampleKeytracking[8];
	bool m_sampleGraphEnabled[8];
	bool m_sampleLoop[8];
	float m_sampleVolume[8];
	float m_samplePanning[8];
	float m_sampleDetune[8];
	float m_samplePhase[8];
	float m_samplePhaseRand[8];
	float m_sampleStart[8];
	float m_sampleEnd[8];

	int m_modIn[64];
	int m_modInNum[64];
	float m_modInAmnt[64];
	float m_modInCurve[64];
	int m_modIn2[64];
	int m_modInNum2[64];
	float m_modInAmnt2[64];
	float m_modInCurve2[64];
	int m_modOutSec[64];
	int m_modOutSig[64];
	int m_modOutSecNum[64];
	bool m_modEnabled[64];
	int m_modCombineType[64];
	bool m_modType[64];
	bool m_modType2[64];

	float m_filtCutoff[8];
	float m_filtReso[8];
	float m_filtGain[8];
	int m_filtType[8];
	int m_filtSlope[8];
	float m_filtInVol[8];
	float m_filtOutVol[8];
	float m_filtWetDry[8];
	float m_filtBal[8];
	float m_filtSatu[8];
	float m_filtFeedback[8];
	float m_filtDetune[8];
	bool m_filtEnabled[8];
	bool m_filtMuted[8];
	bool m_filtKeytracking[8];

	float m_cutoff;
	int m_mode;
	float m_reso;
	float m_dbgain;
	float m_Fs;
	float m_a0;
	float m_a1;
	float m_a2;
	float m_b0;
	float m_b1;
	float m_b2;
	float m_alpha;
	float m_w0;
	float m_A;
	float m_f;
	float m_k;
	float m_p;
	float m_scale;
	float m_r;

	float m_humanizer[8] = {0};

	float m_unisonDetuneAmounts[8][32] = {{0}};
	float m_subUnisonDetuneAmounts[64][32] = {{0}};

	// Usually used for CPU improvements
	float m_temp1;
	float m_temp2;
	float m_temp3;
	float m_temp4;
	float m_temp5;
	float m_temp6;
	float m_temp7;

	float m_curModVal[2] = {0};
	float m_curModVal2[2] = {0};
	float m_curModValCurve[2] = {0};
	float m_curModVal2Curve[2] = {0};
	float m_comboModVal[2] = {0};
	float m_comboModValMono = 0;

	float m_sample_morerealindex[8][32] = {{0}};
	double m_sample_step[8][32] = {{0}};

	float m_unisonVoicesMinusOne = 0;
	float m_subUnisonVoicesMinusOne = 0;

	int m_filtFeedbackLoc[8] = {0};

	float m_macro[18];

	// For DC offset removal
	float m_averageSampleValue[2] = {0};

	sample_t m_mainsample[8][32] = {{0}};
	sample_t m_subsample[64][32] = {{0}};
	sample_t m_samplesample[8][2] = {{0}};

	float m_progress;
	float m_progress2;
	float m_progress3;
	int m_intprogress;

	sampleFrame m_sampleMainOsc;
	sampleFrame m_sampleSubOsc;

	friend class Microwave;
	
};


class MicrowaveManualView: public QTextEdit
{
	Q_OBJECT
public:
	static MicrowaveManualView* getInstance()
	{
		static MicrowaveManualView instance;
		return &instance;
	}
	static void finalize()
	{
	}

private:
	MicrowaveManualView();
	static QString s_manualText;
};




#endif
