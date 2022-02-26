/*
 * SampleClip.h
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
 
#ifndef SAMPLE_CLIP_H
#define SAMPLE_CLIP_H

#include "Clip.h"

class SampleBuffer;


class SampleClip : public Clip
{
	Q_OBJECT
	mapPropertyFromModel(bool,isRecord,setRecord,m_recordModel);
public:
	SampleClip( Track * _track );
	SampleClip( const SampleClip& orig );
	virtual ~SampleClip();

	SampleClip& operator=( const SampleClip& that ) = delete;

	void changeLength( const TimePos & _length ) override;
	const QString & sampleFile() const;

	void saveSettings( QDomDocument & _doc, QDomElement & _parent ) override;
	void loadSettings( const QDomElement & _this ) override;
	inline QString nodeName() const override
	{
		return "sampleclip";
	}

	SampleBuffer* sampleBuffer()
	{
		return m_sampleBuffer;
	}

	TimePos sampleLength() const;
	void setSampleStartFrame( f_cnt_t startFrame );
	void setSamplePlayLength( f_cnt_t length );
	ClipView * createView( TrackView * _tv ) override;


	bool isPlaying() const;
	void setIsPlaying(bool isPlaying);

public slots:
	void setSampleBuffer( SampleBuffer* sb );
	void setSampleFile( const QString & _sf );
	void updateLength();
	void toggleRecord();
	void playbackPositionChanged();
	void updateTrackClips();


private:
	SampleBuffer* m_sampleBuffer;
	BoolModel m_recordModel;
	bool m_isPlaying;

	friend class SampleClipView;


signals:
	void sampleChanged();
	void wasReversed();
} ;



#endif
