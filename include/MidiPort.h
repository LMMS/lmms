/*
 * MidiPort.h - abstraction of MIDI ports which are part of LMMS' MIDI
 *              sequencing system
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

#ifndef _MIDI_PORT_H
#define _MIDI_PORT_H

#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QMap>

#include "Midi.h"
#include "MidiTime.h"
#include "AutomatableModel.h"


class MidiClient;
class MidiEvent;
class MidiEventProcessor;
class MidiPortMenu;


// class for abstraction of MIDI-port
class MidiPort : public Model, public SerializingObject
{
	Q_OBJECT
	mapPropertyFromModel(int,inputChannel,setInputChannel,m_inputChannelModel);
	mapPropertyFromModel(int,outputChannel,setOutputChannel,m_outputChannelModel);
	mapPropertyFromModel(int,inputController,setInputController,m_inputControllerModel);
	mapPropertyFromModel(int,outputController,setOutputController,m_outputControllerModel);
	mapPropertyFromModel(int,fixedInputVelocity,setFixedInputVelocity,m_fixedInputVelocityModel);
	mapPropertyFromModel(int,fixedOutputVelocity,setFixedOutputVelocity,m_fixedOutputVelocityModel);
	mapPropertyFromModel(int,fixedOutputNote,setFixedOutputNote,m_fixedOutputNoteModel);
	mapPropertyFromModel(int,outputProgram,setOutputProgram,m_outputProgramModel);
	mapPropertyFromModel(int,baseVelocity,setBaseVelocity,m_baseVelocityModel);
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
	typedef Modes Mode;

	MidiPort( const QString& name,
			MidiClient* client,
			MidiEventProcessor* eventProcessor,
			Model* parent = NULL,
			Mode mode = Disabled );
	virtual ~MidiPort();

	void setName( const QString& name );

	Mode mode() const
	{
		return m_mode;
	}

	void setMode( Mode mode );

	bool isInputEnabled() const
	{
		return mode() == Input || mode() == Duplex;
	}

	bool isOutputEnabled() const
	{
		return mode() == Output || mode() == Duplex;
	}

	int realOutputChannel() const
	{
		return outputChannel() - 1;
	}

	void processInEvent( const MidiEvent& event, const MidiTime& time = MidiTime() );
	void processOutEvent( const MidiEvent& event, const MidiTime& time = MidiTime() );


	virtual void saveSettings( QDomDocument& doc, QDomElement& thisElement );
	virtual void loadSettings( const QDomElement& thisElement );

	virtual QString nodeName() const
	{
		return "midiport";
	}

	void subscribeReadablePort( const QString& port, bool subscribe = true );
	void subscribeWritablePort( const QString& port, bool subscribe = true );

	const Map& readablePorts() const
	{
		return m_readablePorts;
	}

	const Map& writablePorts() const
	{
		return m_writablePorts;
	}

	MidiPortMenu* m_readablePortsMenu;
	MidiPortMenu* m_writablePortsMenu;


public slots:
	void updateMidiPortMode();


private slots:
	void updateReadablePorts();
	void updateWritablePorts();
	void updateOutputProgram();


private:
	MidiClient* m_midiClient;
	MidiEventProcessor* m_midiEventProcessor;

	Mode m_mode;

	IntModel m_inputChannelModel;
	IntModel m_outputChannelModel;
	IntModel m_inputControllerModel;
	IntModel m_outputControllerModel;
	IntModel m_fixedInputVelocityModel;
	IntModel m_fixedOutputVelocityModel;
	IntModel m_fixedOutputNoteModel;
	IntModel m_outputProgramModel;
	IntModel m_baseVelocityModel;
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
