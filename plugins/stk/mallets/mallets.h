/*
 * mallets.h - tuned instruments that one would bang upon
 *
 * Copyright (c) 2006-2008 Danny McRae <khjklujn/at/users.sourceforge.net>
 * 
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


#ifndef _MALLET_H
#define _MALLET_H

#include "Instrmnt.h"

#include "combobox.h"
#include "Instrument.h"
#include "InstrumentView.h"
#include "knob.h"
#include "NotePlayHandle.h"
#include "led_checkbox.h"

// As of Stk 4.4 all classes and types have been moved to the namespace "stk".
// However in older versions this namespace does not exist, therefore declare it
// so this plugin builds with all versions of Stk.
namespace stk { } ;
using namespace stk;


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
			const uint8_t _delay,
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
			const uint8_t _delay,
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
			const uint8_t _delay,
			const sample_rate_t _sample_rate );

	inline ~malletsSynth()
	{
		m_voice->noteOff( 0.0 );
		delete[] m_delay;
		delete m_voice;
	}

	inline sample_t nextSampleLeft()
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
	
	inline sample_t nextSampleRight()
	{
		StkFloat s = m_delay[m_delayRead];
		m_delayRead++;
		return( s );
	}

	inline void setFrequency( const StkFloat _pitch )
	{
		if( m_voice )
		{
			m_voice->setFrequency( _pitch );
		}
	}


protected:
	Instrmnt * m_voice;

	StkFloat * m_delay;
	uint8_t m_delayRead;
	uint8_t m_delayWrite;
};




class malletsInstrument : public Instrument
{
	Q_OBJECT
public:
	malletsInstrument( InstrumentTrack * _instrument_track );
	virtual ~malletsInstrument();

	virtual void playNote( NotePlayHandle * _n,
						sampleFrame * _working_buffer );
	virtual void deleteNotePluginData( NotePlayHandle * _n );


	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	virtual QString nodeName() const;

	virtual PluginView * instantiateView( QWidget * _parent );


private:
	FloatModel m_hardnessModel;
	FloatModel m_positionModel;
	FloatModel m_vibratoGainModel;
	FloatModel m_vibratoFreqModel;
	FloatModel m_stickModel;

	FloatModel m_modulatorModel;
	FloatModel m_crossfadeModel;
	FloatModel m_lfoSpeedModel;
	FloatModel m_lfoDepthModel;
	FloatModel m_adsrModel;

	FloatModel m_pressureModel;
	FloatModel m_motionModel;
	FloatModel m_vibratoModel;
	FloatModel m_velocityModel;

	BoolModel m_strikeModel;

	ComboBoxModel m_presetsModel;
	FloatModel m_spreadModel;

	QVector<sample_t> m_scalers;

	bool m_filesMissing;


	friend class malletsInstrumentView;

} ;


class malletsInstrumentView: public InstrumentView
{
	Q_OBJECT
public:
	malletsInstrumentView( malletsInstrument * _instrument,
				QWidget * _parent );
	virtual ~malletsInstrumentView();

public slots:
	void changePreset();

private:
	virtual void modelChanged();

	void setWidgetBackground( QWidget * _widget, const QString & _pic );
	QWidget * setupModalBarControls( QWidget * _parent );
	QWidget * setupTubeBellControls( QWidget * _parent );
	QWidget * setupBandedWGControls( QWidget * _parent );

	QWidget * m_modalBarWidget;
	knob * m_hardnessKnob;
	knob * m_positionKnob;
	knob * m_vibratoGainKnob;
	knob * m_vibratoFreqKnob;
	knob * m_stickKnob;

	QWidget * m_tubeBellWidget;
	knob * m_modulatorKnob;
	knob * m_crossfadeKnob;
	knob * m_lfoSpeedKnob;
	knob * m_lfoDepthKnob;
	knob * m_adsrKnob;

	QWidget * m_bandedWGWidget;
	knob * m_pressureKnob;
	knob * m_motionKnob;
	knob * m_vibratoKnob;
	knob * m_velocityKnob;
	ledCheckBox * m_strikeLED;

	comboBox * m_presetsCombo;
	knob * m_spreadKnob;
};

#endif
