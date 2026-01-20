



#include "SfzOpcodeState.h"
#include <QDebug>
#include <QRegularExpression>

namespace lmms
{


SfzOpcodeState::SfzOpcodeState()
{
	// Certain arrays need to be initialized here in the constructor, since doing it in the header file is difficult
	m_hicc.fill(127);
}



bool SfzOpcodeState::setOpcodeByStrings(const QString& name, const QString& value)
{
	bool successful = true;

	//
	// File paths
	//
	if (name == "sample")
	{
		m_sampleFile = value;
	}
	else if (name == "default_path")
	{
		m_default_path = value;
	}


	//
	// Trigger Type
	//
	else if (name == "trigger")
	{
		if (value == "attack") { m_trigger = TriggerType::Attack; }
		else if (value == "release") { m_trigger = TriggerType::Release; }
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
	// Key Trigger
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
	// Velocity Trigger
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
	// Round Robin Trigger
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
	// Amplitude Envelope Generator (ampeg)
	//
	else if (name == "ampeg_delay") { m_ampeg_delay = value.toFloat(&successful); }
	else if (name == "ampeg_attack") { m_ampeg_attack = value.toFloat(&successful); }
	else if (name == "ampeg_hold") { m_ampeg_hold = value.toFloat(&successful); }
	else if (name == "ampeg_decay") { m_ampeg_decay = value.toFloat(&successful); }
	else if (name == "ampeg_sustain") { m_ampeg_sustain = value.toFloat(&successful); }
	else if (name == "ampeg_release") { m_ampeg_release = value.toFloat(&successful); }
	// Velocity modulation amounts
	else if (name == "ampeg_vel2delay") { m_ampeg_vel2delay = value.toFloat(&successful); }
	else if (name == "ampeg_vel2attack") { m_ampeg_vel2attack = value.toFloat(&successful); }
	else if (name == "ampeg_vel2hold") { m_ampeg_vel2hold = value.toFloat(&successful); }
	else if (name == "ampeg_vel2decay") { m_ampeg_vel2decay = value.toFloat(&successful); }
	else if (name == "ampeg_vel2sustain") { m_ampeg_vel2sustain = value.toFloat(&successful); }
	else if (name == "ampeg_vel2release") { m_ampeg_vel2release = value.toFloat(&successful); }
	// Midi CC modulation amounts
	else if (name.startsWith("ampeg_delay_oncc") || name.startsWith("ampeg_delaycc"))
	{
		m_ampeg_delay_oncc.at(ccNumberFromOpcode(name)) = value.toFloat(&successful);
	}
	else if (name.startsWith("ampeg_attack_oncc") || name.startsWith("ampeg_attackcc"))
	{
		m_ampeg_attack_oncc.at(ccNumberFromOpcode(name)) = value.toFloat(&successful);
	}
	else if (name.startsWith("ampeg_hold_oncc") || name.startsWith("ampeg_holdcc"))
	{
		m_ampeg_hold_oncc.at(ccNumberFromOpcode(name)) = value.toFloat(&successful);
	}
	else if (name.startsWith("ampeg_decay_oncc") || name.startsWith("ampeg_decaycc"))
	{
		m_ampeg_decay_oncc.at(ccNumberFromOpcode(name)) = value.toFloat(&successful);
	}
	else if (name.startsWith("ampeg_sustain_oncc") || name.startsWith("ampeg_sustaincc"))
	{
		m_ampeg_sustain_oncc.at(ccNumberFromOpcode(name)) = value.toFloat(&successful);
	}
	else if (name.startsWith("ampeg_release_oncc") || name.startsWith("ampeg_releasecc"))
	{
		m_ampeg_release_oncc.at(ccNumberFromOpcode(name)) = value.toFloat(&successful);
	}


	//
	// Misc Volume
	//
	else if (name == "volume" || name == "group_volume")
	{
		m_volume = value.toFloat(&successful);
	}
	else if (name.startsWith("gain_oncc") || name.startsWith("gain_cc"))
	{
		m_gain_oncc.at(ccNumberFromOpcode(name)) = value.toFloat(&successful);
	}
	else if (name == "pan")
	{
		m_pan = value.toFloat(&successful);
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


	if (!successful)
	{
		qDebug() << "[SFZ Parser] Unable to convert value to number:" << name << "=" << value;
		return false;
	}
	return true;
}



int SfzOpcodeState::ccNumberFromOpcode(const QString& opcode)
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


int SfzOpcodeState::stringToKeyNum(QString keyString, bool* successful)
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


QString SfzOpcodeState::keyNumToString(int keyNum)
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