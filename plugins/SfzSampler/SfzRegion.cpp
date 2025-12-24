





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
		int triggerKey = trigger.key().value();
		int triggerVelocity = trigger.velocity().value();

		// `key` opcode
		if (m_key != std::nullopt)
		{
			if (triggerKey != m_key.value()) { return false; }
		}
		// `lokey` and `hikey` opcodes
		if (triggerKey > m_hikey.value() || triggerKey < m_lokey.value()) { return false; }

		// `lovel` and `hivel` opcodes
		if (triggerVelocity > m_hivel.value() || triggerVelocity < m_lovel.value()) { return false; }

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
		for (size_t i = 0; i <= m_activeSounds.size(); ++i)
		{
			auto& regionPlayState = m_activeSounds[i];
			if (!regionPlayState.active())
			{
				regionPlayState = SfzRegionPlayState(this, trigger);
				// If this new index is above the current max active index, update it
				m_maxActiveIndex = std::max(m_maxActiveIndex, i);
				foundOpenPosition = true;
				break;
			}
		}
		if (!foundOpenPosition) { qDebug() << "[SFZ Player] Could not find vacant position in RegionPlayState buffer!"; }
	}

	// Loop through all the active sounds to check if any need to be deactivated/released by the trigger
	for (size_t i = 0; i <= m_maxActiveIndex; ++i)
	{
		auto& regionPlayState = m_activeSounds[i];

		if (regionPlayState.active())
		{
			regionPlayState.processTrigger(trigger);
			// If this was the max active index and the trigger caused it to deactivate, figure out what the next active index is
			if (!regionPlayState.active() && i == m_maxActiveIndex) { recalculateMaxActiveIndex(); }
		}
	}
}


bool SfzRegion::play(SampleFrame* workingBuffer, const fpp_t frames)
{
	bool anythingPlayed = false;
	for (size_t i = 0; i <= m_maxActiveIndex; ++i)
	{
		auto& regionPlayState = m_activeSounds[i];
		if (!regionPlayState.active()) { continue; }

		anythingPlayed = regionPlayState.play(workingBuffer, frames) || anythingPlayed;
		// If the play state deactivated during playback, and this was the max active index, figure out what the new max active index is
		if (!regionPlayState.active() && i == m_maxActiveIndex) { recalculateMaxActiveIndex(); }
	}
	return anythingPlayed;
}


void SfzRegion::recalculateMaxActiveIndex()
{
	// Loop backward from the old max active index to find the next play state which is active
	while (m_maxActiveIndex > 0)
	{
		if (m_activeSounds[m_maxActiveIndex].active()) { return; }
		else { m_maxActiveIndex--; }
	}
}


bool SfzRegion::initializeSample(const QDir& parentDirectory)
{
	if (m_sampleFile == std::nullopt)
	{
		// It's weird for a region not to have a sample defined. That's literally all regions do, play samples, right?
		qDebug() << "[SFZ Player] Warning: `sample` opcode not assigned";
		return false;
	}

	// TODO is simply adding the default path sufficient?
	if (auto buffer = gui::SampleLoader::createBufferFromFile(parentDirectory.absoluteFilePath(m_default_path.value_or("") + m_sampleFile.value())))
	{
		m_sample = SfzSampleBuffer(buffer->data(), buffer->size(), buffer->sampleRate());
		return true;
	}
	return false;
}


} // namespace lmms