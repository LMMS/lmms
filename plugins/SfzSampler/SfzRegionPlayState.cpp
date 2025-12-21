

#include "SfzRegionPlayState.h"

#include "SfzRegion.h"

namespace lmms
{

SfzRegionPlayState::SfzRegionPlayState(const SfzRegion* region, const SfzTrigger& trigger)
	: m_active(true)
	, m_trigger(trigger)
	, m_region(region)
{
}


bool SfzRegionPlayState::play(SampleFrame* buffer, const fpp_t frames)
{
	// If the sound is not active (note was released, off_by triggered, etc) then don't render any audio
	if (!m_active) { return false; }
	// If the initial m_frameCount is negative, that means the note hasn't started yet
	if (m_frameCount < -static_cast<int>(frames)) { m_frameCount += frames; return false; } // If the note doesn't start in this buffer, don't play anything
	// If the start is within this buffer, get the number of frames until it starts
	const f_cnt_t startFrameOffset = std::max(0, -m_frameCount);
	const f_cnt_t framesToPlay = frames - startFrameOffset;

	// Calculate pitch difference relative to original sample
	const int semitoneDifference = m_trigger.key().value_or(-1) - m_region->m_pitch_keycenter.value_or(-1);
	const float freqRatio = std::exp2(-semitoneDifference * m_region->m_pitch_keytrack.value_or(-1) / 1200.0f);
	
	// Initially render the sample into the buffer
	// TODO what about if other stuff is left in buffer?
	m_region->sample().play(buffer + startFrameOffset, &m_samplePlaybackState, framesToPlay, Sample::Loop::Off, freqRatio);

	for (f_cnt_t f = 0; f < frames; ++f)
	{
		m_frameCount++;
	}

	return true;
}


void SfzRegionPlayState::processTrigger(const SfzTrigger& trigger)
{
	if (trigger.key() == m_trigger.key() && trigger.type() == SfzTrigger::Type::NoteOff)
	{
		m_active = false; // testing
	}
}

} // namespace lmms