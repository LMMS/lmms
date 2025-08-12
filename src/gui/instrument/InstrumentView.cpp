/*
 * InstrumentView.cpp - base-class for views of all Instruments
 *
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QIcon>

#include "InstrumentView.h"
#include "embed.h"
#include "InstrumentTrackWindow.h"

namespace lmms::gui
{

InstrumentView::InstrumentView( Instrument * _Instrument, QWidget * _parent ) :
	PluginView( _Instrument, _parent )
{
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	setModel( _Instrument );
	setAttribute( Qt::WA_DeleteOnClose, true );
}




InstrumentView::~InstrumentView()
{
	if( instrumentTrackWindow() )
	{
		instrumentTrackWindow()->m_instrumentView = nullptr;
	}
}




void InstrumentView::setModel( Model * _model, bool )
{
	if( dynamic_cast<Instrument *>( _model ) != nullptr )
	{
		ModelView::setModel( _model );
		instrumentTrackWindow()->setWindowIcon( model()->logo()->pixmap() );
		connect( model(), SIGNAL(destroyed(QObject*)), this, SLOT(close()));
	}
}




InstrumentTrackWindow * InstrumentView::instrumentTrackWindow()
{
	return( dynamic_cast<InstrumentTrackWindow *>(
					parentWidget()->parentWidget() ) );
}





} // namespace lmms::gui
