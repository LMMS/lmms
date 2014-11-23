/*
 * papu_Instrument.h - GameBoy papu based instrument
 *
 * Copyright (c) 2008 <Attila Herman <attila589/at/gmail.com>
 *				Csaba Hruska <csaba.hruska/at/gmail.com>
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

#ifndef _PAPU_H
#define _PAPU_H

#include <QtCore/QObject>
#include "Instrument.h"
#include "InstrumentView.h"
#include "knob.h"
#include "graph.h"

class papuInstrumentView;
class NotePlayHandle;
class pixmapButton;

class papuInstrument : public Instrument
{
	Q_OBJECT
public:

	papuInstrument( InstrumentTrack * _instrument_track );
	virtual ~papuInstrument();

	virtual void playNote( NotePlayHandle * _n,
						sampleFrame * _working_buffer );
	virtual void deleteNotePluginData( NotePlayHandle * _n );


	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	virtual QString nodeName() const;

	virtual f_cnt_t desiredReleaseFrames() const;

	virtual PluginView * instantiateView( QWidget * _parent );


/*public slots:
	void updateKnobHint();
	void updateKnobToolTip();*/

private:
	FloatModel m_ch1SweepTimeModel;
	BoolModel m_ch1SweepDirModel;
	FloatModel m_ch1SweepRtShiftModel;
	FloatModel m_ch1WavePatternDutyModel;
	FloatModel m_ch1VolumeModel;
	BoolModel m_ch1VolSweepDirModel;
	FloatModel m_ch1SweepStepLengthModel;

	FloatModel m_ch2WavePatternDutyModel;
	FloatModel m_ch2VolumeModel;
	BoolModel m_ch2VolSweepDirModel;
	FloatModel m_ch2SweepStepLengthModel;

	BoolModel m_ch3OnModel;
	FloatModel m_ch3VolumeModel;

	FloatModel m_ch4VolumeModel;
	BoolModel m_ch4VolSweepDirModel;
	FloatModel m_ch4SweepStepLengthModel;
	FloatModel m_ch4ShiftClockFreqModel;
	BoolModel m_ch4ShiftRegWidthModel;
	FloatModel m_ch4FreqDivRatioModel;

	FloatModel m_so1VolumeModel;
	FloatModel m_so2VolumeModel;
	BoolModel m_ch1So1Model;
	BoolModel m_ch2So1Model;
	BoolModel m_ch3So1Model;
	BoolModel m_ch4So1Model;
	BoolModel m_ch1So2Model;
	BoolModel m_ch2So2Model;
	BoolModel m_ch3So2Model;
	BoolModel m_ch4So2Model;
	FloatModel m_trebleModel;
	FloatModel m_bassModel;

	graphModel  m_graphModel;

	friend class papuInstrumentView;
} ;


class papuInstrumentView : public InstrumentView
{
	Q_OBJECT
public:
	papuInstrumentView( Instrument * _instrument, QWidget * _parent );
	virtual ~papuInstrumentView();

private:
	virtual void modelChanged();

	knob * m_ch1SweepTimeKnob;
	pixmapButton * m_ch1SweepDirButton;
	knob * m_ch1SweepRtShiftKnob;
	knob * m_ch1WavePatternDutyKnob;
	knob * m_ch1VolumeKnob;
	pixmapButton * m_ch1VolSweepDirButton;
	knob * m_ch1SweepStepLengthKnob;

	knob * m_ch2WavePatternDutyKnob;
	knob * m_ch2VolumeKnob;
	pixmapButton * m_ch2VolSweepDirButton;
	knob * m_ch2SweepStepLengthKnob;

	knob * m_ch3VolumeKnob;

	knob * m_ch4VolumeKnob;
	pixmapButton * m_ch4VolSweepDirButton;
	knob * m_ch4SweepStepLengthKnob;
	pixmapButton * m_ch4ShiftRegWidthButton;

	knob * m_so1VolumeKnob;
	knob * m_so2VolumeKnob;
	pixmapButton * m_ch1So1Button;
	pixmapButton * m_ch2So1Button;
	pixmapButton * m_ch3So1Button;
	pixmapButton * m_ch4So1Button;
	pixmapButton * m_ch1So2Button;
	pixmapButton * m_ch2So2Button;
	pixmapButton * m_ch3So2Button;
	pixmapButton * m_ch4So2Button;
	knob * m_trebleKnob;
	knob * m_bassKnob;

	graph * m_graph;

/*protected slots:
	void updateKnobHint();
	void updateKnobToolTip();*/
} ;


#endif
