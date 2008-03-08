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


class QMenu;
class QAction;

class tabWidget;
class ledCheckBox;
class lcdSpinBox;


class instrumentMidiIOView : public QWidget, public modelView
{
	Q_OBJECT
public:
	instrumentMidiIOView( QWidget * _parent );
	virtual ~instrumentMidiIOView();


protected slots:
	void activatedReadablePort( QAction * _item );
	void activatedWriteablePort( QAction * _item );

	void updateReadablePortsMenu( void );
	void updateWriteablePortsMenu( void );


private:
	virtual void modelChanged( void );

	tabWidget * m_setupTabWidget;
	lcdSpinBox * m_inputChannelSpinBox;
	lcdSpinBox * m_outputChannelSpinBox;
	ledCheckBox * m_receiveCheckBox;
	ledCheckBox * m_sendCheckBox;
	ledCheckBox * m_defaultVelocityInCheckBox;
	ledCheckBox * m_defaultVelocityOutCheckBox;

	QMenu * m_readablePorts;
	QMenu * m_writeablePorts;


	friend class instrumentTrackWindow;

} ;


#endif
