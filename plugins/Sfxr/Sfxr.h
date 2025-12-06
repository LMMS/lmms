/*
 * Sfxr.h - declaration of classes of the LMMS sfxr plugin
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

#include <array>

#include "AutomatableModel.h"
#include "Instrument.h"
#include "InstrumentView.h"

namespace lmms
{


enum class SfxrWave
{
	Square, Saw, Sine, Noise, Count
};
constexpr auto NumSfxrWaves = static_cast<std::size_t>(SfxrWave::Count);

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



class SfxrInstrument;

namespace gui
{
class AutomatableButtonGroup;
class Knob;
class PixmapButton;
class SfxrInstrumentView;
}



class SfxrSynth
{
public:
	SfxrSynth( const SfxrInstrument * s );
	virtual ~SfxrSynth() = default;

	void resetSample( bool restart );
	void update( SampleFrame* buffer, const int32_t frameNum );

	bool isPlaying() const;

private:
	const SfxrInstrument * s;
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
	std::array<float, 1024> phaser_buffer;
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
		FloatModel(val, 0.f, 1.f, 0.001f, parent, displayName)
	{
	}
	/* purpose: prevent the initial value of the model from being changed */
	void loadSettings( const QDomElement& element, const QString& name = QString( "value" ) ) override
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
		FloatModel(val, -1.f, 1.f, 0.001f, parent, displayName)
	{
	}
	/* purpose: prevent the initial value of the model from being changed */
	void loadSettings( const QDomElement& element, const QString& name = QString( "value" ) ) override
	{
		float oldInitValue = initValue();
		FloatModel::loadSettings(element, name);
		float oldValue = value();
		setInitValue(oldInitValue);
		setValue(oldValue);
	}
};

class SfxrInstrument : public Instrument
{
	Q_OBJECT
public:
	SfxrInstrument(InstrumentTrack * _instrument_track );
	~SfxrInstrument() override = default;

	void playNote( NotePlayHandle * _n, SampleFrame* _working_buffer ) override;
	void deleteNotePluginData( NotePlayHandle * _n ) override;

	void saveSettings( QDomDocument & _doc,
							QDomElement & _parent ) override;
	void loadSettings( const QDomElement & _this ) override;

	QString nodeName() const override;

	gui::PluginView* instantiateView( QWidget * _parent ) override;

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

	friend class gui::SfxrInstrumentView;
	friend class SfxrSynth;
};


namespace gui
{


class SfxrInstrumentView : public InstrumentViewFixedSize
{
	Q_OBJECT
public:
	SfxrInstrumentView( Instrument * _instrument,
					QWidget * _parent );

	~SfxrInstrumentView() override = default;

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
	void modelChanged() override;

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

	AutomatableButtonGroup * m_waveBtnGroup;
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


} // namespace gui

} // namespace lmms

#endif
