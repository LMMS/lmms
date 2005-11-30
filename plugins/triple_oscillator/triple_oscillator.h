/*
 * triple_oscillator.h - declaration of class tripleOscillator a powerful
 *                       instrument-plugin with 3 oscillators
 *
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include "sample_buffer.h"


class knob;
class pixmapButton;
class notePlayHandle;


const int NUM_OF_OSCILLATORS = 3;


class tripleOscillator : public instrument
{
	Q_OBJECT
public:
	tripleOscillator( channelTrack * _channel ) FASTCALL;
	virtual ~tripleOscillator();

	virtual void FASTCALL playNote( notePlayHandle * _n );
	virtual void FASTCALL deleteNotePluginData( notePlayHandle * _n );


	virtual void FASTCALL saveSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void FASTCALL loadSettings( const QDomElement & _this );

	virtual QString nodeName( void ) const;


protected slots:
	// Slots for Osc 1
	void osc01SinWaveCh( bool _on );
	void osc01TriangleWaveCh( bool _on );
	void osc01SawWaveCh( bool _on );
	void osc01SquareWaveCh( bool _on );
	void osc01MoogSawWaveCh( bool _on );
	void osc01ExpWaveCh( bool _on );
	void osc01WhiteNoiseCh( bool _on );
	void osc01UserDefWaveCh( bool _on );
	void osc01UserDefWaveDblClick( void );

	// Slots for Osc 2
	void osc02SinWaveCh( bool _on );
	void osc02TriangleWaveCh( bool _on );
	void osc02SawWaveCh( bool _on );
	void osc02SquareWaveCh( bool _on );
	void osc02MoogSawWaveCh( bool _on );
	void osc02ExpWaveCh( bool _on );
	void osc02WhiteNoiseCh( bool _on );
	void osc02UserDefWaveCh( bool _on );
	void osc02UserDefWaveDblClick( void );

	// Slots for Osc 3
	void osc03SinWaveCh( bool _on );
	void osc03TriangleWaveCh( bool _on );
	void osc03SawWaveCh( bool _on );
	void osc03SquareWaveCh( bool _on );
	void osc03MoogSawWaveCh( bool _on );
	void osc03ExpWaveCh( bool _on );
	void osc03WhiteNoiseCh( bool _on );
	void osc03UserDefWaveCh( bool _on );
	void osc03UserDefWaveDblClick( void );

	// modulation-type-button slots
	void fm1BtnToggled( bool _on );
	void am1BtnToggled( bool _on );
	void mix1BtnToggled( bool _on );
	void sync1BtnToggled( bool _on );

	void fm2BtnToggled( bool _on );
	void am2BtnToggled( bool _on );
	void mix2BtnToggled( bool _on );
	void sync2BtnToggled( bool _on );


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
		pixmapButton * sinWaveBtn;
		pixmapButton * triangleWaveBtn;
		pixmapButton * sawWaveBtn;
		pixmapButton * sqrWaveBtn;
		pixmapButton * moogSawWaveBtn;
		pixmapButton * expWaveBtn;
		pixmapButton * whiteNoiseWaveBtn;
		pixmapButton * usrWaveBtn;
		sampleBuffer m_sampleBuffer;
	} m_osc[NUM_OF_OSCILLATORS];

	struct oscPtr
	{
		oscillator * oscLeft;
		oscillator * oscRight;
	} ;


	void FASTCALL doSinWaveBtn( oscillatorData * _osc );
	void FASTCALL doTriangleWaveBtn( oscillatorData * _osc );
	void FASTCALL doSawWaveBtn( oscillatorData * _osc );
	void FASTCALL doSqrWaveBtn( oscillatorData * _osc );
	void FASTCALL doMoogSawWaveBtn( oscillatorData * _osc );
	void FASTCALL doExpWaveBtn( oscillatorData * _osc );
	void FASTCALL doWhiteNoiseWaveBtn( oscillatorData * _osc );
	void FASTCALL doUsrWaveBtn( oscillatorData * _osc );
	pixmapButton * FASTCALL getModulationButton(
			oscillator::modulationAlgos _modulation_algo, int _n );
	void FASTCALL setModulationAlgo(
		oscillator::modulationAlgos _new_modulation_algo, int _n );
	oscillator::modulationAlgos FASTCALL getModulationAlgo( int _n );


	pixmapButton * m_fm1OscBtn;
	pixmapButton * m_am1OscBtn;
	pixmapButton * m_mix1OscBtn;
	pixmapButton * m_sync1OscBtn;
	pixmapButton * m_fm2OscBtn;
	pixmapButton * m_am2OscBtn;
	pixmapButton * m_mix2OscBtn;
	pixmapButton * m_sync2OscBtn;

	oscillator::modulationAlgos m_modulationAlgo1;
	oscillator::modulationAlgos m_modulationAlgo2;

} ;


#endif
