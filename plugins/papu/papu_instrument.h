/*
 * papu_Instrument.h - GameBoy papu based instrument
 *
 * Copyright (c) 2008 <Attila Herman <attila589/at/gmail.com>
 *				Csaba Hruska <csaba.hruska/at/gmail.com>
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

#ifndef _PAPU_H
#define _PAPU_H

#include <QObject>
#include "Instrument.h"
#include "InstrumentView.h"
#include "Knob.h"
#include "Graph.h"

class papuInstrumentView;
class NotePlayHandle;
class PixmapButton;

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

	Knob * m_ch1SweepTimeKnob;
	PixmapButton * m_ch1SweepDirButton;
	Knob * m_ch1SweepRtShiftKnob;
	Knob * m_ch1WavePatternDutyKnob;
	Knob * m_ch1VolumeKnob;
	PixmapButton * m_ch1VolSweepDirButton;
	Knob * m_ch1SweepStepLengthKnob;

	Knob * m_ch2WavePatternDutyKnob;
	Knob * m_ch2VolumeKnob;
	PixmapButton * m_ch2VolSweepDirButton;
	Knob * m_ch2SweepStepLengthKnob;

	Knob * m_ch3VolumeKnob;

	Knob * m_ch4VolumeKnob;
	PixmapButton * m_ch4VolSweepDirButton;
	Knob * m_ch4SweepStepLengthKnob;
	PixmapButton * m_ch4ShiftRegWidthButton;

	Knob * m_so1VolumeKnob;
	Knob * m_so2VolumeKnob;
	PixmapButton * m_ch1So1Button;
	PixmapButton * m_ch2So1Button;
	PixmapButton * m_ch3So1Button;
	PixmapButton * m_ch4So1Button;
	PixmapButton * m_ch1So2Button;
	PixmapButton * m_ch2So2Button;
	PixmapButton * m_ch3So2Button;
	PixmapButton * m_ch4So2Button;
	Knob * m_trebleKnob;
	Knob * m_bassKnob;

	Graph * m_graph;

/*protected slots:
	void updateKnobHint();
	void updateKnobToolTip();*/
} ;


#endif
