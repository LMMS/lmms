/*
 * Groove.h - classes for addinng swing/funk/groove/slide (you can't name it but you can feel it)
 *  to midi which is not precise enough at 192 ticks per tact to make your arse move.
 *
 * In it simplest terms a groove is a subtle delay on some notes in a pattern.
 *
 * Copyright (c) 2005-2008 teknopaul <teknopaul/at/users.sourceforge.net>
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
 **/
#ifndef GROOVE_H
#define GROOVE_H

#include <QtGui/QWidget>

#include "lmms_basics.h"
#include "MidiTime.h"
#include "Note.h"
#include "Pattern.h"
#include "SerializingObject.h"

class Groove : public SerializingObject
{

public:
	Groove();

	/**
	 * Groove should return true if the note should be played in the curr_time tick,
	 * at the start of the tick or any time before the next tick.
	 *
	 * cur_start - the tick about to be played
	 * n - the note to be played, or not played
	 * p - the pattern to which the note belongs
	 *
	 * default implementation (no groove) would be   return n.pos() == cur_start ? 0 : -1;
	 *
	 * returns 0 to play now on the tick, -1 to not play at all and the new offset
	 *         that the note should be shifted if it is to be played later in this tick.
	 */
	virtual int isInTick(MidiTime * _cur_start, fpp_t _frames, f_cnt_t _offset,
						 Note * _n, Pattern * _p ) ;

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _element );
	virtual void loadSettings( const QDomElement & _this );

	virtual QWidget * instantiateView( QWidget * _parent );

	virtual QString nodeName() const
	{
		return "none";
	}

};

#endif // GROOVE_H
