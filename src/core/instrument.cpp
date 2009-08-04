#ifndef SINGLE_SOURCE_COMPILE

/*
 * instrument.cpp - base-class for all instrument-plugins (synths, samplers etc)
 *
 * Copyright (c) 2005-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "instrument.h"
#include "instrument_view.h"
#include "instrument_track.h"
#include "dummy_instrument.h"
#include "note_play_handle.h"
#include "embed.h"


instrument::instrument( instrumentTrack * _instrument_track,
					const descriptor * _descriptor ) :
	plugin( _descriptor, NULL/* _instrument_track*/ ),
	m_instrumentTrack( _instrument_track )
{
}




instrument::~instrument()
{
}




void instrument::play( sampleFrame * )
{
}




void instrument::deleteNotePluginData( notePlayHandle * )
{
}




f_cnt_t instrument::beatLen( notePlayHandle * ) const
{
	return( 0 );
}




instrument * instrument::instantiate( const QString & _plugin_name,
					instrumentTrack * _instrument_track )
{
	plugin * p = plugin::instantiate( _plugin_name, _instrument_track,
							_instrument_track );
	// check whether instantiated plugin is an instrument
	if( dynamic_cast<instrument *>( p ) != NULL )
	{
		// everything ok, so return pointer
		return( dynamic_cast<instrument *>( p ) );
	}

	// not quite... so delete plugin and return dummy instrument
	delete p;
	return( new dummyInstrument( _instrument_track ) );
}




bool instrument::isFromTrack( const track * _track ) const
{
	return( m_instrumentTrack == _track );
}



void instrument::applyRelease( sampleFrame * buf, const notePlayHandle * _n )
{
	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	const fpp_t fpp = engine::getMixer()->framesPerPeriod();
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




QString instrument::fullDisplayName() const
{
	return getInstrumentTrack()->displayName();
}







instrumentView::instrumentView( instrument * _instrument, QWidget * _parent ) :
	pluginView( _instrument, _parent )
{
	setModel( _instrument );
	setFixedSize( 250, 250 );
	setAttribute( Qt::WA_DeleteOnClose, TRUE );
}




instrumentView::~instrumentView()
{
	if( getInstrumentTrackWindow() )
	{
		getInstrumentTrackWindow()->m_instrumentView = NULL;
	}
}




void instrumentView::setModel( ::model * _model, bool )
{
	if( dynamic_cast<instrument *>( _model ) != NULL )
	{
		modelView::setModel( _model );
		getInstrumentTrackWindow()->setWindowIcon(
				model()->getDescriptor()->logo->pixmap() );
		connect( model(), SIGNAL( destroyed( QObject * ) ),
					this, SLOT( close() ) );
	}
}




instrumentTrackWindow * instrumentView::getInstrumentTrackWindow( void )
{
	return( dynamic_cast<instrumentTrackWindow *>(
					parentWidget()->parentWidget() ) );
}

#endif
