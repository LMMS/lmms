/*
 * PatternStore.h - model-component of Pattern Editor
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


#ifndef PATTERN_STORE_H
#define PATTERN_STORE_H

#include "TrackContainer.h"
#include "ComboBoxModel.h"


/*
 * PatternStore is the backend of Pattern Editor:
 *
 * +----------------------------------+
 * | PatternStore (TrackContainer)    |
 * |                                  |
 * | +------------------------------+ |
 * | | Track 1   [Clip A]  [Clip B] | |
 * | +------------------------------+ |
 * |                                  |
 * | +------------------------------+ |
 * | | Track 2   [Clip C]  [Clip D] | |
 * | +------------------------------+ |
 * +----------------------------------+
 *
 * Pattern Editor displays one pattern at a time, but they all belong to the same PatternStore.
 * A "pattern" is not an object, it's just a word we use to describe a column of clips.
 * In the illustration above, Clip A and Clip C would be "Pattern 1".
 *
 * Do not confuse tracks and clips in the PatternStore with PatternTracks and PatternClips.
 * - PatternTracks are used in the Song Editor. Each one reference a "pattern" in the PatternStore.
 * - PatternClips are stored inside PatternTracks. They are just empty placeholders.
 */
class LMMS_EXPORT PatternStore : public TrackContainer
{
	Q_OBJECT
	mapPropertyFromModel(int, currentPattern, setCurrentPattern, m_patternComboBoxModel);
public:
	PatternStore();
	virtual ~PatternStore();

	virtual bool play(TimePos start, const fpp_t frames, const f_cnt_t frameBase, int clipNum = -1);

	void updateAfterTrackAdd() override;

	inline QString nodeName() const override
	{
		return "bbtrackcontainer"; // TODO rename to patternstore
	}

	bar_t lengthOfPattern(int pattern) const;
	inline bar_t lengthOfCurrentPattern()
	{
		return lengthOfPattern(currentPattern());
	}
	int numOfPatterns() const;
	void removePattern(int pattern);

	void swapPattern(int p1, int p2);

	void updatePatternTrack(Clip* clip);
	void fixIncorrectPositions();
	void createClipsForPattern(int pattern);

	AutomatedValueMap automatedValuesAt(TimePos time, int clipNum) const override;

public slots:
	void play();
	void stop();
	void updateComboBox();
	void currentPatternChanged();


private:
	ComboBoxModel m_patternComboBoxModel;


	// Where the pattern selection combo box is
	friend class PatternEditorWindow;

} ;


#endif
