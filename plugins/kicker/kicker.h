/*
 * kicker.h - bassdrum-synthesizer
 *
 * Copyright (c) 2006-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _KICKER_H
#define _KICKER_H

#include <QtCore/QObject>
#include "Instrument.h"
#include "InstrumentView.h"
#include "knob.h"


class kickerInstrumentView;
class NotePlayHandle;


class kickerInstrument : public Instrument
{
public:
	kickerInstrument( InstrumentTrack * _instrument_track );
	virtual ~kickerInstrument();

	virtual void playNote( NotePlayHandle * _n,
						sampleFrame * _working_buffer );
	virtual void deleteNotePluginData( NotePlayHandle * _n );


	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	virtual QString nodeName() const;

	virtual f_cnt_t desiredReleaseFrames() const
	{
		return( 512 );
	}

	virtual PluginView * instantiateView( QWidget * _parent );


private:
	FloatModel m_startFreqModel;
	FloatModel m_endFreqModel;
	FloatModel m_decayModel;
	FloatModel m_distModel;
	FloatModel m_gainModel;

	friend class kickerInstrumentView;

} ;



class kickerInstrumentView : public InstrumentView
{
	Q_OBJECT
public:
	kickerInstrumentView( Instrument * _instrument, QWidget * _parent );
	virtual ~kickerInstrumentView();

private:
	virtual void modelChanged();

	knob * m_startFreqKnob;
	knob * m_endFreqKnob;
	knob * m_decayKnob;
	knob * m_distKnob;
	knob * m_gainKnob;

} ;



#endif
