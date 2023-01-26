/*
 * InstrumentTrackWindow.h - declaration of InstrumentTrackWindow class
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

#ifndef INSTRUMENT_TRACK_WINDOW_H
#define INSTRUMENT_TRACK_WINDOW_H

#include <QWidget>

#include "ModelView.h"
#include "SerializingObject.h"

class QLabel;
class QLineEdit;
class QWidget;

namespace lmms
{

class InstrumentTrack;

namespace gui
{

class EffectRackView;
class MixerLineLcdSpinBox;
class InstrumentFunctionArpeggioView;
class InstrumentFunctionNoteStackingView;
class InstrumentMidiIOView;
class InstrumentMiscView;
class InstrumentSoundShapingView;
class InstrumentTrackShapingView;
class InstrumentTrackView;
class Knob;
class LcdSpinBox;
class LeftRightNav;
class PianoView;
class PluginView;
class TabWidget;


class InstrumentTrackWindow : public QWidget, public ModelView,
								public SerializingObjectHook
{
	Q_OBJECT
public:
	InstrumentTrackWindow( InstrumentTrackView * _tv );
	~InstrumentTrackWindow() override;

	// parent for all internal tab-widgets
	TabWidget * tabWidgetParent()
	{
		return m_tabWidget;
	}

	InstrumentTrack * model()
	{
		return castModel<InstrumentTrack>();
	}

	const InstrumentTrack * model() const
	{
		return castModel<InstrumentTrack>();
	}

	void setInstrumentTrackView( InstrumentTrackView * _tv );

	InstrumentTrackView *instrumentTrackView()
	{
		return m_itv;
	}


	PianoView * pianoView()
	{
		return m_pianoView;
	}

	static void dragEnterEventGeneric( QDragEnterEvent * _dee );

	void dragEnterEvent( QDragEnterEvent * _dee ) override;
	void dropEvent( QDropEvent * _de ) override;


public slots:
	void textChanged( const QString & _new_name );
	void toggleVisibility( bool _on );
	void updateName();
	void updateInstrumentView();


protected:
	// capture close-events for toggling instrument-track-button
	void closeEvent( QCloseEvent * _ce ) override;
	void focusInEvent( QFocusEvent * _fe ) override;

	void saveSettings( QDomDocument & _doc, QDomElement & _this ) override;
	void loadSettings( const QDomElement & _this ) override;


protected slots:
	void saveSettingsBtnClicked();
	void viewNextInstrument();
	void viewPrevInstrument();

private:
	void modelChanged() override;
	void viewInstrumentInDirection(int d);
	//! adjust size of any child widget of the main tab
	//! required to keep the old look when using a variable sized tab widget
	void adjustTabSize(QWidget *w);

	InstrumentTrack * m_track;
	InstrumentTrackView * m_itv;

	// widgets on the top of an instrument-track-window
	QLineEdit * m_nameLineEdit;
	LeftRightNav * m_leftRightNav;
	Knob * m_volumeKnob;
	Knob * m_panningKnob;
	Knob * m_pitchKnob;
	QLabel * m_pitchLabel;
	LcdSpinBox* m_pitchRangeSpinBox;
	QLabel * m_pitchRangeLabel;
	MixerLineLcdSpinBox * m_mixerChannelNumber;



	// tab-widget with all children
	TabWidget * m_tabWidget;
	PluginView * m_instrumentView;
	InstrumentSoundShapingView * m_ssView;
	InstrumentFunctionNoteStackingView* m_noteStackingView;
	InstrumentFunctionArpeggioView* m_arpeggioView;
	InstrumentMidiIOView * m_midiView;
	EffectRackView * m_effectView;
	InstrumentMiscView *m_miscView;


	// test-piano at the bottom of every instrument-settings-window
	PianoView * m_pianoView;

	friend class InstrumentView;
	friend class InstrumentTrackView;
} ;


} // namespace gui

} // namespace lmms

#endif
