/*
 * plucked_string_synth.cpp - instrument which uses the Karplus-Strong-algorithm
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#include "qt3support.h"

#ifdef QT4

#include <Qt/QtXml>
#include <QMap>

#else

#include <qdom.h>
#include <qmap.h>

#endif


#include "plucked_string_synth.h"
#include "channel_track.h"
#include "note_play_handle.h"
#include "templates.h"
#include "buffer_allocator.h"
#include "knob.h"

#include "embed.cpp"


extern "C"
{

plugin::descriptor pluckedstringsynth_plugin_descriptor =
{
	STRINGIFY_PLUGIN_NAME( PLUGIN_NAME ),
	"PluckedStringSynth",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"cheap synthesis of guitar/harp-like sounds" ),
	"Tobias Doerffel <tobydox/at/users.sf.net>",
	0x0100,
	plugin::INSTRUMENT,
	PLUGIN_NAME::findEmbeddedData( "logo.png" )
} ;

}


// TODO: make this synth stereo for better better spacial (room) feeling and
// add distortion

pluckedStringSynth::pluckedStringSynth( channelTrack * _channel_track ) :
	instrument( _channel_track, &pluckedstringsynth_plugin_descriptor )
{
	m_pickKnob = new knob( knobDark_28, this, tr( "Pick position" ) );
	m_pickKnob->setRange( 0.0f, 0.5f, 0.005f );
 	m_pickKnob->setValue( 0.0f, TRUE );
	m_pickKnob->move( 86, 134 );
	m_pickKnob->setHintText( tr( "Pick position:" ) + " ", "" );

	m_pickupKnob = new knob( knobDark_28, this, tr( "Pickup position" ) );
	m_pickupKnob->setRange( 0.0f, 0.5f, 0.005f );
	m_pickupKnob->setValue( 0.05f, TRUE );
	m_pickupKnob->move( 138, 134 );
	m_pickupKnob->setHintText( tr( "Pickup position:" ) + " ", "" );
#ifdef QT4
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap(
								"artwork" ) );
	setPalette( pal );
#else
	setErasePixmap( PLUGIN_NAME::getIconPixmap( "artwork" ) );
#endif
}




pluckedStringSynth::~pluckedStringSynth()
{
}




void pluckedStringSynth::saveSettings( QDomDocument & _doc,
							QDomElement & _parent )
{
	QDomElement pss_de = _doc.createElement( nodeName() );
	pss_de.setAttribute( "pick", QString::number( m_pickKnob->value() ) );
	pss_de.setAttribute( "pickup", QString::number(
						m_pickupKnob->value() ) );
	_parent.appendChild( pss_de );
}




void pluckedStringSynth::loadSettings( const QDomElement & _this )
{
	m_pickKnob->setValue( _this.attribute( "pick" ).toFloat() );
	m_pickupKnob->setValue( _this.attribute( "pickup" ).toFloat() );
}




QString pluckedStringSynth::nodeName( void ) const
{
	return( pluckedstringsynth_plugin_descriptor.name );
}




void pluckedStringSynth::playNote( notePlayHandle * _n )
{
	if ( _n->totalFramesPlayed() == 0 )
	{
		float freq = getChannelTrack()->frequency( _n );
		_n->m_pluginData = new pluckSynth( freq, m_pickKnob->value(),
							m_pickupKnob->value() );
	}

	const Uint32 frames = mixer::inst()->framesPerAudioBuffer();
	sampleFrame * buf = bufferAllocator::alloc<sampleFrame>( frames );

	pluckSynth * ps = static_cast<pluckSynth *>( _n->m_pluginData );
	for( Uint32 frame = 0; frame < frames; ++frame )
	{
		const sampleType cur = ps->nextStringSample();
		for( Uint8 chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
		{
			buf[frame][chnl] = cur;
		}
	}

	getChannelTrack()->processAudioBuffer( buf, frames, _n );

	bufferAllocator::free( buf );
}




void pluckedStringSynth::deleteNotePluginData( notePlayHandle * _n )
{
	delete static_cast<pluckSynth *>( _n->m_pluginData );
}





pluckSynth::delayLine * FASTCALL pluckSynth::initDelayLine( int _len )
{
	delayLine * dl = new pluckSynth::delayLine[_len];
	dl->length = _len;
	if( _len > 0 )
	{
		dl->data = new sampleType[_len];
	}
	else
	{
		dl->data = NULL;
	}

	dl->pointer = dl->data;
	dl->end = dl->data + _len - 1;

	return( dl );
}




void FASTCALL pluckSynth::freeDelayLine( delayLine * _dl )
{
	if( _dl && _dl->data )
	{
		delete[] _dl->data;
	}

	_dl->data = NULL;
	delete[] _dl;
}




pluckSynth::pluckSynth( float _pitch, float _pick, float _pickup )
{
	const float AMP = 1.5f;
	int rail_length = static_cast<int>( mixer::inst()->sampleRate() / 2 /
								_pitch ) + 1;
	// Round pick position to nearest spatial sample.
	// A pick position at x = 0 is not allowed.
	int pick_sample = static_cast<int>( tMax<float>( rail_length * _pick,
								1.0f ) );
	float initial_shape[rail_length];

	m_upperRail = pluckSynth::initDelayLine( rail_length );
	m_lowerRail = pluckSynth::initDelayLine( rail_length );

//#define METALLIC_PLUCK
#ifdef METALLIC_PLUCK
	for ( int i = 0; i < rail_length; i++ )
	{
		initial_shape[i] = rand() * AMP / RAND_MAX;
	}

	initial_shape[pick_sample] = 0.5;
	initial_shape[pick_sample+1] = 0.5;

#else
	float upslope = AMP / pick_sample;
	const float downslope = AMP / ( rail_length - pick_sample - 1 );

	for( int i = 0; i < pick_sample; i++ )
	{
		initial_shape[i] = upslope * i;
	}

	for( int i = pick_sample; i < rail_length; i++ )
	{
		initial_shape[i] = downslope * ( rail_length - 1 - i );
	}
#endif

	// Initial conditions for the ideal plucked string.
	// "Past history" is measured backward from the end of the array.
	pluckSynth::setDelayLine( m_lowerRail, initial_shape, 0.5f );
	pluckSynth::setDelayLine( m_upperRail, initial_shape, 0.5f );

	m_pickupLoc = static_cast<int>( _pickup * rail_length );
}




extern "C"
{

// neccessary for getting instance out of shared lib
plugin * lmms_plugin_main( void * _data )
{
	return( new pluckedStringSynth(
				static_cast<channelTrack *>( _data ) ) );
}


}


