/*
 * SidInstrument.h - ResID based software-synthesizer
 *
 * Copyright (c) 2008 Csaba Hruska <csaba.hruska/at/gmail.com>
 *                    Attila Herman <attila589/at/gmail.com>
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


#ifndef _SID_H
#define _SID_H

#include "AutomatableModel.h"
#include "Instrument.h"
#include "InstrumentView.h"

namespace lmms
{


class NotePlayHandle;  // IWYU pragma: keep

namespace gui
{
class Knob;
class AutomatableButtonGroup;
class SidInstrumentView;
class PixmapButton;
}

class VoiceObject : public Model
{
	Q_OBJECT
public:
	enum class WaveForm {
		Square = 0,
		Triangle,
		Saw,
		Noise,
		Count
	};
	constexpr static auto NumWaveShapes = static_cast<std::size_t>(WaveForm::Count);

	VoiceObject( Model * _parent, int _idx );
	~VoiceObject() override = default;


private:
	FloatModel m_pulseWidthModel;
	FloatModel m_attackModel;
	FloatModel m_decayModel;
	FloatModel m_sustainModel;
	FloatModel m_releaseModel;
	FloatModel m_coarseModel;
	IntModel m_waveFormModel;
	BoolModel m_syncModel;
	BoolModel m_ringModModel;
	BoolModel m_filteredModel;
	BoolModel m_testModel;

	friend class SidInstrument;
	friend class gui::SidInstrumentView;
} ;

class SidInstrument : public Instrument
{
	Q_OBJECT
public:
	enum class FilterType {
		HighPass = 0,
		BandPass,
		LowPass,
		Count
	};
	constexpr static auto NumFilterTypes = static_cast<std::size_t>(FilterType::Count);
	
	enum class ChipModel {
		MOS6581 = 0,
		MOS8580,
		Count
	};
	constexpr static auto NumChipModels = static_cast<std::size_t>(ChipModel::Count);

	SidInstrument( InstrumentTrack * _instrument_track );
	~SidInstrument() override = default;

	void playNote( NotePlayHandle * _n,
						SampleFrame* _working_buffer ) override;
	void deleteNotePluginData( NotePlayHandle * _n ) override;


	void saveSettings( QDomDocument & _doc, QDomElement & _parent ) override;
	void loadSettings( const QDomElement & _this ) override;

	QString nodeName() const override;

	float desiredReleaseTimeMs() const override;

	gui::PluginView* instantiateView( QWidget * _parent ) override;


/*public slots:
	void updateKnobHint();
	void updateKnobToolTip();*/

private:
	// voices
	VoiceObject * m_voice[3];

	// filter	
	FloatModel m_filterFCModel;
	FloatModel m_filterResonanceModel;
	IntModel m_filterModeModel;
	
	// misc
	BoolModel m_voice3OffModel;
	FloatModel m_volumeModel;

	IntModel m_chipModel;

	friend class gui::SidInstrumentView;

} ;


namespace gui
{


class SidInstrumentView : public InstrumentViewFixedSize
{
	Q_OBJECT
public:
	SidInstrumentView( Instrument * _instrument, QWidget * _parent );
	~SidInstrumentView() override = default;

private:
	void modelChanged() override;
	
	AutomatableButtonGroup * m_passBtnGrp;
	AutomatableButtonGroup * m_sidTypeBtnGrp;

	struct voiceKnobs
	{
		voiceKnobs( Knob * a,
					Knob * d,
					Knob * s,
					Knob * r,
					Knob * pw,
					Knob * crs,
					AutomatableButtonGroup * wfbg,
					PixmapButton * syncb,
					PixmapButton * ringb,
					PixmapButton * filterb,
					PixmapButton * testb ) :
			m_attKnob( a ),
			m_decKnob( d ),
			m_sustKnob( s ),
			m_relKnob( r ),
			m_pwKnob( pw ),
			m_crsKnob( crs ),
			m_waveFormBtnGrp( wfbg ),
			m_syncButton( syncb ),
			m_ringModButton( ringb ),
			m_filterButton( filterb ),
			m_testButton( testb )
		{
		}
		voiceKnobs() = default;
		Knob * m_attKnob;
		Knob * m_decKnob;
		Knob * m_sustKnob;
		Knob * m_relKnob;
		Knob * m_pwKnob;
		Knob * m_crsKnob;
		AutomatableButtonGroup * m_waveFormBtnGrp;
		PixmapButton * m_syncButton;
		PixmapButton * m_ringModButton;
		PixmapButton * m_filterButton;
		PixmapButton * m_testButton;
	} ;

	voiceKnobs m_voiceKnobs[3];

	Knob * m_volKnob;
	Knob * m_resKnob;
	Knob * m_cutKnob;
	PixmapButton * m_offButton;

protected slots:
	void updateKnobHint();
	void updateKnobToolTip();
} ;


} // namespace gui

} // namespace lmms

#endif
