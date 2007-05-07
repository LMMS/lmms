/*
 * triple_oscillator.h - declaration of class tripleOscillator a powerful
 *                       instrument-plugin with 3 oscillators
 *
 * Copyright (c) 2004-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


class oscillatorObject : public QObject
{
	Q_OBJECT
private:
	oscillator::waveShapes m_waveShape;
	volumeKnob * m_volKnob;
	knob * m_panKnob;
	knob * m_coarseKnob;
	knob * m_fineLKnob;
	knob * m_fineRKnob;
	knob * m_phaseOffsetKnob;
	knob * m_stereoPhaseDetuningKnob;
	automatableButtonGroup * m_waveBtnGrp;
	pixmapButton * m_usrWaveBtn;
	sampleBuffer * m_sampleBuffer;
	oscillator::modulationAlgos m_modulationAlgo;

	float m_volumeLeft;
	float m_volumeRight;
	// normalized detuning -> x/sampleRate
	float m_detuningLeft;
	float m_detuningRight;
	// normalized offset -> x/360
	float m_phaseOffsetLeft;
	float m_phaseOffsetRight;

	oscillatorObject( void );
	virtual ~oscillatorObject();

	friend class tripleOscillator;


private slots:
	void oscWaveCh( int _n );
	void oscUserDefWaveDblClick( void );

	void modCh( int _n );

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
	tripleOscillator( instrumentTrack * _channel );
	virtual ~tripleOscillator();

	virtual void FASTCALL playNote( notePlayHandle * _n,
						bool _try_parallelizing );
	virtual void FASTCALL deleteNotePluginData( notePlayHandle * _n );


	virtual void FASTCALL saveSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void FASTCALL loadSettings( const QDomElement & _this );

	virtual void FASTCALL setParameter( const QString & _param,
						const QString & _value );

	virtual QString nodeName( void ) const;


protected slots:
	void updateAllDetuning( void );


private:
	instrumentTrack * m_instrumentTrack;

	oscillatorObject m_osc[NUM_OF_OSCILLATORS];

	struct oscPtr
	{
		oscillator * oscLeft;
		oscillator * oscRight;
	} ;

	automatableButtonGroup * m_mod1BtnGrp;
	automatableButtonGroup * m_mod2BtnGrp;

} ;


#endif
