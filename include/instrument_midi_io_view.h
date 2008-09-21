/*
 * instrument_midi_io_view.h - tab-widget in instrument-track-window for setting
 *                             up MIDI-related stuff
 *
 * Copyright (c) 2005-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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


#ifndef _INSTRUMENT_MIDI_IO_VIEW_H
#define _INSTRUMENT_MIDI_IO_VIEW_H

#include <QtGui/QWidget>

#include "mv_base.h"


class groupBox;
class lcdSpinBox;
class midiPortMenu;
class QToolButton;


class instrumentMidiIOView : public QWidget, public modelView
{
public:
	instrumentMidiIOView( QWidget * _parent );
	virtual ~instrumentMidiIOView();


private:
	virtual void modelChanged( void );

	groupBox * m_midiInputGroupBox;
	lcdSpinBox * m_inputChannelSpinBox;
	lcdSpinBox * m_fixedInputVelocitySpinBox;
	QToolButton * m_rpBtn;

	groupBox * m_midiOutputGroupBox;
	lcdSpinBox * m_outputChannelSpinBox;
	lcdSpinBox * m_fixedOutputVelocitySpinBox;
	lcdSpinBox * m_outputProgramSpinBox;
	QToolButton * m_wpBtn;

} ;


#endif
