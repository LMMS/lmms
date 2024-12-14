/*
 * Mallets.h - tuned instruments that one would bang upon
 *
 * Copyright (c) 2006-2008 Danny McRae <khjklujn/at/users.sourceforge.net>
 * 
 * 
 * This file is part of LMMS - https://lmms.io
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

#include <stk/Instrmnt.h>

#include "ComboBox.h"
#include "Instrument.h"
#include "InstrumentView.h"
#include "Knob.h"
#include "NotePlayHandle.h"
#include "LedCheckBox.h"

// As of Stk 4.4 all classes and types have been moved to the namespace "stk".
// However in older versions this namespace does not exist, therefore declare it
// so this plugin builds with all versions of Stk.
namespace stk { } ;

namespace lmms
{


using namespace stk;

static const int MALLETS_PRESET_VERSION = 1;


namespace gui
{
class MalletsInstrumentView;
} // namespace gui


class MalletsSynth
{
public:
	// ModalBar
	MalletsSynth( const StkFloat _pitch,
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
	MalletsSynth( const StkFloat _pitch,
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
	MalletsSynth( const StkFloat _pitch,
			const StkFloat _velocity,
			const StkFloat _control2,
			const StkFloat _control4,
			const StkFloat _control11,
			const int _control16,
			const StkFloat _control64,
			const StkFloat _control128,
			const uint8_t _delay,
			const sample_rate_t _sample_rate );

	inline ~MalletsSynth()
	{
		if (m_voice) {m_voice->noteOff(0.0);}
		delete[] m_delay;
		delete m_voice;
	}

	inline sample_t nextSampleLeft()
	{
		if( m_voice == nullptr )
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

	inline void setFrequency(const StkFloat _pitch)
	{
		if (m_voice) { m_voice->setFrequency(_pitch); }
	}

	// ModalBar updates
	inline void setVibratoGain(const StkFloat _control8)
	{
		// bug in stk, Control Number 8 and 1 swapped in ModalBar
		// we send the control number for stick direct mix instead
		if (m_voice) { m_voice->controlChange(8, _control8); }
	}

	inline void setVibratoFreq(const StkFloat _control11)
	{
		if (m_voice) { m_voice->controlChange(11, _control11); }
	}

	// Tubular Bells updates
	inline void setADSR(const StkFloat _control128)
	{
		if (m_voice) { m_voice->controlChange(128, _control128); }
	}

	inline void setLFODepth(const StkFloat _control1)
	{
		if (m_voice) { m_voice->controlChange(1, _control1); }
	}

	inline void setLFOSpeed(const StkFloat _control11)
	{
		if (m_voice) { m_voice->controlChange(11, _control11); }
	}

	inline int presetIndex()
	{
		return m_presetIndex;
	}

	inline void setPresetIndex(int presetIndex)
	{
		m_presetIndex = presetIndex;
	}


protected:
	int m_presetIndex;
	Instrmnt * m_voice;

	StkFloat * m_delay;
	uint8_t m_delayRead;
	uint8_t m_delayWrite;
};




class MalletsInstrument : public Instrument
{
	Q_OBJECT
public:
	MalletsInstrument( InstrumentTrack * _instrument_track );
	~MalletsInstrument() override = default;

	void playNoteImpl(NotePlayHandle* nph, CoreAudioDataMut out) override;
	void deleteNotePluginData( NotePlayHandle * _n ) override;


	void saveSettings( QDomDocument & _doc, QDomElement & _parent ) override;
	void loadSettings( const QDomElement & _this ) override;

	QString nodeName() const override;

	gui::PluginView* instantiateView( QWidget * _parent ) override;


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
	FloatModel m_randomModel;
	IntModel m_versionModel;
	BoolModel m_isOldVersionModel;

	QVector<sample_t> m_scalers;

	bool m_filesMissing;


	friend class gui::MalletsInstrumentView;

} ;

namespace gui
{


class MalletsInstrumentView: public InstrumentViewFixedSize
{
	Q_OBJECT
public:
	MalletsInstrumentView( MalletsInstrument * _instrument,
				QWidget * _parent );
	~MalletsInstrumentView() override = default;

public slots:
	void changePreset();

private:
	void modelChanged() override;

	void setWidgetBackground( QWidget * _widget, const QString & _pic );
	QWidget * setupModalBarControls( QWidget * _parent );
	QWidget * setupTubeBellControls( QWidget * _parent );
	QWidget * setupBandedWGControls( QWidget * _parent );

	QWidget * m_modalBarWidget;
	Knob * m_hardnessKnob;
	Knob * m_positionKnob;
	Knob * m_vibratoGainKnob;
	Knob * m_vibratoFreqKnob;
	Knob * m_stickKnob;

	QWidget * m_tubeBellWidget;
	Knob * m_modulatorKnob;
	Knob * m_crossfadeKnob;
	Knob * m_lfoSpeedKnob;
	Knob * m_lfoDepthKnob;
	Knob * m_adsrKnob;

	QWidget * m_bandedWGWidget;
	Knob * m_pressureKnob;
//	Knob * m_motionKnob;
//	Knob * m_vibratoKnob;
	Knob * m_velocityKnob;
//	LedCheckBox * m_strikeLED;

	ComboBox * m_presetsCombo;
	Knob * m_spreadKnob;
	Knob * m_randomKnob;
};


} // namespace gui

} // namespace lmms

#endif
