/*
 * Instrument.cpp - base-class for all instrument-plugins (synths, samplers etc)
 *
 * Copyright (c) 2005-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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

#include "Instrument.h"
#include "InstrumentTrack.h"
#include "DummyInstrument.h"
#include "NotePlayHandle.h"
#include "embed.h"
#include "engine.h"


Instrument::Instrument( InstrumentTrack * _instrument_track,
					const Descriptor * _descriptor ) :
	Plugin( _descriptor, NULL/* _instrument_track*/ ),
	m_instrumentTrack( _instrument_track )
{
}




Instrument::~Instrument()
{
}




void Instrument::play( sampleFrame * )
{
}




void Instrument::deleteNotePluginData( NotePlayHandle * )
{
}




f_cnt_t Instrument::beatLen( NotePlayHandle * ) const
{
	return( 0 );
}




Instrument * Instrument::instantiate( const QString & _plugin_name,
					InstrumentTrack * _instrument_track )
{
	Plugin * p = Plugin::instantiate( _plugin_name, _instrument_track,
							_instrument_track );
	// check whether instantiated plugin is an instrument
	if( dynamic_cast<Instrument *>( p ) != NULL )
	{
		// everything ok, so return pointer
		return dynamic_cast<Instrument *>( p );
	}

	// not quite... so delete plugin and return dummy instrument
	delete p;
	return( new DummyInstrument( _instrument_track ) );
}




bool Instrument::isFromTrack( const track * _track ) const
{
	return( m_instrumentTrack == _track );
}




void Instrument::applyRelease( sampleFrame * buf, const NotePlayHandle * _n )
{
	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	const fpp_t fpp = engine::mixer()->framesPerPeriod();
	const f_cnt_t fl = _n->framesLeft();
	if( fl <= desiredReleaseFrames()+fpp )
	{
		for( fpp_t f = (fpp_t)( ( fl > desiredReleaseFrames() ) ?
				( qMax( fpp - desiredReleaseFrames(), 0 ) +
					fl % fpp ) : 0 ); f < frames; ++f )
		{
			const float fac = (float)( fl-f-1 ) /
							desiredReleaseFrames();
			for( ch_cnt_t ch = 0; ch < DEFAULT_CHANNELS; ++ch )
			{
				buf[f][ch] *= fac;
			}
		}
	}
}




QString Instrument::fullDisplayName() const
{
	return instrumentTrack()->displayName();
}



