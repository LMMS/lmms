





#include "SfzRegion.h"

#include "Engine.h"
#include "AudioEngine.h"
#include "SampleLoader.h"

#include <QDebug>

namespace lmms
{


SfzRegion::SfzRegion(SfzOpcodeState opcodeState)
	: SfzOpcodeState(opcodeState)
{
}

bool SfzRegion::triggerConditionsMet(const SfzGlobalState& globalState, const SfzTrigger& trigger)
{
	if (trigger.type() == SfzTrigger::Type::NoteOn)
	{
		int triggerKey = trigger.key().value_or(-1);
		int triggerVelocity = trigger.velocity().value_or(-1);

		// `key` opcode
		if (m_key != std::nullopt)
		{
			if (triggerKey != m_key.value_or(-1)) { return false; }
		}
		// `lokey` and `hikey` opcodes
		if (triggerKey > m_hikey.value_or(-1) || triggerKey < m_lokey.value_or(-1)) { return false; }

		// `lovel` and `hivel` opcodes
		if (triggerVelocity > m_hivel.value_or(-1) || triggerVelocity < m_lovel.value_or(-1)) { return false; }

		// If all the contitions passed, return true and spawn a sound
		return true;
	}
	return false;
}

void SfzRegion::processTrigger(const SfzGlobalState& globalState, const SfzTrigger& trigger)
{
	// If the trigger conditions are met, spawn a new sound
	if (triggerConditionsMet(globalState, trigger))
	{
		// Loop through array to find open position
		bool foundOpenPosition = false;
		for (auto& regionPlayState : m_activeSounds)
		{
			if (!regionPlayState.active())
			{
				regionPlayState = SfzRegionPlayState(this, trigger);
				foundOpenPosition = true;
				break;
			}
		}
		if (!foundOpenPosition) { qDebug() << "[SFZ Player] Could not find vacant position in RegionPlayState buffer!"; }
	}

	// Loop through all the active sounds to check if any need to be deactivated/released by the trigger
	for (auto& regionPlayState : m_activeSounds)
	{
		if (regionPlayState.active())
		{
			regionPlayState.processTrigger(trigger);
		}
	}
}


bool SfzRegion::play(SampleFrame* workingBuffer, SampleFrame* temporaryBuffer, const fpp_t frames)
{
	bool anythingPlayed = false;
	for (auto& regionPlayState : m_activeSounds)
	{
		anythingPlayed = anythingPlayed || regionPlayState.play(temporaryBuffer, frames);
		if (anythingPlayed)
		{
			for (f_cnt_t f = 0; f < frames; ++f) { workingBuffer[f] += temporaryBuffer[f]; }
		}
	}
	return anythingPlayed;
}



bool SfzRegion::initializeSample(const QDir& parentDirectory)
{
	if (m_sampleFile == std::nullopt)
	{
		// It's weird for a region not to have a sample defined. That's literally all regions do, play samples, right?
		qDebug() << "[SFZ Player] Warning: `sample` opcode not assigned";
		return false;
	}

	if (auto buffer = gui::SampleLoader::createBufferFromFile(parentDirectory.absoluteFilePath(m_sampleFile.value_or(""))))
	{
		m_sample = Sample(std::move(buffer));
		return true;
	}
	return false;
}


} // namespace lmms