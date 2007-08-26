/*
 * mallets.h - tuned instruments that one would bang upon
 *
 * Copyright (c) 2006-2007 Danny McRae <khjklujn/at/users.sourceforge.net>
 * 
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


#ifndef _MALLET_H
#define _MALLET_H

#include "Instrmnt.h"

#include "combobox.h"
#include "instrument.h"
#include "led_checkbox.h"
#include "mixer.h"

class knob;
class notePlayHandle;


class malletsSynth
{
public:
	// ModalBar
	malletsSynth( const StkFloat _pitch,
			const StkFloat _velocity,
			const StkFloat _control1,
			const StkFloat _control2,
			const StkFloat _control4,
			const StkFloat _control8,
			const StkFloat _control11,
			const int _control16,
			const Uint8 _delay,
			const sample_rate_t _sample_rate );

	// TubeBell
	malletsSynth( const StkFloat _pitch,
			const StkFloat _velocity,
			const int _preset,
			const StkFloat _control1,
			const StkFloat _control2,
			const StkFloat _control4,
			const StkFloat _control11,
			const StkFloat _control128,
			const Uint8 _delay,
			const sample_rate_t _sample_rate );

	// BandedWG
	malletsSynth( const StkFloat _pitch,
			const StkFloat _velocity,
			const StkFloat _control2,
			const StkFloat _control4,
			const StkFloat _control11,
			const int _control16,
			const StkFloat _control64,
			const StkFloat _control128,
			const Uint8 _delay,
			const sample_rate_t _sample_rate );

	inline ~malletsSynth( void )
	{
		m_voice->noteOff( 0.0 );
		delete[] m_delay;
		delete m_voice;
	}

	inline sample_t nextSampleLeft( void )
	{
		if( m_voice == NULL )
		{
			return( 0.0f );
		}
		else
		{
			StkFloat s = m_voice->tick();
			m_delay[m_delayWrite] = s;
			m_delayWrite++;
			return( s );
		}
	}
	
	inline sample_t nextSampleRight( void )
	{
		StkFloat s = m_delay[m_delayRead];
		m_delayRead++;
		return( s );
	}


protected:
	Instrmnt * m_voice;
	
	StkFloat * m_delay;
	Uint8 m_delayRead;
	Uint8 m_delayWrite;
};




class mallets : public instrument
{
	Q_OBJECT
public:
	mallets( instrumentTrack * _channel_track );
	virtual ~mallets();

	virtual void FASTCALL playNote( notePlayHandle * _n,
						bool _try_parallelizing );
	virtual void FASTCALL deleteNotePluginData( notePlayHandle * _n );


	virtual void FASTCALL saveSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void FASTCALL loadSettings( const QDomElement & _this );

	virtual QString nodeName( void ) const;

public slots:
	void changePreset( int _preset );

private:
	void setWidgetBackground( QWidget * _widget, const QString & _pic );
	QWidget * setupModalBarControls( QWidget * _parent, track * _track );
	QWidget * setupTubeBellControls( QWidget * _parent, track * _track );
	QWidget * setupBandedWGControls( QWidget * _parent, track * _track );
	comboBox * setupPresets( QWidget * _parent, track * _track );
	
	QWidget * m_modalBarWidget;
	knob * m_hardness;
	knob * m_position;
	knob * m_vibratoGain;
	knob * m_vibratoFreq;
	knob * m_stick;
	
	QWidget * m_tubeBellWidget;
	knob * m_modulator;
	knob * m_crossfade;
	knob * m_lfoSpeed;
	knob * m_lfoDepth;
	knob * m_adsr;
	
	QWidget * m_bandedWGWidget;
	knob * m_pressure;
	knob * m_motion;
	knob * m_vibrato;
	knob * m_velocity;
	ledCheckBox * m_strike;
	
	comboBox * m_presets;
	knob * m_spread;
	
	QVector<sample_t> m_scalers;
	sampleFrame * m_buffer;
	bool m_filesMissing;

} ;


#endif
