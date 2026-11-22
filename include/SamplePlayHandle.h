
/*
 * SamplePlayHandle.h - play-handle for playing a sample
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_SAMPLE_PLAY_HANDLE_H
#define LMMS_SAMPLE_PLAY_HANDLE_H

#include "Sample.h"
#include "PlayHandle.h"

namespace lmms
{


class PatternTrack;
class SampleClip;
class Track;


class LMMS_EXPORT SamplePlayHandle : public PlayHandle
{
public:
	explicit SamplePlayHandle(Sample* sample);
	explicit SamplePlayHandle(SampleClip* clip);

	inline bool affinityMatters() const override
	{
		return true;
	}


	void play( SampleFrame* buffer ) override;
	bool isFinished() const override;

	bool isFromTrack( const Track * _track ) const override;

	f_cnt_t totalFrames() const;
	inline f_cnt_t framesDone() const
	{
		return( m_frame );
	}

	void setAutoFinish(bool _enable)
	{
		m_autoFinish = _enable;
	}

	void setPatternTrack(PatternTrack* pt)
	{
		m_patternTrack = pt;
	}

private:
	Sample* m_sample = nullptr;
	Sample::PlaybackState m_state;
	std::unique_ptr<AudioBusHandle> m_audioBusHandle;
	f_cnt_t m_frame = 0;
	Track* m_track = nullptr;
	PatternTrack* m_patternTrack = nullptr;
	bool m_autoFinish = true;
} ;


} // namespace lmms

#endif // LMMS_SAMPLE_PLAY_HANDLE_H
