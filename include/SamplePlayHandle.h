
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
#include "AutomatableModel.h"
#include "PlayHandle.h"

namespace lmms
{


class PatternTrack;
class SampleClip;
class Track;


class LMMS_EXPORT SamplePlayHandle : public PlayHandle
{
public:
	SamplePlayHandle(Sample* sample, bool ownAudioBusHandle = true);
	SamplePlayHandle( const QString& sampleFile );
	SamplePlayHandle( SampleClip* clip );
	~SamplePlayHandle() override;

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
	void setDoneMayReturnTrue( bool _enable )
	{
		m_doneMayReturnTrue = _enable;
	}

	void setPatternTrack(PatternTrack* pt)
	{
		m_patternTrack = pt;
	}

	void setVolumeModel( FloatModel * _model )
	{
		m_volumeModel = _model;
	}


private:
	Sample* m_sample;
	bool m_doneMayReturnTrue;

	f_cnt_t m_frame;
	Sample::PlaybackState m_state;

	const bool m_ownAudioBusHandle;

	FloatModel m_defaultVolumeModel;
	FloatModel * m_volumeModel;
	Track * m_track;

	PatternTrack* m_patternTrack;

} ;


} // namespace lmms

#endif // LMMS_SAMPLE_PLAY_HANDLE_H
