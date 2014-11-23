/*
 * TripleOscillator.h - declaration of class TripleOscillator a powerful
 *                      instrument-plugin with 3 oscillators
 *
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

#ifndef _TRIPLE_OSCILLATOR_H
#define _TRIPLE_OSCILLATOR_H

#include "Instrument.h"
#include "InstrumentView.h"
#include "Oscillator.h"
#include "AutomatableModel.h"


class automatableButtonGroup;
class knob;
class NotePlayHandle;
class pixmapButton;
class SampleBuffer;

const int NUM_OF_OSCILLATORS = 3;


class OscillatorObject : public Model
{
	Q_OBJECT
public:
	OscillatorObject( Model * _parent, int _idx );
	virtual ~OscillatorObject();


private:
	FloatModel m_volumeModel;
	FloatModel m_panModel;
	FloatModel m_coarseModel;
	FloatModel m_fineLeftModel;
	FloatModel m_fineRightModel;
	FloatModel m_phaseOffsetModel;
	FloatModel m_stereoPhaseDetuningModel;
	IntModel m_waveShapeModel;
	IntModel m_modulationAlgoModel;
	SampleBuffer* m_sampleBuffer;

	float m_volumeLeft;
	float m_volumeRight;

	// normalized detuning -> x/sampleRate
	float m_detuningLeft;
	float m_detuningRight;
	// normalized offset -> x/360
	float m_phaseOffsetLeft;
	float m_phaseOffsetRight;

	friend class TripleOscillator;
	friend class TripleOscillatorView;


private slots:
	void oscUserDefWaveDblClick();

	void updateVolume();
	void updateDetuningLeft();
	void updateDetuningRight();
	void updatePhaseOffsetLeft();
	void updatePhaseOffsetRight();

} ;




class TripleOscillator : public Instrument
{
	Q_OBJECT
public:
	TripleOscillator( InstrumentTrack * _track );
	virtual ~TripleOscillator();

	virtual void playNote( NotePlayHandle * _n,
						sampleFrame * _working_buffer );
	virtual void deleteNotePluginData( NotePlayHandle * _n );


	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	virtual QString nodeName() const;

	virtual f_cnt_t desiredReleaseFrames() const
	{
		return( 128 );
	}

	virtual PluginView * instantiateView( QWidget * _parent );


protected slots:
	void updateAllDetuning();


private:
	OscillatorObject * m_osc[NUM_OF_OSCILLATORS];

	struct oscPtr
	{
		Oscillator * oscLeft;
		Oscillator * oscRight;
	} ;


	friend class TripleOscillatorView;

} ;



class TripleOscillatorView : public InstrumentView
{
	Q_OBJECT
public:
	TripleOscillatorView( Instrument * _instrument, QWidget * _parent );
	virtual ~TripleOscillatorView();


private:
	virtual void modelChanged();

	automatableButtonGroup * m_mod1BtnGrp;
	automatableButtonGroup * m_mod2BtnGrp;

	struct OscillatorKnobs
	{
		OscillatorKnobs( knob * v,
					knob * p,
					knob * c,
					knob * fl,
					knob * fr,
					knob * po,
					knob * spd,
					pixmapButton * uwb,
					automatableButtonGroup * wsbg ) :
			m_volKnob( v ),
			m_panKnob( p ),
			m_coarseKnob( c ),
			m_fineLeftKnob( fl ),
			m_fineRightKnob( fr ),
			m_phaseOffsetKnob( po ),
			m_stereoPhaseDetuningKnob( spd ),
			m_userWaveButton( uwb ),
			m_waveShapeBtnGrp( wsbg )
		{
		}
		OscillatorKnobs()
		{
		}
		knob * m_volKnob;
		knob * m_panKnob;
		knob * m_coarseKnob;
		knob * m_fineLeftKnob;
		knob * m_fineRightKnob;
		knob * m_phaseOffsetKnob;
		knob * m_stereoPhaseDetuningKnob;
		pixmapButton * m_userWaveButton;
		automatableButtonGroup * m_waveShapeBtnGrp;

	} ;

	OscillatorKnobs m_oscKnobs[NUM_OF_OSCILLATORS];
} ;



#endif
