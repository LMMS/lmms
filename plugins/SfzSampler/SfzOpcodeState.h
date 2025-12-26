
#ifndef LMMS_SFZ_OPCODE_STATE_H
#define LMMS_SFZ_OPCODE_STATE_H

//#include "SfzOpcodes.h"
#include <QString>
#include <optional>
#include <array>

namespace lmms
{

class SfzOpcodeState
{
public:
	bool setOpcodeByStrings(const QString& name, const QString& value);
	static int keyNumFromString(QString keyString, bool* successful);
	static int ccNumberFromOpcode(const QString& opcode);

//private:
	std::optional<QString> m_sampleFile = std::nullopt;
	std::optional<QString> m_default_path = std::nullopt;

	// TODO maybe these don't all have to be optionals
	std::optional<int> m_key = std::nullopt;
	std::optional<int> m_lokey = 0;
	std::optional<int> m_hikey = 127;

	std::optional<int> m_lovel = 0;
	std::optional<int> m_hivel = 127;

	std::optional<int> m_pitch_keycenter = 60;
	std::optional<int> m_pitch_keytrack = 100;

	std::optional<float> m_ampeg_delay = 0;
	std::optional<float> m_ampeg_attack = 0;
	std::optional<float> m_ampeg_hold = 0;
	std::optional<float> m_ampeg_decay = 0;
	std::optional<float> m_ampeg_sustain = 100;
	std::optional<float> m_ampeg_release = 0.001;
	std::array<float, 128> m_ampeg_release_oncc = {};

	//! Default midi CC values
	std::array<int, 128> m_set_cc = {};

	friend class SfzRegion;
};


} // namespace lmms


#endif // LMMS_SFZ_OPCODE_STATE_H