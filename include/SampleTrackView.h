/*
 * SampleTrackView.h
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

#ifndef LMMS_GUI_SAMPLE_TRACK_VIEW_H
#define LMMS_GUI_SAMPLE_TRACK_VIEW_H


#include "SampleTrack.h"
#include "TrackView.h"

namespace lmms
{

namespace gui
{

class Knob;
class MixerChannelLcdSpinBox;
class SampleTrackWindow;
class TrackLabelButton;


class SampleTrackView : public TrackView
{
	Q_OBJECT
public:
	SampleTrackView( SampleTrack* Track, TrackContainerView* tcv );
	~SampleTrackView() override;

	SampleTrackWindow * getSampleTrackWindow()
	{
		return m_window;
	}

	SampleTrack * model()
	{
		return castModel<SampleTrack>();
	}

	const SampleTrack * model() const
	{
		return castModel<SampleTrack>();
	}


	QMenu * createMixerMenu( QString title, QString newMixerLabel ) override;


public slots:
	void showEffects();
	void updateIndicator();


protected:
	void modelChanged() override;
	QString nodeName() const override
	{
		return "SampleTrackView";
	}

	void dragEnterEvent(QDragEnterEvent *dee) override;
	void dropEvent(QDropEvent *de) override;

private slots:
	void assignMixerLine( int channelIndex );
	void createMixerLine();


private:
	SampleTrackWindow * m_window;
	MixerChannelLcdSpinBox* m_mixerChannelNumber;
	Knob * m_volumeKnob;
	Knob * m_panningKnob;
	FadeButton * m_activityIndicator;

	TrackLabelButton * m_tlb;

	FadeButton * getActivityIndicator() override
	{
		return m_activityIndicator;
	}

	friend class SampleTrackWindow;
} ;


} // namespace gui

} // namespace lmms

#endif // LMMS_GUI_SAMPLE_TRACK_VIEW_H
