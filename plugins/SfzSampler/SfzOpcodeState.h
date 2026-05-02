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

#include "SfzOpcodes.h"
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
	Opcode<TriggerType> m_trigger {"trigger", TriggerType::Attack};

	//
	// Key Conditions
	//
	// Adding "key" as an alias here, since setting key=something automatically sets all lokey, hikey, and pitch_keycenter to the same value
	KeyOpcode m_lokey {{"lokey", "key"}, 0};
	KeyOpcode m_hikey {{"hikey", "key"}, 127};

	//
	// Key Switches
	//
	KeyOpcode m_sw_lokey {"sw_lokey", 0};
	KeyOpcode m_sw_hikey {"sw_hikey", 127};
	OptionalKeyOpcode m_sw_last {"sw_last", std::nullopt}; // The SFZ opcode spec says this should default to 0, but I don't think that's right?
	OptionalKeyOpcode m_sw_default {"sw_default", std::nullopt};
	StringOpcode m_sw_label {"sw_label", std::nullopt};

	//
	// Velocity Conditions
	//
	Opcode<int> m_lovel {"lovel", 0};
	Opcode<int> m_hivel {"hivel", 127};

	//
	// Round Robin Conditions
	//
	Opcode<int> m_seq_length {"seq_length", 1};
	Opcode<int> m_seq_position {"seq_position", 1};

	//
	// Random Conditions
	//
	FloatOpcode m_lorand {"lorand", 0.0f};
	FloatOpcode m_hirand {"hirand", 1.0f};


	//
	// Sample playback
	//
	Opcode<int> m_offset {"offset", 0}; // sample play offset in frames

	Opcode<LoopMode> m_loop_mode {"loop_mode", LoopMode::NoLoop};

	//
	// Delay
	//
	ModulatableOpcode m_delay {"delay", 0.0f}; // In seconds
	FloatOpcode m_delay_random {"delay_random", 0.0f};


	//
	// Pitch
	//
	Opcode<int> m_tune {"tune", 0}; // in cents
	KeyOpcode m_pitch_keycenter {{"pitch_keycenter", "key"}, 60}; // "key" as an alias since it automatically sets "lokey, hikey, and pitch_keycenter to the same value
	Opcode<int> m_pitch_keytrack {"pitch_keytrack", 100}; // in cents
	Opcode<int> m_pitch_veltrack {"pitch_veltrack", 0}; // in cents

	//
	// Filter
	//
	Opcode<FilterType> m_fil_type {"fil_type", FilterType::Lowpass2Pole};
	OptionalFloatOpcode m_cutoff {"cutoff", std::nullopt};
	FloatOpcode m_resonance {"resonance", 0.0f};
	Opcode<int> m_fil_veltrack {"fil_veltrack", 0}; // in cents


	//
	// Amplitude
	//
	ModulatableOpcode m_amplitude {"amplitude", 100.0f}; // In percent
	// Overall amplitute velocity modulation
	FloatOpcode m_amp_veltrack {"amp_veltrack", 100.0f};


	//
	// Amplitude Envelope Generator (ampeg)
	//
	EnvelopeOpcodes m_ampeg {"ampeg"};


	//
	// Pitch Envelope Generator (pitcheg)
	//
	EnvelopeOpcodes m_pitcheg {"pitcheg"};


	//
	// Misc Volume
	//
	// Gain is the same as volume. Some opcodes use the word volume, some use gain, both are decibals
	// It seems that "gain" by itself isn't a real opcode, but gain_ccN and gain_onccN are... just to make the logic eaiser, I have gain as a normal alias.
	ModulatableOpcode m_volume {{"volume", "gain", "group_volume"}, 0.0f}; // In decibals
	ModulatableOpcode m_pan {"pan", 0.0f};


	//
	// Midi CC
	//
	// These are handled separately from the other opcodes (for now) because they are more like arrays of opcodes
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