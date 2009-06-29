/*
 * papu_instrument.h - GameBoy papu based instrument
 *
 * Copyright (c) 2008 <Attila Herman <attila589/at/gmail.com>
 *				Csaba Hruska <csaba.hruska/at/gmail.com>
 *
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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
#include "instrument.h"
#include "InstrumentView.h"
#include "knob.h"
#include "graph.h"

class papuInstrumentView;
class notePlayHandle;
class pixmapButton;

class papuInstrument : public instrument
{
	Q_OBJECT
public:

	papuInstrument( instrumentTrack * _instrument_track );
	virtual ~papuInstrument();

	virtual void playNote( notePlayHandle * _n,
						sampleFrame * _working_buffer );
	virtual void deleteNotePluginData( notePlayHandle * _n );


	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	virtual QString nodeName( void ) const;

	virtual f_cnt_t desiredReleaseFrames( void ) const;

	virtual pluginView * instantiateView( QWidget * _parent );


/*public slots:
	void updateKnobHint( void );
	void updateKnobToolTip( void );*/

private:
	knobModel m_ch1SweepTimeModel;
	boolModel m_ch1SweepDirModel;
	knobModel m_ch1SweepRtShiftModel;
	knobModel m_ch1WavePatternDutyModel;
	knobModel m_ch1VolumeModel;
	boolModel m_ch1VolSweepDirModel;
	knobModel m_ch1SweepStepLengthModel;

	knobModel m_ch2WavePatternDutyModel;
	knobModel m_ch2VolumeModel;
	boolModel m_ch2VolSweepDirModel;
	knobModel m_ch2SweepStepLengthModel;

	boolModel m_ch3OnModel;
	knobModel m_ch3VolumeModel;

	knobModel m_ch4VolumeModel;
	boolModel m_ch4VolSweepDirModel;
	knobModel m_ch4SweepStepLengthModel;
	knobModel m_ch4ShiftClockFreqModel;
	boolModel m_ch4ShiftRegWidthModel;
	knobModel m_ch4FreqDivRatioModel;

	knobModel m_so1VolumeModel;
	knobModel m_so2VolumeModel;
	boolModel m_ch1So1Model;
	boolModel m_ch2So1Model;
	boolModel m_ch3So1Model;
	boolModel m_ch4So1Model;
	boolModel m_ch1So2Model;
	boolModel m_ch2So2Model;
	boolModel m_ch3So2Model;
	boolModel m_ch4So2Model;
	knobModel m_trebleModel;
	knobModel m_bassModel;

	graphModel  m_graphModel;

	friend class papuInstrumentView;
} ;


class papuInstrumentView : public InstrumentView
{
	Q_OBJECT
public:
	papuInstrumentView( instrument * _instrument, QWidget * _parent );
	virtual ~papuInstrumentView();

private:
	virtual void modelChanged( void );

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

	pixmapButton * m_ch3OnButton;
	knob * m_ch3VolumeKnob;

	knob * m_ch4VolumeKnob;
	pixmapButton * m_ch4VolSweepDirButton;
	knob * m_ch4SweepStepLengthKnob;
	knob * m_ch4ShiftClockFreqKnob;
	pixmapButton * m_ch4ShiftRegWidthButton;
	knob * m_ch4FreqDivRatioKnob;

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
	void updateKnobHint( void );
	void updateKnobToolTip( void );*/
} ;


#endif
