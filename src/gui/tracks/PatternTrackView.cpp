/*
 * PatternTrackView.cpp
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
 
#include "PatternTrackView.h"

#include "Engine.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "PatternEditor.h"
#include "PatternStore.h"
#include "PatternTrack.h"
#include "TrackLabelButton.h"

namespace lmms::gui
{

PatternTrackView::PatternTrackView(PatternTrack* pt, TrackContainerView* tcv) :
	TrackView(pt, tcv),
	m_patternTrack(pt)
{
	setFixedHeight( 32 );
	// drag'n'drop with pattern tracks only causes troubles (and makes no sense too), so disable it
	setAcceptDrops( false );

	m_trackLabel = new TrackLabelButton( this, getTrackSettingsWidget() );
	m_trackLabel->setIcon( embed::getIconPixmap("pattern_track"));
	m_trackLabel->move( 3, 1 );
	m_trackLabel->show();
	connect( m_trackLabel, SIGNAL(clicked(bool)),
			this, SLOT(clickedTrackLabel()));
	setModel(pt);
}




PatternTrackView::~PatternTrackView()
{
	getGUI()->patternEditor()->m_editor->removeViewsForPattern(PatternTrack::s_infoMap[m_patternTrack]);
}




bool PatternTrackView::close()
{
	getGUI()->patternEditor()->m_editor->removeViewsForPattern(PatternTrack::s_infoMap[m_patternTrack]);
	return TrackView::close();
}




void PatternTrackView::clickedTrackLabel()
{
	Engine::patternStore()->setCurrentPattern(m_patternTrack->patternIndex());
	getGUI()->mainWindow()->togglePatternEditorWin(true);
}


} // namespace lmms::gui
