/*
 * mallets2.h - More tuned instruments that one would bang upon
 *
 * Copyright (c) 2016 Oskar Wallgren <oskarwallgren13/at/gmail.com>
 *
 * Improved version of Mallets by Danny McRae and Tobias Doerffel.
 * Copyright (c) 2006-2008 Danny McRae <khjklujn/at/users.sourceforge.net>
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


class mallets2Synth
{
public:
	// ModalBar
	mallets2Synth( 	const StkFloat pitch,
			const StkFloat velocity,
			const int modalPresets,
			const StkFloat stickHardness,
			const StkFloat stickPosition,
			const StkFloat stickMix,
			const StkFloat vibratoGain,
			const StkFloat vibratoFrequency,
			const uint8_t delay,
			const StkFloat randomness,
			const sample_rate_t _sample_rate );

	inline ~mallets2Synth()
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




class mallets2Instrument : public Instrument
{
	Q_OBJECT
public:
	mallets2Instrument( InstrumentTrack * _instrument_track );
	virtual ~mallets2Instrument();

	virtual void playNote( NotePlayHandle * _n,
						sampleFrame * _working_buffer );
	virtual void deleteNotePluginData( NotePlayHandle * _n );


	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	virtual QString nodeName() const;

	virtual PluginView * instantiateView( QWidget * _parent );


private:
	ComboBoxModel m_presetsModel;
	FloatModel m_hardnessModel;
	FloatModel m_positionModel;
	FloatModel m_stickModel;
	FloatModel m_vibratoGainModel;
	FloatModel m_vibratoFreqModel;
	FloatModel m_spreadModel;
	FloatModel m_randomModel;

	QVector<sample_t> m_scalers;

	bool m_filesMissing;


	friend class mallets2InstrumentView;

} ;


class mallets2InstrumentView: public InstrumentView
{
	Q_OBJECT
public:
	mallets2InstrumentView( mallets2Instrument * _instrument,
				QWidget * _parent );
	virtual ~mallets2InstrumentView();

public slots:
	void changePreset();

private:
	virtual void modelChanged();

	void setWidgetBackground( QWidget * _widget, const QString & _pic );
	QWidget * setupModalBarControls( QWidget * _parent );

	QWidget * m_modalBarWidget;
	ComboBox * m_presetsCombo;
	Knob * m_hardnessKnob;
	Knob * m_positionKnob;
	Knob * m_stickKnob;
	Knob * m_vibratoGainKnob;
	Knob * m_vibratoFreqKnob;
	Knob * m_spreadKnob;
	Knob * m_randomKnob;
};

#endif
