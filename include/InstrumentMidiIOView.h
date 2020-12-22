/*
 * InstrumentMidiIOView.h - widget in instrument-track-window for setting
 *                          up MIDI-related stuff
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

#ifndef INSTRUMENT_MIDI_IO_VIEW_H
#define INSTRUMENT_MIDI_IO_VIEW_H

#include <QWidget>

#include "ModelView.h"


class GroupBox;
class LcdSpinBox;
class QToolButton;
class LedCheckBox;
class InstrumentTrack;


class InstrumentMidiIOView : public QWidget, public ModelView
{
	Q_OBJECT
public:
	InstrumentMidiIOView( QWidget* parent );
	virtual ~InstrumentMidiIOView();


private:
	void modelChanged() override;

	GroupBox * m_midiInputGroupBox;
	LcdSpinBox * m_inputChannelSpinBox;
	LcdSpinBox * m_fixedInputVelocitySpinBox;
	QToolButton * m_rpBtn;

	GroupBox * m_midiOutputGroupBox;
	LcdSpinBox * m_outputChannelSpinBox;
	LcdSpinBox * m_fixedOutputVelocitySpinBox;
	LcdSpinBox * m_outputProgramSpinBox;
	LcdSpinBox * m_fixedOutputNoteSpinBox;
	QToolButton * m_wpBtn;

	LcdSpinBox* m_baseVelocitySpinBox;

} ;

class InstrumentMiscView : public QWidget
{
	Q_OBJECT
public:
	InstrumentMiscView( InstrumentTrack *it, QWidget* parent );
	~InstrumentMiscView();

	GroupBox * pitchGroupBox()
	{
		return m_pitchGroupBox;
	}

private:

	GroupBox * m_pitchGroupBox;

};

#endif
