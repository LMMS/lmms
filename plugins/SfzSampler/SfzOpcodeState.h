/*
 * SfzOpcodeState.h - Representation of a <region>-like object in an SFZ file, which stores all the configurations regarding how and when a sample should be played
 *
 * Copyright (c) 2026 Keratin
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

#ifndef LMMS_SFZ_OPCODE_STATE_H
#define LMMS_SFZ_OPCODE_STATE_H

#include <QString>
#include <optional>
#include <array>

namespace lmms
{

class SfzOpcodeState
{
public:
	SfzOpcodeState(); // We need a constructor to initialize things like m_hicc

	//! Given an opcode assignment such as `lokey=45`, passing "m_lokey" and "45" to this function
	//! will take those values and figure out if "lokey" is a valid opcode, and whether "45" is a valid value for it.
	//! If so, it will set the internal member variable corresponding to it, and return false.
	//! If it was unsucessful, it will print an error message and return false.
	bool setOpcodeByStrings(const QString& name, const QString& value);

	//! Helper function for converting strings like "A5" or "B#2" into integers representing keys on the midi keyboard.
	static int stringToKeyNum(QString keyString, bool* successful);
	//! Helper function for converting integers like 24 or 45 representing midi keys back into strings representing the key and octave on the keyboard (essentially the inverse of `stringToKeyNum`)
	static QString keyNumToString(int keyNum);
	//! Helper function for taking an opcode name such as "set_cc45" and extracting the number "45" from it.
	//! If the string does not contain a substring in the form "ccN" where N is a number, it will return print an error and return 0.
	//! Likewise, if the number is outside the range 0-127, it will print an error and return 0.
	static int ccNumberFromOpcode(const QString& opcode);

	// Normal MIDI CC's range from 0 to 127. More advanced SFZ's go beyond that, but for now we cap it at 128. This should be extended in the future.
	static constexpr const int NumMidiCCs = 128;

	/***********************************************************************/
	// SFZ OPCODE DEFINITIONS
	/***********************************************************************/

	//
	// File Paths
	//
	std::optional<QString> m_sampleFile = std::nullopt;
	std::optional<QString> m_default_path = std::nullopt;


	//
	// Trigger Type
	//
	enum class TriggerType
	{
		Attack,
		Release,
		//First, // TODO To be implemented
		//Legato, // TODO To be implemented
		//ReleaseKey // TODO To be implemented
	};
	TriggerType m_trigger = TriggerType::Attack;

	//
	// Key Conditions
	//
	int m_lokey = 0;
	int m_hikey = 127;

	//
	// Key Switches
	//
	int m_sw_lokey = 0;
	int m_sw_hikey = 127;
	std::optional<int> m_sw_last = std::nullopt; // The SFZ opcode spec says this should default to 0, but I don't think that's right?
	std::optional<int> m_sw_default = std::nullopt;
	std::optional<QString> m_sw_label = std::nullopt;

	//
	// Velocity Conditions
	//
	int m_lovel = 0;
	int m_hivel = 127;

	//
	// Round Robin Conditions
	//
	int m_seq_length = 1;
	int m_seq_position = 1;

	//
	// Random Conditions
	//
	float m_lorand = 0.0f;
	float m_hirand = 1.0f;


	//
	// Sample playback
	//
	int m_offset = 0; // sample play offset in frames

	enum class LoopMode
	{
		NoLoop,
		OneShot,
		//LoopContinuous, // To be implemented
		//LoopSustain
	};
	LoopMode m_loop_mode = LoopMode::NoLoop;

	//
	// Delay
	//
	float m_delay = 0; // In seconds
	float m_delay_random = 0;


	//
	// Pitch
	//
	int m_tune = 0; // in cents
	int m_pitch_keycenter = 60;
	int m_pitch_keytrack = 100; // in cents
	int m_pitch_veltrack = 0; // in cents

	//
	// Filter
	//
	enum class FilterType
	{
		Lowpass1Pole,
		Lowpass2Pole,
		Highpass1Pole,
		Highpass2Pole,
		Bandpass2Pole,
		Bandstop2Pole
	};
	FilterType m_fil_type = FilterType::Lowpass2Pole;
	std::optional<float> m_cutoff = std::nullopt;
	float m_resonance = 0.0f;
	int m_fil_veltrack = 0; // in cents


	//
	// Amplitude
	//
	float m_amplitude = 100.0f; // In percent
	std::array<float, NumMidiCCs> m_amplitude_oncc = {};

	// Overall amplitute velocity modulation
	float m_amp_veltrack = 100;


	//
	// Amplitude Envelope Generator (ampeg)
	//
	float m_ampeg_delay = 0.0f;
	float m_ampeg_attack = 0.0f;
	float m_ampeg_hold = 0.0f;
	float m_ampeg_decay = 0.0f;
	float m_ampeg_sustain = 100.0f;
	float m_ampeg_release = 0.001f;
	// Velocity modulation amount
	float m_ampeg_vel2delay = 0.0f;
	float m_ampeg_vel2attack = 0.0f;
	float m_ampeg_vel2hold = 0.0f;
	float m_ampeg_vel2decay = 0.0f;
	float m_ampeg_vel2sustain = 0.0f;
	float m_ampeg_vel2release = 0.0f;
	// Midi CC modulation amounts
	std::array<float, NumMidiCCs> m_ampeg_delay_oncc = {};
	std::array<float, NumMidiCCs> m_ampeg_attack_oncc = {};
	std::array<float, NumMidiCCs> m_ampeg_hold_oncc = {};
	std::array<float, NumMidiCCs> m_ampeg_decay_oncc = {};
	std::array<float, NumMidiCCs> m_ampeg_sustain_oncc = {};
	std::array<float, NumMidiCCs> m_ampeg_release_oncc = {};


	//
	// Misc Volume
	//
	float m_volume = 0.0f; // In decibals
	// Gain is the same as volume. Some opcodes use the word volume, some use gain, both are decibals
	std::array<float, NumMidiCCs> m_gain_oncc = {};
	float m_pan = 0.0f;
	std::array<float, NumMidiCCs> m_pan_oncc = {};


	//
	// Midi CC
	//
	std::array<int, NumMidiCCs> m_locc = {};
	std::array<int, NumMidiCCs> m_hicc = {}; // NOTE this needs to be initialized to contain all 127 in the constructor, since it's hard to fill an array directly within a header file
	//! Default midi CC values
	std::array<int, NumMidiCCs> m_set_cc = {};
	//! Human readable names for the controls
	std::array<std::optional<QString>, NumMidiCCs> m_label_cc = {};

	friend class SfzRegion;
};


} // namespace lmms


#endif // LMMS_SFZ_OPCODE_STATE_H