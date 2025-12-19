
#ifndef LMMS_SFZ_OPCODE_STATE_H
#define LMMS_SFZ_OPCODE_STATE_H

//#include "SfzOpcodes.h"
#include <QString>
#include <optional>

namespace lmms
{

class SfzOpcodeState
{
public:
	bool setOpcodeByStrings(const QString& name, const QString& value);
	static int keyNumFromString(QString keyString, bool* successful);

//private:
	std::optional<QString> m_sample = std::nullopt;

	std::optional<int> m_key = std::nullopt;
	std::optional<int> m_lokey = 0;
	std::optional<int> m_hikey = 127;

	std::optional<int> m_lovel = 0;
	std::optional<int> m_hivel = 127;

	std::optional<int> m_pitch_keycenter = 60;

	std::optional<float> m_ampeg_delay = 0;
	std::optional<float> m_ampeg_attack = 0;
	std::optional<float> m_ampeg_hold = 0;
	std::optional<float> m_ampeg_decay = 0;
	std::optional<float> m_ampeg_sustain = 100;
	std::optional<float> m_ampeg_release = 0.001;

	friend class SfzRegion;
};


} // namespace lmms


#endif // LMMS_SFZ_OPCODE_STATE_H