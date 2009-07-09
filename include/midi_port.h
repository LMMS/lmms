/*
 * midi_port.h - abstraction of MIDI-ports which are part of LMMS's MIDI-
 *               sequencing system
 *
 * Copyright (c) 2005-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _MIDI_PORT_H
#define _MIDI_PORT_H

#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QPair>

#include "midi.h"
#include "automatable_model.h"


class midiClient;
class MidiEventProcessor;
class midiPortMenu;
class midiTime;


// class for abstraction of MIDI-port
class midiPort : public model, public serializingObject
{
	Q_OBJECT
	mapPropertyFromModel(int,inputChannel,setInputChannel,
							m_inputChannelModel);
	mapPropertyFromModel(int,outputChannel,setOutputChannel,
							m_outputChannelModel);
	mapPropertyFromModel(int,inputController,setInputController,
						m_inputControllerModel);
	mapPropertyFromModel(int,outputController,setOutputController,
						m_outputControllerModel);
	mapPropertyFromModel(int,fixedInputVelocity,setFixedInputVelocity,
						m_fixedInputVelocityModel);
	mapPropertyFromModel(int,fixedOutputVelocity,setFixedOutputVelocity,
						m_fixedOutputVelocityModel);
	mapPropertyFromModel(int,outputProgram,setOutputProgram,
						m_outputProgramModel);
	mapPropertyFromModel(bool,isReadable,setReadable,m_readableModel);
	mapPropertyFromModel(bool,isWritable,setWritable,m_writableModel);
public:
	typedef QMap<QString, bool> map;

	enum Modes
	{
		Disabled,	// don't route any MIDI-events (default)
		Input,		// from MIDI-client to MIDI-event-processor
		Output,		// from MIDI-event-processor to MIDI-client
		Duplex		// both directions
	} ;

	midiPort( const QString & _name,
			midiClient * _mc,
			MidiEventProcessor * _mep,
			model * _parent = NULL,
			Modes _mode = Disabled );
	virtual ~midiPort();

	void setName( const QString & _name );

	inline Modes mode() const
	{
		return m_mode;
	}

	void setMode( Modes _mode );

	inline bool inputEnabled() const
	{
		return mode() == Input || mode() == Duplex;
	}

	inline bool outputEnabled() const
	{
		return mode() == Output || mode() == Duplex;
	}

	inline int realOutputChannel() const
	{
		return outputChannel() - 1;
	}

	void processInEvent( const midiEvent & _me, const midiTime & _time );
	void processOutEvent( const midiEvent & _me, const midiTime & _time );


	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	virtual QString nodeName() const
	{
		return "midiport";
	}

	void subscribeReadablePort( const QString & _port,
						bool _subscribe = TRUE );
	void subscribeWritablePort( const QString & _port,
						bool _subscribe = TRUE );

	const map & readablePorts() const
	{
		return m_readablePorts;
	}

	const map & writablePorts() const
	{
		return m_writablePorts;
	}

	void unsubscribeAllReadablePorts();
	void unsubscribeAllWriteablePorts();
	inline void unsubscribeAllPorts()
	{
		unsubscribeAllReadablePorts();
		unsubscribeAllWriteablePorts();
	}

	midiPortMenu * m_readablePortsMenu;
	midiPortMenu * m_writablePortsMenu;


public slots:
	void updateMidiPortMode();


private slots:
	void updateReadablePorts();
	void updateWritablePorts();
	void updateOutputProgram();


private:
	midiClient * m_midiClient;
	MidiEventProcessor * m_midiEventProcessor;

	Modes m_mode;

	intModel m_inputChannelModel;
	intModel m_outputChannelModel;
	intModel m_inputControllerModel;
	intModel m_outputControllerModel;
	intModel m_fixedInputVelocityModel;
	intModel m_fixedOutputVelocityModel;
	intModel m_outputProgramModel;
	boolModel m_readableModel;
	boolModel m_writableModel;

	map m_readablePorts;
	map m_writablePorts;


	friend class controllerConnectionDialog;
	friend class instrumentMidiIOView;


signals:
	void readablePortsChanged();
	void writablePortsChanged();
	void modeChanged();


} ;


typedef QList<midiPort *> midiPortList;


#endif
