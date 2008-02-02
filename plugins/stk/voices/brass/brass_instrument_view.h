/*
 *
 * Copyright (c) 2008 Danny McRae <khjklujn/at/users.sourceforge.net>
 * 
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


#ifndef _BRASS_INSTRUMENT_VIEW_H
#define _BRASS_INSTRUMENT_VIEW_H

#include "knob.h"

#include "stk_instrument_view.h"
#include "brass_instrument.h"


class brassInstrumentView: public stkInstrumentView<brassInstrument>
{
public:
	brassInstrumentView( brassInstrument * _instrument, QWidget * _parent );
	virtual ~brassInstrumentView( void );
	
private:
	virtual void modelChanged( void );
	
	knob * m_lipTension;
	knob * m_slideLength;
	knob * m_vibratoFrequency;
	knob * m_vibratoGain;
};

#endif
