
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


	//
	// File Paths
	//
	std::optional<QString> m_sampleFile = std::nullopt;
	std::optional<QString> m_default_path = std::nullopt;


	//
	// Key Trigger
	//
	std::optional<int> m_key = std::nullopt;
	int m_lokey = 0;
	int m_hikey = 127;


	//
	// Velocity Trigger
	//
	int m_lovel = 0;
	int m_hivel = 127;


	//
	// Sample playback
	//
	int m_offset = 0; // sample play offset in frames

	enum class LoopMode
	{
		NoLoop,
		OneShot,
		//LoopContinuous, // To be implemented
		//LoopSustain
	};
	LoopMode m_loop_mode = LoopMode::NoLoop;


	//
	// Pitch
	//
	int m_tune = 0; // in cents
	int m_pitch_keycenter = 60;
	int m_pitch_keytrack = 100; // in cents
	int m_pitch_veltrack = 0; // in cents

	//
	// Filter
	//
	enum class FilterType
	{
		Lowpass1Pole,
		Lowpass2Pole,
		Highpass1Pole,
		Highpass2Pole,
		Bandpass2Pole,
		Bandstop2Pole
	};
	FilterType m_fil_type = FilterType::Lowpass2Pole;
	std::optional<float> m_cutoff = std::nullopt;
	float m_resonance = 0.0f;
	int m_fil_veltrack = 0; // in cents


	//
	// Amplitude
	//
	// Overall amplitute velocity modulation
	float m_amp_veltrack = 100;


	//
	// Amplitude Envelope Generator (ampeg)
	//
	float m_ampeg_delay = 0.0f;
	float m_ampeg_attack = 0.0f;
	float m_ampeg_hold = 0.0f;
	float m_ampeg_decay = 0.0f;
	float m_ampeg_sustain = 100.0f;
	float m_ampeg_release = 0.001f;
	// Velocity modulation amount
	float m_ampeg_vel2delay = 0.0f;
	float m_ampeg_vel2attack = 0.0f;
	float m_ampeg_vel2hold = 0.0f;
	float m_ampeg_vel2decay = 0.0f;
	float m_ampeg_vel2sustain = 0.0f;
	float m_ampeg_vel2release = 0.0f;
	// Midi CC modulation amounts
	std::array<float, 128> m_ampeg_delay_oncc = {};
	std::array<float, 128> m_ampeg_attack_oncc = {};
	std::array<float, 128> m_ampeg_hold_oncc = {};
	std::array<float, 128> m_ampeg_decay_oncc = {};
	std::array<float, 128> m_ampeg_sustain_oncc = {};
	std::array<float, 128> m_ampeg_release_oncc = {};


	//
	// Misc Volume
	//
	float m_volume = 0.0f; // In decibals
	float m_pan = 0.0f;


	//
	// Midi CC
	//
	//! Default midi CC values
	std::array<int, 128> m_set_cc = {};

	friend class SfzRegion;
};


} // namespace lmms


#endif // LMMS_SFZ_OPCODE_STATE_H