

#include "SfzRegionPlayState.h"

#include "SfzRegion.h"

#include "lmms_math.h"

#include "MicroTimer.h"
#include <QDebug>

namespace lmms
{

SfzRegionPlayState::SfzRegionPlayState(const SfzRegion* region, const SfzTrigger& trigger)
	: m_active(true)
	, m_trigger(trigger)
	, m_region(region)
{
}



float SfzRegionPlayState::envelopeGenerator(const f_cnt_t delay, const f_cnt_t attack, const f_cnt_t hold, const f_cnt_t decay, const float sustain, const f_cnt_t release) const
{
	// If the note hasn't started yet, don't do anything
	if (m_frameCount < 0) { return 0.0f; }
	// If the note has already been released, return the release amplitude
	if (m_released && m_frameCount > m_releaseFrame)
	{
		// According to https://sfzformat.com/tutorials/envelope_generators/, release and decay follow exponential curves (linear in dB) which go from 0 to -90 dB
		return dbfsToAmp(-90 * static_cast<float>(m_frameCount - m_releaseFrame) / release);
	}
	// If it hasn't been released yet, do the normal envelope shape
	else if (static_cast<f_cnt_t>(m_frameCount) < delay)
	{
		return 0.f;
	}
	else if (static_cast<f_cnt_t>(m_frameCount) < delay + attack)
	{
		return static_cast<float>(m_frameCount - delay) / attack;
	}
	else if (static_cast<f_cnt_t>(m_frameCount) < delay + attack + hold)
	{
		return 1.0f;
	}
	else if (static_cast<f_cnt_t>(m_frameCount) < delay + attack + hold + decay)
	{
		// Follow an exponential curve from 0 to -90 dB, but stop at the sustain value
		return std::max(sustain, dbfsToAmp(-90 * static_cast<float>(m_frameCount - (delay + attack + hold)) / decay));
	}
	else
	{
		return sustain;
	}
}




bool SfzRegionPlayState::play(SampleFrame* buffer, const fpp_t frames)
{
	// If the sound is not active (note was released, off_by triggered, etc) then don't render any audio
	if (!m_active) { return false; }

	static int totalMicroseconds = 0;
	static int totalSquaredMicroseconds = 0;
	static int totalCalls = 0;
	static int minElapsed = 10000000;
	static int maxElapsed = 0;
	MicroTimer profiler;

	// If the initial m_frameCount is negative, that means the note hasn't started yet
	if (m_frameCount < -static_cast<int>(frames)) { m_frameCount += frames; return false; } // If the note doesn't start in this buffer, don't play anything
	// If the start is within this buffer, get the number of frames until it starts
	const f_cnt_t startFrameOffset = std::max(0, -m_frameCount);
	const f_cnt_t framesToPlay = frames - startFrameOffset;

	// Calculate pitch difference relative to original sample
	const int semitoneDifference = m_trigger.key().value() - m_region->m_pitch_keycenter.value();
	const float freqRatio = std::exp2(semitoneDifference * m_region->m_pitch_keytrack.value() / 1200.0f);

	// Sample rate of sample
	const float sampleSampleRate = m_region->sample().sampleRate();
	// Sample rate of LMMS
	const float sampleRate = Engine::audioEngine()->outputSampleRate();
	// Play the sample faster/slower to match the correct sample rate
	freqRatio *= sampleSampleRate / sampleRate;

	// Amplitude envelope
	const f_cnt_t ampegDelayFrames = m_region->m_ampeg_delay.value() * sampleRate;
	const f_cnt_t ampegAttackFrames = m_region->m_ampeg_attack.value() * sampleRate;
	const f_cnt_t ampegHoldFrames = m_region->m_ampeg_hold.value() * sampleRate;
	const f_cnt_t ampegDecayFrames = m_region->m_ampeg_decay.value() * sampleRate;
	const float ampegSustain = m_region->m_ampeg_sustain.value() / 100.0f; // Sustain is stored in percent, so divide by 100 to get ratio
	const f_cnt_t ampegReleaseFrames = m_region->m_ampeg_release.value() * sampleRate;

	for (f_cnt_t f = 0; f < frames; ++f)
	{
		const float ampeg = envelopeGenerator(
			ampegDelayFrames,
			ampegAttackFrames,
			ampegHoldFrames,
			ampegDecayFrames,
			ampegSustain,
			ampegReleaseFrames
		);
		buffer[f][0] += m_region->sample().at(m_sampleFrame, 0) * ampeg;
		buffer[f][1] += m_region->sample().at(m_sampleFrame, 1) * ampeg;
		m_sampleFrame = std::min(static_cast<float>(m_region->sample().size()), m_sampleFrame + freqRatio);
		m_frameCount++;
	}

	// Deactive the voice if it has been released and the release has finished
	if (m_released && static_cast<f_cnt_t>(m_frameCount - m_releaseFrame) > ampegReleaseFrames)
	{
		m_active = false;
	}

	int elapsed = profiler.elapsed(); totalMicroseconds += elapsed; totalSquaredMicroseconds += elapsed * elapsed; totalCalls++; minElapsed = std::min(minElapsed, elapsed); maxElapsed = std::max(maxElapsed, elapsed);
	float mean = 1.0f * totalMicroseconds / totalCalls, variance = (1.0f * totalSquaredMicroseconds - 1.0f * totalMicroseconds * totalMicroseconds / totalCalls / totalCalls) / totalCalls;
	qDebug() << "SfzRegionPlayState::play profiler:" << elapsed << "Min:" << minElapsed << "Max:" << maxElapsed << "Total calls" << totalCalls << "Mean:" << mean << "Stdev:" << std::sqrt(variance) << "Stdev of mean:" << sqrt(variance / totalCalls);

	return true;
}


void SfzRegionPlayState::processTrigger(const SfzTrigger& trigger)
{
	if (trigger.key() == m_trigger.key() && trigger.type() == SfzTrigger::Type::NoteOff)
	{
		m_released = true;
		m_releaseFrame = m_frameCount; // testing
	}
}

} // namespace lmms