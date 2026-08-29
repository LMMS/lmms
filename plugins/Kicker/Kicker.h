/*
 * Kicker.h - drum synthesizer
 *
 * Copyright (c) 2006-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2014 grejppi <grejppi/at/gmail.com>
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

#ifndef LMMS_KICKER_H
#define LMMS_KICKER_H

#include "AutomatableModel.h"
#include "Instrument.h"
#include "InstrumentView.h"
#include "TempoSyncKnobModel.h"


namespace lmms
{

#define KICKER_PRESET_VERSION 1


class NotePlayHandle;  // IWYU pragma: keep

namespace gui
{
class Knob;
class LedCheckBox;
class KickerInstrumentView;
}


class KickerInstrument : public Instrument
{
	Q_OBJECT
public:
	KickerInstrument( InstrumentTrack * _instrument_track );
	~KickerInstrument() override = default;

	void playNote( NotePlayHandle * _n,
						SampleFrame* _working_buffer ) override;
	void deleteNotePluginData( NotePlayHandle * _n ) override;

	void saveSettings(QDomDocument& doc, QDomElement& elem) override;
	void loadSettings(const QDomElement& elem) override;

	QString nodeName() const override;

	float desiredReleaseTimeMs() const override
	{
		return 12.f;
	}

	gui::PluginView* instantiateView( QWidget * _parent ) override;


private:
	FloatModel m_startFreqModel;
	FloatModel m_endFreqModel;
	TempoSyncKnobModel m_decayModel;
	FloatModel m_distModel;
	FloatModel m_distEndModel;
	FloatModel m_gainModel;
	FloatModel m_envModel;
	FloatModel m_noiseModel;
	FloatModel m_clickModel;
	FloatModel m_slopeModel;

	BoolModel m_startNoteModel;
	BoolModel m_endNoteModel;

	IntModel m_versionModel;

	friend class gui::KickerInstrumentView;

} ;


namespace gui
{


class KickerInstrumentView : public InstrumentViewFixedSize
{
	Q_OBJECT
public:
	KickerInstrumentView( Instrument * _instrument, QWidget * _parent );
	~KickerInstrumentView() override = default;

private:
	void modelChanged() override;

	Knob * m_startFreqKnob;
	Knob * m_endFreqKnob;
	Knob * m_decayKnob;
	Knob * m_distKnob;
	Knob * m_distEndKnob;
	Knob * m_gainKnob;
	Knob * m_envKnob;
	Knob * m_noiseKnob;
	Knob * m_clickKnob;
	Knob * m_slopeKnob;

	LedCheckBox * m_startNoteToggle;
	LedCheckBox * m_endNoteToggle;

} ;


} // namespace gui

} // namespace lmms

#endif // LMMS_KICKER_H
