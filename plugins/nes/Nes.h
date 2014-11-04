/* Nes.h - A NES instrument plugin for LMMS
 *                        
 * Copyright (c) 2014 Vesa Kivimäki
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef NES_H
#define NES_H

#include "Instrument.h"
#include "InstrumentView.h"
#include "AutomatableModel.h"
#include "automatable_button.h"
#include "TempoSyncKnob.h"
#include "NotePlayHandle.h"
#include "pixmap_button.h"


#define makeknob( name, x, y, hint, unit, oname ) 		\
	name = new knob( knobStyled, this ); 				\
	name ->move( x, y );								\
	name ->setHintText( tr( hint ) + " ", unit );		\
	name ->setObjectName( oname );						\
	name ->setFixedSize( 29, 29 );

#define makenesled( name, x, y, ttip ) \
	name = new pixmapButton( this, NULL ); 	\
	name -> setCheckable( true );			\
	name -> move( x, y );					\
	name -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "nesled_on" ) ); \
	name -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "nesled_off" ) ); \
	toolTip::add( name, tr( ttip ) );

#define makedcled( name, x, y, ttip, active ) \
	pixmapButton * name = new pixmapButton( this, NULL ); 	\
	name -> move( x, y );					\
	name -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( active ) ); \
	name -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "nesdc_off" ) ); \
	toolTip::add( name, tr( ttip ) );

const float NES_SIMPLE_FILTER = 1.0 / 20.0; // simulate nes analog audio output
const float NFB = 895000.0f;
const float NOISE_FREQS[16] = 
	{ NFB/5, NFB/9, NFB/17, NFB/33, NFB/65, NFB/97, NFB/129, NFB/161, NFB/193, NFB/255, NFB/381, NFB/509, NFB/763, NFB/1017, NFB/2035, NFB/4069 };
const uint16_t LFSR_INIT = 1;
const float DUTY_CYCLE[4] = { 0.125, 0.25, 0.5, 0.75 };
const float DITHER_AMP = 1.0 / 60.0;
const float MIN_FREQ = 10.0;
const int TRIANGLE_WAVETABLE[32] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
									15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 };

const float NES_DIST = 0.9f; // simulate the slight nonlinear distortion in nes audio output

const float NES_MIXING_12 = 1.0 / 20.0;
const float NES_MIXING_34 = 1.0 / 12.0;
const float NES_MIXING_ALL = 1.0 / ( NES_MIXING_12 + NES_MIXING_34 ); // constants to simulate the hardwired mixing values for nes channels

const int MIN_WLEN = 4;


class NesInstrument;

class NesObject
{
public:
	NesObject( NesInstrument * nes, const sample_rate_t samplerate, NotePlayHandle * nph );
	virtual ~NesObject();
	
	void renderOutput( sampleFrame * buf, fpp_t frames );
	void updateVibrato( float * freq );
	void updatePitch();
	
	void updateLFSR( bool mode )
	{
		uint16_t LFSRx;
		if( mode )
		{
				LFSRx = m_LFSR & ( 1 << 8 ); // get bit 13
				LFSRx <<= 6; // shit to left so it overlaps with bit 14
		}
		else
		{
				LFSRx = m_LFSR & ( 1 << 13 ); // get bit 13
				LFSRx <<= 1; // shit to left so it overlaps with bit 14
		}
		m_LFSR ^= LFSRx; // xor bit 14 with bit 8/13 depending on mode
		m_LFSR <<= 1; // shift bit 14 to bit 15
		
		// cycle bit 14 to 0
		if( m_LFSR & ( 1 << 15 ) ) // if bit 15 is set
		{ 
			m_LFSR++; // set bit 0 - we know it to be 0 because of the left shift so we can just inc here
		}
	}
	
	inline bool LFSR() // returns true if bit 14 is set
	{
		if( m_LFSR & ( 1 << 14 ) ) 
		{
			return true; 
		}
		return false;
	}
	
	inline int wavelength( float freq )
	{
		return static_cast<int>( m_samplerate / freq );
	}
	
	inline float signedPow( float f, float e )
	{
		return f < 0 
			? powf( qAbs( f ), e ) * -1.0f
			: powf( f, e );
	}
	
	inline int nearestNoiseFreq( float f )
	{
		int n = 15;
		for( int i = 15; i >= 0; i-- )
		{
			if( f >= NOISE_FREQS[ i ] )
			{
				n = i;
			}
		}
		return n;
	}
	
private:
	NesInstrument * m_parent;
	const sample_rate_t m_samplerate;
	NotePlayHandle * m_nph;
	
	int m_pitchUpdateCounter;
	int m_pitchUpdateFreq;
	
	int m_ch1Counter;
	int m_ch2Counter;
	int m_ch3Counter;
	int m_ch4Counter;
	
	int m_ch1EnvCounter;
	int m_ch2EnvCounter;
	int m_ch4EnvCounter;
	
	int m_ch1EnvValue;
	int m_ch2EnvValue;
	int m_ch4EnvValue;
	
	int m_ch1SweepCounter;
	int m_ch2SweepCounter;
	int m_ch4SweepCounter;
	
	uint16_t m_LFSR;
	
	float m_12Last;
	float m_34Last;

	float m_lastNoteFreq;
	float m_lastNoiseFreq;
	
	int m_maxWlen;
	
	float m_nsf;

// wavelengths	
	int m_wlen1;
	int m_wlen2;
	int m_wlen3;
	int m_wlen4;
	
// vibrato
	int m_vibratoPhase;
};


class NesInstrument : public Instrument
{
	Q_OBJECT
public:
	NesInstrument( InstrumentTrack * instrumentTrack );
	virtual ~NesInstrument();
	
	virtual void playNote( NotePlayHandle * n,
						sampleFrame * workingBuffer );
	virtual void deleteNotePluginData( NotePlayHandle * n );


	virtual void saveSettings( QDomDocument & doc,
							QDomElement & element );
	virtual void loadSettings( const QDomElement & element );

	virtual QString nodeName() const;

	virtual f_cnt_t desiredReleaseFrames() const
	{
		return( 8 );
	}
	
	virtual PluginView * instantiateView( QWidget * parent );
	
public slots:
	void updateFreq1();
	void updateFreq2();
	void updateFreq3();

protected:
	//freq  helpers
	float m_freq1;
	float m_freq2;
	float m_freq3;
	
private:
	// channel 1
	BoolModel	m_ch1Enabled;
	FloatModel	m_ch1Crs;
	FloatModel	m_ch1Volume;
	
	BoolModel	m_ch1EnvEnabled;
	BoolModel	m_ch1EnvLooped;
	FloatModel	m_ch1EnvLen;
	
	IntModel	m_ch1DutyCycle;
	
	BoolModel	m_ch1SweepEnabled;
	FloatModel	m_ch1SweepAmt;
	FloatModel	m_ch1SweepRate;
	
	// channel 2
	BoolModel	m_ch2Enabled;
	FloatModel	m_ch2Crs;
	FloatModel	m_ch2Volume;
	
	BoolModel	m_ch2EnvEnabled;
	BoolModel	m_ch2EnvLooped;
	FloatModel	m_ch2EnvLen;
	
	IntModel	m_ch2DutyCycle;
	
	BoolModel	m_ch2SweepEnabled;
	FloatModel	m_ch2SweepAmt;
	FloatModel	m_ch2SweepRate;	
	
	//channel 3
	BoolModel	m_ch3Enabled;
	FloatModel	m_ch3Crs;
	FloatModel	m_ch3Volume;

	//channel 4
	BoolModel	m_ch4Enabled;
	FloatModel	m_ch4Volume;
	
	BoolModel	m_ch4EnvEnabled;
	BoolModel	m_ch4EnvLooped;
	FloatModel	m_ch4EnvLen;
	
	BoolModel	m_ch4NoiseMode;
	BoolModel	m_ch4NoiseFreqMode;
	FloatModel	m_ch4NoiseFreq;
	
	FloatModel	m_ch4Sweep;
	BoolModel	m_ch4NoiseQuantize;
	
	//master
	FloatModel	m_masterVol;
	FloatModel	m_vibrato;
	
	
	friend class NesObject;
	friend class NesInstrumentView;
};


class NesInstrumentView : public InstrumentView
{
	Q_OBJECT
public:
	NesInstrumentView( Instrument * instrument,
					QWidget * parent );
	virtual ~NesInstrumentView();	

private:
	virtual void modelChanged();
	
	// channel 1
	pixmapButton * 	m_ch1EnabledBtn;
	knob *			m_ch1CrsKnob;
	knob *			m_ch1VolumeKnob;
	
	pixmapButton *	m_ch1EnvEnabledBtn;
	pixmapButton *	m_ch1EnvLoopedBtn;
	knob *			m_ch1EnvLenKnob;
	
	automatableButtonGroup *	m_ch1DutyCycleGrp;
	
	pixmapButton *	m_ch1SweepEnabledBtn;
	knob *			m_ch1SweepAmtKnob;
	knob *			m_ch1SweepRateKnob;
	
	// channel 2
	pixmapButton * 	m_ch2EnabledBtn;
	knob *			m_ch2CrsKnob;
	knob *			m_ch2VolumeKnob;
	
	pixmapButton *	m_ch2EnvEnabledBtn;
	pixmapButton *	m_ch2EnvLoopedBtn;
	knob *			m_ch2EnvLenKnob;
	
	automatableButtonGroup *	m_ch2DutyCycleGrp;
	
	pixmapButton *	m_ch2SweepEnabledBtn;
	knob *			m_ch2SweepAmtKnob;
	knob *			m_ch2SweepRateKnob;
	
	//channel 3
	pixmapButton * 	m_ch3EnabledBtn;
	knob *			m_ch3CrsKnob;
	knob *			m_ch3VolumeKnob;

	//channel 4
	pixmapButton * 	m_ch4EnabledBtn;
	knob *			m_ch4VolumeKnob;
	
	pixmapButton * 	m_ch4EnvEnabledBtn;
	pixmapButton * 	m_ch4EnvLoopedBtn;
	knob *			m_ch4EnvLenKnob;
	
	pixmapButton * 	m_ch4NoiseModeBtn;
	pixmapButton * 	m_ch4NoiseFreqModeBtn;
	knob *			m_ch4NoiseFreqKnob;
	
	knob *			m_ch4SweepKnob;
	pixmapButton *	m_ch4NoiseQuantizeBtn;
	
	//master
	knob *			m_masterVolKnob;
	knob *			m_vibratoKnob;	
	
	static QPixmap *	s_artwork;
};

#endif
