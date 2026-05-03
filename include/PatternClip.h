/*
 * PatternClip.h
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

#ifndef LMMS_PATTERN_CLIP_H
#define LMMS_PATTERN_CLIP_H

#include "Clip.h"

namespace lmms
{

/*! \brief Dummy clip for PatternTracks
 *
 *  Only used in the Song (Editor). See PatternStore.h for more info.
*/
class PatternClip : public Clip
{
public:
	PatternClip(Track* track);
	~PatternClip() override = default;

	void saveSettings( QDomDocument & _doc, QDomElement & _parent ) override;
	void loadSettings( const QDomElement & _this ) override;
	inline QString nodeName() const override
	{
		return "patternclip";
	}

	int patternIndex();

	gui::ClipView * createView( gui::TrackView * _tv ) override;

	PatternClip* clone() override
	{
		return new PatternClip(*this);
	}

private:
	friend class PatternClipView;
} ;


} // namespace lmms

#endif // LMMS_PATTERN_CLIP_H
