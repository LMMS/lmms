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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
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
class volumeKnob;

const int NUM_OF_OSCILLATORS = 3;


class tripleOscillator : public instrument
{
	Q_OBJECT
public:
	tripleOscillator( instrumentTrack * _channel );
	virtual ~tripleOscillator();

	virtual void FASTCALL playNote( notePlayHandle * _n );
	virtual void FASTCALL deleteNotePluginData( notePlayHandle * _n );


	virtual void FASTCALL saveSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void FASTCALL loadSettings( const QDomElement & _this );

	virtual void FASTCALL setParameter( const QString & _param,
						const QString & _value );

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

	void updateVolume( const QVariant & _i );
	void updateDetuningLeft( const QVariant & _i );
	void updateDetuningRight( const QVariant & _i );
	void updateAllDetuning( void );
	void updatePhaseOffsetLeft( const QVariant & _i );
	void updatePhaseOffsetRight( const QVariant & _i );


private:
	instrumentTrack * m_instrumentTrack;

	struct oscillatorData
	{
		oscillator::waveShapes waveShape;
		volumeKnob * volKnob;
		knob * panKnob;
		knob * coarseKnob;
		knob * fineLKnob;
		knob * fineRKnob;
		knob * phaseOffsetKnob;
		knob * stereoPhaseDetuningKnob;
		automatableButtonGroup * waveBtnGrp;
		pixmapButton * usrWaveBtn;
		sampleBuffer * m_sampleBuffer;
		float volumeLeft;
		float volumeRight;
		// normalized detuning -> x/sampleRate
		float detuningLeft;
		float detuningRight;
		// normalized offset -> x/360
		float phaseOffsetLeft;
		float phaseOffsetRight;
	} m_osc[NUM_OF_OSCILLATORS];

	struct oscPtr
	{
		oscillator * oscLeft;
		oscillator * oscRight;
	} ;


	oscillator::modulationAlgos * FASTCALL getModulationAlgo( int _n );

	oscillator::modulationAlgos m_modulationAlgo1;
	oscillator::modulationAlgos m_modulationAlgo2;
	const oscillator::modulationAlgos m_modulationAlgo3;

	automatableButtonGroup * m_mod1BtnGrp;
	automatableButtonGroup * m_mod2BtnGrp;

} ;


#endif
