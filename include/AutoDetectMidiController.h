/*
 * AutoDetectMidiController.h - says from which controller last event is
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail.com>
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


#ifndef AUTO_DETECT_MIDI_CONTROLLER_H
#define AUTO_DETECT_MIDI_CONTROLLER_H

#include <QString>

#include "MidiController.h"
#include "MidiClient.h"
#include "MidiTime.h"


class AutoDetectMidiController : public MidiController
{
	Q_OBJECT
public:
	AutoDetectMidiController( Model* parent );

	virtual ~AutoDetectMidiController();

	virtual void processInEvent( const MidiEvent& event, const MidiTime& time, f_cnt_t offset = 0 );

	MidiController* copyToMidiController( Model* parent );

	void useDetected();


public slots:
	void reset();


private:
	int m_detectedMidiChannel;
	int m_detectedMidiController;
	QString m_detectedMidiPort;

	friend class ControllerRackView;

} ;

#endif
