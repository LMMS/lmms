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

#include "instrument.h"
#include "instrument_view.h"
#include "knob.h"


class kickerInstrumentView;
class notePlayHandle;


class kickerInstrument : public instrument
{
public:
	kickerInstrument( instrumentTrack * _instrument_track );
	virtual ~kickerInstrument();

	virtual void FASTCALL playNote( notePlayHandle * _n,
						bool _try_parallelizing );
	virtual void FASTCALL deleteNotePluginData( notePlayHandle * _n );


	virtual void FASTCALL saveSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void FASTCALL loadSettings( const QDomElement & _this );

	virtual QString nodeName( void ) const;

	virtual f_cnt_t desiredReleaseFrames( void ) const
	{
		return( 512 );
	}

	virtual pluginView * instantiateView( QWidget * _parent );


private:
	knobModel m_startFreqModel;
	knobModel m_endFreqModel;
	knobModel m_decayModel;
	knobModel m_distModel;
	knobModel m_gainModel;

	friend class kickerInstrumentView;

} ;



class kickerInstrumentView : public instrumentView
{
public:
	kickerInstrumentView( instrument * _instrument, QWidget * _parent );
	virtual ~kickerInstrumentView();

private:
	virtual void modelChanged( void );

	knob * m_startFreqKnob;
	knob * m_endFreqKnob;
	knob * m_decayKnob;
	knob * m_distKnob;
	knob * m_gainKnob;

} ;



#endif
