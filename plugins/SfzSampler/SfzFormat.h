
#ifndef LMMS_SFZFORMAT_H
#define LMMS_SFZFORMAT_H

#include <QString>

#include <optional>


namespace lmms {


enum LoopMode
{
	LoopContinuous,
};

struct SfzSettingState
{
	std::optional<QString> sampleFile;
	int sampleIndex = -1;
	std::optional<int> lokey;
	std::optional<int> hikey;
	std::optional<int> lovel;
	std::optional<int> hivel;
	std::optional<int> pitch_keycenter;
	std::optional<LoopMode> loop_mode;

	std::optional<float> ampeg_release;
	std::optional<float> ampeg_hold;
	std::optional<float> ampeg_decay;
	std::optional<float> ampeg_sustain;
};

class SfzRegion
{
public:
	SfzSettingState m_settings;
};

class SfzGroup
{
public:
	SfzSettingState m_globalSettings;
	std::vector<SfzRegion> m_regions;
};

class SfzSettings
{
public:
	SfzSettingState m_globalSettings;
	std::vector<SfzGroup> m_groups;
};



}




#endif // LMMS_SFZFORMAT_H
