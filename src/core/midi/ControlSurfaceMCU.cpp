/*
 * ControlSurfaceMCU.cpp - A controller to receive MIDI MCU control
 *
 * Copyright (c) 2025 - altrouge
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

#include "ControlSurfaceMCU.h"

#include <QDomElement>

#include "AudioEngine.h"
#include "ControlSurface.h"
#include "Engine.h"
#include "MidiClient.h"
#include "Song.h"

namespace lmms {

namespace {
enum class MCUEvents : int16_t
{
	// SELECT_CHANNEL_1 = Octave::Octave_1 + Key::C, // C1
	// SELECT_CHANNEL_2, // C#1
	// SELECT_CHANNEL_3, // D1
	// SELECT_CHANNEL_4, // D#1
	// SELECT_CHANNEL_5, // E1
	// SELECT_CHANNEL_6, // F1
	// SELECT_CHANNEL_7, // F#1
	// SELECT_CHANNEL_8, // G1
	// REC_READY_CHANNEL_1, // C-1
	// REC_READY_CHANNEL_2, // C#-1
	// REC_READY_CHANNEL_3, // D-1
	// REC_READY_CHANNEL_4, // D#-1
	// REC_READY_CHANNEL_5, // E-1
	// REC_READY_CHANNEL_6, // F-1
	// REC_READY_CHANNEL_7, // F#-1
	// REC_READY_CHANNEL_8, // G-1
	// ...
	SHIFT = Octave::Octave_4 + Key::Ais,	  // A#4
	OPTION = Octave::Octave_4 + Key::H,		  // B4
	CONTROL = Octave::Octave_5 + Key::C,	  // C5
	ALT = Octave::Octave_5 + Key::Cis,		  // C#5
	PREVIOUS_FRM = Octave::Octave_6 + Key::C, // C6
	NEXT_FRM = Octave::Octave_6 + Key::Cis,	  // C#6
	LOOP = Octave::Octave_6 + Key::D,		  // D6
	PI_ = Octave::Octave_6 + Key::Dis,		  // D#6
	PO = Octave::Octave_6 + Key::E,			  // E6
	HOME = Octave::Octave_6 + Key::F,		  // F6
	END = Octave::Octave_6 + Key::Fis,		  // F#6
	REWIND = Octave::Octave_6 + Key::G,		  // G6
	FFWD = Octave::Octave_6 + Key::Gis,		  // G#6
	STOP = Octave::Octave_6 + Key::A,		  // A6
	PLAY = Octave::Octave_6 + Key::Ais,		  // A#6
	RECORD = Octave::Octave_6 + Key::H,		  // B6 (German notations)
};

enum class MCUControlChangeEvents : int16_t
{
	VPOT_1_ROTATION = 16,
	VPOT_2_ROTATION = 17,
	VPOT_3_ROTATION = 18,
	VPOT_4_ROTATION = 19,
	VPOT_5_ROTATION = 20,
	VPOT_6_ROTATION = 21,
	VPOT_7_ROTATION = 22,
	VPOT_8_ROTATION = 23,
	JOG_WHEEL = 60,
};

constexpr int keyUpVelocity = 127;
constexpr int keyDownVelocity = 0;

constexpr int CLOCKWISE_1 = 1;
constexpr int CLOCKWISE_2 = 2; // Faster motion.
constexpr int COUNTER_CLOCKWISE_1 = 65;
constexpr int COUNTER_CLOCKWISE_2 = 66; // Faster motion.
} // namespace

void ControlSurfaceMCU::processInEvent(const MidiEvent& event, const TimePos& time, f_cnt_t offset)
{
	switch (event.type())
	{
	case MidiNoteOn: {
		// Only apply the actions on KeyUp.
		if (event.velocity() == keyUpVelocity)
		{
			switch (event.key())
			{
			case static_cast<int>(MCUEvents::RECORD):
				emit m_controlSurface.requestRecord();
				break;
			case static_cast<int>(MCUEvents::LOOP):
				emit m_controlSurface.requestLoop();
				break;
			case static_cast<int>(MCUEvents::STOP):
				emit m_controlSurface.requestStop();
				break;
			case static_cast<int>(MCUEvents::PLAY):
				emit m_controlSurface.requestPlay();
				break;
			default:
				break;
			}
		}
		break;
	}
	case MidiControlChange: {
		switch (event.key())
		{
		case static_cast<int>(MCUControlChangeEvents::JOG_WHEEL):
			switch (event.velocity())
			{
			case CLOCKWISE_1:
				emit m_controlSurface.requestNextInstrumentTrack();
				break;
			case COUNTER_CLOCKWISE_1:
				emit m_controlSurface.requestPreviousInstrumentTrack();
				break;
			}
			break;
		}
		break;
	}
	case MidiPitchBend: {
		// Fader change.
		break;
	}
	default:
		break;
	}
}

void ControlSurfaceMCU::processOutEvent(const MidiEvent& event, const TimePos& time, f_cnt_t offset)
{
}

ControlSurfaceMCU::ControlSurfaceMCU(const QString& device)
	: MidiEventProcessor()
	, m_midiPort(QString::fromStdString("mcu_controller"), Engine::audioEngine()->midiClient(), this)
{
	if (Engine::audioEngine()->midiClient()->readablePorts().indexOf(device) >= 0)
	{
		m_midiPort.subscribeReadablePort(device, true);
	}
}
} // namespace lmms
