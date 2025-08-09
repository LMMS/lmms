/*
 * Monstro.h - a semi-modular 3-osc synth with modulation matrix
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
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


#ifndef MONSTRO_H
#define MONSTRO_H

#include <vector>

#include "ComboBoxModel.h"
#include "Instrument.h"
#include "InstrumentView.h"
#include "AutomatableModel.h"
#include "TempoSyncKnob.h"
#include "PixmapButton.h"
#include "Oscillator.h"
#include "lmms_math.h"
#include "BandLimitedWave.h"

//
//	UI Macros
//

#define makeknob( name, x, y, hint, unit, oname ) 		\
	name = new Knob( KnobType::Styled, view ); 				\
	name ->move( x, y );								\
	name ->setHintText( hint, unit );             \
	name ->setObjectName( oname );						\
	name ->setFixedSize( 20, 20 );

#define maketsknob( name, x, y, hint, unit, oname ) 		\
	name = new TempoSyncKnob( KnobType::Styled, view ); 				\
	name ->move( x, y );								\
	name ->setHintText( hint, unit );		\
	name ->setObjectName( oname );						\
	name ->setFixedSize( 20, 20 );

#define maketinyled( name, x, y, ttip ) \
	name = new PixmapButton( view, nullptr ); 	\
	name -> setCheckable( true );			\
	name -> move( x, y );					\
	name -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "tinyled_on" ) ); \
	name -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "tinyled_off" ) ); \
	name->setToolTip(ttip);

namespace lmms
{


namespace gui
{


// UI constants
const int O1ROW = 22;
const int O2ROW = 22 + 39;
const int O3ROW = 22 + 39 * 2;
const int LFOROW = 22 + 39 * 3;
const int E1ROW = 22 + 39 * 4;
const int E2ROW = 22 + 39 * 5;

const int KNOBCOL1 = 16;
const int KNOBCOL2 = 16 + 30;
const int KNOBCOL3 = 16 + 30 * 2;
const int KNOBCOL4 = 16 + 30 * 3;
const int KNOBCOL5 = 16 + 30 * 4;
const int KNOBCOL6 = 16 + 30 * 5;
const int KNOBCOL7 = 16 + 30 * 6;

const int LFOCOL1 = KNOBCOL2;
const int LFOCOL2 = KNOBCOL2 + 26;
const int LFOCOL3 = KNOBCOL2 + 26*2;
const int LFOCOL4 = 171;
const int LFOCOL5 = 171 + 26;
const int LFOCOL6 = 171 + 26*2;

const int MATCOL1 = 32;
const int MATCOL2 = 32 + 25;
const int MATCOL3 = 32 + 25*2;
const int MATCOL4 = 32 + 25*3;
const int MATCOL5 = 149;
const int MATCOL6 = 149 + 25;
const int MATCOL7 = 149 + 25*2;
const int MATCOL8 = 149 + 25*3;

const int MATROW1 = 22;
const int MATROW2 = 22 + 39;
const int MATROW3 = 22 + 39*2;
const int MATROW4 = 22 + 39*3;
const int MATROW5 = 22 + 39*4;
const int MATROW6 = 22 + 39*5;

const int OPVIEW = 0;
const int MATVIEW = 1;


} // namespace gui

// waveform enumerators
const int WAVE_SINE = 0;
const int WAVE_TRI = 1;
const int WAVE_SAW = 2;
const int WAVE_RAMP = 3;
const int WAVE_SQR = 4;
const int WAVE_MOOG = 5;

const int WAVE_SQRSOFT = 6;
const int WAVE_SINABS = 7;
const int WAVE_EXP = 8;
const int WAVE_NOISE = 9;

const int WAVE_TRI_D = 10;
const int WAVE_SAW_D = 11;
const int WAVE_RAMP_D = 12;
const int WAVE_SQR_D = 13;
const int WAVE_MOOG_D = 14;

const int NUM_WAVES = 15;

// lfo-specific enumerators
const int WAVE_RANDOM = 9;
const int WAVE_RANDOM_SMOOTH = 10;
const int NUM_LFO_WAVES = 11;

// modulation enumerators
const int MOD_MIX = 0;
const int MOD_AM = 1;
const int MOD_FM = 2;
const int MOD_PM = 3;
const int NUM_MODS = 4;

const float MODCLIP = 2.0;

const float MIN_FREQ = 18.0f;
const float MAX_FREQ = 48000.0f;

const float INTEGRATOR = 3.0f / 7.0f;

const float FM_AMOUNT = 0.25f;

const float PW_MIN = 0.25f;
const float PW_MAX = 100.0f - PW_MIN;

class MonstroInstrument;

namespace gui
{
class MonstroView;
class ComboBox;
}


class MonstroSynth
{
public:
	MonstroSynth( MonstroInstrument * _i, NotePlayHandle * _nph );
	virtual ~MonstroSynth() = default;

	void renderOutput( fpp_t _frames, SampleFrame* _buf );

private:

	MonstroInstrument * m_parent;
	NotePlayHandle * m_nph;

	inline void updateModulators(float * env1, float * env2, float * lfo1, float * lfo2, f_cnt_t frames);

	// linear interpolation
/*	inline sample_t interpolate( sample_t s1, sample_t s2, float x )
	{
		return s1 + ( s2 - s1 ) * x;
	}*/ // using interpolation.h from now on

	inline sample_t calcSlope( int slope,  sample_t s );

	// checks for lower bound for phase, upper bound is already checked by oscillator-functions in both
	// oscillator.h and bandlimitedwave.h so we save some cpu by only checking lower bound
	inline float lowBoundCheck( float ph )
	{
		return ph < 0.0f ? ph - ( static_cast<int>( ph ) - 1.0f ) : ph;
	}

	inline sample_t oscillate( int _wave, const float _ph, float _wavelen )
	{
		switch( _wave )
		{
			case WAVE_SINE:
				return Oscillator::sinSample( _ph );
				break;
			case WAVE_TRI:
				//return Oscillator::triangleSample( _ph );
				return BandLimitedWave::oscillate( _ph, _wavelen, BandLimitedWave::Waveform::BLTriangle );
				break;
			case WAVE_SAW:
				//return Oscillator::sawSample( _ph );
				return BandLimitedWave::oscillate( _ph, _wavelen, BandLimitedWave::Waveform::BLSaw );
				break;
			case WAVE_RAMP:
				//return Oscillator::sawSample( _ph ) * -1.0;
				return BandLimitedWave::oscillate( _ph, _wavelen, BandLimitedWave::Waveform::BLSaw ) * -1.0;
				break;
			case WAVE_SQR:
				//return Oscillator::squareSample( _ph );
				return BandLimitedWave::oscillate( _ph, _wavelen, BandLimitedWave::Waveform::BLSquare );
				break;
			case WAVE_SQRSOFT:
			{
				const float ph = fraction( _ph );
				if( ph < 0.1 )	return Oscillator::sinSample( ph * 5 + 0.75 );
				else if( ph < 0.5 ) return 1.0f;
				else if( ph < 0.6 ) return Oscillator::sinSample( ph * 5 + 0.75 );
				else return -1.0f;
				break;
			}
			case WAVE_MOOG:
				//return Oscillator::moogSawSample( _ph );
				return BandLimitedWave::oscillate( _ph, _wavelen, BandLimitedWave::Waveform::BLMoog );
				break;
			case WAVE_SINABS:
				return qAbs( Oscillator::sinSample( _ph ) );
				break;
			case WAVE_EXP:
				return Oscillator::expSample( _ph );
				break;
			case WAVE_NOISE:
				return Oscillator::noiseSample( _ph );
				break;

			case WAVE_TRI_D:
				return Oscillator::triangleSample( _ph );
				break;
			case WAVE_SAW_D:
				return Oscillator::sawSample( _ph );
				break;
			case WAVE_RAMP_D:
				return Oscillator::sawSample( _ph ) * -1.0;
				break;
			case WAVE_SQR_D:
				return Oscillator::squareSample( _ph );
				break;
			case WAVE_MOOG_D:
				return Oscillator::moogSawSample( _ph );
				break;

		}
		return 0.0;
	}


	float m_osc1l_phase;
	float m_osc1r_phase;
	float m_osc2l_phase;
	float m_osc2r_phase;
	float m_osc3l_phase;
	float m_osc3r_phase;

	sample_t m_env_phase [2];
	float m_lfo_phase [2];
	sample_t m_lfo_last [2];
	sample_t m_lfo_next [2];
	float m_lfo_inc [2];
	float m_lfo_rate [2];	
	float m_env_sus [2];

	int m_lfovalue[2];
	int m_lfoatt[2];
	float m_env_pre[2];
	float m_env_att[2];
	float m_env_hold[2];
	float m_env_dec[2];
	float m_env_rel[2];

	sample_t m_osc1l_last;
	sample_t m_osc1r_last;

	sample_t m_l_last;
	sample_t m_r_last;

	float m_ph2l_last;
	float m_ph2r_last;

	float m_ph3l_last;
	float m_ph3r_last;

	bool m_invert2l;
	bool m_invert3l;
	bool m_invert2r;
	bool m_invert3r;
	
	int m_counter2l;
	int m_counter2r;
	int m_counter3l;
	int m_counter3r;

	std::vector<float> m_lfo[2];
	std::vector<float> m_env[2];
};

class MonstroInstrument : public Instrument
{
	Q_OBJECT

#define setwavemodel( name ) 												\
		name .addItem( tr( "Sine wave" ), std::make_unique<PluginPixmapLoader>( "sin" ) );		\
		name .addItem( tr( "Bandlimited Triangle wave" ), std::make_unique<PluginPixmapLoader>( "tri" ) );	\
		name .addItem( tr( "Bandlimited Saw wave" ), std::make_unique<PluginPixmapLoader>( "saw" ) );			\
		name .addItem( tr( "Bandlimited Ramp wave" ), std::make_unique<PluginPixmapLoader>( "ramp" ) );		\
		name .addItem( tr( "Bandlimited Square wave" ), std::make_unique<PluginPixmapLoader>( "sqr" ) );		\
		name .addItem( tr( "Bandlimited Moog saw wave" ), std::make_unique<PluginPixmapLoader>( "moog" ) );	\
		name .addItem( tr( "Soft square wave" ), std::make_unique<PluginPixmapLoader>( "sqrsoft" ) );		\
		name .addItem( tr( "Absolute sine wave" ), std::make_unique<PluginPixmapLoader>( "sinabs" ) );		\
		name .addItem( tr( "Exponential wave" ), std::make_unique<PluginPixmapLoader>( "exp" ) );	\
		name .addItem( tr( "White noise" ), std::make_unique<PluginPixmapLoader>( "noise" ) );	\
		name .addItem( tr( "Digital Triangle wave" ), std::make_unique<PluginPixmapLoader>( "tri" ) );	\
		name .addItem( tr( "Digital Saw wave" ), std::make_unique<PluginPixmapLoader>( "saw" ) );			\
		name .addItem( tr( "Digital Ramp wave" ), std::make_unique<PluginPixmapLoader>( "ramp" ) );		\
		name .addItem( tr( "Digital Square wave" ), std::make_unique<PluginPixmapLoader>( "sqr" ) );		\
		name .addItem( tr( "Digital Moog saw wave" ), std::make_unique<PluginPixmapLoader>( "moog" ) );


#define setlfowavemodel( name ) 												\
		name .addItem( tr( "Sine wave" ), std::make_unique<PluginPixmapLoader>( "sin" ) );		\
		name .addItem( tr( "Triangle wave" ), std::make_unique<PluginPixmapLoader>( "tri" ) );	\
		name .addItem( tr( "Saw wave" ), std::make_unique<PluginPixmapLoader>( "saw" ) );			\
		name .addItem( tr( "Ramp wave" ), std::make_unique<PluginPixmapLoader>( "ramp" ) );		\
		name .addItem( tr( "Square wave" ), std::make_unique<PluginPixmapLoader>( "sqr" ) );		\
		name .addItem( tr( "Moog saw wave" ), std::make_unique<PluginPixmapLoader>( "moog" ) );	\
		name .addItem( tr( "Soft square wave" ), std::make_unique<PluginPixmapLoader>( "sqrsoft" ) );		\
		name .addItem( tr( "Abs. sine wave" ), std::make_unique<PluginPixmapLoader>( "sinabs" ) );		\
		name .addItem( tr( "Exponential wave" ), std::make_unique<PluginPixmapLoader>( "exp" ) );	\
		name .addItem( tr( "Random" ), std::make_unique<PluginPixmapLoader>( "rand" ) );	\
		name .addItem( tr( "Random smooth" ), std::make_unique<PluginPixmapLoader>( "rand" ) );

public:
	MonstroInstrument( InstrumentTrack * _instrument_track );
	~MonstroInstrument() override = default;

	void playNote( NotePlayHandle * _n,
						SampleFrame* _working_buffer ) override;
	void deleteNotePluginData( NotePlayHandle * _n ) override;

	void saveSettings( QDomDocument & _doc,
							QDomElement & _this ) override;
	void loadSettings( const QDomElement & _this ) override;

	QString nodeName() const override;

	float desiredReleaseTimeMs() const override;

	gui::PluginView* instantiateView( QWidget * _parent ) override;

public slots:
	void updateVolume1();
	void updateVolume2();
	void updateVolume3();
	void updateFreq1();
	void updateFreq2();
	void updateFreq3();
	void updatePO1();
	void updatePO2();
	void updatePO3();
	void updateEnvelope1();
	void updateEnvelope2();
	void updateLFOAtts();
	void updateSamplerate();
	void updateSlope1();
	void updateSlope2();
	
protected:
	float m_osc1l_vol;
	float m_osc1r_vol;
	float m_osc2l_vol;
	float m_osc2r_vol;
	float m_osc3l_vol;
	float m_osc3r_vol;

	float m_osc1l_freq;
	float m_osc1r_freq;
	float m_osc2l_freq;
	float m_osc2r_freq;
	float m_osc3_freq;

	float m_osc1l_po;
	float m_osc1r_po;
	float m_osc2l_po;
	float m_osc2r_po;
	float m_osc3l_po;
	float m_osc3r_po;

	float m_env1_pre;
	float m_env1_att;
	float m_env1_hold;
	float m_env1_dec;
	float m_env1_rel;

	float m_env2_pre;
	float m_env2_att;
	float m_env2_hold;
	float m_env2_dec;
	float m_env2_rel;

	f_cnt_t m_env1_len;
	f_cnt_t m_env2_len;

	f_cnt_t m_env1_relF;
	f_cnt_t m_env2_relF;

	float m_slope [2];

	f_cnt_t m_lfo1_att;
	f_cnt_t m_lfo2_att;

	sample_rate_t m_samplerate;
	fpp_t m_fpp;
	
	float m_integrator;
	float m_fmCorrection;
	int m_counterMax;

private:
	inline float leftCh( float _vol, float _pan )
	{
		return ( _pan <= 0 ? 1.0 : 1.0 - ( _pan / 100.0 ) ) * _vol / 100.0;
	}

	inline float rightCh( float _vol, float _pan )
	{
		return ( _pan >= 0 ? 1.0 : 1.0 + ( _pan / 100.0 ) ) * _vol / 100.0;
	}

//////////////////////////////////////
//            models of             //
//	    operator view knobs         //
//                                  //
//////////////////////////////////////

	FloatModel	m_osc1Vol;
	FloatModel	m_osc1Pan;
	FloatModel	m_osc1Crs;
	FloatModel	m_osc1Ftl;
	FloatModel	m_osc1Ftr;
	FloatModel	m_osc1Spo;
	FloatModel	m_osc1Pw;
	BoolModel	m_osc1SSR;
	BoolModel	m_osc1SSF;

	FloatModel	m_osc2Vol;
	FloatModel	m_osc2Pan;
	FloatModel	m_osc2Crs;
	FloatModel	m_osc2Ftl;
	FloatModel	m_osc2Ftr;
	FloatModel	m_osc2Spo;
	ComboBoxModel	m_osc2Wave;
	BoolModel	m_osc2SyncH;
	BoolModel	m_osc2SyncR;

	FloatModel	m_osc3Vol;
	FloatModel	m_osc3Pan;
	FloatModel	m_osc3Crs;
	FloatModel	m_osc3Spo;
	FloatModel	m_osc3Sub;
	ComboBoxModel	m_osc3Wave1;
	ComboBoxModel	m_osc3Wave2;
	BoolModel	m_osc3SyncH;
	BoolModel	m_osc3SyncR;

	ComboBoxModel	m_lfo1Wave;
	TempoSyncKnobModel	m_lfo1Att;
	TempoSyncKnobModel	m_lfo1Rate;
	FloatModel	m_lfo1Phs;

	ComboBoxModel	m_lfo2Wave;
	TempoSyncKnobModel	m_lfo2Att;
	TempoSyncKnobModel	m_lfo2Rate;
	FloatModel	m_lfo2Phs;

	TempoSyncKnobModel	m_env1Pre;
	TempoSyncKnobModel	m_env1Att;
	TempoSyncKnobModel	m_env1Hold;
	TempoSyncKnobModel	m_env1Dec;
	FloatModel	m_env1Sus;
	TempoSyncKnobModel	m_env1Rel;
	FloatModel	m_env1Slope;

	TempoSyncKnobModel	m_env2Pre;
	TempoSyncKnobModel	m_env2Att;
	TempoSyncKnobModel	m_env2Hold;
	TempoSyncKnobModel	m_env2Dec;
	FloatModel	m_env2Sus;
	TempoSyncKnobModel	m_env2Rel;
	FloatModel	m_env2Slope;

	IntModel	m_o23Mod;

	IntModel	m_selectedView;

//////////////////////////////////////
//          models of               //
//	modulation matrix view knobs    //
//                                  //
//////////////////////////////////////

	FloatModel 	m_vol1env1;
	FloatModel	m_vol1env2;
	FloatModel	m_vol1lfo1;
	FloatModel	m_vol1lfo2;

	FloatModel 	m_vol2env1;
	FloatModel	m_vol2env2;
	FloatModel	m_vol2lfo1;
	FloatModel	m_vol2lfo2;

	FloatModel 	m_vol3env1;
	FloatModel	m_vol3env2;
	FloatModel	m_vol3lfo1;
	FloatModel	m_vol3lfo2;

	FloatModel 	m_phs1env1;
	FloatModel	m_phs1env2;
	FloatModel	m_phs1lfo1;
	FloatModel	m_phs1lfo2;

	FloatModel 	m_phs2env1;
	FloatModel	m_phs2env2;
	FloatModel	m_phs2lfo1;
	FloatModel	m_phs2lfo2;

	FloatModel 	m_phs3env1;
	FloatModel	m_phs3env2;
	FloatModel	m_phs3lfo1;
	FloatModel	m_phs3lfo2;

	FloatModel 	m_pit1env1;
	FloatModel	m_pit1env2;
	FloatModel	m_pit1lfo1;
	FloatModel	m_pit1lfo2;

	FloatModel 	m_pit2env1;
	FloatModel	m_pit2env2;
	FloatModel	m_pit2lfo1;
	FloatModel	m_pit2lfo2;

	FloatModel 	m_pit3env1;
	FloatModel	m_pit3env2;
	FloatModel	m_pit3lfo1;
	FloatModel	m_pit3lfo2;

	FloatModel 	m_pw1env1;
	FloatModel	m_pw1env2;
	FloatModel	m_pw1lfo1;
	FloatModel	m_pw1lfo2;

	FloatModel 	m_sub3env1;
	FloatModel	m_sub3env2;
	FloatModel	m_sub3lfo1;
	FloatModel	m_sub3lfo2;

	friend class MonstroSynth;
	friend class gui::MonstroView;

};


namespace gui
{


class MonstroView : public InstrumentViewFixedSize
{
	Q_OBJECT
public:
	MonstroView( Instrument * _instrument,
					QWidget * _parent );
	~MonstroView() override = default;

protected slots:
	void updateLayout();

private:
	void modelChanged() override;

	void setWidgetBackground( QWidget * _widget, const QString & _pic );
	QWidget * setupOperatorsView( QWidget * _parent );
	QWidget * setupMatrixView( QWidget * _parent );

//////////////////////////////////////
//                                  //
//	    operator view knobs         //
//                                  //
//////////////////////////////////////

	Knob *	m_osc1VolKnob;
	Knob *	m_osc1PanKnob;
	Knob *	m_osc1CrsKnob;
	Knob *	m_osc1FtlKnob;
	Knob *	m_osc1FtrKnob;
	Knob *	m_osc1SpoKnob;
	Knob *	m_osc1PwKnob;
	PixmapButton * m_osc1SSRButton;
	PixmapButton * m_osc1SSFButton;

	Knob *	m_osc2VolKnob;
	Knob *	m_osc2PanKnob;
	Knob *	m_osc2CrsKnob;
	Knob *	m_osc2FtlKnob;
	Knob *	m_osc2FtrKnob;
	Knob *	m_osc2SpoKnob;
	ComboBox *	m_osc2WaveBox;
	PixmapButton * m_osc2SyncHButton;
	PixmapButton * m_osc2SyncRButton;

	Knob *	m_osc3VolKnob;
	Knob *	m_osc3PanKnob;
	Knob *	m_osc3CrsKnob;
	Knob *	m_osc3SpoKnob;
	Knob *	m_osc3SubKnob;
	ComboBox *	m_osc3Wave1Box;
	ComboBox *	m_osc3Wave2Box;
	PixmapButton * m_osc3SyncHButton;
	PixmapButton * m_osc3SyncRButton;

	ComboBox *	m_lfo1WaveBox;
	TempoSyncKnob *	m_lfo1AttKnob;
	TempoSyncKnob *	m_lfo1RateKnob;
	Knob *	m_lfo1PhsKnob;

	ComboBox *	m_lfo2WaveBox;
	TempoSyncKnob *	m_lfo2AttKnob;
	TempoSyncKnob *	m_lfo2RateKnob;
	Knob *	m_lfo2PhsKnob;

	TempoSyncKnob *	m_env1PreKnob;
	TempoSyncKnob *	m_env1AttKnob;
	TempoSyncKnob *	m_env1HoldKnob;
	TempoSyncKnob *	m_env1DecKnob;
	Knob *	m_env1SusKnob;
	TempoSyncKnob *	m_env1RelKnob;
	Knob *	m_env1SlopeKnob;

	TempoSyncKnob *	m_env2PreKnob;
	TempoSyncKnob *	m_env2AttKnob;
	TempoSyncKnob *	m_env2HoldKnob;
	TempoSyncKnob *	m_env2DecKnob;
	Knob *	m_env2SusKnob;
	TempoSyncKnob *	m_env2RelKnob;
	Knob *	m_env2SlopeKnob;

	AutomatableButtonGroup * m_o23ModGroup;

	AutomatableButtonGroup * m_selectedViewGroup;

	QWidget * m_operatorsView;
	QWidget * m_matrixView;

/////////////////////////////////
//                             //
//    matrix view knobs        //
//                             //
/////////////////////////////////

	Knob * 	m_vol1env1Knob;
	Knob *	m_vol1env2Knob;
	Knob *	m_vol1lfo1Knob;
	Knob *	m_vol1lfo2Knob;

	Knob * 	m_vol2env1Knob;
	Knob *	m_vol2env2Knob;
	Knob *	m_vol2lfo1Knob;
	Knob *	m_vol2lfo2Knob;

	Knob * 	m_vol3env1Knob;
	Knob *	m_vol3env2Knob;
	Knob *	m_vol3lfo1Knob;
	Knob *	m_vol3lfo2Knob;

	Knob * 	m_phs1env1Knob;
	Knob *	m_phs1env2Knob;
	Knob *	m_phs1lfo1Knob;
	Knob *	m_phs1lfo2Knob;

	Knob * 	m_phs2env1Knob;
	Knob *	m_phs2env2Knob;
	Knob *	m_phs2lfo1Knob;
	Knob *	m_phs2lfo2Knob;

	Knob * 	m_phs3env1Knob;
	Knob *	m_phs3env2Knob;
	Knob *	m_phs3lfo1Knob;
	Knob *	m_phs3lfo2Knob;

	Knob * 	m_pit1env1Knob;
	Knob *	m_pit1env2Knob;
	Knob *	m_pit1lfo1Knob;
	Knob *	m_pit1lfo2Knob;

	Knob * 	m_pit2env1Knob;
	Knob *	m_pit2env2Knob;
	Knob *	m_pit2lfo1Knob;
	Knob *	m_pit2lfo2Knob;

	Knob * 	m_pit3env1Knob;
	Knob *	m_pit3env2Knob;
	Knob *	m_pit3lfo1Knob;
	Knob *	m_pit3lfo2Knob;

	Knob * 	m_pw1env1Knob;
	Knob *	m_pw1env2Knob;
	Knob *	m_pw1lfo1Knob;
	Knob *	m_pw1lfo2Knob;

	Knob * 	m_sub3env1Knob;
	Knob *	m_sub3env2Knob;
	Knob *	m_sub3lfo1Knob;
	Knob *	m_sub3lfo2Knob;

};


} // namespace gui

} // namespace lmms

#endif
