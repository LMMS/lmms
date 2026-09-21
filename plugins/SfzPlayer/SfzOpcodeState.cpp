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

	// There are some special exceptions with some opcodes which need to be handled separately
	// For example, when modulation the `pan` with `pan_oncc10`, according to the SFZ website, the curve type should default to bipolar [-1 to 1], instead of [0 to 1]
	// Actually nevermind, it seems like some SFZ's assume it is the default 0-1 curve? Whatever, it's fine
	/*if (m_pan.value_curvecc.at(10) == std::nullopt)
	{
		m_pan.value_curvecc.at(10) = CurveType::Bipolar;
	}*/
	// Also, apparantly `amplitude` modulation type defaults to multiplication, not addition
	if (m_amplitude.modulationType == std::nullopt)
	{
		m_amplitude.modulationType = ModulatableOpcode::ModulationType::Mult;
	}
}



bool SfzOpcodeState::setOpcodeByStrings(const QString& name, const QString& value)
{
	// These lists hold pointers to all of the opcodes to make them easier to iterate over
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


	// A few of the per-CC opcodes are handled separately here for now, just to make the code simpler
	if (name.startsWith("locc"))
	{
		int ccNumber = ccNumberFromOpcode(name); // TODO have some kind of error handling for invalid cc numbers
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
		int ccNumber = ccNumberFromOpcode(name);
		m_label_cc.at(ccNumber) = value;
		parsed = true;
		successful = true;
	}


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
