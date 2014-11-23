/*
 * Watsyn.h - a 4-oscillator modulating wavetable synth
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
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


#ifndef WATSYN_H
#define WATSYN_H

#include "Instrument.h"
#include "InstrumentView.h"
#include "graph.h"
#include "AutomatableModel.h"
#include "automatable_button.h"
#include "TempoSyncKnob.h"
#include "NotePlayHandle.h"
#include "pixmap_button.h"
#include <samplerate.h>


#define makeknob( name, x, y, hint, unit, oname ) 		\
	name = new knob( knobStyled, this ); 				\
	name ->move( x, y );								\
	name ->setHintText( tr( hint ) + " ", unit );		\
	name ->setObjectName( oname );						\
	name ->setFixedSize( 19, 19 );

#define maketsknob( name, x, y, hint, unit, oname ) 		\
	name = new TempoSyncKnob( knobStyled, this ); 				\
	name ->move( x, y );								\
	name ->setHintText( tr( hint ) + " ", unit );		\
	name ->setObjectName( oname );						\
	name ->setFixedSize( 19, 19 );

#define A1ROW 26
#define A2ROW 49
#define B1ROW 72
#define B2ROW 95


const int GRAPHLEN = 220; // don't change - must be same as the size of the widget

const int WAVERATIO = 32; // oversampling ratio

const int WAVELEN = GRAPHLEN * WAVERATIO;
const int PMOD_AMT = WAVELEN / 2;

const int	MOD_MIX = 0;
const int	MOD_AM = 1;
const int	MOD_RM = 2;
const int	MOD_PM = 3;
const int  NUM_MODS = 4;

const int	A1_OSC = 0;
const int	A2_OSC = 1;
const int	B1_OSC = 2;
const int	B2_OSC = 3;
const int	NUM_OSCS = 4;

class WatsynInstrument;

class WatsynObject
{
public:
	WatsynObject( 	float * _A1wave, float * _A2wave,
					float * _B1wave, float * _B2wave,
					int _amod, int _bmod, const sample_rate_t _samplerate, NotePlayHandle * _nph, fpp_t _frames,
					WatsynInstrument * _w );
	virtual ~WatsynObject();

	void renderOutput( fpp_t _frames );

	inline sampleFrame * abuf() const
	{
		return m_abuf;
	}
	inline sampleFrame * bbuf() const
	{
		return m_bbuf;
	}
	inline sample_rate_t samplerate() const
	{
		return m_samplerate;
	}

private:
	int m_amod;
	int m_bmod;

	const sample_rate_t m_samplerate;
	NotePlayHandle * m_nph;

	fpp_t m_fpp;

	WatsynInstrument * m_parent;

	sampleFrame * m_abuf;
	sampleFrame * m_bbuf;

	float m_lphase [NUM_OSCS];
	float m_rphase [NUM_OSCS];

	float m_A1wave [WAVELEN];
	float m_A2wave [WAVELEN];
	float m_B1wave [WAVELEN];
	float m_B2wave [WAVELEN];
};

class WatsynInstrument : public Instrument
{
	Q_OBJECT
public:
	WatsynInstrument( InstrumentTrack * _instrument_track );
	virtual ~WatsynInstrument();

	virtual void playNote( NotePlayHandle * _n,
						sampleFrame * _working_buffer );
	virtual void deleteNotePluginData( NotePlayHandle * _n );


	virtual void saveSettings( QDomDocument & _doc,
							QDomElement & _this );
	virtual void loadSettings( const QDomElement & _this );

	virtual QString nodeName() const;

	virtual f_cnt_t desiredReleaseFrames() const
	{
		return( 64 );
	}

	virtual PluginView * instantiateView( QWidget * _parent );

public slots:
	void updateVolumes();
	void updateFreqA1();
	void updateFreqA2();
	void updateFreqB1();
	void updateFreqB2();
	void updateWaveA1();
	void updateWaveA2();
	void updateWaveB1();
	void updateWaveB2();

protected:
	float m_lvol [NUM_OSCS];
    float m_rvol [NUM_OSCS];

	float m_lfreq [NUM_OSCS];
	float m_rfreq [NUM_OSCS];

private:
	inline float leftCh( float _vol, float _pan )
	{
		return ( _pan <= 0 ? 1.0 : 1.0 - ( _pan / 100.0 ) ) * _vol / 100.0;
	}

	inline float rightCh( float _vol, float _pan )
	{
		return ( _pan >= 0 ? 1.0 : 1.0 + ( _pan / 100.0 ) ) * _vol / 100.0;
	}

	// memcpy utilizing libsamplerate (src) for sinc interpolation
	inline void srccpy( float * _dst, float * _src )
	{
		int err;
		const int margin = 64;
		
		// copy to temp array
		float tmps [ GRAPHLEN + margin ]; // temp array in stack
		float * tmp = &tmps[0];

		memcpy( tmp, _src, sizeof( float ) * GRAPHLEN );
		memcpy( tmp + GRAPHLEN, _src, sizeof( float ) * margin );
		SRC_STATE * src_state = src_new( SRC_SINC_FASTEST, 1, &err );
		SRC_DATA src_data;
		src_data.data_in = tmp;
		src_data.input_frames = GRAPHLEN + margin;
		src_data.data_out = _dst;
		src_data.output_frames = WAVELEN;
		src_data.src_ratio = static_cast<double>( WAVERATIO );
		src_data.end_of_input = 0;
		err = src_process( src_state, &src_data ); 
		if( err ) { qDebug( "Watsyn SRC error: %s", src_strerror( err ) ); }
		src_delete( src_state );
	}

	// memcpy utilizing cubic interpolation
/*	inline void cipcpy( float * _dst, float * _src )
	{
		// calculate cyclic tangents
		float tang[GRAPHLEN];
		tang[0] = ( _src[1] - _src[ GRAPHLEN - 1] ) / 2;
		tang[ GRAPHLEN - 1 ] = ( _src[0] - _src[ GRAPHLEN - 2 ] ) / 2;
		for( int i = 1; i < GRAPHLEN-1; i++ )
		{
			tang[i] = ( _src[i+1] - _src[i-1] ) / 2;
		}
		
		// calculate cspline
		for( int i=0; i < WAVELEN; i++ )
		{
			const float s1 = _src[ i / WAVERATIO ];
			const float s2 = _src[ ( i / WAVERATIO + 1 ) % GRAPHLEN ];
			const float m1 = tang[ i / WAVERATIO ];
			const float m2 = tang[ ( i / WAVERATIO + 1 ) % GRAPHLEN ];
			
			const float x = static_cast<float>( i % WAVERATIO ) / WAVERATIO;
			const float x2 = x * x;
			const float x3 = x * x * x;
			
			_dst[i] = ( ( x3 * 2.0 - x2 * 3.0 + 1.0 ) * s1 ) +
				( ( x3 * -2.0 + x2 * 3.0 ) * s2 ) +
				( ( x3 - x2 * 2 + x ) * m1 ) +
				( ( x3 - x2 ) * m2 );		
		}
	}*/


	FloatModel a1_vol;
	FloatModel a2_vol;
	FloatModel b1_vol;
	FloatModel b2_vol;

	FloatModel a1_pan;
	FloatModel a2_pan;
	FloatModel b1_pan;
	FloatModel b2_pan;

	FloatModel a1_mult;
	FloatModel a2_mult;
	FloatModel b1_mult;
	FloatModel b2_mult;

	FloatModel a1_ltune;
	FloatModel a2_ltune;
	FloatModel b1_ltune;
	FloatModel b2_ltune;

	FloatModel a1_rtune;
	FloatModel a2_rtune;
	FloatModel b1_rtune;
	FloatModel b2_rtune;

	graphModel a1_graph;
	graphModel a2_graph;
	graphModel b1_graph;
	graphModel b2_graph;

	FloatModel m_abmix;

	FloatModel m_envAmt;

	TempoSyncKnobModel m_envAtt;
	TempoSyncKnobModel m_envHold;
	TempoSyncKnobModel m_envDec;

	FloatModel m_xtalk;

	IntModel m_amod;
	IntModel m_bmod;

	IntModel m_selectedGraph;
	
	float A1_wave [WAVELEN];
	float A2_wave [WAVELEN];
	float B1_wave [WAVELEN];
	float B2_wave [WAVELEN];

	friend class WatsynObject;
	friend class WatsynView;
};


class WatsynView : public InstrumentView
{
	Q_OBJECT
public:
	WatsynView( Instrument * _instrument,
					QWidget * _parent );
	virtual ~WatsynView();

protected slots:
	void updateLayout();

	void sinWaveClicked();
	void triWaveClicked();
	void sawWaveClicked();
	void sqrWaveClicked();

	void smoothClicked();
	void normalizeClicked();
	void invertClicked();
	void phaseLeftClicked();
	void phaseRightClicked();
	void loadClicked();

private:
	virtual void modelChanged();

// knobs
	knob * a1_volKnob;
	knob * a2_volKnob;
	knob * b1_volKnob;
	knob * b2_volKnob;

	knob * a1_panKnob;
	knob * a2_panKnob;
	knob * b1_panKnob;
	knob * b2_panKnob;

	knob * a1_multKnob;
	knob * a2_multKnob;
	knob * b1_multKnob;
	knob * b2_multKnob;

	knob * a1_ltuneKnob;
	knob * a2_ltuneKnob;
	knob * b1_ltuneKnob;
	knob * b2_ltuneKnob;

	knob * a1_rtuneKnob;
	knob * a2_rtuneKnob;
	knob * b1_rtuneKnob;
	knob * b2_rtuneKnob;

	knob * m_abmixKnob;

	knob * m_envAmtKnob;

	TempoSyncKnob * m_envAttKnob;
	TempoSyncKnob * m_envHoldKnob;
	TempoSyncKnob * m_envDecKnob;

	knob * m_xtalkKnob;

	automatableButtonGroup * m_selectedGraphGroup;
	automatableButtonGroup * m_aModGroup;
	automatableButtonGroup * m_bModGroup;

	graph * a1_graph;
	graph * a2_graph;
	graph * b1_graph;
	graph * b2_graph;

	pixmapButton * m_sinWaveButton;
	pixmapButton * m_triWaveButton;
	pixmapButton * m_sawWaveButton;
	pixmapButton * m_sqrWaveButton;
	pixmapButton * m_normalizeButton;
	pixmapButton * m_invertButton;
	pixmapButton * m_smoothButton;
	pixmapButton * m_phaseLeftButton;
	pixmapButton * m_phaseRightButton;
	pixmapButton * m_loadButton;

};

#endif
