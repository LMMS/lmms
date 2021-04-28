/*
 * SampleTrack.h - class SampleTrack, a track which provides arrangement of samples
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

#ifndef SAMPLE_TRACK_H
#define SAMPLE_TRACK_H

#include <QLayout>

#include "AudioPort.h"
#include "FadeButton.h"
#include "FxMixer.h"
#include "SampleTCO.h"
#include "SampleTrackView.h"
#include "Track.h"


class SampleTrack : public Track
{
	Q_OBJECT
public:
	SampleTrack( TrackContainer* tc );
	virtual ~SampleTrack();

	virtual bool play( const TimePos & _start, const fpp_t _frames,
						const f_cnt_t _frame_base, int _tco_num = -1 ) override;
	TrackView * createView( TrackContainerView* tcv ) override;
	TrackContentObject* createTCO(const TimePos & pos) override;


	virtual void saveTrackSpecificSettings( QDomDocument & _doc,
							QDomElement & _parent ) override;
	void loadTrackSpecificSettings( const QDomElement & _this ) override;

	inline IntModel * effectChannelModel()
	{
		return &m_effectChannelModel;
	}

	inline AudioPort * audioPort()
	{
		return &m_audioPort;
	}

	QString nodeName() const override
	{
		return "sampletrack";
	}

	bool isPlaying()
	{
		return m_isPlaying;
	}

	void setPlaying(bool playing)
	{
		if (m_isPlaying != playing) { emit playingChanged(); }
		m_isPlaying = playing;
	}

signals:
	void playingChanged();

public slots:
	void updateTcos();
	void setPlayingTcos( bool isPlaying );
	void updateEffectChannel();

private:
	FloatModel m_volumeModel;
	FloatModel m_panningModel;
	IntModel m_effectChannelModel;
	AudioPort m_audioPort;
	bool m_isPlaying;



	friend class SampleTrackView;
	friend class SampleTrackWindow;

} ;



#endif