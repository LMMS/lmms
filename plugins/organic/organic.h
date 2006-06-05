/*
 * organic.h - additive synthesizer for organ-like sounds
 *
 * Copyright (c) 2006 Andreas Brandmaier <andy/at/brandmaier/dot/de>
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


#ifndef _ORGANIC_H
#define _ORGANIC_H


#include "instrument.h"
#include "spc_bg_hndl_widget.h"
#include "led_checkbox.h"
#include "oscillator.h"

class QPixmap;

class knob;
class notePlayHandle;
class pixmapButton;
class volumeKnob;


class organicInstrument : public instrument, public specialBgHandlingWidget
{
	Q_OBJECT
public:
	organicInstrument( instrumentTrack * _channel_track );
	virtual ~organicInstrument();

	virtual void FASTCALL playNote( notePlayHandle * _n );
	virtual void FASTCALL deleteNotePluginData( notePlayHandle * _n );


	virtual void FASTCALL saveSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void FASTCALL loadSettings( const QDomElement & _this );

	virtual QString nodeName( void ) const;
	
	int intRand( int min, int max );


public slots:

	void oscButtonChanged( void );
	void randomiseSettings();

private:

	float inline waveshape(float in, float amount);

	
	// fast atan, fast rather than accurate
	inline float fastatan( float x )
	{
    	return (x / (1.0 + 0.28 * (x * x)));
	}
	
	static QPixmap * s_artwork;
	
	int m_num_oscillators;
	
	struct oscillatorData
	{
		oscillator::waveShapes waveShape;
		knob * oscKnob;
		volumeKnob * volKnob;
		knob * panKnob;
		knob * detuneKnob;
		float harmonic;
		float volumeLeft;
		float volumeRight;
		// normalized detuning -> x/sampleRate
		float detuningLeft;
		float detuningRight;
		// normalized offset -> x/360
		float phaseOffsetLeft;
		float phaseOffsetRight;
	};
	
	oscillatorData* m_osc;
	
	struct oscPtr
	{
		oscillator * oscLeft;
		oscillator * oscRight;
	} ;

	const oscillator::modulationAlgos m_modulationAlgo;

		knob * fx1Knob;
		knob * volKnob;
		pixmapButton * m_randBtn;

private slots:

	void updateVolume( int _i );
	void updateDetuning( int _i );
	void updateAllDetuning( void );

} ;


#include "organic.moc"


#endif
