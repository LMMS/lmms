/*
 * organic.h - additive synthesizer for organ-like sounds
 *
 * Copyright (c) 2006-2008 Andreas Brandmaier <andy/at/brandmaier/dot/de>
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

#include "Instrument.h"
#include "InstrumentView.h"
#include "Oscillator.h"
#include "AutomatableModel.h"

class QPixmap;

class knob;
class NotePlayHandle;
class pixmapButton;


class OscillatorObject : public Model
{
	Q_OBJECT
private:
	int m_numOscillators;
	IntModel m_waveShape;
	FloatModel m_oscModel;
	FloatModel m_volModel;
	FloatModel m_panModel;
	FloatModel m_detuneModel;

	float m_harmonic;
	float m_volumeLeft;
	float m_volumeRight;
	// normalized detuning -> x/sampleRate
	float m_detuningLeft;
	float m_detuningRight;
	// normalized offset -> x/360
	float m_phaseOffsetLeft;
	float m_phaseOffsetRight;

	OscillatorObject( Model * _parent, int _index );
	virtual ~OscillatorObject();

	friend class organicInstrument;
	friend class organicInstrumentView;


private slots:
	void oscButtonChanged();
	void updateVolume();
	void updateDetuning();

} ;


class organicInstrument : public Instrument
{
	Q_OBJECT
public:
	organicInstrument( InstrumentTrack * _instrument_track );
	virtual ~organicInstrument();

	virtual void playNote( NotePlayHandle * _n,
						sampleFrame * _working_buffer );
	virtual void deleteNotePluginData( NotePlayHandle * _n );


	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	virtual QString nodeName() const;
	
	int intRand( int min, int max );


public slots:
	void randomiseSettings();


private:
	float inline waveshape(float in, float amount);

	
	// fast atan, fast rather than accurate
	inline float fastatan( float x )
	{
    	return (x / (1.0 + 0.28 * (x * x)));
	}
	
	int m_numOscillators;
	
	OscillatorObject ** m_osc;
	
	struct oscPtr
	{
		Oscillator * oscLeft;
		Oscillator * oscRight;
	} ;

	const IntModel m_modulationAlgo;

	FloatModel  m_fx1Model;
	FloatModel  m_volModel;

	virtual PluginView * instantiateView( QWidget * _parent );

private slots:
	void updateAllDetuning();

	friend class organicInstrumentView;
} ;


class organicInstrumentView : public InstrumentView
{
	Q_OBJECT
public:
	organicInstrumentView( Instrument * _instrument, QWidget * _parent );
	virtual ~organicInstrumentView();

private:
	virtual void modelChanged();

	struct OscillatorKnobs
	{
		OscillatorKnobs( knob * v,
					knob * o,
					knob * p,
					knob * dt ) :
			m_volKnob( v ),
			m_oscKnob( o ),
			m_panKnob( p ),
			m_detuneKnob( dt )
		{
		}
		OscillatorKnobs()
		{
		}
		
		knob * m_volKnob;
		knob * m_oscKnob;
		knob * m_panKnob;
		knob * m_detuneKnob;
	} ;

	OscillatorKnobs * m_oscKnobs;

	knob * m_fx1Knob;
	knob * m_volKnob;
	pixmapButton * m_randBtn;

	int m_numOscillators;
	
	static QPixmap * s_artwork;
};


#endif
