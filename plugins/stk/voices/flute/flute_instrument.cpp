/*
 *
 * Copyright (c) 2008 Danny McRae <khjklujn/at/users.sourceforge.net>
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


#include "engine.h"
#include "note_play_handle.h"

#include "flute_instrument.h"
#include "flute_instrument_view.h"
#include "flute_processor.h"

#undef SINGLE_SOURCE_COMPILE
#include "embedded_resources.h"


extern "C"
{

plugin::descriptor flute_plugin_descriptor =
{
	STRINGIFY_PLUGIN_NAME( PLUGIN_NAME ),
	"Flute",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"Flute noises" ),
	"Danny McRae <khjklujn/at/users.sf.net>",
	0x0100,
	plugin::Instrument,
	new QPixmap( PLUGIN_NAME::getIconPixmap( "logo" ) ),
	NULL
} ;

}


fluteInstrument::fluteInstrument( instrumentTrack * _channel_track ):
	stkInstrument<fluteProcessor, fluteModel>( _channel_track, &flute_plugin_descriptor )
{
	model()->jetDelay()->setTrack( _channel_track );
	model()->noiseGain()->setTrack( _channel_track );
	model()->vibratoFrequency()->setTrack( _channel_track );
	model()->vibratoGain()->setTrack( _channel_track );
	model()->breathPressure()->setTrack( _channel_track );
}




fluteInstrument::~fluteInstrument()
{
}




QString fluteInstrument::nodeName( void ) const
{
	return( flute_plugin_descriptor.name );
}




pluginView * fluteInstrument::instantiateView( QWidget * _parent )
{
	return( new fluteInstrumentView( this, _parent ) );
}




extern "C"
{

// neccessary for getting instance out of shared lib
plugin * lmms_plugin_main( model * _model, void * _data )
{
	return( new fluteInstrument( static_cast<instrumentTrack *>( _data ) ) );
}


}


