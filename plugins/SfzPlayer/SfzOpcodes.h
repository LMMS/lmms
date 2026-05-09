/*
 * SfzOpcodeState.h - Defining all supported opcodes, along with helper classes/structs/functions for parsing and storing data
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

#ifndef LMMS_SFZ_OPCODES_H
#define LMMS_SFZ_OPCODES_H

#include <QString>
#include <optional>
#include <variant>
#include <array>
#include <vector>

namespace lmms
{


//! Normal MIDI CC's range from 0 to 127. More advanced SFZ's go beyond that, but for now we cap it at 128. This should be extended in the future.
constexpr const int NumMidiCCs = 128;


/* Helper Functions */

//! Helper function for converting strings like "A5" or "B#2" into integers representing keys on the midi keyboard.
int stringToKeyNum(QString keyString, bool* successful);
//! Helper function for converting integers like 24 or 45 representing midi keys back into strings representing the key and octave on the keyboard (essentially the inverse of `stringToKeyNum`)
QString keyNumToString(int keyNum);
//! Helper function for taking an opcode name such as "set_cc45" and extracting the number "45" from it.
//! If the string does not contain a substring in the form "ccN" where N is a number, it will return print an error and return 0.
//! Likewise, if the number is outside the range 0-127, it will print an error and return 0.
int ccNumberFromOpcode(const QString& opcode);



/* Helper structs for opcodes */

//! Due to issues with having different template object pointers in an array, this base class which all opcodes derive from makes things easier
struct BaseOpcode
{
	//! Given a string like "pitch_keytrack=1200" which has been split into name/value as "pitch_keytrack", "1200", this function updates the internal value if the name matches.
	virtual void parseFromString(const QString& opcodeName, const QString& opcodeValue, bool* parsed, bool* successful) = 0;
};
//! A base struct for all opcodes, just a name(s) and a value.
template<typename T>
struct Opcode : BaseOpcode
{
	// Using a vector here, since some opcodes have multiple aliases
	std::vector<QString> m_opcodeNames;
	T m_value;

	Opcode(QString name, T defaultValue) : m_opcodeNames({name}), m_value(defaultValue) {}
	Opcode(std::vector<QString> names, T defaultValue) : m_opcodeNames(names), m_value(defaultValue) {}
	virtual void setValue(const T& value) { m_value = value; }
	virtual const T value() const { return m_value; } // TODO override operators instead

	void parseFromString(const QString& opcodeName, const QString& opcodeValue, bool* parsed, bool* successful) override;
};

//! Float/decimal opcodes (such as amplitude, panning, etc)
using FloatOpcode = Opcode<float>;

//! Some float opcodes may take on a null default value (like filter cutoff)
using OptionalFloatOpcode = Opcode<std::optional<float>>;

//! Integer opcodes (such as lovel, seq_length, seq_position, offset, etc)
using IntOpcode = Opcode<int>;

//! Key opcodes (such as lokey, hikey, pitch_keycenter, etc)
using KeyOpcode = Opcode<int>;

//! Some key opcodes (sw_last, sw_default) make sense to have null default values
using OptionalKeyOpcode = Opcode<std::optional<int>>;

//! String opcodes (such as sample file path)
using StringOpcode = Opcode<std::optional<QString>>;

//! Many opcodes can be modulated by MIDI CC knobs, so each of them needs an array to store the modulation weights for each one.
struct ModulatableOpcode : FloatOpcode
{
	std::array<float, NumMidiCCs> value_oncc = {};
	//! Additionally, each modulation can be mapped with a curve, such as to go from 0 to 1, -1 to 1, something nonlinear, etc, so here's a helper enum for that
	std::array<std::optional<int>, NumMidiCCs> value_curvecc = {}; // This uses std::optional because some opcode curves are meant to default to certain values, (pan_curvecc7 defaults to bipolar), so we need a way to tell if it was set or not.
	//! Also, I never knew this, but apparantly some opcodes can have different modulation types set with opcode_mod=add or opcode_mod=mult, for whether to add the modulation or multiply by it https://sfzformat.com/opcodes/_mod/
	enum class ModulationType
	{
		Add,
		Mult
	};
	std::optional<ModulationType> modulationType; // Once again using std::optional since some opcodes have different defaults, and we need to know if it was intentionally set or not so that we don't override it.
	//! Store the current total midi CC modulation amounts for the different targets, just so that we don't
	// have to recalculate them every buffer, instead only when a trigger occurs.
	float cachedModulation = 0.0f;

	ModulatableOpcode(QString name = "", float defaultValue = 0.0f) : FloatOpcode(name, defaultValue) {};
	ModulatableOpcode(std::vector<QString> names = {}, float defaultValue = 0.0f) : FloatOpcode(names, defaultValue) {};
	//! Redefine the value() function to return the sum of the base opcode value and whatever the modulation is currently. Or, if the modultion type if multiplication, multiply the two.
	const float value() const override
	{
		return (modulationType.value_or(ModulationType::Add) == ModulationType::Add)
			? m_value + cachedModulation
			: m_value * cachedModulation;
	}
	//! Helper function for parsing these kinds of opcodes, where you have both `opcode` and `opcode_onccN` where N is the midi cc number, so
	// that the code isn't duplicated for every modulatable parameter.
	void parseFromString(const QString& opcodeName, const QString& opcodeValue, bool* parsed, bool* successful) override;
	//! Helper function to update the cached CC modulation amount every time a midi Control Change trigger occurs
	void updateCachedModulation(const std::array<int, NumMidiCCs>& ccValues);
};
// Helper class for curve types. This may have to be reworked when custom <curve> headers are added
enum CurveType
{
	Default = 0, // 0 to 1
	Bipolar = 1, // -1 to 1
	Inverted = 2, // 1 to 0
	BipolarInverted = 3, // 1 to -1
	//Concave = 4, // TODO not implemented yet
	//XfinPowerCurve = 5, // TODO not implemented yet
	//XfoutPowerCurve = 6, // TODO not implemented yet
};

// Things like amplitude, pitch, and filter freq envelopes and lfo's all have very similar parameters, so it makes sense to put
// them all together in handy definitions
struct EnvelopeOpcodes
{
	// Envelope parameters (these can be modulated by midi CCs)
	ModulatableOpcode delay;
	ModulatableOpcode attack;
	ModulatableOpcode hold;
	ModulatableOpcode decay;
	ModulatableOpcode sustain;
	ModulatableOpcode release;
	// Depth is only used for pitcheg and fileg, not ampeg, but we have it here anyway
	ModulatableOpcode depth;
	// Velocity modulation amount (TODO: not used yet)
	FloatOpcode vel2delay;
	FloatOpcode vel2attack;
	FloatOpcode vel2hold;
	FloatOpcode vel2decay;
	FloatOpcode vel2sustain;
	FloatOpcode vel2release;
	FloatOpcode vel2depth;

	// Initialize opcodes with correct names and default values
	EnvelopeOpcodes(QString name)
		: delay(name + "_delay", 0.0f)
		, attack(name + "_attack", 0.0f)
		, hold(name + "_hold", 0.0f)
		, decay(name + "_decay", 0.0f)
		, sustain(name + "_sustain", 100.0f)
		, release(name + "_release", 0.0f)
		, depth(name + "_depth", 0.0f)
		, vel2delay(name + "_vel2delay", 0.0f)
		, vel2attack(name + "_vel2attack", 0.0f)
		, vel2hold(name + "_vel2hold", 0.0f)
		, vel2decay(name + "_vel2decay", 0.0f)
		, vel2sustain(name + "_vel2sustain", 0.0f)
		, vel2release(name + "_vel2release", 0.0f)
		, vel2depth(name + "_vel2depth", 0.0f)
	{}
	//! Helper function to update all of the cached modulations for the individual opcodes
	void updateCachedModulation(const std::array<int, NumMidiCCs>& ccValues)
	{
		delay.updateCachedModulation(ccValues);
		attack.updateCachedModulation(ccValues);
		hold.updateCachedModulation(ccValues);
		decay.updateCachedModulation(ccValues);
		sustain.updateCachedModulation(ccValues);
		release.updateCachedModulation(ccValues);
		depth.updateCachedModulation(ccValues);
	}
	//! Helper function for parsing all these envelope generator opcodes
	void parseEnvelopeGeneratorOpcode(const QString& opcode, const QString& value, bool* parsed, bool* successful);
};

// Simple SFZ1 LFO Opcodes, for amplfo, pitchlfo, and fillfo
struct LfoOpcodes
{
	ModulatableOpcode delay;
	ModulatableOpcode fade;
	ModulatableOpcode freq;
	ModulatableOpcode depth;

	// Initialize opcodes with correct names and default values
	LfoOpcodes(QString name)
		: delay(name + "_delay", 0.0f)
		, fade(name + "_fade", 0.0f)
		, freq(name + "_freq", 0.0f)
		, depth(name + "_depth", 0.0f)
	{}
	//! Helper function to update all of the cached modulations for the individual opcodes
	void updateCachedModulation(const std::array<int, NumMidiCCs>& ccValues)
	{
		delay.updateCachedModulation(ccValues);
		fade.updateCachedModulation(ccValues);
		freq.updateCachedModulation(ccValues);
		depth.updateCachedModulation(ccValues);
	}
	//! Helper function for parsing these lfo generator opcodes
	void parseLfoGeneratorOpcode(const QString& opcode, const QString& value, bool* parsed, bool* successful);
};




// Specific Template specializations for Enum-like opcodes


enum class TriggerType
{
	Attack,
	Release,
	//First, // TODO To be implemented
	//Legato, // TODO To be implemented
	//ReleaseKey // TODO To be implemented
};
template<> void Opcode<TriggerType>::parseFromString(const QString& opcodeName, const QString& opcodeValue, bool* parsed, bool* successful);



enum class LoopMode
{
	NoLoop,
	OneShot,
	//LoopContinuous, // To be implemented
	//LoopSustain
};
template<> void Opcode<LoopMode>::parseFromString(const QString& opcodeName, const QString& opcodeValue, bool* parsed, bool* successful);



enum class FilterType
{
	Lowpass1Pole,
	Lowpass2Pole,
	Highpass1Pole,
	Highpass2Pole,
	Bandpass2Pole,
	Bandstop2Pole
};
template<> void Opcode<FilterType>::parseFromString(const QString& opcodeName, const QString& opcodeValue, bool* parsed, bool* successful);



} // namespace lmms


#endif // LMMS_SFZ_OPCODES_H
