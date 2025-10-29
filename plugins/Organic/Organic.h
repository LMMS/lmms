/*
 * Organic.h - additive synthesizer for organ-like sounds
 *
 * Copyright (c) 2006-2015 Andreas Brandmaier <andy/at/brandmaier/dot/de>
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

#ifndef LMMS_ORGANIC_H
#define LMMS_ORGANIC_H

#include <QString>

#include "Instrument.h"
#include "InstrumentView.h"
#include "AutomatableModel.h"


namespace lmms
{


class NotePlayHandle;  // IWYU pragma: keep
class Oscillator;

namespace gui
{
class Knob;
class PixmapButton;
class OrganicInstrumentView;
} // namespace gui

const int NUM_OSCILLATORS = 8;
const int NUM_HARMONICS = 18;
const QString HARMONIC_NAMES[NUM_HARMONICS] =  {
	"Octave below",
	"Fifth below",
	"Fundamental",
	"2nd harmonic",
	"3rd harmonic",
	"4th harmonic",
	"5th harmonic",
	"6th harmonic",
	"7th harmonic",
	"8th harmonic",
	"9th harmonic",
	"10th harmonic",
	"11th harmonic",
	"12th harmonic",
	"13th harmonic",
	"14th harmonic",
	"15th harmonic",
	"16th harmonic"
	};
	
const QString WAVEFORM_NAMES[6] = {
	"Sine wave",
	"Saw wave",
	"Square wave",
	"Triangle wave",
	"Moog saw wave",
	"Exponential wave"
	};
	
const float CENT = 1.0f / 1200.0f;

class OscillatorObject : public Model
{
	Q_OBJECT
private:
	int m_numOscillators;
	IntModel m_waveShape;
	FloatModel m_oscModel;
	FloatModel m_harmModel;
	FloatModel m_volModel;
	FloatModel m_panModel;
	FloatModel m_detuneModel;

	float m_volumeLeft;
	float m_volumeRight;
	// normalized detuning -> x/sampleRate
	float m_detuningLeft;
	float m_detuningRight;
	// normalized offset -> x/360
	float m_phaseOffsetLeft;
	float m_phaseOffsetRight;

	OscillatorObject( Model * _parent, int _index );
	~OscillatorObject() override = default;

	friend class OrganicInstrument;
	friend class gui::OrganicInstrumentView;


private slots:
	void oscButtonChanged();
	void updateVolume();
	void updateDetuning();

} ;


class OrganicInstrument : public Instrument
{
	Q_OBJECT
public:
	OrganicInstrument( InstrumentTrack * _instrument_track );
	~OrganicInstrument() override;

	void playNote( NotePlayHandle * _n,
						SampleFrame* _working_buffer ) override;
	void deleteNotePluginData( NotePlayHandle * _n ) override;


	void saveSettings(QDomDocument& doc, QDomElement& elem) override;
	void loadSettings(const QDomElement& elem) override;

	QString nodeName() const override;

	int intRand( int min, int max );

	static float * s_harmonics;

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
		float phaseOffsetLeft[NUM_OSCILLATORS];
		float phaseOffsetRight[NUM_OSCILLATORS];		
	} ;

	const IntModel m_modulationAlgo;

	FloatModel  m_fx1Model;
	FloatModel  m_volModel;

	gui::PluginView* instantiateView( QWidget * _parent ) override;


private slots:
	void updateAllDetuning();

	friend class gui::OrganicInstrumentView;
} ;

namespace gui
{


class OrganicInstrumentView : public InstrumentViewFixedSize
{
	Q_OBJECT
public:
	OrganicInstrumentView( Instrument * _instrument, QWidget * _parent );
	~OrganicInstrumentView() override;

private:
	void modelChanged() override;

	struct OscillatorKnobs
	{
		OscillatorKnobs( 
					Knob * h,
					Knob * v,
					Knob * o,
					Knob * p,
					Knob * dt ) :
			m_harmKnob( h ),
			m_volKnob( v ),
			m_oscKnob( o ),
			m_panKnob( p ),
			m_detuneKnob( dt )
		{
		}
		OscillatorKnobs() = default;

		Knob * m_harmKnob;
		Knob * m_volKnob;
		Knob * m_oscKnob;
		Knob * m_panKnob;
		Knob * m_detuneKnob;
	} ;

	OscillatorKnobs * m_oscKnobs;

	Knob * m_fx1Knob;
	Knob * m_volKnob;
	PixmapButton * m_randBtn;

	int m_numOscillators;

	
protected slots:
	void updateKnobHint();
};


} // namespace gui

} // namespace lmms

#endif // LMMS_ORGANIC_H
