/*
 * InstrumentView.cpp - base-class for views of all Instruments
 *
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QtGui/QIcon>

#include "InstrumentView.h"
#include "embed.h"
#include "Instrument.h"
#include "InstrumentTrack.h"
#include "string_pair_drag.h"


InstrumentView::InstrumentView( Instrument * _Instrument, QWidget * _parent ) :
	PluginView( _Instrument, _parent )
{
	setModel( _Instrument );
	setFixedSize( 250, 250 );
	setAttribute( Qt::WA_DeleteOnClose, TRUE );
}




InstrumentView::~InstrumentView()
{
	if( instrumentTrackWindow() )
	{
		instrumentTrackWindow()->m_instrumentView = NULL;
	}
}




void InstrumentView::setModel( Model * _model, bool )
{
	if( dynamic_cast<Instrument *>( _model ) != NULL )
	{
		ModelView::setModel( _model );
		instrumentTrackWindow()->setWindowIcon( model()->descriptor()->logo->pixmap() );
		connect( model(), SIGNAL( destroyed( QObject * ) ), this, SLOT( close() ) );
	}
}




InstrumentTrackWindow * InstrumentView::instrumentTrackWindow( void )
{
	return( dynamic_cast<InstrumentTrackWindow *>(
					parentWidget()->parentWidget() ) );
}

