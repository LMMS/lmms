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
	//! If so, it will set the internal member variable corresponding to it, and return true.
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


	/* Helper structs for opcodes */

	// A base struct for all opcodes, just a name and a value.
	template<typename T>
	struct Opcode
	{
		QString m_opcodeName;
		T m_value;

		Opcode(QString name, T defaultValue) : m_opcodeName(name), m_value(defaultValue) {}
		void setValue(const T& value) { m_value = value; }
		const T& value() const { return m_value; }

		// Function for taking in a string like "pitch_keytrack=1200", split into name/value as "pitch_keytrack", "1200", and updating the value if the name matches.
		virtual void parseFromString(const QString& opcodeName, const QString& opcodeValue, bool* successful) {};
	};

	// Float/decimal opcodes (such as amplitude, panning, etc)
	struct FloatOpcode : Opcode<float>
	{
		FloatOpcode(QString name = "", float defaultValue = 0.0f) : Opcode<float>(name, defaultValue) {};
		//void parseFromString(const QString& opcodeName, const QString& opcodeValue, bool* successful) override;
	};

	// Key opcodes (such as lokey, hikey, pitch_keycenter, etc)
	struct KeyOpcode : Opcode<int>
	{
		KeyOpcode(QString name = "", int defaultValue = 0) : Opcode<int>(name, defaultValue) {};
		//void parseFromString(const QString& opcodeName, const QString& opcodeValue, bool* successful) override;
	};

	// String opcodes (such as sample file path)
	struct StringOpcode : Opcode<std::optional<QString>>
	{
		StringOpcode(QString name, std::optional<QString> defaultValue) : Opcode<std::optional<QString>>(name, defaultValue) {};
		//void parseFromString(const QString& opcodeName, const QString& opcodeValue, bool* successful) override;
	};

	// Many opcodes can be modulated by MIDI CC knobs, so each of them needs an array to store the modulation weights for each one.
	struct ModulatableOpcode : FloatOpcode
	{
		std::array<float, NumMidiCCs> value_oncc = {};
		//! Store the current total midi CC modulation amounts for the different targets, just so that we don't
		// have to recalculate them every buffer, instead only when a trigger occurs.
		float cachedModulation = 0.0f;

		ModulatableOpcode(QString name = "", float defaultValue = 0.0f) : FloatOpcode(name, defaultValue) {};
		// Helper function for parsing these kinds of opcodes, where you have both `opcode` and `opcode_onccN` where N is the midi cc number, so
		// that the code isn't duplicated for every modulatable parameter.
		void parseFromString(const QString& opcodeName, const QString& opcodeValue, bool* successful) override;
		// Helper function to update the cached CC modulation amount every time a midi Control Change trigger occurs
		void updateCachedModulation(const std::array<int, SfzOpcodeState::NumMidiCCs>& ccValues);
	};

	// Additionally, things like amplitude, pitch, and filter freq envelopes and lfo's all have very similar parameters, so it makes sense to put
	// them all together in a handy definition
	struct EnvelopeOpcodes
	{
		// Envelope parameters (these can be modulated by midi CCs)
		ModulatableOpcode delay;
		ModulatableOpcode attack;
		ModulatableOpcode hold;
		ModulatableOpcode decay;
		ModulatableOpcode sustain;
		ModulatableOpcode release;
		// Velocity modulation amount
		FloatOpcode vel2delay;
		FloatOpcode vel2attack;
		FloatOpcode vel2hold;
		FloatOpcode vel2decay;
		FloatOpcode vel2sustain;
		FloatOpcode vel2release;

		// Initialize opcodes with correct names and default values
		EnvelopeOpcodes(QString name)
			: delay(name + "_delay", 0.0f)
			, attack(name + "_attack", 0.0f)
			, hold(name + "_hold", 0.0f)
			, decay(name + "_decay", 0.0f)
			, sustain(name + "_sustain", 100.0f)
			, release(name + "_release", 0.0f)
			, vel2delay(name + "_vel2delay", 0.0f)
			, vel2attack(name + "_vel2attack", 0.0f)
			, vel2hold(name + "_vel2hold", 0.0f)
			, vel2decay(name + "_vel2decay", 0.0f)
			, vel2sustain(name + "_vel2sustain", 0.0f)
			, vel2release(name + "_vel2release", 0.0f)
		{}
		// Helper function to update all of the cached modulations for the individual opcodes
		void updateCachedModulation(const std::array<int, SfzOpcodeState::NumMidiCCs>& ccValues)
		{
			delay.updateCachedModulation(ccValues);
			attack.updateCachedModulation(ccValues);
			hold.updateCachedModulation(ccValues);
			decay.updateCachedModulation(ccValues);
			sustain.updateCachedModulation(ccValues);
			release.updateCachedModulation(ccValues);
		}
		// Helper function for parsing all these envelope generator opcodes, so that the code isn't duplicated for the amplitude, pitch, and filter envelopes
		void parseEnvelopeGeneratorOpcode(const QString& opcode, const QString& value, bool* successful);
	};

	/***********************************************************************/
	// SFZ OPCODE DEFINITIONS
	/***********************************************************************/

	//
	// File Paths
	//
	StringOpcode m_sampleFile {"sample", std::nullopt};
	StringOpcode m_default_path {"default_path", std::nullopt};


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
	Opcode<TriggerType> m_trigger {"trigger", TriggerType::Attack};

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
	EnvelopeOpcodes m_ampeg {"ampeg"};


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



// Template specializations

// Trigger Type
template<> void SfzOpcodeState::Opcode<SfzOpcodeState::TriggerType>::parseFromString(const QString& opcodeName, const QString& opcodeValue, bool* successful);



} // namespace lmms


#endif // LMMS_SFZ_OPCODE_STATE_H