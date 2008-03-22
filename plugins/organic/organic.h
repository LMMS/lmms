/*
 * organic.h - additive synthesizer for organ-like sounds
 *
 * Copyright (c) 2006-2007 Andreas Brandmaier <andy/at/brandmaier/dot/de>
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


#ifndef _ORGANIC_H
#define _ORGANIC_H


#include "instrument.h"
#include "instrument_view.h"
#include "oscillator.h"
#include "automatable_model.h"

class QPixmap;

class knob;
class notePlayHandle;
class pixmapButton;
class volumeKnob;


class oscillatorObject : public model
{
	Q_OBJECT
private:
	int m_numOscillators;
	intModel m_waveShape;
	floatModel m_oscModel;
	floatModel m_volModel;
	floatModel m_panModel;
	floatModel m_detuneModel;

	float m_harmonic;
	float m_volumeLeft;
	float m_volumeRight;
	// normalized detuning -> x/sampleRate
	float m_detuningLeft;
	float m_detuningRight;
	// normalized offset -> x/360
	float m_phaseOffsetLeft;
	float m_phaseOffsetRight;

	oscillatorObject( model * _parent, track * _track );
	virtual ~oscillatorObject();

	friend class organicInstrument;
	friend class organicInstrumentView;


private slots:
	void oscButtonChanged( void );
	void updateVolume( void );
	void updateDetuning( void );

} ;


class organicInstrument : public instrument
{
	Q_OBJECT
public:
	organicInstrument( instrumentTrack * _channel_track );
	virtual ~organicInstrument();

	virtual void FASTCALL playNote( notePlayHandle * _n,
						bool _try_parallelizing );
	virtual void FASTCALL deleteNotePluginData( notePlayHandle * _n );


	virtual void FASTCALL saveSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void FASTCALL loadSettings( const QDomElement & _this );

	virtual QString nodeName( void ) const;
	
	int intRand( int min, int max );


public slots:
	void randomiseSettings( void );


private:
	float inline waveshape(float in, float amount);

	
	// fast atan, fast rather than accurate
	inline float fastatan( float x )
	{
    	return (x / (1.0 + 0.28 * (x * x)));
	}
	
	int m_numOscillators;
	
	oscillatorObject ** m_osc;
	
	struct oscPtr
	{
		oscillator * oscLeft;
		oscillator * oscRight;
	} ;

	const intModel m_modulationAlgo;

	floatModel  m_fx1Model;
	floatModel  m_volModel;

	virtual pluginView * instantiateView( QWidget * _parent );

private slots:
	void updateAllDetuning( void );

	friend class organicInstrumentView;
} ;


class organicInstrumentView : public instrumentView
{
public:
	organicInstrumentView( instrument * _instrument, QWidget * _parent );
	virtual ~organicInstrumentView();

private:
	virtual void modelChanged( void );

	struct oscillatorKnobs
	{
		oscillatorKnobs( volumeKnob * v,
					knob * o,
					knob * p,
					knob * dt ) :
			m_volKnob( v ),
			m_oscKnob( o ),
			m_panKnob( p ),
			m_detuneKnob( dt )
		{
		}
		oscillatorKnobs()
		{
		}
		
		volumeKnob * m_volKnob;
		knob * m_oscKnob;
		knob * m_panKnob;
		knob * m_detuneKnob;
	} ;

	oscillatorKnobs * m_oscKnobs;

	knob * m_fx1Knob;
	knob * m_volKnob;
	pixmapButton * m_randBtn;

	int m_numOscillators;
	
	static QPixmap * s_artwork;
};


#endif
