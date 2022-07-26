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

#include "ModalBar.h"

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

	inline void setFrequency( const StkFloat _pitch )
	{
		if( m_voice )
		{
			m_voice->setFrequency( _pitch );
		}
	}

	inline void setFixed(const int modal, const StkFloat ratio, const StkFloat radius, const StkFloat gain)
	{
		if( m_voice )
		{
			m_voice->setRatioAndRadius(modal, ratio, radius);
			m_voice->setModeGain(modal, gain);
		}
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
	ModalBar * m_voice;

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

	void playNote( NotePlayHandle * _n,
						sampleFrame * _working_buffer ) override;
	void deleteNotePluginData( NotePlayHandle * _n ) override;


	void saveSettings( QDomDocument & _doc, QDomElement & _parent ) override;
	void loadSettings( const QDomElement & _this ) override;

	QString nodeName() const override;

	gui::PluginView* instantiateView( QWidget * _parent ) override;


public slots:
	//void changeFreqModel();
	//void changePreset();


private:
	BoolModel m_f0fixedModel, m_f1fixedModel, m_f2fixedModel, m_f3fixedModel;
	FloatModel m_freq0Model, m_freq1Model, m_freq2Model, m_freq3Model;
	FloatModel m_fixedFreq0Model, m_fixedFreq1Model, m_fixedFreq2Model, m_fixedFreq3Model;
	FloatModel m_res0Model, m_res1Model, m_res2Model, m_res3Model;
	FloatModel m_vol0Model, m_vol1Model, m_vol2Model, m_vol3Model;
	FloatModel m_hardnessModel;
	FloatModel m_positionModel;
	FloatModel m_vibratoGainModel;
	FloatModel m_vibratoFreqModel;
	FloatModel m_stickModel;

	ComboBoxModel m_presetsModel;
	FloatModel m_spreadModel;
	FloatModel m_randomModel;
	IntModel m_versionModel;

	static constexpr StkFloat m_presets[9][4][4] = {
		{{1.0, 3.99, 10.65, -2443},		// Marimba
		{0.9996, 0.9994, 0.9994, 0.999},
		{0.04, 0.01, 0.01, 0.008},
		{0.429688, 0.445312, 0.093750}},
		{{1.0, 2.01, 3.9, 14.37}, 		// Vibraphone
		{0.99995, 0.99991, 0.99992, 0.9999},
		{0.025, 0.015, 0.015, 0.015 },
		{0.390625,0.570312,0.078125}},
		{{1.0, 4.08, 6.669, -3725.0},		// Agogo
		{0.999, 0.999, 0.999, 0.999},
		{0.06, 0.05, 0.03, 0.02},
		{0.609375,0.359375,0.140625}},
		{{1.0, 2.777, 7.378, 15.377},		// Wood1
		{0.996, 0.994, 0.994, 0.99},
		{0.04, 0.01, 0.01, 0.008},
		{0.460938,0.375000,0.046875}},
		{{1.0, 2.777, 7.378, 15.377},		// Reso
		{0.99996, 0.99994, 0.99994, 0.9999},
		{0.02, 0.005, 0.005, 0.004},
		{0.453125,0.250000,0.101562}},
		{{1.0, 1.777, 2.378, 3.377},		// Wood2
		{0.996, 0.994, 0.994, 0.99},
		{0.04, 0.01, 0.01, 0.008},
		{0.312500,0.445312,0.109375}},
		{{1.0, 1.004, 1.013, 2.377},		// Beats
		{0.9999, 0.9999, 0.9999, 0.999},
		{0.02, 0.005, 0.005, 0.004},
		{0.398438,0.296875,0.070312}},
		{{1.0, 4.0, -1320.0, -3960.0},		// 2Fix
		{0.9996, 0.999, 0.9994, 0.999},
		{0.04, 0.01, 0.01, 0.008},
		{0.453125,0.453125,0.070312}},
		{{1.0, 1.217, 1.475, 1.729},		// Clump
		{0.999, 0.999, 0.999, 0.999},
		{0.03, 0.03, 0.03, 0.03 },
		{0.390625,0.570312,0.078125}}
	};

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
	void changeFreqModel();

private:
	void modelChanged() override;

	void setWidgetBackground( QWidget * _widget, const QString & _pic );
	QWidget * setupModalBarControls( QWidget * _parent );

	QWidget * m_modalBarWidget;
	LedCheckBox * m_f0fixedLED; LedCheckBox * m_f1fixedLED; LedCheckBox * m_f2fixedLED; LedCheckBox * m_f3fixedLED;
	Knob * m_freq0Knob; Knob * m_freq1Knob; Knob * m_freq2Knob; Knob * m_freq3Knob;
	Knob * m_res0Knob; Knob * m_res1Knob; Knob * m_res2Knob; Knob * m_res3Knob;
	Knob * m_vol0Knob; Knob * m_vol1Knob; Knob * m_vol2Knob; Knob * m_vol3Knob;
	Knob * m_hardnessKnob;
	Knob * m_positionKnob;
	Knob * m_vibratoGainKnob;
	Knob * m_vibratoFreqKnob;
	Knob * m_stickKnob;

	ComboBox * m_presetsCombo;
	Knob * m_spreadKnob;
	Knob * m_randomKnob;
};


} // namespace gui

} // namespace lmms

#endif
