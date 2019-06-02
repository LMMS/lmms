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

#define setwavemodel( name )\
		name->clear();\
		name->addItem( tr( "None" ), make_unique<PluginPixmapLoader>( "none" ) );\
		name->addItem( tr( "Pulse Width" ), make_unique<PluginPixmapLoader>( "sin" ) );\
		name->addItem( tr( "Weird 1" ), make_unique<PluginPixmapLoader>( "noise" ) );\
		name->addItem( tr( "Weird 2" ), make_unique<PluginPixmapLoader>( "noise" ) );\
		name->addItem( tr( "Asym To Right" ), make_unique<PluginPixmapLoader>( "saw" ) );\
		name->addItem( tr( "Asym To Left" ), make_unique<PluginPixmapLoader>( "ramp" ) );\
		name->addItem( tr( "Bidirectional Asym" ), make_unique<PluginPixmapLoader>( "tri" ) );\
		name->addItem( tr( "Squish To Center" ), make_unique<PluginPixmapLoader>( "exp" ) );\
		name->addItem( tr( "Stretch From Center" ), make_unique<PluginPixmapLoader>( "sinabs" ) );\
		name->addItem( tr( "Stretch And Squish" ), make_unique<PluginPixmapLoader>( "tri" ) );\
		name->addItem( tr( "Cut Off Right" ), make_unique<PluginPixmapLoader>( "saw" ) );\
		name->addItem( tr( "Cut Off Left" ), make_unique<PluginPixmapLoader>( "ramp" ) );\
		name->addItem( tr( "Squarify" ), make_unique<PluginPixmapLoader>( "sqr" ) );\
		name->addItem( tr( "Pulsify" ), make_unique<PluginPixmapLoader>( "sqr" ) );\
		name->addItem( tr( "Flip" ), make_unique<PluginPixmapLoader>( "tri" ) );\
		name->addItem( tr( "Clip" ), make_unique<PluginPixmapLoader>( "sqr" ) );\
		name->addItem( tr( "Inverse Clip" ), make_unique<PluginPixmapLoader>( "sqr" ) );\
		name->addItem( tr( "Sine" ), make_unique<PluginPixmapLoader>( "sin" ) );\
		name->addItem( tr( "Atan" ), make_unique<PluginPixmapLoader>( "tri" ) );\
		name->addItem( tr( "Sync" ), make_unique<PluginPixmapLoader>( "saw" ) );\
		name->addItem( tr( "Sync Half Interpolate" ), make_unique<PluginPixmapLoader>( "saw" ) );\
		name->addItem( tr( "Sync Interpolate" ), make_unique<PluginPixmapLoader>( "saw" ) );\
		name->addItem( tr( "Mirror" ), make_unique<PluginPixmapLoader>( "sinabs" ) );\
		name->addItem( tr( "Diagonal Morph" ), make_unique<PluginPixmapLoader>( "saw" ) );\
		name->addItem( tr( "Sideways Morph" ), make_unique<PluginPixmapLoader>( "saw" ) );

#define modinmodel( name )\
		name->clear();\
		name->addItem( tr( "None" ), make_unique<PluginPixmapLoader>( "none" ) );\
		name->addItem( tr( "Main OSC" ), make_unique<PluginPixmapLoader>( "sqr" ) );\
		name->addItem( tr( "Sub OSC" ), make_unique<PluginPixmapLoader>( "sin" ) );\
		name->addItem( tr( "Sample OSC" ), make_unique<PluginPixmapLoader>( "noise" ) );\
		name->addItem( tr( "Filter Output" ), make_unique<PluginPixmapLoader>( "moog" ) );\
		name->addItem( tr( "Velocity" ), make_unique<PluginPixmapLoader>( "letter_v" ) );\
		name->addItem( tr( "Panning" ), make_unique<PluginPixmapLoader>( "letter_p" ) );\
		name->addItem( tr( "Humanizer" ), make_unique<PluginPixmapLoader>( "letter_h" ) );\
		name->addItem( tr( "Macro" ), make_unique<PluginPixmapLoader>( "letter_m" ) );

#define modsectionsmodel( name )\
		name->clear();\
		name->addItem( tr( "None" ), make_unique<PluginPixmapLoader>( "none" ) );\
		name->addItem( tr( "Main OSC" ), make_unique<PluginPixmapLoader>( "sqr" ) );\
		name->addItem( tr( "Sub OSC" ), make_unique<PluginPixmapLoader>( "sin" ) );\
		name->addItem( tr( "Sample OSC" ), make_unique<PluginPixmapLoader>( "noise" ) );\
		name->addItem( tr( "Matrix" ), make_unique<PluginPixmapLoader>( "ramp" ) );\
		name->addItem( tr( "Filter Input" ), make_unique<PluginPixmapLoader>( "moog" ) );\
		name->addItem( tr( "Filter Parameters" ), make_unique<PluginPixmapLoader>( "letter_f" ) );\
		name->addItem( tr( "Macro" ), make_unique<PluginPixmapLoader>( "letter_m" ) );

#define mainoscsignalsmodel( name )\
		name->clear();\
		name->addItem( tr( "None" ), make_unique<PluginPixmapLoader>( "none" ) );\
		name->addItem( tr( "Morph" ), make_unique<PluginPixmapLoader>( "tri" ) );\
		name->addItem( tr( "Range" ), make_unique<PluginPixmapLoader>( "sqr" ) );\
		name->addItem( tr( "Modify" ), make_unique<PluginPixmapLoader>( "moog" ) );\
		name->addItem( tr( "Detune" ), make_unique<PluginPixmapLoader>( "saw" ) );\
		name->addItem( tr( "Phase" ), make_unique<PluginPixmapLoader>( "sin" ) );\
		name->addItem( tr( "Volume" ), make_unique<PluginPixmapLoader>( "letter_v" ) );\
		name->addItem( tr( "Panning" ), make_unique<PluginPixmapLoader>( "letter_p" ) );\
		name->addItem( tr( "Unison Number" ), make_unique<PluginPixmapLoader>( "ramp" ) );\
		name->addItem( tr( "Unison Detune" ), make_unique<PluginPixmapLoader>( "saw" ) );\
		name->addItem( tr( "Unison Morph" ), make_unique<PluginPixmapLoader>( "tri" ) );\
		name->addItem( tr( "Unison Modify" ), make_unique<PluginPixmapLoader>( "moog" ) );

#define subsignalsmodel( name )\
		name->clear();\
		name->addItem( tr( "None" ), make_unique<PluginPixmapLoader>( "none" ) );\
		name->addItem( tr( "Detune" ), make_unique<PluginPixmapLoader>( "saw" ) );\
		name->addItem( tr( "Phase" ), make_unique<PluginPixmapLoader>( "sin" ) );\
		name->addItem( tr( "Volume" ), make_unique<PluginPixmapLoader>( "letter_v" ) );\
		name->addItem( tr( "Panning" ), make_unique<PluginPixmapLoader>( "letter_p" ) );\
		name->addItem( tr( "Length" ), make_unique<PluginPixmapLoader>( "letter_l" ) );\
		name->addItem( tr( "Rate Limit" ), make_unique<PluginPixmapLoader>( "letter_r" ) );\
		name->addItem( tr( "Unison Voice Number" ), make_unique<PluginPixmapLoader>( "ramp" ) );\
		name->addItem( tr( "Unison Detune" ), make_unique<PluginPixmapLoader>( "saw" ) );

#define samplesignalsmodel( name )\
		name->clear();\
		name->addItem( tr( "None" ), make_unique<PluginPixmapLoader>( "none" ) );\
		name->addItem( tr( "Detune" ), make_unique<PluginPixmapLoader>( "saw" ) );\
		name->addItem( tr( "Phase" ), make_unique<PluginPixmapLoader>( "sin" ) );\
		name->addItem( tr( "Volume" ), make_unique<PluginPixmapLoader>( "letter_v" ) );\
		name->addItem( tr( "Panning" ), make_unique<PluginPixmapLoader>( "letter_p" ) );

#define matrixsignalsmodel( name )\
		name->clear();\
		name->addItem( tr( "None" ), make_unique<PluginPixmapLoader>( "none" ) );\
		name->addItem( tr( "Amount" ), make_unique<PluginPixmapLoader>( "letter_a" ) );\
		name->addItem( tr( "Curve" ), make_unique<PluginPixmapLoader>( "letter_c" ) );\
		name->addItem( tr( "Secondary Amount" ), make_unique<PluginPixmapLoader>( "letter_a" ) );\
		name->addItem( tr( "Secondary Curve" ), make_unique<PluginPixmapLoader>( "letter_c" ) );\
		name->addItem( tr( "Input Section" ), make_unique<PluginPixmapLoader>( "letter_i" ) );\
		name->addItem( tr( "Input Number" ), make_unique<PluginPixmapLoader>( "letter_i" ) );\
		name->addItem( tr( "Secondary Input Section" ), make_unique<PluginPixmapLoader>( "letter_i" ) );\
		name->addItem( tr( "Secondary Input Number" ), make_unique<PluginPixmapLoader>( "letter_i" ) );\
		name->addItem( tr( "Output Section 1" ), make_unique<PluginPixmapLoader>( "letter_o" ) );\
		name->addItem( tr( "Output Section 2" ), make_unique<PluginPixmapLoader>( "letter_o" ) );\
		name->addItem( tr( "Output Section Number" ), make_unique<PluginPixmapLoader>( "letter_o" ) );\

#define filtersignalsmodel( name )\
		name->clear();\
		name->addItem( tr( "None" ), make_unique<PluginPixmapLoader>( "none" ) );\
		name->addItem( tr( "Cutoff Frequency" ), make_unique<PluginPixmapLoader>( "moog" ) );\
		name->addItem( tr( "Resonance" ), make_unique<PluginPixmapLoader>( "ramp" ) );\
		name->addItem( tr( "db Gain" ), make_unique<PluginPixmapLoader>( "ramp" ) );\
		name->addItem( tr( "Filter Type" ), make_unique<PluginPixmapLoader>( "ramp" ) );\
		name->addItem( tr( "Slope" ), make_unique<PluginPixmapLoader>( "ramp" ) );\
		name->addItem( tr( "Input Volume" ), make_unique<PluginPixmapLoader>( "sin" ) );\
		name->addItem( tr( "Output Volume" ), make_unique<PluginPixmapLoader>( "ramp" ) );\
		name->addItem( tr( "Wet/Dry" ), make_unique<PluginPixmapLoader>( "ramp" ) );\
		name->addItem( tr( "Balance/Panning" ), make_unique<PluginPixmapLoader>( "ramp" ) );\
		name->addItem( tr( "Saturation" ), make_unique<PluginPixmapLoader>( "ramp" ) );\
		name->addItem( tr( "Feedback" ), make_unique<PluginPixmapLoader>( "ramp" ) );\
		name->addItem( tr( "Detune" ), make_unique<PluginPixmapLoader>( "ramp" ) );

#define mod8model( name )\
		name->clear();\
		name->addItem( tr( "1" ), make_unique<PluginPixmapLoader>( "number_1" ) );\
		name->addItem( tr( "2" ), make_unique<PluginPixmapLoader>( "number_2" ) );\
		name->addItem( tr( "3" ), make_unique<PluginPixmapLoader>( "number_3" ) );\
		name->addItem( tr( "4" ), make_unique<PluginPixmapLoader>( "number_4" ) );\
		name->addItem( tr( "5" ), make_unique<PluginPixmapLoader>( "number_5" ) );\
		name->addItem( tr( "6" ), make_unique<PluginPixmapLoader>( "number_6" ) );\
		name->addItem( tr( "7" ), make_unique<PluginPixmapLoader>( "number_7" ) );\
		name->addItem( tr( "8" ), make_unique<PluginPixmapLoader>( "number_8" ) );

#define matrixoutmodel( name )\
		name->clear();\
		name->addItem( tr( "1" ) );\
		name->addItem( tr( "2" ) );\
		name->addItem( tr( "3" ) );\
		name->addItem( tr( "4" ) );\
		name->addItem( tr( "5" ) );\
		name->addItem( tr( "6" ) );\
		name->addItem( tr( "7" ) );\
		name->addItem( tr( "8" ) );\
		name->addItem( tr( "9" ) );\
		name->addItem( tr( "10" ) );\
		name->addItem( tr( "11" ) );\
		name->addItem( tr( "12" ) );\
		name->addItem( tr( "13" ) );\
		name->addItem( tr( "14" ) );\
		name->addItem( tr( "15" ) );\
		name->addItem( tr( "16" ) );\
		name->addItem( tr( "17" ) );\
		name->addItem( tr( "18" ) );

#define filtertypesmodel( name )\
		name->clear();\
		name->addItem( tr( "Lowpass" ), make_unique<PluginPixmapLoader>( "filter_lowpass" ) );\
		name->addItem( tr( "Highpass" ), make_unique<PluginPixmapLoader>( "filter_highpass" ) );\
		name->addItem( tr( "Bandpass" ), make_unique<PluginPixmapLoader>( "filter_bandpass" ) );\
		name->addItem( tr( "Low Shelf" ), make_unique<PluginPixmapLoader>( "filter_lowshelf" ) );\
		name->addItem( tr( "High Shelf" ), make_unique<PluginPixmapLoader>( "filter_highshelf" ) );\
		name->addItem( tr( "Peak" ), make_unique<PluginPixmapLoader>( "filter_peak" ) );\
		name->addItem( tr( "Notch" ), make_unique<PluginPixmapLoader>( "filter_notch" ) );\
		name->addItem( tr( "Allpass" ), make_unique<PluginPixmapLoader>( "filter_allpass" ) );\
		name->addItem( tr( "Moog Lowpass (Note: Slope is double)" ), make_unique<PluginPixmapLoader>( "filter_moog" ) );

#define filterslopesmodel( name )\
		name->clear();\
		name->addItem( tr( "12 db" ), make_unique<PluginPixmapLoader>( "number_1" ) );\
		name->addItem( tr( "24 db" ), make_unique<PluginPixmapLoader>( "number_2" ) );\
		name->addItem( tr( "36 db" ), make_unique<PluginPixmapLoader>( "number_3" ) );\
		name->addItem( tr( "48 db" ), make_unique<PluginPixmapLoader>( "number_4" ) );\
		name->addItem( tr( "60 db" ), make_unique<PluginPixmapLoader>( "number_5" ) );\
		name->addItem( tr( "72 db" ), make_unique<PluginPixmapLoader>( "number_6" ) );\
		name->addItem( tr( "84 db" ), make_unique<PluginPixmapLoader>( "number_7" ) );\
		name->addItem( tr( "96 db" ), make_unique<PluginPixmapLoader>( "number_8" ) );

#define modcombinetypemodel( name )\
		name->clear();\
		name->addItem( tr( "Add Bidirectional" ), make_unique<PluginPixmapLoader>( "number_1" ) );\
		name->addItem( tr( "Multiply Bidirectional" ), make_unique<PluginPixmapLoader>( "number_2" ) );\
		name->addItem( tr( "Add Unidirectional" ), make_unique<PluginPixmapLoader>( "number_3" ) );\
		name->addItem( tr( "Multiply Unidirectional" ), make_unique<PluginPixmapLoader>( "number_4" ) );

#define oversamplemodel( name )\
		name.clear();\
		name.addItem( tr( "1x" ), make_unique<PluginPixmapLoader>( "number_1" ) );\
		name.addItem( tr( "2x" ), make_unique<PluginPixmapLoader>( "number_2" ) );\
		name.addItem( tr( "3x" ), make_unique<PluginPixmapLoader>( "number_3" ) );\
		name.addItem( tr( "4x" ), make_unique<PluginPixmapLoader>( "number_4" ) );\
		name.addItem( tr( "5x" ), make_unique<PluginPixmapLoader>( "number_5" ) );\
		name.addItem( tr( "6x" ), make_unique<PluginPixmapLoader>( "number_6" ) );\
		name.addItem( tr( "7x" ), make_unique<PluginPixmapLoader>( "number_7" ) );\
		name.addItem( tr( "8x" ), make_unique<PluginPixmapLoader>( "number_8" ) );

#define loadmodemodel( name )\
		name.clear();\
		name.addItem( tr( "Lock waveform edges to zero crossings" ) );\
		name.addItem( tr( "Load sample without changes" ) );\
		name.addItem( tr( "Load wavetable file" ) );\
		name.addItem( tr( "Autocorrelation (static pitch detection)" ) );

#define oversamplemodemodel( name )\
		name.clear();\
		name.addItem( tr( "Decimate" ), make_unique<PluginPixmapLoader>( "number_1" ) );\
		name.addItem( tr( "Average" ), make_unique<PluginPixmapLoader>( "number_2" ) );

#define visimove( name, x, y )\
	if( x >= 0 && x <= 250 )\
	{\
		name->move( x, y );\
		name->setVisible( true );\
	}\
	else\
	{\
		name->move( 0, 0 );\
		name->setVisible( false );\
	}


// Create the knob, set its tooltip, set its default color
#define makeknob( name, knobtype, hint, tooltip )\
	name = new MicrowaveKnob( knobtype, this );\
	name->setHintText( hint, "" );\
	ToolTip::add( name, tooltip );\
	name->knobView = this;\
	name->setarcColor( QColor(46,74,80) );\
	name->setlineColor( QColor(102,198,199) );\
	name->setInnerColor( QColor(64,92,97) );\
	connect( name, &MicrowaveKnob::updateScroll, this, &MicrowaveView::updateScroll );


// Constant variables for use in many situations.  Strongly related to srccpy stuff.

const int STOREDSUBWAVELEN = 2048;
const int STOREDMAINWAVELEN = 2048;

// WAVERATIO is how much larger (multiplication) the waveforms are after interpolation.
// I'd like to increase this, but I will not until project saving/loading is sped up in LMMS, since this increases that drastically.
// I would prefer it to be 8, maybe even 16.
// This number divided by 2 is the number of megabytes of RAM each wavetable will use up.
const int WAVERATIO = 4;

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
	void setMatrixLocation( int loc1, int loc2, int loc3 );

	void setWhichMacroKnob( int which );

	MicrowaveView * knobView;
signals:
	void sendToMatrixAsOutput();
	void switchToMatrixKnob();
	void updateScroll();
	void setMacroTooltip();
	void chooseMacroColor();
	void refreshMacroColor();
	void setMacroColortoDefault();
protected:
	virtual void contextMenuEvent( QContextMenuEvent * _me );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );
private:
	int matrixLocation[3] = {0};
	bool ignoreShift = false;
	int whichMacroKnob = -1;
};


class Microwave : public Instrument
{
	Q_OBJECT

public:
	Microwave(InstrumentTrack * _instrument_track );
	virtual ~Microwave();
	virtual PluginView * instantiateView( QWidget * _parent );
	virtual QString nodeName() const;
	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );
	virtual void playNote( NotePlayHandle * _n, sampleFrame * _working_buffer );
	virtual void deleteNotePluginData( NotePlayHandle * _n );

	virtual f_cnt_t desiredReleaseFrames() const
	{
		return( 64 );
	}

	void switchMatrixSections( int source, int destination );

	inline void fillSubOsc( int which, bool doInterpolate = true );
	inline void fillMainOsc( int which, bool doInterpolate = true );

	float scroll = 0;
	bool viewOpen = false;

protected slots:
	void valueChanged( int, int );
	void morphMaxChanged( int );
	void sampLenChanged( int );
	void subSampLenChanged( int );
	void mainEnabledChanged( int );
	void subEnabledChanged( int );
	void modEnabledChanged( int );
	void filtEnabledChanged( int );
	void sampleEnabledChanged( int );
	void samplesChanged( int, int );
	void interpolateChanged( int );
	void subInterpolateChanged( int );

private:

	// Stolen from WatSyn, with a few changes
	// memcpy utilizing libsamplerate (src) for sinc interpolation
	inline void srccpy( float * _dst, float * _src, int wavelength )
	{
		int err;
		const int margin = 64;
		
		// copy to temp array
		std::unique_ptr<float[]> tmps(new float[wavelength + margin]);
		float * tmp = &tmps.get()[0];

		memcpy( tmp, _src, sizeof( float ) * wavelength );
		memcpy( tmp + wavelength, _src, sizeof( float ) * margin );
		SRC_STATE * src_state = src_new( SRC_SINC_FASTEST, 1, &err );
		SRC_DATA src_data;
		src_data.data_in = tmp;
		src_data.input_frames = wavelength + margin;
		src_data.data_out = _dst;
		src_data.output_frames = wavelength * WAVERATIO;
		src_data.src_ratio = static_cast<double>( WAVERATIO );
		src_data.end_of_input = 0;
		err = src_process( src_state, &src_data ); 
		if( err ) { qDebug( "Microwave SRC error: %s", src_strerror( err ) ); }
		src_delete( src_state );
	}

	FloatModel * morph[8];
	FloatModel * range[8];
	FloatModel * modify[8];
	ComboBoxModel * modifyMode[8];
	FloatModel * vol[8];
	FloatModel * pan[8];
	FloatModel * detune[8];
	FloatModel * phase[8];
	FloatModel * phaseRand[8];
	BoolModel * enabled[8];
	BoolModel * muted[8];
	FloatModel * sampLen[8];
	FloatModel * morphMax[8];
	FloatModel * unisonVoices[8];
	FloatModel * unisonDetune[8];
	FloatModel * unisonMorph[8];
	FloatModel * unisonModify[8];
	BoolModel * keytracking[8];
	FloatModel * tempo[8];
	BoolModel * interpolate[8];

	BoolModel * subEnabled[64];
	BoolModel * subMuted[64];
	BoolModel * subKeytrack[64];
	BoolModel * subNoise[64];
	FloatModel * subVol[64];
	FloatModel * subPanning[64];
	FloatModel * subDetune[64];
	FloatModel * subPhase[64];
	FloatModel * subPhaseRand[64];
	FloatModel * subSampLen[64];
	FloatModel * subTempo[64];
	FloatModel * subRateLimit[64];
	FloatModel * subUnisonNum[64];
	FloatModel * subUnisonDetune[64];
	BoolModel * subInterpolate[64];

	BoolModel * sampleEnabled[8];
	BoolModel * sampleGraphEnabled[8];
	BoolModel * sampleMuted[8];
	BoolModel * sampleKeytracking[8];
	BoolModel * sampleLoop[8];
	FloatModel * sampleVolume[8];
	FloatModel * samplePanning[8];
	FloatModel * sampleDetune[8];
	FloatModel * samplePhase[8];
	FloatModel * samplePhaseRand[8];
	FloatModel * sampleStart[8];
	FloatModel * sampleEnd[8];

	ComboBoxModel * modIn[64];
	IntModel * modInNum[64];
	FloatModel * modInAmnt[64];
	FloatModel * modInCurve[64];
	ComboBoxModel * modIn2[64];
	IntModel * modInNum2[64];
	FloatModel * modInAmnt2[64];
	FloatModel * modInCurve2[64];
	ComboBoxModel * modOutSec[64];
	ComboBoxModel * modOutSig[64];
	IntModel * modOutSecNum[64];
	BoolModel * modEnabled[64];
	ComboBoxModel * modCombineType[64];
	BoolModel * modType[64];
	BoolModel * modType2[64];

	FloatModel * filtCutoff[8];
	FloatModel * filtReso[8];
	FloatModel * filtGain[8];
	ComboBoxModel * filtType[8];
	ComboBoxModel * filtSlope[8];
	FloatModel * filtInVol[8];
	FloatModel * filtOutVol[8];
	FloatModel * filtWetDry[8];
	FloatModel * filtBal[8];
	FloatModel * filtSatu[8];
	FloatModel * filtFeedback[8];
	FloatModel * filtDetune[8];
	BoolModel * filtEnabled[8];
	BoolModel * filtMuted[8];
	BoolModel * filtKeytracking[8];

	FloatModel * macro[18];
	QString macroTooltips[18] = {};
	int macroColors[18][3] = {{0}};

	FloatModel  visvol;

	FloatModel  loadAlg;
	FloatModel  loadChnl;

	IntModel  mainNum;
	IntModel  subNum;
	IntModel  sampNum;
	ComboBoxModel  oversample;
	ComboBoxModel oversampleMode;
	ComboBoxModel  loadMode;

	graphModel  graph;
	
	BoolModel visualize;

	float storedwaveforms[8][STOREDMAINARRAYLEN] = {{0}};
	float waveforms[8][MAINARRAYLEN] = {{0}};
	bool mainFilled[8] = {false};
	int currentTab = 0;
	float storedsubs[64][STOREDSUBWAVELEN] = {{0}};
	float subs[64][SUBWAVELEN] = {{0}};
	bool subFilled[64] = {false};
	float sampGraphs[1024] = {0};
	std::vector<float> samples[8][2];

	BoolModel mainFlipped;
	BoolModel subFlipped;

	BoolModel removeDC;

	SampleBuffer sampleBuffer;

	//Below is for passing to mSynth initialization
	float morphArr[8] = {0};
	float rangeArr[8] = {0};
	float modifyArr[8] = {0};
	int modifyModeArr[8] = {0};
	float volArr[8] = {0};
	float panArr[8] = {0};
	float detuneArr[8] = {0};
	float phaseArr[8] = {0};
	float phaseRandArr[8] = {0};
	bool enabledArr[8] = {false};
	bool mutedArr[8] = {false};
	float sampLenArr[8] = {0};
	float morphMaxArr[8] = {0};
	float unisonVoicesArr[8] = {0};
	float unisonDetuneArr[8] = {0};
	float unisonMorphArr[8] = {0};
	float unisonModifyArr[8] = {0};
	bool keytrackingArr[8] = {true};
	float tempoArr[8] = {0};
	bool interpolateArr[8] = {true};

	int modInArr[64] = {0};
	int modInNumArr[64] = {0};
	float modInAmntArr[64] = {0};
	float modInCurveArr[64] = {0};
	int modIn2Arr[64] = {0};
	int modInNum2Arr[64] = {0};
	float modInAmnt2Arr[64] = {0};
	float modInCurve2Arr[64] = {0};
	int modOutSecArr[64] = {0};
	int modOutSigArr[64] = {0};
	int modOutSecNumArr[64] = {0};
	bool modEnabledArr[64] = {false};
	int modCombineTypeArr[64] = {0};
	bool modTypeArr[64] = {0};
	bool modType2Arr[64] = {0};
	
	bool subEnabledArr[64] = {false};
	float subVolArr[64] = {0};
	float subPhaseArr[64] = {0};
	float subPhaseRandArr[64] = {0};
	float subDetuneArr[64] = {0};
	bool subMutedArr[64] = {false};
	bool subKeytrackArr[64] = {false};
	float subSampLenArr[64] = {0};
	bool subNoiseArr[64] = {false};
	float subPanningArr[64] = {0};
	float subTempoArr[64] = {0};
	float subRateLimitArr[64] = {0};
	float subUnisonNumArr[64] = {0};
	float subUnisonDetuneArr[64] = {0};
	bool subInterpolateArr[64] = {true};
	
	float filtInVolArr[8] = {0};
	int filtTypeArr[8] = {0};
	int filtSlopeArr[8] = {0};
	float filtCutoffArr[8] = {0};
	float filtResoArr[8] = {0};
	float filtGainArr[8] = {0};
	float filtSatuArr[8] = {0};
	float filtWetDryArr[8] = {0};
	float filtBalArr[8] = {0};
	float filtOutVolArr[8] = {0};
	bool filtEnabledArr[8] = {false};
	float filtFeedbackArr[8] = {0};
	float filtDetuneArr[8] = {0};
	bool filtKeytrackingArr[8] = {false};
	bool filtMutedArr[8] = {false};

	bool sampleEnabledArr[8] = {false};
	bool sampleGraphEnabledArr[8] = {false};
	bool sampleMutedArr[8] = {false};
	bool sampleKeytrackingArr[8] = {false};
	bool sampleLoopArr[8] = {false};
	float sampleVolumeArr[8] = {0};
	float samplePanningArr[8] = {0};
	float sampleDetuneArr[8] = {0};
	float samplePhaseArr[8] = {0};
	float samplePhaseRandArr[8] = {0};
	float sampleStartArr[8] = {0};
	float sampleEndArr[8] = {1, 1, 1, 1, 1, 1, 1, 1};

	float macroArr[18] = {0};
	//Above is for passing to mSynth initialization

	int maxMainEnabled = 0;// The highest number of main oscillator sections that must be looped through
	int maxModEnabled = 0;// The highest number of matrix sections that must be looped through
	int maxSubEnabled = 0;// The highest number of sub oscillator sections that must be looped through
	int maxSampleEnabled = 0;// The highest number of sample sections that must be looped through
	int maxFiltEnabled = 0;// The highest number of filter sections that must be looped through

	float visualizerValues[204] = {0};

	QString wavetableSaveStrings[8] = {""};
	bool updateWavetable[8] = {true,true,true,true,true,true,true,true};

	ConstNotePlayHandleList nphList;

	InstrumentTrack * microwaveTrack = this->instrumentTrack();

	Microwave * mwc;
	
	friend class MicrowaveView;
	friend class mSynth;
	friend class MicrowaveKnob;
};



class MicrowaveView : public InstrumentView
{
	Q_OBJECT
public:
	MicrowaveView( Instrument * _instrument,
					QWidget * _parent );

	virtual ~MicrowaveView();

	void sendToMatrixAsOutput( int loc1, int loc2, int loc3 );
	void switchToMatrixKnob( MicrowaveKnob * theKnob, int loc1, int loc2, int loc3 );
	void setMacroTooltip( MicrowaveKnob * theKnob, int which );
	void chooseMacroColor( MicrowaveKnob * theKnob, int which );
	void refreshMacroColor( MicrowaveKnob * theKnob, int which );
	void setMacroColortoDefault( MicrowaveKnob * theKnob, int which );
	void setGraphEnabledColor( bool isEnabled );

protected slots:
	void updateScroll();
	void mainNumChanged();
	void subNumChanged();
	void sampNumChanged();
	void modOutSecChanged( int i );
	void modInChanged( int i );
	void modIn2Changed( int i );
	void tabChanged( int tabnum );
	void visualizeToggled( bool value );
	void sinWaveClicked();
	void triangleWaveClicked();
	void sqrWaveClicked();
	void sawWaveClicked();
	void noiseWaveClicked();
	void usrWaveClicked();
	void smoothClicked( void  );
	void chooseWavetableFile( );
	void openWavetableFile( QString fileName = "" );
	void openWavetableFileBtnClicked();
	void openSampleFile();
	void openSampleFileBtnClicked();

	void modUpClicked( int );
	void modDownClicked( int );
	void i1Clicked( int );
	void i2Clicked( int );

	void confirmWavetableLoadClicked();

	void tabBtnClicked( int );

	void manualBtnClicked();

	void updateBackground();

	void flipperClicked();

	void XBtnClicked();
	void MatrixXBtnClicked();

	void normalizeClicked();
	void desawClicked();

	void modEnabledChanged();

private:
	virtual void modelChanged();

	void wheelEvent( QWheelEvent * _me );
	virtual void dropEvent( QDropEvent * _de );
	virtual void dragEnterEvent( QDragEnterEvent * _dee );

	PixmapButton * sinWaveBtn;
	PixmapButton * triangleWaveBtn;
	PixmapButton * sqrWaveBtn;
	PixmapButton * sawWaveBtn;
	PixmapButton * whiteNoiseWaveBtn;
	PixmapButton * smoothBtn;
	PixmapButton * usrWaveBtn;

	PixmapButton * sinWave2Btn;
	PixmapButton * triangleWave2Btn;
	PixmapButton * sqrWave2Btn;
	PixmapButton * sawWave2Btn;
	PixmapButton * whiteNoiseWave2Btn;
	PixmapButton * smooth2Btn;
	PixmapButton * usrWave2Btn;

	PixmapButton * XBtn;// For leaving wavetable loading section
	PixmapButton * MatrixXBtn;// For when you send something to the Matrix via right click

	PixmapButton * normalizeBtn;
	PixmapButton * desawBtn;

	PixmapButton * openWavetableButton;
	PixmapButton * confirmLoadButton;
	PixmapButton * openSampleButton;

	PixmapButton * modUpArrow[8];
	PixmapButton * modDownArrow[8];
	PixmapButton * i1Button[8];
	PixmapButton * i2Button[8];

	PixmapButton * tab1Btn;
	PixmapButton * tab2Btn;
	PixmapButton * tab3Btn;
	PixmapButton * tab4Btn;
	PixmapButton * tab5Btn;
	PixmapButton * tab6Btn;

	PixmapButton * mainFlipBtn;
	PixmapButton * subFlipBtn;

	PixmapButton * manualBtn;

	PixmapButton * removeDCBtn;
	ComboBox * oversampleModeBox;

	MicrowaveKnob * morphKnob;
	MicrowaveKnob * rangeKnob;
	MicrowaveKnob * modifyKnob;
	ComboBox * modifyModeBox;
	MicrowaveKnob * volKnob;
	MicrowaveKnob * panKnob;
	MicrowaveKnob * detuneKnob;
	MicrowaveKnob * phaseKnob;
	MicrowaveKnob * phaseRandKnob;
	LedCheckBox * enabledToggle;
	LedCheckBox * mutedToggle;
	MicrowaveKnob * sampLenKnob;
	MicrowaveKnob * morphMaxKnob;
	MicrowaveKnob * unisonVoicesKnob;
	MicrowaveKnob * unisonDetuneKnob;
	MicrowaveKnob * unisonMorphKnob;
	MicrowaveKnob * unisonModifyKnob;
	LedCheckBox * keytrackingToggle;
	MicrowaveKnob * tempoKnob;
	LedCheckBox * interpolateToggle;

	LedCheckBox * subEnabledToggle;
	LedCheckBox * subMutedToggle;
	LedCheckBox * subKeytrackToggle;
	LedCheckBox * subNoiseToggle;
	MicrowaveKnob * subVolKnob;
	MicrowaveKnob * subPanningKnob;
	MicrowaveKnob * subDetuneKnob;
	MicrowaveKnob * subPhaseKnob;
	MicrowaveKnob * subPhaseRandKnob;
	MicrowaveKnob * subSampLenKnob;
	MicrowaveKnob * subTempoKnob;
	MicrowaveKnob * subRateLimitKnob;
	MicrowaveKnob * subUnisonNumKnob;
	MicrowaveKnob * subUnisonDetuneKnob;
	LedCheckBox * subInterpolateToggle;

	LedCheckBox * sampleEnabledToggle;
	LedCheckBox * sampleGraphEnabledToggle;
	LedCheckBox * sampleMutedToggle;
	LedCheckBox * sampleKeytrackingToggle;
	LedCheckBox * sampleLoopToggle;
	MicrowaveKnob * sampleVolumeKnob;
	MicrowaveKnob * samplePanningKnob;
	MicrowaveKnob * sampleDetuneKnob;
	MicrowaveKnob * samplePhaseKnob;
	MicrowaveKnob * samplePhaseRandKnob;
	MicrowaveKnob * sampleStartKnob;
	MicrowaveKnob * sampleEndKnob;

	ComboBox * modInBox[8];
	LcdSpinBox * modInNumBox[8];
	MicrowaveKnob * modInAmntKnob[8];
	MicrowaveKnob * modInCurveKnob[8];
	ComboBox * modInBox2[8];
	LcdSpinBox * modInNumBox2[8];
	MicrowaveKnob * modInAmntKnob2[8];
	MicrowaveKnob * modInCurveKnob2[8];
	ComboBox * modOutSecBox[8];
	ComboBox * modOutSigBox[8];
	LcdSpinBox * modOutSecNumBox[8];
	LedCheckBox * modEnabledToggle[8];
	ComboBox * modCombineTypeBox[8];
	LedCheckBox * modTypeToggle[8];
	LedCheckBox * modType2Toggle[8];

	MicrowaveKnob * filtCutoffKnob[8];
	MicrowaveKnob * filtResoKnob[8];
	MicrowaveKnob * filtGainKnob[8];
	ComboBox * filtTypeBox[8];
	ComboBox * filtSlopeBox[8];
	MicrowaveKnob * filtInVolKnob[8];
	MicrowaveKnob * filtOutVolKnob[8];
	MicrowaveKnob * filtWetDryKnob[8];
	MicrowaveKnob * filtBalKnob[8];
	MicrowaveKnob * filtSatuKnob[8];
	MicrowaveKnob * filtFeedbackKnob[8];
	MicrowaveKnob * filtDetuneKnob[8];
	LedCheckBox * filtEnabledToggle[8];
	LedCheckBox * filtMutedToggle[8];
	LedCheckBox * filtKeytrackingToggle[8];

	MicrowaveKnob * macroKnob[18];

	LcdSpinBox * subNumBox;
	LcdSpinBox * sampNumBox;
	LcdSpinBox * mainNumBox;

	ComboBox * oversampleBox;
	ComboBox * loadModeBox;

	QLineEdit * modNumText[8];

	MicrowaveKnob * loadAlgKnob;
	MicrowaveKnob * loadChnlKnob;

	MicrowaveKnob * visvolKnob;

	Graph * graph;
	LedCheckBox * visualizeToggle;

	QScrollBar * effectScrollBar;
	QScrollBar * matrixScrollBar;

	QLabel * filtForegroundLabel;
	QLabel * filtBoxesLabel;
	QLabel * matrixForegroundLabel;
	QLabel * matrixBoxesLabel;

	QPalette pal = QPalette();
	QPixmap tab1ArtworkImg = PLUGIN_NAME::getIconPixmap("tab1_artwork");
	QPixmap tab1FlippedArtworkImg = PLUGIN_NAME::getIconPixmap("tab1_artwork_flipped");
	QPixmap tab2ArtworkImg = PLUGIN_NAME::getIconPixmap("tab2_artwork");
	QPixmap tab2FlippedArtworkImg = PLUGIN_NAME::getIconPixmap("tab2_artwork_flipped");
	QPixmap tab3ArtworkImg = PLUGIN_NAME::getIconPixmap("tab3_artwork");
	QPixmap tab4ArtworkImg = PLUGIN_NAME::getIconPixmap("tab4_artwork");
	QPixmap tab5ArtworkImg = PLUGIN_NAME::getIconPixmap("tab5_artwork");
	QPixmap tab6ArtworkImg = PLUGIN_NAME::getIconPixmap("tab6_artwork");
	QPixmap tab7ArtworkImg = PLUGIN_NAME::getIconPixmap("tab7_artwork");

	QString wavetableFileName = "";

	Microwave * microwave;

	float temp1;
	float temp2;

	int tabWhenSendingToMatrix;

	Microwave * b = castModel<Microwave>();

	friend class mSynth;
};


class mSynth
{
	MM_OPERATORS
public:
	mSynth( NotePlayHandle * _nph,
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
		std::vector<float> (&samples)[8][2] );
		
	virtual ~mSynth();
	
	void nextStringSample( sampleFrame &outputSample, float (&waveforms)[8][MAINARRAYLEN], float (&subs)[64][SUBWAVELEN], float * sampGraphs, std::vector<float> (&samples)[8][2], int maxFiltEnabled, int maxModEnabled, int maxSubEnabled, int maxSampleEnabled, int maxMainEnabled, int sample_rate, Microwave * mwc, bool removeDC, bool isOversamplingSample, float (&storedsubs)[64][STOREDSUBWAVELEN] );

	inline float detuneWithCents( float pitchValue, float detuneValue );

	inline void refreshValue( int which, int num, Microwave * mwc );

	inline float realfmod( float k, float n );

private:
	double sample_realindex[8][32] = {{0}};
	double sample_subindex[64][32] = {{0}};
	double sample_sampleindex[8] = {0};
	NotePlayHandle* nph;

	int noteDuration;
	float noteFreq;

	float lastMainOscVal[8][2] = {{0}};
	float lastSubVal[64][2] = {{0}};
	float lastSubNoiseVal[64][32] = {{0}};
	double sample_step_sub = 0;
	float noiseSampRand = 0;
	float lastSampleVal[8][2] = {{0}};
	double sample_step_sample = 0;

	float lastMainOscEnvVal[8][2] = {{0}};
	float lastSubEnvVal[64][2] = {{0}};
	float lastSampleEnvVal[8][2] = {{0}};

	bool lastMainOscEnvDone[8] = {false};
	bool lastSubEnvDone[64] = {false};
	bool lastSampleEnvDone[8] = {false};

	int subNoiseDirection[64][32] = {{0}};

	int loopStart = 0;
	int loopEnd = 0;
	float currentRangeValInvert = 0;
	int currentSampLen = 0;
	int currentIndex = 0;

	float filtInputs[8][2] = {{0}};// [filter number][channel]
	float filtOutputs[8][2] = {{0}};// [filter number][channel]
	float filtPrevSampIn[8][8][5][2] = {{{{0}}}};// [filter number][slope][samples back in time][channel]
	float filtPrevSampOut[8][8][5][2] = {{{{0}}}};// [filter number][slope][samples back in time][channel]
	float filtModOutputs[8][2] = {{0}};// [filter number][channel]

	std::vector<float> filtDelayBuf[8][2];// [filter number][channel]

	float filty1[2] = {0};
	float filty2[2] = {0};
	float filty3[2] = {0};
	float filty4[2] = {0};
	float filtoldx[2] = {0};
	float filtoldy1[2] = {0};
	float filtoldy2[2] = {0};
	float filtoldy3[2] = {0};
	float filtx[2] = {0};

	int modValType[128] = {0};
	int modValNum[128] = {0};

	int numberToReset = 0;


	float morph[8];
	float range[8];
	float modify[8];
	int modifyMode[8];
	float vol[8];
	float pan[8];
	float detune[8];
	float phase[8];
	float phaseRand[8];
	bool enabled[8];
	bool muted[8];
	float sampLen[8];
	float morphMax[8];
	float unisonVoices[8];
	float unisonDetune[8];
	float unisonMorph[8];
	float unisonModify[8];
	bool keytracking[8];
	float tempo[8];
	bool interpolate[64];

	bool subEnabled[64];
	bool subMuted[64];
	bool subKeytrack[64];
	bool subNoise[64];
	float subVol[64];
	float subPanning[64];
	float subDetune[64];
	float subPhase[64];
	float subPhaseRand[64];
	float subSampLen[64];
	float subTempo[64];
	float subRateLimit[64];
	float subUnisonNum[64];
	float subUnisonDetune[64];
	bool subInterpolate[64];

	bool sampleEnabled[8];
	bool sampleMuted[8];
	bool sampleKeytracking[8];
	bool sampleGraphEnabled[8];
	bool sampleLoop[8];
	float sampleVolume[8];
	float samplePanning[8];
	float sampleDetune[8];
	float samplePhase[8];
	float samplePhaseRand[8];
	float sampleStart[8];
	float sampleEnd[8];

	int modIn[64];
	int modInNum[64];
	float modInAmnt[64];
	float modInCurve[64];
	int modIn2[64];
	int modInNum2[64];
	float modInAmnt2[64];
	float modInCurve2[64];
	int modOutSec[64];
	int modOutSig[64];
	int modOutSecNum[64];
	bool modEnabled[64];
	int modCombineType[64];
	bool modType[64];
	bool modType2[64];

	float filtCutoff[8];
	float filtReso[8];
	float filtGain[8];
	int filtType[8];
	int filtSlope[8];
	float filtInVol[8];
	float filtOutVol[8];
	float filtWetDry[8];
	float filtBal[8];
	float filtSatu[8];
	float filtFeedback[8];
	float filtDetune[8];
	bool filtEnabled[8];
	bool filtMuted[8];
	bool filtKeytracking[8];

	float cutoff;
	int mode;
	float reso;
	float dbgain;
	float Fs;
	float a0;
	float a1;
	float a2;
	float b0;
	float b1;
	float b2;
	float alpha;
	float w0;
	float A;
	float f;
	float k;
	float p;
	float scale;
	float r;

	float humanizer[8] = {0};

	float unisonDetuneAmounts[8][32] = {{0}};
	float subUnisonDetuneAmounts[64][32] = {{0}};

	// Usually used for CPU improvements
	float temp1;
	float temp2;
	float temp3;
	float temp4;
	float temp5;
	float temp6;
	float temp7;

	float curModVal[2] = {0};
	float curModVal2[2] = {0};
	float curModValCurve[2] = {0};
	float curModVal2Curve[2] = {0};
	float comboModVal[2] = {0};
	float comboModValMono = 0;

	float sample_morerealindex[8][32] = {{0}};
	double sample_step[8][32] = {{0}};

	float unisonVoicesMinusOne = 0;
	float subUnisonVoicesMinusOne = 0;

	int filtFeedbackLoc[8] = {0};

	float macro[18];

	// For DC offset removal
	float averageSampleValue[2] = {0};

	sample_t mainsample[8][32] = {{0}};
	sample_t subsample[64][32] = {{0}};
	sample_t samplesample[8][2] = {{0}};

	float progress;
	float progress2;
	float progress3;
	int intprogress;

	sampleFrame sampleMainOsc;
	sampleFrame sampleSubOsc;

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
