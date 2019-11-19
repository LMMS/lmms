/*
 * InstrumentView.h - definition of InstrumentView-class
 *
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef INSTRUMENT_VIEW_H
#define INSTRUMENT_VIEW_H

#include "Instrument.h"
#include "PluginView.h"

class InstrumentTrackWindow;


//! Instrument view with variable size
class LMMS_EXPORT InstrumentView : public PluginView
{
public:
	InstrumentView( Instrument * _instrument, QWidget * _parent );
	~InstrumentView() override;

	Instrument * model()
	{
		return( castModel<Instrument>() );
	}

	const Instrument * model() const
	{
		return( castModel<Instrument>() );
	}

	void setModel( Model * _model, bool = false ) override;

	InstrumentTrackWindow * instrumentTrackWindow();

} ;




//! Instrument view with fixed LMMS-default size
class LMMS_EXPORT InstrumentViewFixedSize : public InstrumentView
{
	QSize sizeHint() const override { return QSize(250, 250); }
	QSize minimumSizeHint() const override { return sizeHint(); }

public:
	using InstrumentView::InstrumentView;
	~InstrumentViewFixedSize() override;
} ;


#endif
