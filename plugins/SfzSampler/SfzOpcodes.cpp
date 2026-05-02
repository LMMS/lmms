/*
 * SfzOpcodes.cpp - Template specializations for parsing different opcode types
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

#include "SfzOpcodes.h"

#include <QDebug>
#include <QRegularExpression>

namespace lmms
{

template<>
void FloatOpcode::parseFromString(const QString& opcodeName, const QString& opcodeValue, bool* parsed, bool* successful)
{
	if (std::find(m_opcodeNames.begin(), m_opcodeNames.end(), opcodeName) == m_opcodeNames.end()) { return; }
	m_value = opcodeValue.toFloat(successful);
	*parsed = true;
}

// Same function but for optional floats
template<>
void OptionalFloatOpcode::parseFromString(const QString& opcodeName, const QString& opcodeValue, bool* parsed, bool* successful)
{
	if (std::find(m_opcodeNames.begin(), m_opcodeNames.end(), opcodeName) == m_opcodeNames.end()) { return; }
	m_value = opcodeValue.toFloat(successful);
	*parsed = true;
}

template<>
void KeyOpcode::parseFromString(const QString& opcodeName, const QString& opcodeValue, bool* parsed, bool* successful)
{
	if (std::find(m_opcodeNames.begin(), m_opcodeNames.end(), opcodeName) == m_opcodeNames.end()) { return; }
	m_value = opcodeValue.toInt(successful);
	// Integer opcodes can also define keys, which can be specified either by a key number of by a string like e4 or c#5
	if (!*successful) { m_value = stringToKeyNum(opcodeValue, successful); }
	*parsed = true;
}

// Same function but for optional keys
template<>
void OptionalKeyOpcode::parseFromString(const QString& opcodeName, const QString& opcodeValue, bool* parsed, bool* successful)
{
	if (std::find(m_opcodeNames.begin(), m_opcodeNames.end(), opcodeName) == m_opcodeNames.end()) { return; }
	m_value = opcodeValue.toInt(successful);
	// Integer opcodes can also define keys, which can be specified either by a key number of by a string like e4 or c#5
	if (!*successful) { m_value = stringToKeyNum(opcodeValue, successful); }
	*parsed = true;
}

template<>
void StringOpcode::parseFromString(const QString& opcodeName, const QString& opcodeValue, bool* parsed, bool* successful)
{
	if (std::find(m_opcodeNames.begin(), m_opcodeNames.end(), opcodeName) == m_opcodeNames.end()) { return; }
	m_value = opcodeValue;
	*parsed = true;
	*successful = true;
}

// Modulatable Opcodes are a type of float opcode which can be adjusted based on different midi CC knob values
// They have both an initial value defined by their opcodename=value
// but also opcodename_oncc123=modulation_amount
// or opcodename_cc123=modulation_amount
// or opcodenamecc123=modulation_amount
// will make the value be increased proportional to modulation_amount when the midi CC knob 123 is turned up
void ModulatableOpcode::parseFromString(const QString& opcodeName, const QString& opcodeValue, bool* parsed, bool* successful)
{
	for (QString alias : m_opcodeNames)
	{
		if (opcodeName == alias)
		{
			m_value = opcodeValue.toFloat(successful);
			*parsed = true;
		}
		else if (opcodeName.startsWith(alias + "_oncc") || opcodeName.startsWith(alias + "_cc") || opcodeName.startsWith(alias + "cc"))
		{
			value_oncc.at(ccNumberFromOpcode(opcodeName)) = opcodeValue.toFloat(successful);
			*parsed = true;
		}
	}
}


// The modulation of Modulatable opcodes can change whenever a midi CC knob changes. There are 128 of them,
// which is a bit much to keep checking every single frame when generating audio.
// Instead, the current modulation value is only calculated and cached each time a midi CC event is sent, which saves a lot of compute.
void ModulatableOpcode::updateCachedModulation(const std::array<int, NumMidiCCs>& ccValues)
{
	cachedModulation = 0.0f;
	// TODO this may be optimized if we only loop through the CC's which are actually used by the region
	for (int i = 0; i < NumMidiCCs; ++i)
	{
		cachedModulation += value_oncc.at(i) * ccValues.at(i) / 128.0f;
	}
}




// Envelope Generator Opcodes
void EnvelopeOpcodes::parseEnvelopeGeneratorOpcode(const QString& opcode, const QString& value, bool* parsed, bool* successful)
{
	// Pass the opcode name/value to all of the bundled opcodes.
	// If it matches one of them, that's great. If not, they will return and nothing will happen.
	// TODO is this error handling with "successful" correct?
	delay.parseFromString(opcode, value, parsed, successful);
	attack.parseFromString(opcode, value, parsed, successful);
	hold.parseFromString(opcode, value, parsed, successful);
	decay.parseFromString(opcode, value, parsed, successful);
	sustain.parseFromString(opcode, value, parsed, successful);
	release.parseFromString(opcode, value, parsed, successful);
	depth.parseFromString(opcode, value, parsed, successful);

	vel2delay.parseFromString(opcode, value, parsed, successful);
	vel2attack.parseFromString(opcode, value, parsed, successful);
	vel2hold.parseFromString(opcode, value, parsed, successful);
	vel2decay.parseFromString(opcode, value, parsed, successful);
	vel2sustain.parseFromString(opcode, value, parsed, successful);
	vel2release.parseFromString(opcode, value, parsed, successful);
	vel2depth.parseFromString(opcode, value, parsed, successful);
}




//
// Bespoke Enum-like Opcodes
//

template<>
void Opcode<TriggerType>::parseFromString(const QString& opcodeName, const QString& opcodeValue, bool* parsed, bool* successful)
{
	if (std::find(m_opcodeNames.begin(), m_opcodeNames.end(), opcodeName) == m_opcodeNames.end()) { return; }
	*parsed = true;

	if (opcodeValue == "attack") { m_value = TriggerType::Attack; }
	else if (opcodeValue == "release") { m_value = TriggerType::Release; }
	//else if (opcodeValue == "first") { m_value = TriggerType::First; } // TODO To be implemented
	//else if (opcodeValue == "legato") { m_value = TriggerType::Legato; } // TODO To be implemented
	//else if (opcodeValue == "release_key") { m_value = TriggerType::ReleaseKey; } // TODO To be implemented
	else
	{
		qDebug() << "[SFZ Parser] Unknown trigger:" << opcodeValue;
		return;
	}
	*successful = true;
}


template<>
void Opcode<LoopMode>::parseFromString(const QString& opcodeName, const QString& opcodeValue, bool* parsed, bool* successful)
{
	if (std::find(m_opcodeNames.begin(), m_opcodeNames.end(), opcodeName) == m_opcodeNames.end()) { return; }
	*parsed = true;

	if (opcodeValue == "no_loop") { m_value = LoopMode::NoLoop; }
	else if (opcodeValue == "one_shot") { m_value = LoopMode::OneShot; }
	//else if (opcodeValue == "loop_continuous") { m_value = LoopMode::LoopContinuous; } // Not implemented yet, since we don't currently have a way to get loop point data from sample files
	//else if (opcodeValue == "loop_sustain") { m_value = LoopMode::LoopSustain; }
	else
	{
		qDebug() << "[SFZ Parser] Unknown loop_mode:" << opcodeValue;
		return;
	}
	*successful = true;
}


template<>
void Opcode<FilterType>::parseFromString(const QString& opcodeName, const QString& opcodeValue, bool* parsed, bool* successful)
{
	if (std::find(m_opcodeNames.begin(), m_opcodeNames.end(), opcodeName) == m_opcodeNames.end()) { return; }
	*parsed = true;

	if (opcodeValue == "lpf_1p") { m_value = FilterType::Lowpass1Pole; }
	else if (opcodeValue == "lpf_2p") { m_value = FilterType::Lowpass2Pole; }
	else if (opcodeValue == "hpf_1p") { m_value = FilterType::Highpass1Pole; }
	else if (opcodeValue == "hpf_2p") { m_value = FilterType::Highpass2Pole; }
	else if (opcodeValue == "bpf_2p") { m_value = FilterType::Bandpass2Pole; }
	else if (opcodeValue == "brf_2p") { m_value = FilterType::Bandstop2Pole; }
	else
	{
		qDebug() << "[SFZ Parser] Unknown filter type:" << opcodeValue;
		return;
	}
	*successful = true;
}







//
// Helper Functions
//

int ccNumberFromOpcode(const QString& opcode)
{
	// Match for `ccN` where N is a number
	QRegularExpression re("cc\\d+");
	QRegularExpressionMatch match = re.match(opcode);
	if (!match.hasMatch()) { qDebug() << "[SFZ Parser] Unable to find CC number in opcode:" << opcode; return 0; }
	// Get the number from that string
	QString numberPart = match.captured(0).split("cc")[1];
	bool successfulToInt = false;
	int ccNumber = numberPart.toInt(&successfulToInt);
	if (!successfulToInt) { qDebug() << "[SFZ Parser] Unable to convert CC number to int:" << opcode; return 0; }
	// Normal midi lets you choose between 0-127 for the CC number. According to the SFZ format website, ARIA and SFZ 2 allow for extended CC numbers. Most have not been implemented here, so to keep things safe, we cap it and give a warning.
	if (ccNumber < 0 || ccNumber > NumMidiCCs) { qDebug() << "[SFZ Parser] Midi CC number" << ccNumber << "is out of range 0-127. This is not yet implmented."; return 0;}
	return ccNumber;
}


int stringToKeyNum(QString keyString, bool* successful)
{
	keyString = keyString.toLower();
	// The last character is the octave number
	int octave = QString(keyString.back()).toInt(successful);
	if (!*successful) {qDebug() << "[SFZ Parser] Unable to parse key string, Invalid octave number:" << keyString; return -1; }
	// The remaining characters at the start define the key
	QString key = keyString.chopped(1);
	int keyOffset = 0;

	if (key == "c") { keyOffset = 0; } // C is 0 since that's where the octaves change
	else if (key == "c#" || key == "db") { keyOffset = 1; }
	else if (key == "d") { keyOffset = 2; }
	else if (key == "d#" || key == "eb") { keyOffset = 3; }
	else if (key == "e") { keyOffset = 4; }
	else if (key == "f") { keyOffset = 5; }
	else if (key == "f#" || key == "gb") { keyOffset = 6; }
	else if (key == "g") { keyOffset = 7; }
	else if (key == "g#" || key == "ab") { keyOffset = 8; }
	else if (key == "a") { keyOffset = 9; }
	else if (key == "a#" || key == "bb") { keyOffset = 10; }
	else if (key == "b") { keyOffset = 11; }
	else
	{
		*successful = false;
		qDebug() << "[SFZ Parser] Unable to parse key string, Invalid key:" << keyString;
		return -1;
	}
	*successful = true;
	// For some reason, C1 is midi key 24, so eveything is offset by 24
	return 24 + keyOffset + 12 * (octave - 1);
}


QString keyNumToString(int keyNum)
{
	QString octave = QString::number((keyNum - 24) / 12 + 1);
	QString key = "";
	switch (keyNum % 12)
	{
	case 0:
		key = "C";
		break;
	case 1:
		key = "C#";
		break;
	case 2:
		key = "D";
		break;
	case 3:
		key = "D#";
		break;
	case 4:
		key = "E";
		break;
	case 5:
		key = "F";
		break;
	case 6:
		key = "F#";
		break;
	case 7:
		key = "G";
		break;
	case 8:
		key = "G#";
		break;
	case 9:
		key = "A";
		break;
	case 10:
		key = "A#";
		break;
	case 11:
		key = "B";
		break;
	}
	return key + octave;
}


} // namespace lmms
