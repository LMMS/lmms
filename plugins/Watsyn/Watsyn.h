/*
 * Watsyn.h - a 4-oscillator modulating wavetable synth
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


#ifndef WATSYN_H
#define WATSYN_H

#include "Instrument.h"
#include "InstrumentView.h"
#include "Graph.h"
#include "AutomatableModel.h"
#include "TempoSyncKnob.h"
#include <samplerate.h>

namespace lmms
{


#define makeknob( name, x, y, hint, unit, oname ) 		\
	name = new Knob( KnobType::Styled, this ); 				\
	name ->move( x, y );								\
	name ->setHintText( hint, unit );		\
	name ->setObjectName( oname );						\
	name ->setFixedSize( 19, 19 );

#define maketsknob( name, x, y, hint, unit, oname ) 		\
	name = new TempoSyncKnob( KnobType::Styled, this ); 				\
	name ->move( x, y );								\
	name ->setHintText( hint, unit );		\
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

namespace gui
{
class AutomatableButtonGroup;
class PixmapButton;
class WatsynView;
}

class WatsynObject
{
public:
	WatsynObject( 	float * _A1wave, float * _A2wave,
					float * _B1wave, float * _B2wave,
					int _amod, int _bmod, const sample_rate_t _samplerate, NotePlayHandle * _nph, fpp_t _frames,
					WatsynInstrument * _w );
	virtual ~WatsynObject();

	void renderOutput( fpp_t _frames );

	inline SampleFrame* abuf() const
	{
		return m_abuf;
	}
	inline SampleFrame* bbuf() const
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

	SampleFrame* m_abuf;
	SampleFrame* m_bbuf;

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
	~WatsynInstrument() override = default;

	void playNote( NotePlayHandle * _n,
						SampleFrame* _working_buffer ) override;
	void deleteNotePluginData( NotePlayHandle * _n ) override;


	void saveSettings( QDomDocument & _doc,
							QDomElement & _this ) override;
	void loadSettings( const QDomElement & _this ) override;

	QString nodeName() const override;

	float desiredReleaseTimeMs() const override
	{
		return 1.5f;
	}

	gui::PluginView* instantiateView( QWidget * _parent ) override;

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
	friend class gui::WatsynView;
};


namespace gui
{


class WatsynView : public InstrumentViewFixedSize
{
	Q_OBJECT
public:
	WatsynView( Instrument * _instrument,
					QWidget * _parent );
	~WatsynView() override = default;

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
	void modelChanged() override;

// knobs
	Knob * a1_volKnob;
	Knob * a2_volKnob;
	Knob * b1_volKnob;
	Knob * b2_volKnob;

	Knob * a1_panKnob;
	Knob * a2_panKnob;
	Knob * b1_panKnob;
	Knob * b2_panKnob;

	Knob * a1_multKnob;
	Knob * a2_multKnob;
	Knob * b1_multKnob;
	Knob * b2_multKnob;

	Knob * a1_ltuneKnob;
	Knob * a2_ltuneKnob;
	Knob * b1_ltuneKnob;
	Knob * b2_ltuneKnob;

	Knob * a1_rtuneKnob;
	Knob * a2_rtuneKnob;
	Knob * b1_rtuneKnob;
	Knob * b2_rtuneKnob;

	Knob * m_abmixKnob;

	Knob * m_envAmtKnob;

	TempoSyncKnob * m_envAttKnob;
	TempoSyncKnob * m_envHoldKnob;
	TempoSyncKnob * m_envDecKnob;

	Knob * m_xtalkKnob;

	AutomatableButtonGroup * m_selectedGraphGroup;
	AutomatableButtonGroup * m_aModGroup;
	AutomatableButtonGroup * m_bModGroup;

	Graph * a1_graph;
	Graph * a2_graph;
	Graph * b1_graph;
	Graph * b2_graph;

	PixmapButton * m_sinWaveButton;
	PixmapButton * m_triWaveButton;
	PixmapButton * m_sawWaveButton;
	PixmapButton * m_sqrWaveButton;
	PixmapButton * m_normalizeButton;
	PixmapButton * m_invertButton;
	PixmapButton * m_smoothButton;
	PixmapButton * m_phaseLeftButton;
	PixmapButton * m_phaseRightButton;
	PixmapButton * m_loadButton;

};


} // namespace gui

} // namespace lmms

#endif
