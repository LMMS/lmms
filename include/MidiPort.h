/*
 * MidiPort.h - abstraction of MIDI ports which are part of LMMS's MIDI-
 *              sequencing system
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
#include "AutomatableModel.h"


class MidiClient;
class MidiEventProcessor;
class MidiPortMenu;
class midiTime;


// class for abstraction of MIDI-port
class MidiPort : public Model, public SerializingObject
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
	typedef QMap<QString, bool> Map;

	enum Modes
	{
		Disabled,	// don't route any MIDI-events (default)
		Input,		// from MIDI-client to MIDI-event-processor
		Output,		// from MIDI-event-processor to MIDI-client
		Duplex		// both directions
	} ;

	MidiPort( const QString & _name,
			MidiClient * _mc,
			MidiEventProcessor * _mep,
			Model * _parent = NULL,
			Modes _mode = Disabled );
	virtual ~MidiPort();

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
						bool _subscribe = true );
	void subscribeWritablePort( const QString & _port,
						bool _subscribe = true );

	const Map & readablePorts() const
	{
		return m_readablePorts;
	}

	const Map & writablePorts() const
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

	MidiPortMenu * m_readablePortsMenu;
	MidiPortMenu * m_writablePortsMenu;


public slots:
	void updateMidiPortMode();


private slots:
	void updateReadablePorts();
	void updateWritablePorts();
	void updateOutputProgram();


private:
	MidiClient * m_midiClient;
	MidiEventProcessor * m_midiEventProcessor;

	Modes m_mode;

	IntModel m_inputChannelModel;
	IntModel m_outputChannelModel;
	IntModel m_inputControllerModel;
	IntModel m_outputControllerModel;
	IntModel m_fixedInputVelocityModel;
	IntModel m_fixedOutputVelocityModel;
	IntModel m_outputProgramModel;
	BoolModel m_readableModel;
	BoolModel m_writableModel;

	Map m_readablePorts;
	Map m_writablePorts;


	friend class ControllerConnectionDialog;
	friend class InstrumentMidiIOView;


signals:
	void readablePortsChanged();
	void writablePortsChanged();
	void modeChanged();

} ;


typedef QList<MidiPort *> MidiPortList;


#endif
