/*
 * triple_oscillator.h - declaration of class tripleOscillator a powerful
 *                       instrument-plugin with 3 oscillators
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifndef _TRIPLE_OSCILLATOR_H
#define _TRIPLE_OSCILLATOR_H


#include "instrument.h"
#include "oscillator.h"


class automatableButtonGroup;
class knob;
class notePlayHandle;
class pixmapButton;
class sampleBuffer;


const int NUM_OF_OSCILLATORS = 3;


class tripleOscillator : public instrument
{
	Q_OBJECT
public:
	tripleOscillator( channelTrack * _channel );
	virtual ~tripleOscillator();

	virtual void FASTCALL playNote( notePlayHandle * _n );
	virtual void FASTCALL deleteNotePluginData( notePlayHandle * _n );


	virtual void FASTCALL saveSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void FASTCALL loadSettings( const QDomElement & _this );

	virtual QString nodeName( void ) const;


protected slots:
	void osc0WaveCh( int _n );
	void osc1WaveCh( int _n );
	void osc2WaveCh( int _n );
	void osc0UserDefWaveDblClick( void );
	void osc1UserDefWaveDblClick( void );
	void osc2UserDefWaveDblClick( void );

	void mod1Ch( int _n );
	void mod2Ch( int _n );


private:
	channelTrack * m_channelTrack;

	struct oscillatorData
	{
		oscillator::waveShapes waveShape;
		knob * volKnob;
		knob * panKnob;
		knob * coarseKnob;
		knob * fineLKnob;
		knob * fineRKnob;
		knob * phaseOffsetKnob;
		knob * stereoPhaseDetuningKnob;
		automatableButtonGroup * waveBtnGrp;
		pixmapButton * usrWaveBtn;
		sampleBuffer * m_sampleBuffer;
	} m_osc[NUM_OF_OSCILLATORS];

	struct oscPtr
	{
		oscillator * oscLeft;
		oscillator * oscRight;
	} ;


/*	pixmapButton * FASTCALL getModulationButton(
			oscillator::modulationAlgos _modulation_algo, int _n );*/
/*	void FASTCALL setModulationAlgo(
		oscillator::modulationAlgos _new_modulation_algo, int _n );*/
	oscillator::modulationAlgos FASTCALL getModulationAlgo( int _n );

	oscillator::modulationAlgos m_modulationAlgo1;
	oscillator::modulationAlgos m_modulationAlgo2;

	automatableButtonGroup * m_mod1BtnGrp;
	automatableButtonGroup * m_mod2BtnGrp;

} ;


#endif
