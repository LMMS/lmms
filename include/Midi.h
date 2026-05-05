/*
 * Midi.h - constants, structs etc. concerning MIDI
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

#ifndef LMMS_MIDI_H
#define LMMS_MIDI_H


namespace lmms
{


enum MidiEventTypes
{
	// messages
	MidiNoteOff = 0x80,
	MidiNoteOn = 0x90,
	MidiKeyPressure = 0xA0,
	MidiControlChange = 0xB0,
	MidiProgramChange = 0xC0,
	MidiChannelPressure = 0xD0,
	MidiPitchBend = 0xE0,
	// system exclusive
	MidiSysEx= 0xF0,
	// system common - never in midi files
	MidiTimeCode= 0xF1,
	MidiSongPosition = 0xF2,
	MidiSongSelect = 0xF3,
	MidiTuneRequest = 0xF6,
	MidiEOX= 0xF7,
	// system real-time - never in midi files
	MidiSync = 0xF8,
	MidiTick = 0xF9,
	MidiStart = 0xFA,
	MidiContinue = 0xFB,
	MidiStop = 0xFC,
	MidiActiveSensing = 0xFE,
	MidiSystemReset = 0xFF,
	// meta event - for midi files only
	MidiMetaEvent = 0xFF
} ;

enum MidiMetaEventTypes
{
	MidiMetaInvalid = 0x00,
	MidiCopyright = 0x02,
	MidiTrackName = 0x03,
	MidiInstName = 0x04,
	MidiLyric = 0x05,
	MidiMarker = 0x06,
	MidiCuePoint = 0x07,
	MidiPortNumber = 0x21,
	MidiEOT = 0x2f,
	MidiSetTempo = 0x51,
	MidiSMPTEOffset = 0x54,
	MidiTimeSignature = 0x58,
	MidiKeySignature = 0x59,
	MidiSequencerEvent = 0x7f,
	MidiMetaCustom = 0x80,
	MidiNotePanning
} ;
using MidiMetaEventType = MidiMetaEventTypes;

enum MidiStandardControllers
{
	MidiControllerBankSelect = 0,
	MidiControllerModulationWheel = 1,
	MidiControllerBreathController = 2,
	MidiControllerFootController = 4,
	MidiControllerPortamentoTime = 5,
	MidiControllerDataEntry = 6,
	MidiControllerMainVolume = 7,
	MidiControllerBalance = 8,
	MidiControllerPan = 10,
	MidiControllerEffectControl1 = 12,
	MidiControllerEffectControl2 = 13,
	MidiControllerSustain = 64,
	MidiControllerPortamento = 65,
	MidiControllerSostenuto = 66,
	MidiControllerSoftPedal = 67,
	MidiControllerLegatoFootswitch = 68,
	MidiControllerRegisteredParameterNumberLSB = 100,
	MidiControllerRegisteredParameterNumberMSB = 101,
	// Channel Mode Messages are controllers too...
	MidiControllerAllSoundOff = 120,
	MidiControllerResetAllControllers = 121,
	MidiControllerLocalControl = 122,
	MidiControllerAllNotesOff = 123,
	MidiControllerOmniOn = 124,
	MidiControllerOmniOff = 125,
	MidiControllerMonoOn = 126,
	MidiControllerPolyOn = 127,

};

enum MidiControllerRegisteredParameterNumbers
{
	MidiPitchBendSensitivityRPN = 0x0000,
	MidiChannelFineTuningRPN = 0x0001,
	MidiChannelCoarseTuningRPN = 0x0002,
	MidiTuningProgramChangeRPN = 0x0003,
	MidiTuningBankSelectRPN = 0x0004,
	MidiModulationDepthRangeRPN = 0x0005,
	MidiNullFunctionNumberRPN = 0x7F7F
};

const int MidiChannelCount = 16;
const int MidiControllerCount = 128;
const int MidiProgramCount = 128;
const int MidiMaxVelocity = 127;
const int MidiDefaultVelocity = MidiMaxVelocity / 2;
const int MidiMaxControllerValue = 127;
const int MidiMaxKey = 127;

const int MidiMaxPanning = 127;
const int MidiMinPanning = -128;

const int MidiMinPitchBend = 0;
const int MidiMaxPitchBend = 16383;


} // namespace lmms

#endif // LMMS_MIDI_H
