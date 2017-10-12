/*
 * MidiController.h - A controller to receive MIDI control-changes
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

#ifndef MIDI_CUSTOMCONTROLLER_H
#define MIDI_CUSTOMCONTROLLER_H

#include <QWidget>

#include "AutomatableModel.h"
#include "MidiController.h"
#include "MidiEventProcessor.h"
#include "MidiPort.h"


class MidiPort;


class MidiCustomController : public MidiController
{
	Q_OBJECT
public:

	//RIKIS
	//each element will display its own behaviour end be receptive to certain events
//	enum ControllerElements {
//		knob,slider,button,wheel,original //the original enum makes the controller behave without the enhancement (compatibility issues)
//	};
	//RIKIS


	MidiCustomController( Model * _parent );
	virtual ~MidiCustomController();

	virtual void processInEvent( const MidiEvent & _me,
					const MidiTime & _time, f_cnt_t offset = 0 );

	virtual void processOutEvent( const MidiEvent& _me,
					const MidiTime & _time, f_cnt_t offset = 0 )
	{
		// No output yet
	}

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _this );
	virtual void loadSettings( const QDomElement & _this );
	virtual QString nodeName() const;

	// Used by controllerConnectionDialog to copy
	void subscribeReadablePorts( const MidiPort::Map & _map );


public slots:
	virtual ControllerDialog * createDialog( QWidget * _parent );
	void updateName();

	//Convenience classes for displaying values
	float getLastValue(){return m_lastValue;}
	int getMidiControllerValue(){return m_MidiControllerValue ;}
	int getMidiVelocity(){return m_MidiVelocity ;}
	int getMidiKey(){return m_MidiKey ;}
	int getMidiPanning(){return m_MidiPanning ;}
	int getMidiPitchbend(){return m_MidiPitchbend ;}
	MidiEventTypes getMidiType(){return m_MidiType;}

protected:
	// The internal per-controller get-value function
	virtual void updateValueBuffer();


	MidiPort m_midiPort;

	int m_MidiControllerValue;
	int m_MidiVelocity;
	int m_MidiKey;
	int m_MidiPanning;
	int m_MidiPitchbend;
	MidiEventTypes m_MidiType;

	float m_lastValue;
	float m_previousValue;

	friend class ControllerConnectionDialog;
	friend class AutoDetectMidiController;

} ;


#endif
