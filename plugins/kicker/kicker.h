/*
 * kicker.h - drum synthesizer
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


#ifndef KICKER_H
#define KICKER_H

#include <QObject>
#include "Instrument.h"
#include "InstrumentView.h"
#include "Knob.h"
#include "LedCheckbox.h"
#include "TempoSyncKnob.h"


#define KICKER_PRESET_VERSION 1


class kickerInstrumentView;
class NotePlayHandle;


class kickerInstrument : public Instrument
{
	Q_OBJECT
public:
	kickerInstrument( InstrumentTrack * _instrument_track );
	virtual ~kickerInstrument();

	virtual void playNote( NotePlayHandle * _n,
						sampleFrame * _working_buffer );
	virtual void deleteNotePluginData( NotePlayHandle * _n );

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	virtual QString nodeName() const;

	virtual Flags flags() const
	{
		return IsNotBendable;
	}

	virtual f_cnt_t desiredReleaseFrames() const
	{
		return( 512 );
	}

	virtual PluginView * instantiateView( QWidget * _parent );


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

	friend class kickerInstrumentView;

} ;



class kickerInstrumentView : public InstrumentViewFixedSize
{
	Q_OBJECT
public:
	kickerInstrumentView( Instrument * _instrument, QWidget * _parent );
	virtual ~kickerInstrumentView();

private:
	virtual void modelChanged();

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



#endif
