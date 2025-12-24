



#include "SfzOpcodeState.h"
#include <QDebug>

namespace lmms
{


bool SfzOpcodeState::setOpcodeByStrings(const QString& name, const QString& value)
{
	bool successful = true;

	if (name == "sample")
	{
		m_sampleFile = value;
	}
	else if (name == "default_path")
	{
		m_default_path = value;
	}

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

	else if (name == "lovel")
	{
		m_lovel = value.toInt(&successful);
	}
	else if (name == "hivel")
	{
		m_hivel = value.toInt(&successful);
	}

	else if (name == "pitch_keycenter")
	{
		m_pitch_keycenter = value.toInt(&successful);
		if (!successful) { m_pitch_keycenter = keyNumFromString(value, &successful); }
	}
	else if (name == "pitch_keytrack")
	{
		m_pitch_keytrack = value.toInt(&successful);
		if (!successful) { m_pitch_keytrack = keyNumFromString(value, &successful); }
	}

	else if (name == "ampeg_delay")
	{
		m_ampeg_delay = value.toFloat(&successful);
	}
	else if (name == "ampeg_attack")
	{
		m_ampeg_attack = value.toFloat(&successful);
	}
	else if (name == "ampeg_hold")
	{
		m_ampeg_hold = value.toFloat(&successful);
	}
	else if (name == "ampeg_decay")
	{
		m_ampeg_decay = value.toFloat(&successful);
	}
	else if (name == "ampeg_sustain")
	{
		m_ampeg_sustain = value.toFloat(&successful);
	}
	else if (name == "ampeg_release")
	{
		m_ampeg_release = value.toFloat(&successful);
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