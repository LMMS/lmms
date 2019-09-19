/*
 * Instrument.cpp - base-class for all instrument-plugins (synths, samplers etc)
 *
 * Copyright (c) 2005-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <cmath>

#include "Instrument.h"
#include "InstrumentTrack.h"
#include "DummyInstrument.h"


Instrument::Instrument(InstrumentTrack * _instrument_track,
			const Descriptor * _descriptor,
			const Descriptor::SubPluginFeatures::Key *key) :
	Plugin(_descriptor, NULL/* _instrument_track*/, key),
	m_instrumentTrack( _instrument_track )
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




Instrument *Instrument::instantiate(const QString &_plugin_name,
	InstrumentTrack *_instrument_track, const Descriptor::SubPluginFeatures::Key *key, bool keyFromDnd)
{
	if(keyFromDnd)
		Q_ASSERT(!key);
	// copy from above // TODO! common cleaner func
	Plugin * p = Plugin::instantiateWithKey(_plugin_name, _instrument_track, key, keyFromDnd);
	if(dynamic_cast<Instrument *>(p))
		return dynamic_cast<Instrument *>(p);
	delete p;
	return( new DummyInstrument( _instrument_track ) );
}




bool Instrument::isFromTrack( const Track * _track ) const
{
	return( m_instrumentTrack == _track );
}


void Instrument::applyFadeIn(  sampleFrame * buf, const NotePlayHandle * _n )
{
    // apply only if it's the start of the note
    if( _n->totalFramesPlayed() == 0 ){
        const fpp_t frames = _n->framesLeftForCurrentPeriod();

        // zero point crossing counts of all channels
        int zero_crossings[DEFAULT_CHANNELS] = {0};
        // maximum zero point crossing of all channels
        int max_zc = 0;
        
        // determine the zero point crossing counts
        for( fpp_t f = 0; f < frames; ++f )
        {
            for( ch_cnt_t ch=0; ch < DEFAULT_CHANNELS; ++ch )
            {
                if( ( buf[f-1][ch] < 0.0 && buf[f][ch] > 0.0 ) ||
                    ( buf[f-1][ch] > 0.0 && buf[f][ch] < 0.0 ) )
                {
                    ++zero_crossings[ch];
                }
            }
        }

        // find the biggest one
        for( ch_cnt_t ch = 0; ch < DEFAULT_CHANNELS; ++ch )
        {
            if( zero_crossings[ch] > max_zc )
            {
                max_zc = zero_crossings[ch];
            }
        }
        
        // calculate the length of the fade in
        fpp_t length = (fpp_t) (
                ( (float)frames - 1 )  /
                ( (float)max_zc / 2.0f + 1.0f ) / 3.0f );

        for( fpp_t f = 0; f < length; ++f ){
            for( ch_cnt_t ch = 0; ch < DEFAULT_CHANNELS; ++ch ){
                // define macro for PI?
                buf[f][ch] *= 0.5 - 0.5 * cosf(3.1415926536f * (float) f / (float) length);
            }
        }
    }
}

void Instrument::applyRelease( sampleFrame * buf, const NotePlayHandle * _n )
{
	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	const fpp_t fpp = Engine::mixer()->framesPerPeriod();
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



