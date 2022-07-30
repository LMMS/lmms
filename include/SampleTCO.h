/*
 * SampleTCO.h
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
 
#ifndef SAMPLE_TCO_H
#define SAMPLE_TCO_H

#include "SampleBuffer.h"
#include "SampleTrack.h"
#include "TrackContentObject.h"
 

class SampleTCO : public TrackContentObject
{
	Q_OBJECT
	mapPropertyFromModel(bool,isRecord,setRecord,m_recordModel);
public:
	SampleTCO( Track * _track );
	SampleTCO( const SampleTCO& orig );
	virtual ~SampleTCO();

	SampleTCO& operator=( const SampleTCO& that ) = delete;

	void changeLength( const TimePos & _length ) override;
	const QString & sampleFile() const;

	void saveSettings( QDomDocument & _doc, QDomElement & _parent ) override;
	void loadSettings( const QDomElement & _this ) override;
	inline QString nodeName() const override
	{
		return "sampletco";
	}

	SampleBuffer* sampleBuffer()
	{
		return m_sampleBuffer;
	}

	TimePos sampleLength() const;
	void setSampleStartFrame( f_cnt_t startFrame );
	void setSamplePlayLength( f_cnt_t length );
	TrackContentObjectView * createView( TrackView * _tv ) override;


	bool isPlaying() const;
	void setIsPlaying(bool isPlaying);

public slots:
	void setSampleBuffer( SampleBuffer* sb );
	void setSampleFile( const QString & _sf );
	void updateLength();
	void toggleRecord();
	void playbackPositionChanged();
	void updateTrackTcos();


private:
	SampleBuffer* m_sampleBuffer;
	BoolModel m_recordModel;
	bool m_isPlaying;

	friend class SampleTCOView;


signals:
	void sampleChanged();
	void wasReversed();
} ;



#endif