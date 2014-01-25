/*
 * sfxr.h - declaration of classes of the LMMS sfxr plugin
 * The original readme file of sfxr can be found in readme.txt in this directory.
 *
 * Copyright (c) 2014 Wong Cho Ching
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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


#ifndef _SFXR_H
#define _SFXR_H

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
const int WAVEFORM_BASE_Y = 14;
const int WAVEFORM_BUTTON_WIDTH = 16;

const int GENERATOR_BASE_X = 110;
const int GENERATOR_BASE_Y = 24;
const int GENERATOR_BUTTON_WIDTH = 16;

const int RAND_BUTTON_X = 160;
const int RAND_BUTTON_Y = 4;

const int MUTA_BUTTON_X = 205;
const int MUTA_BUTTON_Y = 4;

const int KNOBS_BASE_X = 20;
const int KNOBS_BASE_Y = 50;
const int KNOB_BLOCK_SIZE_X = 40;
const int KNOB_BLOCK_SIZE_Y = 40;


class SfxrSynth
{
public:
	SfxrSynth( float * sample, int length, notePlayHandle * _nph,
			bool _interpolation, float factor,
			const sample_rate_t _sample_rate );
	virtual ~SfxrSynth();

	sample_t nextStringSample();


private:
	int sample_index;
	float sample_realindex;
	float* sample_shape;
	notePlayHandle* nph;
	const int sample_length;
	const sample_rate_t sample_rate;

	bool interpolation;

};

/**
 * @brief A class that simplify the constructor of FloatModel, with value [0,1]
 */
class SfxrZeroToOneFloatModel : public FloatModel
{
public:
	SfxrZeroToOneFloatModel(float val, Model * parent):
		FloatModel( val, 0.0, 1.0, 0.001, parent)
	{
	}
};

/**
 * @brief A class that simplify the constructor of FloatModel, with value [-1,1]
 */
class SfxrNegPosOneFloatModel : public FloatModel
{
public:
	SfxrNegPosOneFloatModel(float val, Model * parent):
		FloatModel( val, -1.0, 1.0, 0.001, parent)
	{
	}
};

class sfxrInstrument : public Instrument
{
	Q_OBJECT
public:
	sfxrInstrument(InstrumentTrack * _instrument_track );
	virtual ~sfxrInstrument();

	virtual void playNote( notePlayHandle * _n,
						sampleFrame * _working_buffer );
	virtual void deleteNotePluginData( notePlayHandle * _n );


	virtual void saveSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	virtual QString nodeName() const;

	virtual f_cnt_t desiredReleaseFrames() const;

	virtual PluginView * instantiateView( QWidget * _parent );

	void resetModels();

protected slots:
	void samplesChanged( int, int );


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
};



class sfxrInstrumentView : public InstrumentView
{
	Q_OBJECT
public:
	sfxrInstrumentView( Instrument * _instrument,
					QWidget * _parent );

	virtual ~sfxrInstrumentView() {};

protected slots:
	void waveFormChanged();
	void genPickup();
	void genLaser();
	void genExplosion();
	void genPowerup();
	void genHit();
	void genJump();
	void genBlip();
	void randomize();
	void mutate();

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
	knob * m_sqrSpeedKnob; //Squre Sweep

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
