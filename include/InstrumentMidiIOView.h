/*
 * InstrumentMidiIOView.h - widget in instrument-track-window for setting
 *                          up MIDI-related stuff
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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

#include <QtGui/QWidget>

#include "ModelView.h"


class groupBox;
class LcdSpinBox;
class QToolButton;


class InstrumentMidiIOView : public QWidget, public ModelView
{
	Q_OBJECT
public:
	InstrumentMidiIOView( QWidget* parent );
	virtual ~InstrumentMidiIOView();


private:
	virtual void modelChanged();

	groupBox * m_midiInputGroupBox;
	LcdSpinBox * m_inputChannelSpinBox;
	LcdSpinBox * m_fixedInputVelocitySpinBox;
	QToolButton * m_rpBtn;

	groupBox * m_midiOutputGroupBox;
	LcdSpinBox * m_outputChannelSpinBox;
	LcdSpinBox * m_fixedOutputVelocitySpinBox;
	LcdSpinBox * m_outputProgramSpinBox;
	LcdSpinBox * m_fixedOutputNoteSpinBox;
	QToolButton * m_wpBtn;

	LcdSpinBox* m_baseVelocitySpinBox;

} ;

#endif
