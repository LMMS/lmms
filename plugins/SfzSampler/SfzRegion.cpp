





#include "SfzRegion.h"

#include "Engine.h"
#include "AudioEngine.h"

#include <QDebug>

namespace lmms
{


SfzRegion::SfzRegion(SfzOpcodeState opcodeState)
	: SfzOpcodeState(opcodeState)
{
	recalculateTotalCCModulation(SfzGlobalState()); // The region objects don't currently have direct access to the global state, so pass in a blank object just to reset the CC modulations to their defaults.

	// Cache which loccN/hiccN opcodes are defined in this region, so that we only have to loop through and check those upon a trigger, not all 128
	for (int i = 0; i < SfzOpcodeState::NumMidiCCs; ++i)
	{
		if (m_locc.at(i) != 0 || m_hicc.at(i) != 127) { m_lohiccDefinedCCNumbers.push_back(i); }
	}
}

bool SfzRegion::triggerConditionsMet(const SfzGlobalState& globalState, const SfzTrigger& trigger)
{
	if (trigger.type() == SfzTrigger::Type::ControlChange) { return false; } // TODO. It is possible for midi CC's to trigger regions, such as using the sustain pedal or on_locc/on_hicc

	// Make sure the trigger type matches
	if (trigger.type() == SfzTrigger::Type::NoteOn && m_trigger != TriggerType::Attack) { return false; }
	if (trigger.type() == SfzTrigger::Type::NoteOff && m_trigger != TriggerType::Release) { return false; }

	// Assuming the trigger has key/vel info (i.e., it's a noteOn/noteOff, not a midi CC event), make sure all the key/vel selectors match
	if (trigger.type() == SfzTrigger::Type::NoteOn || trigger.type() == SfzTrigger::Type::NoteOff)
	{
		int triggerKey = trigger.key().value();
		int triggerVelocity = trigger.velocity().value();

		// Ensure the key was pressed between the `lokey` and `hikey` opcodes
		if (triggerKey > m_hikey || triggerKey < m_lokey) { return false; }

		// And had velocity between `lovel` and `hivel` opcodes
		if (triggerVelocity > m_hivel || triggerVelocity < m_lovel) { return false; }

		// If a keyswitch range was defined, ensure the last pressed valid switch key in that range matches the specified keyswitch for this region
		// The argument `true` at the end signifies that only switch keys will be considered
		// TODO add unit tests
		if (m_sw_last != std::nullopt && globalState.lastKeyPressedInRange(m_sw_lokey, m_sw_hikey, m_sw_default, true) != m_sw_last) { return false; }
	}

	// If midi CC ranges are defined, make sure the current CC values are within range
	// Only loop over the CC's which have lo/hiccN defined, instead of checking all 128 every time
	for (const int i : m_lohiccDefinedCCNumbers)
	{
		const int ccValue = globalState.midiCCValue(i);
		if (ccValue > m_hicc.at(i) || ccValue < m_locc.at(i)) { return false; }
	}

	// If all conditions up until now have passed, that means we're ready to play sound. However, if round-robin is set up, we only do it if it's our turn.
	m_roundRobinCount++;
	if (m_roundRobinCount % m_seq_length != m_seq_position - 1 /*Minus 1 because the opcode is 1-indexed*/) { return false; } // Not our turn

	// If all the contitions passed, return true and spawn a sound
	return true;
}

void SfzRegion::processTrigger(SfzGlobalState& globalState, const SfzTrigger& trigger)
{
	// Notify the global state whether a switch key has been pressed, so that it can correctly track when `sw_last` is met
	if (trigger.type() == SfzTrigger::Type::NoteOn && m_sw_last != std::nullopt && m_sw_last == trigger.key().value())
	{
		globalState.switchKeyPressed(trigger.key().value()); // TODO this can probably be moved somewhere else so that it isn't done for every region (if you have 10000+ regions, it needs to be optimized)
	}

	// Before spawning a sound, do some pre-calculation of the midi CC modulation amounts so that we don't have to do it every buffer
	if (trigger.type() == SfzTrigger::Type::ControlChange)
	{
		recalculateTotalCCModulation(globalState);
	}
}



float SfzRegion::totalCCModulation(const std::array<float, SfzOpcodeState::NumMidiCCs>& ccModulationAmounts, const SfzGlobalState& globalState) const
{
	float total = 0.0f;
	// TODO this may be optimized if we only loop through the CC's which are actually used by the region
	for (int i = 0; i < SfzOpcodeState::NumMidiCCs; ++i)
	{
		total += ccModulationAmounts.at(i) * globalState.midiCCValue(i) / 128.0f;
	}
	return total;
}

void SfzRegion::recalculateTotalCCModulation(const SfzGlobalState& globalState)
{
	m_amplitude_totalCC = totalCCModulation(m_amplitude_oncc, globalState);

	m_ampeg_delay_totalCC = totalCCModulation(m_ampeg_delay_oncc, globalState);
	m_ampeg_attack_totalCC = totalCCModulation(m_ampeg_attack_oncc, globalState);
	m_ampeg_hold_totalCC = totalCCModulation(m_ampeg_hold_oncc, globalState);
	m_ampeg_decay_totalCC = totalCCModulation(m_ampeg_decay_oncc, globalState);
	m_ampeg_sustain_totalCC = totalCCModulation(m_ampeg_sustain_oncc, globalState);
	m_ampeg_release_totalCC = totalCCModulation(m_ampeg_release_oncc, globalState);

	m_gain_totalCC = totalCCModulation(m_gain_oncc, globalState);
	// TODO more
}



bool SfzRegion::initializeSample(const QDir& parentDirectory, SfzSamplePool& samplePool)
{
	if (m_sampleFile == std::nullopt)
	{
		// It's weird for a region not to have a sample defined. That's literally all regions do, play samples, right?
		qDebug() << "[SFZ Player] Warning: `sample` opcode not assigned";
		return false;
	}

	QString path = parentDirectory.absoluteFilePath(m_default_path.value_or("") + m_sampleFile.value());
	// The sample pool handles making sure the same sample isn't loaded twice, which would waste memory
	m_sample = samplePool.loadSample(path);

	return m_sample != nullptr;
}


} // namespace lmms