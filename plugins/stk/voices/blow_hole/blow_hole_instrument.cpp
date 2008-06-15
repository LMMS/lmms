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

#include "blow_hole_instrument.h"
#include "blow_hole_instrument_view.h"
#include "blow_hole_processor.h"

#undef SINGLE_SOURCE_COMPILE
#include "embedded_resources.h"


extern "C"
{

plugin::descriptor blowhole_plugin_descriptor =
{
	STRINGIFY_PLUGIN_NAME( PLUGIN_NAME ),
	"Blow Hole",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"Blown tube noises" ),
	"Danny McRae <khjklujn/at/users.sf.net>",
	0x0100,
	plugin::Instrument,
	new pluginPixmapLoader( "logo" ),
	NULL
} ;

}


blowHoleInstrument::blowHoleInstrument( instrumentTrack * _channel_track ):
	stkInstrument<blowHoleProcessor, blowHoleModel>( _channel_track, &blowhole_plugin_descriptor )
{
	model()->reedStiffness()->setTrack( _channel_track );
	model()->noiseGain()->setTrack( _channel_track );
	model()->toneholeState()->setTrack( _channel_track );
	model()->registerState()->setTrack( _channel_track );
	model()->breathPressure()->setTrack( _channel_track );
}




blowHoleInstrument::~blowHoleInstrument()
{
}




QString blowHoleInstrument::nodeName( void ) const
{
	return( blowhole_plugin_descriptor.name );
}




pluginView * blowHoleInstrument::instantiateView( QWidget * _parent )
{
	return( new blowHoleInstrumentView( this, _parent ) );
}




extern "C"
{

// neccessary for getting instance out of shared lib
plugin * lmms_plugin_main( model * _model, void * _data )
{
	return( new blowHoleInstrument( static_cast<instrumentTrack *>( _data ) ) );
}


}


