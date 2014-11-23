/*
 * sfxr.h - declaration of classes of the LMMS sfxr plugin
 * Originally written by Tomas Pettersson. For the original license,
 * please read readme.txt in this directory
 *
 * Copyright (c) 2014 Wong Cho Ching
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


#ifndef SFXR_H
#define SFXR_H

#include "Instrument.h"
#include "InstrumentView.h"
#include "knob.h"
#include "graph.h"
#include "pixmap_button.h"
#include "led_checkbox.h"


enum SfxrWaves
{
	SQR_WAVE, SAW_WAVE, SINE_WAVE, NOISE_WAVE, WAVES_NUM
};

const int WAVEFORM_BASE_X = 20;
const int WAVEFORM_BASE_Y = 15;
const int WAVEFORM_BUTTON_WIDTH = 16;

const int GENERATOR_BASE_X = 110;
const int GENERATOR_BASE_Y = 25;
const int GENERATOR_BUTTON_WIDTH = 16;

const int RAND_BUTTON_X = 160;
const int RAND_BUTTON_Y = 4;

const int MUTA_BUTTON_X = 205;
const int MUTA_BUTTON_Y = 4;

const int KNOBS_BASE_X = 20;
const int KNOBS_BASE_Y = 50;
const int KNOB_BLOCK_SIZE_X = 40;
const int KNOB_BLOCK_SIZE_Y = 40;




class sfxrInstrument;



class SfxrSynth
{
public:
	SfxrSynth( const sfxrInstrument * s );
	virtual ~SfxrSynth();

	void resetSample( bool restart );
	void update( sampleFrame * buffer, const int32_t frameNum );

	bool isPlaying() const;

private:
	const sfxrInstrument * s;
	bool playing_sample;
	int phase;
	double fperiod;
	double fmaxperiod;
	double fslide;
	double fdslide;
	int period;
	float square_duty;
	float square_slide;
	int env_stage;
	int env_time;
	int env_length[3];
	float env_vol;
	float fphase;
	float fdphase;
	int iphase;
	float phaser_buffer[1024];
	int ipp;
	float noise_buffer[32];
	float fltp;
	float fltdp;
	float fltw;
	float fltw_d;
	float fltdmp;
	float fltphp;
	float flthp;
	float flthp_d;
	float vib_phase;
	float vib_speed;
	float vib_amp;
	int rep_time;
	int rep_limit;
	int arp_time;
	int arp_limit;
	double arp_mod;

} ;



/**
 * @brief A class that simplify the constructor of FloatModel, with value [0,1]
 */
class SfxrZeroToOneFloatModel : public FloatModel
{
public:
	SfxrZeroToOneFloatModel( float val, Model * parent, const QString& displayName ):
		FloatModel( val, 0.0, 1.0, 0.001, parent, displayName )
	{
	}
	/* purpose: prevent the initial value of the model from being changed */
	virtual void loadSettings( const QDomElement& element, const QString& name = QString( "value" ) )
	{
		float oldInitValue = initValue();
		FloatModel::loadSettings(element, name);
		float oldValue = value();
		setInitValue(oldInitValue);
		setValue(oldValue);
	}
};

/**
 * @brief A class that simplify the constructor of FloatModel, with value [-1,1]
 */
class SfxrNegPosOneFloatModel : public FloatModel
{
public:
	SfxrNegPosOneFloatModel(float val, Model * parent, const QString& displayName ):
		FloatModel( val, -1.0, 1.0, 0.001, parent, displayName )
	{
	}
	/* purpose: prevent the initial value of the model from being changed */
	virtual void loadSettings( const QDomElement& element, const QString& name = QString( "value" ) )
	{
		float oldInitValue = initValue();
		FloatModel::loadSettings(element, name);
		float oldValue = value();
		setInitValue(oldInitValue);
		setValue(oldValue);
	}
};

class sfxrInstrument : public Instrument
{
	Q_OBJECT
public:
	sfxrInstrument(InstrumentTrack * _instrument_track );
	virtual ~sfxrInstrument();

	virtual void playNote( NotePlayHandle * _n, sampleFrame * _working_buffer );
	virtual void deleteNotePluginData( NotePlayHandle * _n );

	virtual void saveSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	virtual QString nodeName() const;

	virtual PluginView * instantiateView( QWidget * _parent );

	void resetModels();


private:
	SfxrZeroToOneFloatModel m_attModel;
	SfxrZeroToOneFloatModel m_holdModel;
	SfxrZeroToOneFloatModel m_susModel;
	SfxrZeroToOneFloatModel m_decModel;

	SfxrZeroToOneFloatModel m_startFreqModel;
	SfxrZeroToOneFloatModel m_minFreqModel;
	SfxrNegPosOneFloatModel m_slideModel;
	SfxrNegPosOneFloatModel m_dSlideModel;
	SfxrZeroToOneFloatModel m_vibDepthModel;
	SfxrZeroToOneFloatModel m_vibSpeedModel;

	SfxrNegPosOneFloatModel m_changeAmtModel;
	SfxrZeroToOneFloatModel m_changeSpeedModel;

	SfxrZeroToOneFloatModel m_sqrDutyModel;
	SfxrNegPosOneFloatModel m_sqrSweepModel;

	SfxrZeroToOneFloatModel m_repeatSpeedModel;

	SfxrNegPosOneFloatModel m_phaserOffsetModel;
	SfxrNegPosOneFloatModel m_phaserSweepModel;

	SfxrZeroToOneFloatModel m_lpFilCutModel;
	SfxrNegPosOneFloatModel m_lpFilCutSweepModel;
	SfxrZeroToOneFloatModel m_lpFilResoModel;
	SfxrZeroToOneFloatModel m_hpFilCutModel;
	SfxrNegPosOneFloatModel m_hpFilCutSweepModel;

	IntModel m_waveFormModel;

	friend class sfxrInstrumentView;
	friend class SfxrSynth;
};



class sfxrInstrumentView : public InstrumentView
{
	Q_OBJECT
public:
	sfxrInstrumentView( Instrument * _instrument,
					QWidget * _parent );

	virtual ~sfxrInstrumentView() {};

protected slots:
	void genPickup();
	void genLaser();
	void genExplosion();
	void genPowerup();
	void genHit();
	void genJump();
	void genBlip();
	void randomize();
	void mutate();

	void previewSound();

private:
	virtual void modelChanged();

	knob * m_attKnob; //Attack Time
	knob * m_holdKnob; //Sustain Time
	knob * m_susKnob; //Sustain Punch
	knob * m_decKnob; //Decay Time

	knob * m_startFreqKnob; //Start Frequency
	knob * m_minFreqKnob; //Min Frequency
	knob * m_slideKnob; //Slide
	knob * m_dSlideKnob; //Delta Slide
	knob * m_vibDepthKnob; //Vibrato Depth
	knob * m_vibSpeedKnob; //Vibrato Speed

	knob * m_changeAmtKnob; //Change Amount
	knob * m_changeSpeedKnob; //Change Speed

	knob * m_sqrDutyKnob; //Squre Duty
    knob * m_sqrSweepKnob; //Squre Sweep

	knob * m_repeatSpeedKnob; //Repeat Speed

	knob * m_phaserOffsetKnob; //Phaser Offset
	knob * m_phaserSweepKnob; //Phaser Sweep

	knob * m_lpFilCutKnob; //LP Filter Cutoff
	knob * m_lpFilCutSweepKnob; //LP Filter Cutoff Sweep
	knob * m_lpFilResoKnob; //LP Filter Resonance
	knob * m_hpFilCutKnob; //HP Filter Cutoff
	knob * m_hpFilCutSweepKnob; //HP Filter Cutoff Sweep

	automatableButtonGroup * m_waveBtnGroup;
	pixmapButton * m_sqrWaveBtn; //NOTE: This button has Squre Duty
								//and Squre Speed configurable
	pixmapButton * m_sawWaveBtn;
	pixmapButton * m_sinWaveBtn;
	pixmapButton * m_noiseWaveBtn;


	pixmapButton * m_pickupBtn;
	pixmapButton * m_laserBtn;
	pixmapButton * m_explosionBtn;
	pixmapButton * m_powerupBtn;
	pixmapButton * m_hitBtn;
	pixmapButton * m_jumpBtn;
	pixmapButton * m_blipBtn;

	pixmapButton * m_randomizeBtn;
	pixmapButton * m_mutateBtn;

	static QPixmap * s_artwork;
};



#endif
