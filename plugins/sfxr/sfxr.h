/*
 * sfxr.h - declaration of classes of the LMMS sfxr plugin
 * Originally written by Tomas Pettersson. For the original license,
 * please read readme.txt in this directory
 *
 * Copyright (c) 2014 Wong Cho Ching
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


#ifndef SFXR_H
#define SFXR_H

#include "Instrument.h"
#include "InstrumentView.h"
#include "Knob.h"
#include "Graph.h"
#include "PixmapButton.h"
#include "LedCheckbox.h"
#include "Memory.h"


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
	MM_OPERATORS
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

	Knob * m_attKnob; //Attack Time
	Knob * m_holdKnob; //Sustain Time
	Knob * m_susKnob; //Sustain Punch
	Knob * m_decKnob; //Decay Time

	Knob * m_startFreqKnob; //Start Frequency
	Knob * m_minFreqKnob; //Min Frequency
	Knob * m_slideKnob; //Slide
	Knob * m_dSlideKnob; //Delta Slide
	Knob * m_vibDepthKnob; //Vibrato Depth
	Knob * m_vibSpeedKnob; //Vibrato Speed

	Knob * m_changeAmtKnob; //Change Amount
	Knob * m_changeSpeedKnob; //Change Speed

	Knob * m_sqrDutyKnob; //Square Wave Duty
    Knob * m_sqrSweepKnob; //Square Wave Duty Sweep

	Knob * m_repeatSpeedKnob; //Repeat Speed

	Knob * m_phaserOffsetKnob; //Phaser Offset
	Knob * m_phaserSweepKnob; //Phaser Sweep

	Knob * m_lpFilCutKnob; //LP Filter Cutoff
	Knob * m_lpFilCutSweepKnob; //LP Filter Cutoff Sweep
	Knob * m_lpFilResoKnob; //LP Filter Resonance
	Knob * m_hpFilCutKnob; //HP Filter Cutoff
	Knob * m_hpFilCutSweepKnob; //HP Filter Cutoff Sweep

	automatableButtonGroup * m_waveBtnGroup;
	PixmapButton * m_sqrWaveBtn; //NOTE: This button has Square Duty
								//and Square Speed configurable
	PixmapButton * m_sawWaveBtn;
	PixmapButton * m_sinWaveBtn;
	PixmapButton * m_noiseWaveBtn;


	PixmapButton * m_pickupBtn;
	PixmapButton * m_laserBtn;
	PixmapButton * m_explosionBtn;
	PixmapButton * m_powerupBtn;
	PixmapButton * m_hitBtn;
	PixmapButton * m_jumpBtn;
	PixmapButton * m_blipBtn;

	PixmapButton * m_randomizeBtn;
	PixmapButton * m_mutateBtn;

	static QPixmap * s_artwork;
};



#endif
