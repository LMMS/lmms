/*
 * MidiPort.h - abstraction of MIDI ports which are part of LMMS' MIDI
 *              sequencing system
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

#ifndef LMMS_MIDI_PORT_H
#define LMMS_MIDI_PORT_H

#include <QString>
#include <QList>
#include <QMap>

#include "Midi.h"
#include "TimePos.h"
#include "AutomatableModel.h"

namespace lmms
{

class MidiClient;
class MidiEvent;
class MidiEventProcessor;

namespace gui
{

class MidiPortMenu;
class ControllerConnectionDialog;
class InstrumentMidiIOView;

}


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
	using Map = QMap<QString, bool>;

	enum class Mode
	{
		Disabled,	// don't route any MIDI-events (default)
		Input,		// from MIDI-client to MIDI-event-processor
		Output,		// from MIDI-event-processor to MIDI-client
		Duplex		// both directions
	} ;

	MidiPort( const QString& name,
			MidiClient* client,
			MidiEventProcessor* eventProcessor,
			Model* parent = nullptr,
			Mode mode = Mode::Disabled );
	~MidiPort() override;

	void setName( const QString& name );

	Mode mode() const
	{
		return m_mode;
	}

	void setMode( Mode mode );

	bool isInputEnabled() const
	{
		return mode() == Mode::Input || mode() == Mode::Duplex;
	}

	bool isOutputEnabled() const
	{
		return mode() == Mode::Output || mode() == Mode::Duplex;
	}

	int realOutputChannel() const
	{
		// There's a possibility of outputChannel being 0 ("--"), which is used to keep all
		// midi channels when forwarding. In that case, realOutputChannel will return the
		// default channel 1 (whose value is 0).
		return outputChannel() ? outputChannel() - 1 : 0;
	}

	void processInEvent( const MidiEvent& event, const TimePos& time = TimePos() );
	void processOutEvent( const MidiEvent& event, const TimePos& time = TimePos() );


	void saveSettings( QDomDocument& doc, QDomElement& thisElement ) override;
	void loadSettings( const QDomElement& thisElement ) override;

	QString nodeName() const override
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

	void invalidateCilent();

	gui::MidiPortMenu* m_readablePortsMenu;
	gui::MidiPortMenu* m_writablePortsMenu;


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


	friend class gui::ControllerConnectionDialog;
	friend class gui::InstrumentMidiIOView;


signals:
	void readablePortsChanged();
	void writablePortsChanged();
	void modeChanged();

} ;

using MidiPortList = QList<MidiPort*>;

} // namespace lmms

#endif // LMMS_MIDI_PORT_H
