/*
 * InstrumentTrackView.h - declaration of InstrumentTrackView class
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

#ifndef LMMS_GUI_INSTRUMENT_TRACK_VIEW_H
#define LMMS_GUI_INSTRUMENT_TRACK_VIEW_H

#include "TrackView.h"

#include "InstrumentTrack.h"


namespace lmms::gui
{


class InstrumentTrackWindow;
class Knob;
class MidiCCRackView;
class MixerChannelLcdSpinBox;
class TrackContainerView;
class TrackLabelButton;


class InstrumentTrackView : public TrackView
{
	Q_OBJECT
public:
	InstrumentTrackView( InstrumentTrack * _it, TrackContainerView* tc );
	~InstrumentTrackView() override;

	InstrumentTrackWindow * getInstrumentTrackWindow();

	InstrumentTrack * model()
	{
		return castModel<InstrumentTrack>();
	}

	const InstrumentTrack * model() const
	{
		return castModel<InstrumentTrack>();
	}

	static InstrumentTrackWindow * topLevelInstrumentTrackWindow();

	QMenu * midiMenu()
	{
		return m_midiMenu;
	}

	// Create a menu for assigning/creating channels for this track
	QMenu * createMixerMenu( QString title, QString newMixerLabel ) override;


protected:
	void modelChanged() override;
	void dragEnterEvent( QDragEnterEvent * _dee ) override;
	void dropEvent( QDropEvent * _de ) override;


private slots:
	void toggleInstrumentWindow( bool _on );
	void toggleMidiCCRack();
	void activityIndicatorPressed();
	void activityIndicatorReleased();

	void midiInSelected();
	void midiOutSelected();
	void midiConfigChanged();

	void assignMixerLine( int channelIndex );
	void createMixerLine();

	void handleConfigChange(QString cls, QString attr, QString value);

private:
	static QPixmap determinePixmap(InstrumentTrack* instrumentTrack);

private:
	InstrumentTrackWindow * m_window;

	// widgets in track-settings-widget
	TrackLabelButton * m_tlb;
	MixerChannelLcdSpinBox* m_mixerChannelNumber;
	Knob * m_volumeKnob;
	Knob * m_panningKnob;
	FadeButton * m_activityIndicator;

	QMenu * m_midiMenu;

	QAction * m_midiInputAction;
	QAction * m_midiOutputAction;

	std::unique_ptr<MidiCCRackView> m_midiCCRackView;

	QPoint m_lastPos;

	FadeButton * getActivityIndicator() override
	{
		return m_activityIndicator;
	}

	friend class InstrumentTrackWindow;
} ;


} // namespace lmms::gui

#endif // LMMS_GUI_INSTRUMENT_TRACK_VIEW_H
