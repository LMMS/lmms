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

#include <QMenu>

#include "Engine.h"
#include "GuiApplication.h"
#include "InstrumentTrack.h"
#include "Mixer.h"
#include "MixerView.h"
#include "PatternEditor.h"
#include "PatternStore.h"
#include "PatternTrack.h"
#include "SampleTrack.h"
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
	getGUI()->patternEditor()->parentWidget()->show();
	getGUI()->patternEditor()->setFocus(Qt::ActiveWindowFocusReason);
}

QMenu* PatternTrackView::createMixerMenu(QString title, QString newMixerLabel)
{
	// We have a different title for Pattern Tracks
	QMenu* mixerMenu = new QMenu(tr("Assign all to mixer channel"));

	mixerMenu->addAction(newMixerLabel, this, SLOT(createMixerLine()));
	mixerMenu->addSeparator();

	for (int i = 0; i < Engine::mixer()->numChannels(); ++i)
	{
		MixerChannel* curChannel = Engine::mixer()->mixerChannel(i);

		auto index = curChannel->m_channelIndex;

		QString label = tr("%1: %2").arg(index).arg(curChannel->m_name);

		mixerMenu->addAction(label, [this, index](){
			assignMixerLine(index);
		});
	}

	return mixerMenu;
}

void PatternTrackView::createMixerLine()
{
	int channelIndex = getGUI()->mixerView()->addNewChannel();
	auto channel = Engine::mixer()->mixerChannel(channelIndex);

	channel->m_name = getPatternTrack()->name();
	if (getTrack()->useColor()) { channel->setColor(getTrack()->color()); }

	assignMixerLine(channelIndex);
}

void PatternTrackView::assignMixerLine(int channelIndex)
{
	// Assign all tracks to channel:
	TrackContainer::TrackList tl = Engine::patternStore()->tracks();

	for (Track* track : tl)
	{
		if(track->type() == Track::TrackTypes::InstrumentTrack)
		{
			InstrumentTrack* t = dynamic_cast<InstrumentTrack*>(track);
			if (t) { t->mixerChannelModel()->setValue(channelIndex); }
		}
		else if (track->type() == Track::TrackTypes::SampleTrack)
		{
			SampleTrack* t = dynamic_cast<SampleTrack*>(track);
			if (t) { t->mixerChannelModel()->setValue(channelIndex); }
		}
	}
}


} // namespace lmms::gui
