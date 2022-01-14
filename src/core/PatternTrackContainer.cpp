/*
 * PatternTrackContainer.cpp - model-component of Pattern Editor
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


#include "PatternTrackContainer.h"

#include "Engine.h"
#include "PatternTrack.h"
#include "Song.h"



PatternTrackContainer::PatternTrackContainer() :
	TrackContainer(),
	m_patternComboBoxModel(this)
{
	connect(&m_patternComboBoxModel, SIGNAL(dataChanged()),
			this, SLOT(currentPatternChanged()));
	// we *always* want to receive updates even in case pattern actually did
	// not change upon setCurrentPattern()-call
	connect(&m_patternComboBoxModel, SIGNAL(dataUnchanged()),
			this, SLOT(currentPatternChanged()));
	setType(PatternContainer);
}




PatternTrackContainer::~PatternTrackContainer()
{
}




bool PatternTrackContainer::play(TimePos start, fpp_t frames, f_cnt_t offset, int clipNum)
{
	bool notePlayed = false;

	if (lengthOfPattern(clipNum) <= 0)
	{
		return false;
	}

	start = start % (lengthOfPattern(clipNum) * TimePos::ticksPerBar());

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




void PatternTrackContainer::updateAfterTrackAdd()
{
	if (numOfPatterns() == 0 && !Engine::getSong()->isLoadingProject())
	{
		Engine::getSong()->addPatternTrack();
	}
}




bar_t PatternTrackContainer::lengthOfPattern(int pattern) const
{
	TimePos maxLength = TimePos::ticksPerBar();

	const TrackList & tl = tracks();
	for (Track * t : tl)
	{
		// Don't create Clips here if they don't exist
		if (pattern < t->numOfClips())
		{
			maxLength = qMax(maxLength, t->getClip(pattern)->length());
		}
	}

	return maxLength.nextFullBar();
}




int PatternTrackContainer::numOfPatterns() const
{
	return Engine::getSong()->countTracks(Track::PatternTrack);
}




void PatternTrackContainer::removePattern(int pattern)
{
	TrackList tl = tracks();
	for (Track * t : tl)
	{
		delete t->getClip(pattern);
		t->removeBar(pattern * DefaultTicksPerBar);
	}
	if (pattern <= currentPattern())
	{
		setCurrentPattern(qMax(currentPattern() - 1, 0));
	}
}




void PatternTrackContainer::swapPattern(int pattern1, int pattern2)
{
	TrackList tl = tracks();
	for (Track * t : tl)
	{
		t->swapPositionOfClips(pattern1, pattern2);
	}
	updateComboBox();
}




void PatternTrackContainer::updatePatternTrack(Clip * clip)
{
	PatternTrack * t = PatternTrack::findPatternTrack(clip->startPosition() / DefaultTicksPerBar);
	if (t != nullptr)
	{
		t->dataChanged();
	}
}




void PatternTrackContainer::fixIncorrectPositions()
{
	TrackList tl = tracks();
	for (Track * t : tl)
	{
		for (int i = 0; i < numOfPatterns(); ++i)
		{
			t->getClip(i)->movePosition(TimePos(i, 0));
		}
	}
}




void PatternTrackContainer::play()
{
	if (Engine::getSong()->playMode() != Song::Mode_PlayPattern)
	{
		Engine::getSong()->playPattern();
	}
	else
	{
		Engine::getSong()->togglePause();
	}
}




void PatternTrackContainer::stop()
{
	Engine::getSong()->stop();
}




void PatternTrackContainer::updateComboBox()
{
	const int curPattern = currentPattern();

	m_patternComboBoxModel.clear();

	for (int i = 0; i < numOfPatterns(); ++i)
	{
		PatternTrack * pt = PatternTrack::findPatternTrack(i);
		m_patternComboBoxModel.addItem(pt->name());
	}
	setCurrentPattern(curPattern);
}




void PatternTrackContainer::currentPatternChanged()
{
	// now update all track-labels (the current one has to become white, the others gray)
	TrackList tl = Engine::getSong()->tracks();
	for (Track * t : tl)
	{
		if (t->type() == Track::PatternTrack)
		{
			t->dataChanged();
		}
	}
}




void PatternTrackContainer::createClipsForPattern(int pattern)
{
	TrackList tl = tracks();
	for (Track * t : tl)
	{
		t->createClipsForPattern(pattern);
	}
}

AutomatedValueMap PatternTrackContainer::automatedValuesAt(TimePos time, int clipNum) const
{
	Q_ASSERT(clipNum >= 0);
	Q_ASSERT(time.getTicks() >= 0);

	auto lengthBars = lengthOfPattern(clipNum);
	auto lengthTicks = lengthBars * TimePos::ticksPerBar();
	if (time > lengthTicks)
	{
		time = lengthTicks;
	}

	return TrackContainer::automatedValuesAt(time + (TimePos::ticksPerBar() * clipNum), clipNum);
}

