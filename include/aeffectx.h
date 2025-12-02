/*
 * aeffectx.h - simple header to allow VeSTige compilation and eventually work
 *
 * Copyright (c) 2006 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
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

#ifndef AEFFECTX_H
#define AEFFECTX_H

#include <stdint.h>
#include <type_traits>

// Calling convention
#ifdef _WIN32
#define VST_CALL_CONV __cdecl
#else
#define VST_CALL_CONV 
#endif


template<typename T>
constexpr int32_t CCONST(T a, T b, T c, T d)
{
	static_assert(std::is_convertible<T,int32_t>::value, "int32 compatibility required.");
	return (
		static_cast<int32_t>(a) << 24 |
		static_cast<int32_t>(b) << 16 |
		static_cast<int32_t>(c) << 8 |
		static_cast<int32_t>(d) << 0);
}

constexpr int audioMasterAutomate = 0;
constexpr int audioMasterVersion = 1;
constexpr int audioMasterCurrentId = 2;
constexpr int audioMasterIdle = 3;
constexpr int audioMasterPinConnected = 4;
// unsupported? 5
constexpr int audioMasterWantMidi = 6;
constexpr int audioMasterGetTime = 7;
constexpr int audioMasterProcessEvents = 8;
constexpr int audioMasterSetTime = 9;
constexpr int audioMasterTempoAt = 10;
constexpr int audioMasterGetNumAutomatableParameters = 11;
constexpr int audioMasterGetParameterQuantization = 12;
constexpr int audioMasterIOChanged = 13;
constexpr int audioMasterNeedIdle = 14;
constexpr int audioMasterSizeWindow = 15;
constexpr int audioMasterGetSampleRate = 16;
constexpr int audioMasterGetBlockSize = 17;
constexpr int audioMasterGetInputLatency = 18;
constexpr int audioMasterGetOutputLatency = 19;
constexpr int audioMasterGetPreviousPlug = 20;
constexpr int audioMasterGetNextPlug = 21;
constexpr int audioMasterWillReplaceOrAccumulate = 22;
constexpr int audioMasterGetCurrentProcessLevel = 23;
constexpr int audioMasterGetAutomationState = 24;
constexpr int audioMasterOfflineStart = 25;
constexpr int audioMasterOfflineRead = 26;
constexpr int audioMasterOfflineWrite = 27;
constexpr int audioMasterOfflineGetCurrentPass = 28;
constexpr int audioMasterOfflineGetCurrentMetaPass = 29;
constexpr int audioMasterSetOutputSampleRate = 30;
// unsupported? 31
constexpr int audioMasterGetSpeakerArrangement = 31; // deprecated in 2.4?
constexpr int audioMasterGetVendorString = 32;
constexpr int audioMasterGetProductString = 33;
constexpr int audioMasterGetVendorVersion = 34;
constexpr int audioMasterVendorSpecific = 35;
constexpr int audioMasterSetIcon = 36;
constexpr int audioMasterCanDo = 37;
constexpr int audioMasterGetLanguage = 38;
constexpr int audioMasterOpenWindow = 39;
constexpr int audioMasterCloseWindow = 40;
constexpr int audioMasterGetDirectory = 41;
constexpr int audioMasterUpdateDisplay = 42;
constexpr int audioMasterBeginEdit = 43;
constexpr int audioMasterEndEdit = 44;
constexpr int audioMasterOpenFileSelector = 45;
constexpr int audioMasterCloseFileSelector = 46; // currently unused
constexpr int audioMasterEditFile = 47; // currently unused
constexpr int audioMasterGetChunkFile = 48; // currently unused
constexpr int audioMasterGetInputSpeakerArrangement = 49; // currently unused

constexpr int effFlagsHasEditor = 1;
constexpr int effFlagsCanReplacing = 1 << 4; // very likely
constexpr int effFlagsIsSynth = 1 << 8; // currently unused

constexpr int effOpen = 0;
constexpr int effClose = 1; // currently unused
constexpr int effSetProgram = 2; // currently unused
constexpr int effGetProgram = 3; // currently unused
constexpr int effSetProgramName = 4;
constexpr int effGetProgramName = 5; // currently unused
constexpr int effGetParamLabel = 6;
constexpr int effGetParamDisplay = 7;
constexpr int effGetParamName = 8; // currently unused
constexpr int effSetSampleRate = 10;
constexpr int effSetBlockSize = 11;
constexpr int effMainsChanged = 12;
constexpr int effEditGetRect = 13;
constexpr int effEditOpen = 14;
constexpr int effEditClose = 15;
constexpr int effEditIdle = 19;
constexpr int effEditTop = 20;
constexpr int effGetChunk = 23;
constexpr int effSetChunk = 24;
constexpr int effProcessEvents = 25;
constexpr int effGetProgramNameIndexed = 29;
constexpr int effGetEffectName = 45;
constexpr int effGetVendorString = 47;
constexpr int effGetProductString = 48;
constexpr int effGetVendorVersion = 49;
constexpr int effCanDo = 51; // currently unused
constexpr int effGetVstVersion = 58; // currently unused

constexpr int kEffectMagic = CCONST( 'V', 's', 't', 'P' );
constexpr int kVstLangEnglish = 1;
constexpr int kVstMidiType = 1;

constexpr int kVstTransportChanged = 1;
constexpr int kVstTransportPlaying = 1 << 1;
constexpr int kVstTransportCycleActive = 1 << 2;
constexpr int kVstTransportRecording = 1 << 3; // currently unused
constexpr int kVstPpqPosValid = 1 << 9;
constexpr int kVstTempoValid = 1 << 10;
constexpr int kVstBarsValid = 1 << 11;
constexpr int kVstCyclePosValid = 1 << 12;
constexpr int kVstTimeSigValid = 1 << 13;
constexpr int kVstSmpteValid = 1 << 14; // currently unused
constexpr int kVstClockValid = 1 << 15; // currently unused

// currently unused
constexpr int kVstSmpte24fps = 0;
constexpr int kVstSmpte25fps = 1;
constexpr int kVstSmpte2997fps = 2;
constexpr int kVstSmpte30fps = 3;
constexpr int kVstSmpte2997dfps = 4;
constexpr int kVstSmpte30dfps = 5;
constexpr int kVstSmpteFilm16mm = 6; // very likely
constexpr int kVstSmpteFilm35mm = 7; // very likely
constexpr int kVstSmpte239fps = 10;
constexpr int kVstSmpte249fps = 11;
constexpr int kVstSmpte599fps = 12;
constexpr int kVstSmpte60fps = 13;




class VstMidiEvent
{
public:
	// 00
	int32_t type;
	// 04
	int32_t byteSize;
	// 08
	int32_t deltaFrames;
	// 0c?
	int32_t flags;
	// 10?
	int32_t noteLength;
	// 14?
	int32_t noteOffset;
	// 18
	char midiData[4];
	// 1c?
	char detune;
	// 1d?
	char noteOffVelocity;
	// 1e?
	char reserved1;
	// 1f?
	char reserved2;

} ;




class VstEvent
{
	char dump[sizeof( VstMidiEvent )];

} ;




class VstEvents
{
public:
	// 00
	int32_t numEvents;
	// 04
	void *reserved;
	// 08
	VstEvent* events[1];

} ;


class AEffect
{
public:
	// Never use virtual functions!!!
	// 00-03
	int32_t magic;
	// dispatcher 04-07
	intptr_t (VST_CALL_CONV * dispatcher)( AEffect * , int32_t , int32_t , intptr_t, void * , float );
	// process, quite sure 08-0b
	void (VST_CALL_CONV * process)( AEffect * , float * * , float * * , int32_t );
	// setParameter 0c-0f
	void (VST_CALL_CONV * setParameter)( AEffect * , int32_t , float );
	// getParameter 10-13
	float (VST_CALL_CONV * getParameter)( AEffect * , int32_t );
	// programs 14-17
	int32_t numPrograms;
	// Params 18-1b
	int32_t numParams;
	// Input 1c-1f
	int32_t numInputs;
	// Output 20-23
	int32_t numOutputs;
	// flags 24-27
	int32_t flags;
	// Fill somewhere 28-2b
	void *ptr1;
	void *ptr2;
	// Zeroes 2c-2f 30-33 34-37 38-3b
	char empty3[4 + 4 + 4];
	// 1.0f 3c-3f
	float unknown_float;
	// An object? pointer 40-43
	void *ptr3;
	// Zeroes 44-47
	void *user;
	// Id 48-4b
	int32_t uniqueID;
	// Don't know 4c-4f
	char unknown1[4];
	// processReplacing 50-53
	void (VST_CALL_CONV * processReplacing)( AEffect * , float * * , float * * , int );

} ;




class VstTimeInfo
{
public:
	// 00
	double samplePos;
	// 08
	double sampleRate;
	// unconfirmed 10
	double nanoSeconds;
	// 18
	double ppqPos;
	// 20
	double tempo;
	// 28
	double barStartPos;
	// 30
	double cycleStartPos;
	// 38
	double cycleEndPos;
	// 40
	int32_t timeSigNumerator;
	// 44
	int32_t timeSigDenominator;
	// 48 unused
	int32_t smpteOffset;
	// 4c unused
	int32_t smpteFrameRate;
	// 50? unused, where does this come from?
	int32_t samplesToNextClock;
	// 54
	int32_t flags;

} ;

using audioMasterCallback = intptr_t (VST_CALL_CONV*)(AEffect*, int32_t, int32_t, intptr_t, void*, float);

#endif // AEFFECTX_H
