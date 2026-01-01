



#include "SfzOpcodeState.h"
#include <QDebug>
#include <QRegularExpression>

namespace lmms
{


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
	// Key Trigger
	//
	else if (name == "key")
	{
		m_key = value.toInt(&successful);
		if (!successful) { m_key = keyNumFromString(value, &successful); }
	}
	else if (name == "lokey")
	{
		m_lokey = value.toInt(&successful);
		if (!successful) { m_lokey = keyNumFromString(value, &successful); }
	}
	else if (name == "hikey")
	{
		m_hikey = value.toInt(&successful);
		if (!successful) { m_hikey = keyNumFromString(value, &successful); }
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
	// Pitch
	//
	else if (name == "tune")
	{
		m_tune = value.toInt(&successful);
	}
	else if (name == "pitch_keycenter")
	{
		m_pitch_keycenter = value.toInt(&successful);
		if (!successful) { m_pitch_keycenter = keyNumFromString(value, &successful); }
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
	else if (name == "pan")
	{
		m_pan = value.toFloat(&successful);
	}


	//
	// Midi CC
	//
	else if (name.startsWith("set_cc"))
	{
		int ccNumber = ccNumberFromOpcode(name);
		m_set_cc.at(ccNumber) = value.toInt(&successful);
	}
	else if (name.startsWith("set_hdcc"))
	{
		int ccNumber = ccNumberFromOpcode(name);
		m_set_cc.at(ccNumber) = value.toFloat(&successful) * 128;
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
	// Match for numbers
	QRegularExpression re("\\d+");
	QRegularExpressionMatch match = re.match(opcode);
	if (!match.hasMatch()) { qDebug() << "[SFZ Parser] Unable to find CC number in opcode:" << opcode; return 0; }
	bool successfulToInt = false;
	int ccNumber = match.captured(0).toInt(&successfulToInt);
	if (!successfulToInt) { qDebug() << "[SFZ Parser] Unable to convert CC number to int:" << opcode; return 0; }
	return ccNumber;
}


int SfzOpcodeState::keyNumFromString(QString keyString, bool* successful)
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



} // namespace lmms