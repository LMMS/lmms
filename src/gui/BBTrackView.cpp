/*
 * BBTrackView.cpp
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 
#include "BBTrackView.h"

#include "BBEditor.h"
#include "BBTrackContainer.h"
#include "Engine.h"
#include "GuiApplication.h"
#include "TrackLabelButton.h"

namespace lmms::gui
{

BBTrackView::BBTrackView( BBTrack * _bbt, TrackContainerView* tcv ) :
	TrackView( _bbt, tcv ),
	m_bbTrack( _bbt )
{
	setFixedHeight( 32 );
	// drag'n'drop with bb-tracks only causes troubles (and makes no sense
	// too), so disable it
	setAcceptDrops( false );

	m_trackLabel = new TrackLabelButton( this, getTrackSettingsWidget() );
	m_trackLabel->setIcon( embed::getIconPixmap( "bb_track" ) );
	m_trackLabel->move( 3, 1 );
	m_trackLabel->show();
	connect( m_trackLabel, SIGNAL( clicked( bool ) ),
			this, SLOT( clickedTrackLabel() ) );
	setModel( _bbt );
}




BBTrackView::~BBTrackView()
{
	getGUI()->getBBEditor()->removeBBView( BBTrack::s_infoMap[m_bbTrack] );
}




bool BBTrackView::close()
{
	getGUI()->getBBEditor()->removeBBView( BBTrack::s_infoMap[m_bbTrack] );
	return TrackView::close();
}




void BBTrackView::clickedTrackLabel()
{
	Engine::getBBTrackContainer()->setCurrentBB( m_bbTrack->index() );
	getGUI()->getBBEditor()->parentWidget()->show();
	getGUI()->getBBEditor()->setFocus( Qt::ActiveWindowFocusReason );
}


} // namespace lmms::gui
