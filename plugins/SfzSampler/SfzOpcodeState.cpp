



#include "SfzOpcodeState.h"

namespace lmms
{


bool SfzOpcodeState::setOpcodeByStrings(const QString& name, const QString& value)
{
	bool successful = true;

	if (name == "sample")
	{
		m_sample = value;
	}

	else if (name == "key")
	{
		m_key = value.toInt(&successful);
	}
	else if (name == "lokey")
	{
		m_lokey = value.toInt(&successful);
	}
	else if (name == "hikey")
	{
		m_hikey = value.toInt(&successful);
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

	return successful;
}



} // namespace lmms