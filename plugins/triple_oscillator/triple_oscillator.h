/*
 * triple_oscillator.h - declaration of class tripleOscillator a powerful
 *                       instrument-plugin with 3 oscillators
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _TRIPLE_OSCILLATOR_H
#define _TRIPLE_OSCILLATOR_H


#include "instrument.h"
#include "instrument_view.h"
#include "oscillator.h"
#include "automatable_model.h"


class automatableButtonGroup;
class knob;
class notePlayHandle;
class pixmapButton;
class sampleBuffer;
class volumeKnob;

const int NUM_OF_OSCILLATORS = 3;


class oscillatorObject : public model
{
	Q_OBJECT
public:
	oscillatorObject( model * _parent, track * _track );
	virtual ~oscillatorObject();


private:
	floatModel m_volumeModel;
	floatModel m_panModel;
	floatModel m_coarseModel;
	floatModel m_fineLeftModel;
	floatModel m_fineRightModel;
	floatModel m_phaseOffsetModel;
	floatModel m_stereoPhaseDetuningModel;
	intModel m_waveShapeModel;
	intModel m_modulationAlgoModel;
	sampleBuffer * m_sampleBuffer;

	float m_volumeLeft;
	float m_volumeRight;

	// normalized detuning -> x/sampleRate
	float m_detuningLeft;
	float m_detuningRight;
	// normalized offset -> x/360
	float m_phaseOffsetLeft;
	float m_phaseOffsetRight;

	friend class tripleOscillator;
	friend class tripleOscillatorView;


private slots:
	void oscUserDefWaveDblClick( void );

	void updateVolume( void );
	void updateDetuningLeft( void );
	void updateDetuningRight( void );
	void updatePhaseOffsetLeft( void );
	void updatePhaseOffsetRight( void );

} ;




class tripleOscillator : public instrument
{
	Q_OBJECT
public:
	tripleOscillator( instrumentTrack * _track );
	virtual ~tripleOscillator();

	virtual void playNote( notePlayHandle * _n, bool _try_parallelizing,
						sampleFrame * _working_buffer );
	virtual void deleteNotePluginData( notePlayHandle * _n );


	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	virtual void setParameter( const QString & _param,
						const QString & _value );

	virtual QString nodeName( void ) const;

	virtual f_cnt_t desiredReleaseFrames( void ) const
	{
		return( 128 );
	}

	virtual pluginView * instantiateView( QWidget * _parent );


protected slots:
	void updateAllDetuning( void );


private:
	instrumentTrack * m_instrumentTrack;

	oscillatorObject * m_osc[NUM_OF_OSCILLATORS];

	struct oscPtr
	{
		oscillator * oscLeft;
		oscillator * oscRight;
	} ;


	friend class tripleOscillatorView;

} ;



class tripleOscillatorView : public instrumentView
{
	Q_OBJECT
public:
	tripleOscillatorView( instrument * _instrument, QWidget * _parent );
	virtual ~tripleOscillatorView();


private:
	virtual void modelChanged( void );

	automatableButtonGroup * m_mod1BtnGrp;
	automatableButtonGroup * m_mod2BtnGrp;

	struct oscillatorKnobs
	{
		oscillatorKnobs( volumeKnob * v,
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
		oscillatorKnobs()
		{
		}
		volumeKnob * m_volKnob;
		knob * m_panKnob;
		knob * m_coarseKnob;
		knob * m_fineLeftKnob;
		knob * m_fineRightKnob;
		knob * m_phaseOffsetKnob;
		knob * m_stereoPhaseDetuningKnob;
		pixmapButton * m_userWaveButton;
		automatableButtonGroup * m_waveShapeBtnGrp;

	} ;

	oscillatorKnobs m_oscKnobs[NUM_OF_OSCILLATORS];
} ;



#endif
