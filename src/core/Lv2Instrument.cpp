/*
 * Lv2Instrument.cpp - implementation of Lv2Instrument class
 *
 * Copyright (c) 2018 Alexandros Theodotou @faiyadesu
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

#include "Lv2Instrument.h"

extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT lv2_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"Organic",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"Additive Synthesizer for organ-like sounds" ),
	"Andreas Brandmaier <andreas/at/brandmaier.de>",
	0x0100,
	Plugin::Instrument,
	NULL,
	NULL,
	NULL
} ;
}

Lv2Instrument::Lv2Instrument(InstrumentTrack * _instrument_track) :
	Instrument( _instrument_track, &lv2_plugin_descriptor )
{

}

Lv2Instrument::~Lv2Instrument()
{

}

Lv2InstrumentView::Lv2InstrumentView(Instrument * _instrument,
		QWidget * _parent) :
	InstrumentView( _instrument, _parent )
{
}

Lv2InstrumentView::~Lv2InstrumentView()
{

}
