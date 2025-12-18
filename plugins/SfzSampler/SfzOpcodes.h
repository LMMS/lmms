
#ifndef LMMS_SFZ_OPCODES_H
#define LMMS_SFZ_OPCODES_H

#include <QString>

namespace lmms
{

enum class SfzOpcodes
{
	Sample,

	Key,
	LoKey,
	HiKey,

	LoVel,
	HiVel,

	PitchKeyCenter,

	AmpEGDelay,
	AmpEGAttack,
	AmpEGHold,
	AmpEGSustain,
	AmpEGDecay,
	AmpEGRelease,
}


std::map<SfzOpcodes, QString> SfzOpcodeNames = {
	{SfzOpcodes::Sample, "sample"},

	{SfzOpcodes::Key, "key"},
	{SfzOpcodes::LoKey, "lokey"},
	{SfzOpcodes::HiKey, "hikey"},

	{SfzOpcodes::LoVel, "lovel"},
	{SfzOpcodes::HiVel, "hivel"},

	{SfzOpcodes::PitchKeyCenter, "pitch_keycenter"},
}


} // namespace lmms


#endif // LMMS_SFZ_OPCODES_H