/*
 * BBTrackContainer.cpp - model-component of BB-Editor
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


#include "BBTrackContainer.h"
#include "BBTrack.h"
#include "Engine.h"
#include "Song.h"



BBTrackContainer::BBTrackContainer() :
	TrackContainer(),
	m_bbComboBoxModel(this)
{
	connect(&m_bbComboBoxModel, SIGNAL(dataChanged()),
			this, SLOT(currentBBChanged()));
	// we *always* want to receive updates even in case BB actually did
	// not change upon setCurrentBB()-call
	connect(&m_bbComboBoxModel, SIGNAL(dataUnchanged()),
			this, SLOT(currentBBChanged()));
	setType(BBContainer);
}




BBTrackContainer::~BBTrackContainer()
{
}




bool BBTrackContainer::play(TimePos start, fpp_t frames, f_cnt_t offset, int clipNum)
{
	bool notePlayed = false;

	if (lengthOfBB(clipNum) <= 0)
	{
		return false;
	}

	start = start % (lengthOfBB(clipNum) * TimePos::ticksPerBar());

	TrackList tl = tracks();
	for (Track * t : tl)
	{
		if (t->play(start, frames, offset, clipNum))
		{
			notePlayed = true;
		}
	}

	return notePlayed;
}




void BBTrackContainer::updateAfterTrackAdd()
{
	if (numOfBBs() == 0 && !Engine::getSong()->isLoadingProject())
	{
		Engine::getSong()->addBBTrack();
	}
}




bar_t BBTrackContainer::lengthOfBB(int bb) const
{
	TimePos maxLength = TimePos::ticksPerBar();

	const TrackList & tl = tracks();
	for (Track * t : tl)
	{
		// Don't create Clips here if they don't exist
		if (bb < t->numOfClips())
		{
			maxLength = qMax(maxLength, t->getClip(bb)->length());
		}
	}

	return maxLength.nextFullBar();
}




int BBTrackContainer::numOfBBs() const
{
	return Engine::getSong()->countTracks(Track::BBTrack);
}




void BBTrackContainer::removeBB(int bb)
{
	TrackList tl = tracks();
	for (Track * t : tl)
	{
		delete t->getClip(bb);
		t->removeBar(bb * DefaultTicksPerBar);
	}
	if (bb <= currentBB())
	{
		setCurrentBB(qMax(currentBB() - 1, 0));
	}
}




void BBTrackContainer::swapBB(int bb1, int bb2)
{
	TrackList tl = tracks();
	for (Track * t : tl)
	{
		t->swapPositionOfClips(bb1, bb2);
	}
	updateComboBox();
}




void BBTrackContainer::updateBBTrack(Clip * clip)
{
	BBTrack * t = BBTrack::findBBTrack(clip->startPosition() / DefaultTicksPerBar);
	if (t != nullptr)
	{
		t->dataChanged();
	}
}




void BBTrackContainer::fixIncorrectPositions()
{
	TrackList tl = tracks();
	for (Track * t : tl)
	{
		for (int i = 0; i < numOfBBs(); ++i)
		{
			t->getClip(i)->movePosition(TimePos(i, 0));
		}
	}
}




void BBTrackContainer::play()
{
	if (Engine::getSong()->playMode() != Song::Mode_PlayBB)
	{
		Engine::getSong()->playBB();
	}
	else
	{
		Engine::getSong()->togglePause();
	}
}




void BBTrackContainer::stop()
{
	Engine::getSong()->stop();
}




void BBTrackContainer::updateComboBox()
{
	const int curBB = currentBB();

	m_bbComboBoxModel.clear();

	for (int i = 0; i < numOfBBs(); ++i)
	{
		BBTrack * bbt = BBTrack::findBBTrack(i);
		m_bbComboBoxModel.addItem(bbt->name());
	}
	setCurrentBB(curBB);
}




void BBTrackContainer::currentBBChanged()
{
	// now update all track-labels (the current one has to become white, the others gray)
	TrackList tl = Engine::getSong()->tracks();
	for (Track * t : tl)
	{
		if (t->type() == Track::BBTrack)
		{
			t->dataChanged();
		}
	}
}




void BBTrackContainer::createClipsForBB(int bb)
{
	TrackList tl = tracks();
	for (Track * t : tl)
	{
		t->createClipsForBB(bb);
	}
}

AutomatedValueMap BBTrackContainer::automatedValuesAt(TimePos time, int clipNum) const
{
	Q_ASSERT(clipNum >= 0);
	Q_ASSERT(time.getTicks() >= 0);

	auto lengthBars = lengthOfBB(clipNum);
	auto lengthTicks = lengthBars * TimePos::ticksPerBar();
	if (time > lengthTicks)
	{
		time = lengthTicks;
	}

	return TrackContainer::automatedValuesAt(time + (TimePos::ticksPerBar() * clipNum), clipNum);
}

