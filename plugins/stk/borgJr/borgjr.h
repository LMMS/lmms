/*
 * borgjr.h - FM synthesis
 *
 * Copyright (c) 2016 Oskar Wallgren <oskar.wallgren13/at/gmail.com>
 *
 * Heavily based on 'Mallets' by Danny McRae and Tobias Doerffel.
 * Copyright (c) 2006-2008 Danny McRae <khjklujn/at/users.sourceforge.net>
 * Copyright (c) 2009-2015 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _BORGJR_H
#define _BORGJR_H

#include <Instrmnt.h>

#include "ComboBox.h"
#include "Instrument.h"
#include "InstrumentView.h"
#include "Knob.h"
#include "NotePlayHandle.h"
#include "LedCheckbox.h"

// As of Stk 4.4 all classes and types have been moved to the namespace "stk".
// However in older versions this namespace does not exist, therefore declare it
// so this plugin builds with all versions of Stk.
namespace stk { } ;
using namespace stk;


class borgjrSynth
{
public:

	// FMSynth
	borgjrSynth( const StkFloat _pitch,
			const StkFloat _velocity,
			const int _preset,
			const StkFloat _control1,
			const StkFloat _control2,
			const StkFloat _control4,
			const StkFloat _control11,
			const StkFloat _control128,
			const uint8_t _delay,
			const sample_rate_t _sample_rate );


	inline ~borgjrSynth()
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




class borgjrInstrument : public Instrument
{
	Q_OBJECT
public:
	borgjrInstrument( InstrumentTrack * _instrument_track );
	virtual ~borgjrInstrument();

	virtual void playNote( NotePlayHandle * _n,
						sampleFrame * _working_buffer );
	virtual void deleteNotePluginData( NotePlayHandle * _n );


	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	virtual QString nodeName() const;

	virtual PluginView * instantiateView( QWidget * _parent );


private:
	FloatModel m_modulatorModel;
	FloatModel m_crossfadeModel;
	FloatModel m_lfoSpeedModel;
	FloatModel m_lfoDepthModel;
	FloatModel m_adsrModel;

	ComboBoxModel m_presetsModel;
	FloatModel m_spreadModel;

	QVector<sample_t> m_scalers;

	bool m_filesMissing;


	friend class borgjrInstrumentView;

} ;


class borgjrInstrumentView: public InstrumentView
{
	Q_OBJECT
public:
	borgjrInstrumentView( borgjrInstrument * _instrument,
				QWidget * _parent );
	virtual ~borgjrInstrumentView();

public slots:
	void changePreset();

private:
	virtual void modelChanged();

	void setWidgetBackground( QWidget * _widget, const QString & _pic );
	QWidget * setupFMSynthControls( QWidget * _parent );

//	QWidget * m_FMSynthWidget;
//	QWidget * m_metalWidget;

	QWidget * m_FMSynthWidget;
	Knob * m_modulatorKnob;
	Knob * m_crossfadeKnob;
	Knob * m_lfoSpeedKnob;
	Knob * m_lfoDepthKnob;
	Knob * m_adsrKnob;

	ComboBox * m_presetsCombo;
	Knob * m_spreadKnob;
};

#endif
