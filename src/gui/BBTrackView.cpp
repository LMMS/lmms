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

#include <QMenu>

#include "BBEditor.h"
#include "BBTrackContainer.h"
#include "Engine.h"
#include "FxMixer.h"
#include "FxMixerView.h"
#include "GuiApplication.h"
#include "InstrumentTrack.h"
#include "SampleTrack.h"
#include "TrackLabelButton.h"

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
	gui->getBBEditor()->removeBBView( BBTrack::s_infoMap[m_bbTrack] );
}




bool BBTrackView::close()
{
	gui->getBBEditor()->removeBBView( BBTrack::s_infoMap[m_bbTrack] );
	return TrackView::close();
}




void BBTrackView::clickedTrackLabel()
{
	Engine::getBBTrackContainer()->setCurrentBB( m_bbTrack->index() );
	gui->getBBEditor()->parentWidget()->show();
	gui->getBBEditor()->setFocus( Qt::ActiveWindowFocusReason );
}

QMenu* BBTrackView::createFxMenu(QString title, QString newFxLabel)
{
	// We have a different title for BB Tracks
	QMenu* fxMenu = new QMenu(tr("Assign all to FX channel"));

	fxMenu->addAction(newFxLabel, this, SLOT(createFxLine()));
	fxMenu->addSeparator();

	for (int i = 0; i < Engine::fxMixer()->numChannels(); ++i)
	{
		FxChannel* curFX = Engine::fxMixer()->effectChannel(i);

		auto index = curFX->m_channelIndex;

		QString label = tr("FX: %1: %2").arg(index).arg(curFX->m_name);

		fxMenu->addAction(label, [this, index](){
			assignFxLine(index);
		});
	}

	return fxMenu;
}

void BBTrackView::createFxLine()
{
	int channelIndex = gui->fxMixerView()->addNewChannel();
	auto channel = Engine::fxMixer()->effectChannel(channelIndex);

	channel->m_name = getBBTrack()->name();
	if (getTrack()->useColor()) { channel->setColor(getTrack()->color()); }

	assignFxLine(channelIndex);
}

void BBTrackView::assignFxLine(int channelIndex)
{
	// Assign all tracks to channel:
	TrackContainer::TrackList tl = Engine::getBBTrackContainer()->tracks();

	for
	(
		TrackContainer::TrackList::iterator it = tl.begin();
		it != tl.end();
		++it
	)
	{
		if((*it)->type() == Track::TrackTypes::InstrumentTrack)
		{
			InstrumentTrack* t = dynamic_cast<InstrumentTrack*>(*it);
			if (t) { t->effectChannelModel()->setValue(channelIndex); }
		}
		else if ((*it)->type() == Track::TrackTypes::SampleTrack)
		{
			SampleTrack* t = dynamic_cast<SampleTrack*>(*it);
			if (t) { t->effectChannelModel()->setValue(channelIndex); }
		}
	}
}
