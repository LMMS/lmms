/*
 * SfzOpcodeState.cpp - Representation of a <region>-like object in an SFZ file, which stores all the configurations regarding how and when a sample should be played
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

#include "SfzOpcodeState.h"
#include <QDebug>

namespace lmms
{


SfzOpcodeState::SfzOpcodeState()
{
	// Certain arrays need to be initialized here in the constructor, since doing it in the header file is difficult
	m_hicc.fill(127);
}



bool SfzOpcodeState::setOpcodeByStrings(const QString& name, const QString& value)
{
	std::vector<BaseOpcode*> opcodeList = {
		&m_sampleFile,
		&m_default_path,

		&m_trigger,

		&m_lokey,
		&m_hikey,

		&m_sw_lokey,
		&m_sw_hikey,
		&m_sw_last,
		&m_sw_default,
		&m_sw_label,

		&m_lovel,
		&m_hivel,

		&m_seq_length,
		&m_seq_position,

		&m_lorand,
		&m_hirand,

		&m_offset,

		&m_loop_mode,

		&m_delay,
		&m_delay_random,

		&m_tune,
		&m_pitch_keycenter,
		&m_pitch_keytrack,
		&m_pitch_veltrack,

		&m_fil_type,
		&m_cutoff,
		&m_resonance,
		&m_fil_veltrack,

		&m_amplitude,
		&m_amp_veltrack,

		&m_volume,
		&m_pan
	};

	std::vector<EnvelopeOpcodes*> envelopeGeneratorList = {
		&m_ampeg,
		&m_pitcheg
	};

	std::vector<LfoOpcodes*> lfoGeneratorList = {
		&m_amplfo,
		&m_pitchlfo
	};

	bool parsed = false;
	bool successful = true;

	for (auto* opcode : opcodeList)
	{
		//if (parsed) { break; } // Do not break even if the opcode has already been parsed, since some opcodes like "key" are effectively aliases for multiple opcodes (lokey, hikey, and pitch_keytrack)
		opcode->parseFromString(name, value, &parsed, &successful);
	}
	for (auto* envelopeGenerator : envelopeGeneratorList)
	{
		envelopeGenerator->parseEnvelopeGeneratorOpcode(name, value, &parsed, &successful);
	}
	for (auto* lfoGenerator : lfoGeneratorList)
	{
		lfoGenerator->parseLfoGeneratorOpcode(name, value, &parsed, &successful);
	}


	// The per-CC opcodes are handled separately here for now, just to make the code simpler
	if (name.startsWith("locc"))
	{
		int ccNumber = ccNumberFromOpcode(name);
		m_locc.at(ccNumber) = value.toInt(&successful);
		parsed = true;
	}
	else if (name.startsWith("hicc"))
	{
		int ccNumber = ccNumberFromOpcode(name);
		m_hicc.at(ccNumber) = value.toInt(&successful);
		parsed = true;
	}
	else if (name.startsWith("set_cc"))
	{
		int ccNumber = ccNumberFromOpcode(name);
		m_set_cc.at(ccNumber) = value.toInt(&successful);
		parsed = true;
	}
	else if (name.startsWith("set_hdcc"))
	{
		int ccNumber = ccNumberFromOpcode(name);
		m_set_cc.at(ccNumber) = value.toFloat(&successful) * 127;
		parsed = true;
	}
	else if (name.startsWith("label_cc"))
	{
		int ccNumber = ccNumberFromOpcode(name); // TODO have some kind of error handling for invalid cc numbers
		m_label_cc.at(ccNumber) = value;
		parsed = true;
		successful = true;
	}


	//
	// Special cases
	//
	/*
	if (name == "key")
	{
		// Setting "key" on its own is equivalent to setting lokey, hikey, and pitch_keycenter all to the same value
		m_lokey.setValue(value.toInt(&successful));
		m_hikey.setValue(value.toInt(&successful));
		m_pitch_keycenter.setValue(value.toInt(&successful));
		if (!successful) {
			m_lokey.setValue(stringToKeyNum(value, &successful));
			m_hikey.setValue(stringToKeyNum(value, &successful));
			m_pitch_keycenter.setValue(stringToKeyNum(value, &successful));
		}
		parsed = true;
	}
		*/

	/*
	m_ampeg.parseEnvelopeGeneratorOpcode(name, value, &successful);
	//
	// File paths
	//
	if (name == "sample")
	{
		m_sampleFile.setValue(value);
	}
	else if (name == "default_path")
	{
		m_default_path.setValue(value);
	}


	//
	// Trigger Type
	//
	else if (name == "trigger")
	{
		if (value == "attack") { m_trigger.setValue(TriggerType::Attack); }
		else if (value == "release") { m_trigger.setValue(TriggerType::Release); }
		//else if (value == "first") { m_trigger = TriggerType::First; } // TODO To be implemented
		//else if (value == "legato") { m_trigger = TriggerType::Legato; } // TODO To be implemented
		//else if (value == "release_key") { m_trigger = TriggerType::ReleaseKey; } // TODO To be implemented
		else
		{
			qDebug() << "[SFZ Parser] Unknown trigger:" << value;
			return false;
		}
	}


	//
	// Key Conditions
	//
	else if (name == "lokey")
	{
		m_lokey = value.toInt(&successful);
		if (!successful) { m_lokey = stringToKeyNum(value, &successful); }
	}
	else if (name == "hikey")
	{
		m_hikey = value.toInt(&successful);
		if (!successful) { m_hikey = stringToKeyNum(value, &successful); }
	}
	else if (name == "key")
	{
		// Setting "key" on its own is equivalent to setting lokey, hikey, and pitch_keycenter all to the same value
		m_lokey = m_hikey = m_pitch_keycenter = value.toInt(&successful);
		if (!successful) { m_lokey = m_hikey = m_pitch_keycenter = stringToKeyNum(value, &successful); }
	}


	//
	// Key Switches
	//
	else if (name == "sw_lokey")
	{
		m_sw_lokey = value.toInt(&successful);
		if (!successful) { m_sw_lokey = stringToKeyNum(value, &successful); }
	}
	else if (name == "sw_hikey")
	{
		m_sw_hikey = value.toInt(&successful);
		if (!successful) { m_sw_hikey = stringToKeyNum(value, &successful); }
	}
	else if (name == "sw_last")
	{
		m_sw_last = value.toInt(&successful);
		if (!successful) { m_sw_last = stringToKeyNum(value, &successful); }
	}
	else if (name == "sw_default")
	{
		m_sw_default = value.toInt(&successful);
		if (!successful) { m_sw_default = stringToKeyNum(value, &successful); }
	}
	else if (name == "sw_label")
	{
		m_sw_label = value;
	}


	//
	// Velocity Conditions
	//
	else if (name == "lovel")
	{
		m_lovel = value.toInt(&successful);
	}
	else if (name == "hivel")
	{
		m_hivel = value.toInt(&successful);
	}


	//
	// Round Robin Conditions
	//
	else if (name == "seq_length")
	{
		m_seq_length = value.toInt(&successful);
	}
	else if (name == "seq_position")
	{
		m_seq_position = value.toInt(&successful);
	}


	//
	// Random Conditions
	//
	else if (name == "lorand")
	{
		m_lorand = value.toFloat(&successful);
	}
	else if (name == "hirand")
	{
		m_hirand = value.toFloat(&successful);
	}


	//
	// Sample playback options
	//
	else if (name == "offset")
	{
		m_offset = value.toInt(&successful);
	}
	else if (name == "loop_mode")
	{
		if (value == "no_loop") { m_loop_mode = LoopMode::NoLoop; }
		else if (value == "one_shot") { m_loop_mode = LoopMode::OneShot; }
		//else if (value == "loop_continuous") { m_loop_mode = LoopMode::LoopContinuous; } // Not implemented yet, since we don't currently have a way to get loop point data from sample files
		//else if (value == "loop_sustain") { m_loop_mode = LoopMode::LoopSustain; }
		else
		{
			qDebug() << "[SFZ Parser] Unknown loop_mode:" << value;
			return false;
		}
	}


	//
	// Delay
	//
	else if (name == "delay")
	{
		m_delay = value.toFloat(&successful);
	}
	else if (name == "delay_random")
	{
		m_delay_random = value.toFloat(&successful);
	}


	//
	// Pitch
	//
	else if (name == "tune")
	{
		m_tune = value.toInt(&successful);
	}
	else if (name == "pitch_keycenter")
	{
		m_pitch_keycenter = value.toInt(&successful);
		if (!successful) { m_pitch_keycenter = stringToKeyNum(value, &successful); }
	}
	else if (name == "pitch_keytrack")
	{
		m_pitch_keytrack = value.toInt(&successful);
	}
	else if (name == "pitch_veltrack")
	{
		m_pitch_veltrack = value.toInt(&successful);
	}


	//
	// Filter
	//
	else if (name == "fil_type")
	{
		if (value == "lpf_1p") { m_fil_type = FilterType::Lowpass1Pole; }
		else if (value == "lpf_2p") { m_fil_type = FilterType::Lowpass2Pole; }
		else if (value == "hpf_1p") { m_fil_type = FilterType::Highpass1Pole; }
		else if (value == "hpf_2p") { m_fil_type = FilterType::Highpass2Pole; }
		else if (value == "bpf_2p") { m_fil_type = FilterType::Bandpass2Pole; }
		else if (value == "brf_2p") { m_fil_type = FilterType::Bandstop2Pole; }
		else
		{
			qDebug() << "[SFZ Parser] Unknown filter type:" << value;
			return false;
		}
	}
	else if (name == "cutoff")
	{
		m_cutoff = value.toFloat(&successful);
	}
	else if (name == "resonance")
	{
		m_resonance = value.toFloat(&successful);
	}
	else if (name == "fil_veltrack")
	{
		m_fil_veltrack = value.toInt(&successful);
	}

	//
	// Amplitude
	//
	else if (name == "amplitude")
	{
		m_amplitude = value.toFloat(&successful);
	}
	else if (name.startsWith("amplitude_oncc") || name.startsWith("amplitude_cc"))
	{
		m_amplitude_oncc.at(ccNumberFromOpcode(name)) = value.toFloat(&successful);
	}


	else if (name == "amp_veltrack")
	{
		m_amp_veltrack = value.toFloat(&successful);
	}


	//
	// Misc Volume
	//
	else if (name == "volume" || name == "group_volume")
	{
		m_volume = value.toFloat(&successful);
	}
	else if (name.startsWith("gain_oncc") || name.startsWith("gain_cc") || name.startsWith("volume_oncc"))
	{
		m_gain_oncc.at(ccNumberFromOpcode(name)) = value.toFloat(&successful);
	}
	else if (name == "pan")
	{
		m_pan = value.toFloat(&successful);
	}
	else if (name.startsWith("pan_oncc") || name.startsWith("pan_cc"))
	{
		m_pan_oncc.at(ccNumberFromOpcode(name)) = value.toFloat(&successful);
	}


	//
	// Midi CC
	//
	else if (name.startsWith("locc"))
	{
		int ccNumber = ccNumberFromOpcode(name);
		m_locc.at(ccNumber) = value.toInt(&successful);
	}
	else if (name.startsWith("hicc"))
	{
		int ccNumber = ccNumberFromOpcode(name);
		m_hicc.at(ccNumber) = value.toInt(&successful);
	}
	else if (name.startsWith("set_cc"))
	{
		int ccNumber = ccNumberFromOpcode(name);
		m_set_cc.at(ccNumber) = value.toInt(&successful);
	}
	else if (name.startsWith("set_hdcc"))
	{
		int ccNumber = ccNumberFromOpcode(name);
		m_set_cc.at(ccNumber) = value.toFloat(&successful) * 127;
	}
	else if (name.startsWith("label_cc"))
	{
		int ccNumber = ccNumberFromOpcode(name);
		m_label_cc.at(ccNumber) = value;
	}

	else
	{
		qDebug() << "[SFZ Parser] Unknown opcode:" << name;
		return false;
	}
	*/


	if (!successful)
	{
		qDebug() << "[SFZ Parser] Unable to convert value from string:" << name << "=" << value;
		return false;
	}
	else if (!parsed)
	{
		qDebug() << "[SFZ Parser] Unknown opcode:" << name;
		return false;
	}
	return true;
}




} // namespace lmms