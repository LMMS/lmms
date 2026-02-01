/*
 * SfzRegionPlayState.cpp - Handles generating audio for a single voice based on the configuration of its parent SfzRegion
 *
 * Copyright (c) 2026 Keratin
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#include "SfzRegionPlayState.h"

#include "SfzRegion.h"

#include "AudioEngine.h"
#include "Engine.h"
#include "lmms_math.h"

#include "MicroTimer.h"
#include <QDebug>

namespace lmms
{

SfzRegionPlayState::SfzRegionPlayState(const SfzRegion* region, const SfzTrigger& trigger)
	: m_active(true)
	, m_filter(Engine::audioEngine()->outputSampleRate())
	, m_trigger(trigger)
	, m_region(region)
{
	const float sampleRate = Engine::audioEngine()->outputSampleRate();
	// Delay the start of the playback by the trigger offset
	m_frameCount -= trigger.frameOffset();
	// And by the delay opcode in seconds
	m_frameCount -= region->m_delay * sampleRate;
	// And any random delay amount
	m_frameCount -= fastRand(1.0f) * region->m_delay_random * sampleRate;

	// Set initial sample start frame offset
	m_sampleFrame += m_region->m_offset;

	// Setup the filter
	switch (m_region->m_fil_type)
	{
	// This is not correct! I'm using the default filters from BasicFilters, but I don't believe they necessarily match the 1 pole vs 2 pole specifications for sfz.
	// For example, the default lowpass is a biquad, which I believe has 2 poles. For now I'm using it for both lowpass types, but it should probably be changed in the future.
	case SfzOpcodeState::FilterType::Lowpass1Pole:
		m_filter.setFilterType(BasicFilters<2>::FilterType::LowPass);
		break;
	case SfzOpcodeState::FilterType::Lowpass2Pole:
		m_filter.setFilterType(BasicFilters<2>::FilterType::LowPass);
		break;
	case SfzOpcodeState::FilterType::Highpass1Pole:
		m_filter.setFilterType(BasicFilters<2>::FilterType::HiPass);
		break;
	case SfzOpcodeState::FilterType::Highpass2Pole:
		m_filter.setFilterType(BasicFilters<2>::FilterType::HiPass);
		break;
	case SfzOpcodeState::FilterType::Bandpass2Pole:
		m_filter.setFilterType(BasicFilters<2>::FilterType::BandPass_CSG); // I am not well versed in the difference between BandPass_CSG and BandPass_CZPG. It seems to be something about how it uses Q/resonance? I'm not sure.
		break;
	case SfzOpcodeState::FilterType::Bandstop2Pole:
		m_filter.setFilterType(BasicFilters<2>::FilterType::Notch);
		break;
	}
}



float SfzRegionPlayState::envelopeGenerator(const f_cnt_t delay, const f_cnt_t attack, const f_cnt_t hold, const f_cnt_t decay, const float sustain, const f_cnt_t release) const
{
	// If the note hasn't started yet, don't do anything
	if (m_frameCount < 0) { return 0.0f; }
	// There is the possibility that the note may have released during attack, decay, etc, so we may need to multiply the release and the normal env values together to get a smooth output
	float normalValue = 1.0f;
	float releaseValue = 1.0f;
	// Calculate the normal envelope value
	if (static_cast<f_cnt_t>(m_frameCount) < delay)
	{
		normalValue = 0.f;
	}
	else if (static_cast<f_cnt_t>(m_frameCount) < delay + attack)
	{
		normalValue = static_cast<float>(m_frameCount - delay) / attack;
	}
	else if (static_cast<f_cnt_t>(m_frameCount) < delay + attack + hold)
	{
		normalValue = 1.0f;
	}
	else if (static_cast<f_cnt_t>(m_frameCount) < delay + attack + hold + decay)
	{
		// Follow an exponential curve from 0 to -90 dB, but stop at the sustain value
		normalValue = std::max(sustain, dbfsToAmp(-90 * static_cast<float>(m_frameCount - (delay + attack + hold)) / decay));
	}
	else
	{
		normalValue = sustain;
	}
	// If the note has already been released, find the release amplitude
	if (m_released && m_frameCount > m_releaseFrame)
	{
		// According to https://sfzformat.com/tutorials/envelope_generators/, release and decay follow exponential curves (linear in dB) which go from 0 to -90 dB
		releaseValue = dbfsToAmp(-90 * static_cast<float>(m_frameCount - m_releaseFrame) / release);
	}
	return releaseValue * normalValue;
}




bool SfzRegionPlayState::play(SampleFrame* buffer, const fpp_t frames)
{
	// If the sound is not active (note was released, off_by triggered, etc) then don't render any audio
	if (!m_active) { return false; }

	// If the initial m_frameCount is negative, that means the note hasn't started yet
	if (m_frameCount < -static_cast<int>(frames)) { m_frameCount += frames; return false; } // If the note doesn't start in this buffer, don't play anything

	// Helper variable
	const float normalizedVelocity = m_trigger.velocity().value() / 127.0f;

	// Calculate pitch difference relative to original sample
	const float semitoneDifference = m_trigger.key().value() - m_region->m_pitch_keycenter;
	// The total pitch depends on 1. the key offset 2. the fine `tune` adjustment 3. the velocity, if pitch_veltrack is nonzero
	const float pitch = semitoneDifference * m_region->m_pitch_keytrack / 100.0f
		+ m_region->m_tune / 100.0f
		+ normalizedVelocity * m_region->m_pitch_veltrack / 100.0f;
	float freqRatio = std::exp2(pitch / 12.0f);

	// Sample rate of sample
	const float sampleSampleRate = m_region->sample()->sampleRate();
	// Sample rate of LMMS
	const float sampleRate = Engine::audioEngine()->outputSampleRate();
	// Play the sample faster/slower to match the correct sample rate
	freqRatio *= sampleSampleRate / sampleRate;

	// Amplitude
	const float amplitude = (m_region->m_amplitude + m_region->m_amplitude_totalCC) / 100.0f; // Amplitude is stored as a percent

	// Amplitude envelope
	const f_cnt_t ampegDelayFrames = (m_region->m_ampeg_delay + m_region->m_ampeg_delay_totalCC) * sampleRate;
	const f_cnt_t ampegAttackFrames = (m_region->m_ampeg_attack + m_region->m_ampeg_attack_totalCC) * sampleRate;
	const f_cnt_t ampegHoldFrames = (m_region->m_ampeg_hold + m_region->m_ampeg_hold_totalCC) * sampleRate;
	const f_cnt_t ampegDecayFrames = (m_region->m_ampeg_decay + m_region->m_ampeg_decay_totalCC) * sampleRate;
	const float ampegSustain = (m_region->m_ampeg_sustain + m_region->m_ampeg_sustain_totalCC) / 100.0f; // Sustain is stored in percent, so divide by 100 to get ratio
	const f_cnt_t ampegReleaseFrames = (m_region->m_ampeg_release + m_region->m_ampeg_release_totalCC) * sampleRate;

	// Amplitude due to velocity
	// If amp_keytrack is 100, then 0 velocity = 0 amp, and 127 velocity = 1.0f amp (as expected)
	// If amp_keytrack is -100, it's the reverse. If amp_keytrack is 0, the volume is not affected by the velocity.
	// Essentially this means lerping between y = x, y = 1, and y = 1 - x, if you think of y being the amp and x being vel/127
	const float ampVelocity = m_region->m_amp_veltrack > 0
		? (normalizedVelocity) * (m_region->m_amp_veltrack / 100) + 1.0f * (1.0f - m_region->m_amp_veltrack / 100)
		: (1.0f - normalizedVelocity) * (m_region->m_amp_veltrack / -100) + 1.0f * (1.0f - m_region->m_amp_veltrack / -100);

	// Amplitude due to volume/gain
	const float ampVolume = dbfsToAmp(m_region->m_volume + m_region->m_gain_totalCC);

	// Panning
	const float pan = (m_region->m_pan + m_region->m_pan_totalCC) / 100;
	const float rightPanAmp = std::min(1.0f, 1.0f + pan);
	const float leftPanAmp = std::min(1.0f, 1.0f - pan);

	// Filter
	const bool filterEnabled = m_region->m_cutoff != std::nullopt;
	// For performance, only update the cutoff frequency/resonance once per buffer
	if (filterEnabled)
	{
		// The cutoff frequency is in hertz, but things like fil_veltrack are defined in cents, so we have to convert them TODO: are we sure it's cents? I saw 20000 being used in Metal GTX which is kind of high for cents (200 octaves?)
		const float filterCutoffPitchOffset = normalizedVelocity * m_region->m_fil_veltrack;
		const float filterCutoff = m_region->m_cutoff.value() + std::exp2(filterCutoffPitchOffset / 12.0f);
		// SFZ has the resonance given in decibals, which is not the same as the resonance passed to the filter, so we have to convert it
		const float q = std::sqrt(2.0f) * dbfsToAmp(m_region->m_resonance); // TODO is this equation correct? I'm sort of basing it off https://www.musicdsp.org/en/latest/Filters/180-cool-sounding-lowpass-with-decibel-measured-resonance.html but I'm not sure.
		m_filter.calcFilterCoeffs(filterCutoff, q);
	}

	for (f_cnt_t f = 0; f < frames; ++f)
	{
		// The amplitude envelope is computed every frame, since doing it once per buffer could result in discontinuities/clicks
		const float ampeg = envelopeGenerator(
			ampegDelayFrames,
			ampegAttackFrames,
			ampegHoldFrames,
			ampegDecayFrames,
			ampegSustain,
			ampegReleaseFrames
		);
		if (filterEnabled) // TODO does this if statement make it faster?
		{
			buffer[f][0] += m_filter.update(m_region->sample()->at(m_sampleFrame, 0) * amplitude * ampeg * ampVelocity * ampVolume * rightPanAmp, 0);
			buffer[f][1] += m_filter.update(m_region->sample()->at(m_sampleFrame, 1) * amplitude * ampeg * ampVelocity * ampVolume * leftPanAmp, 1);
		}
		else
		{
			buffer[f][0] += m_region->sample()->at(m_sampleFrame, 0) * amplitude * ampeg * ampVelocity * ampVolume * rightPanAmp;
			buffer[f][1] += m_region->sample()->at(m_sampleFrame, 1) * amplitude * ampeg * ampVelocity * ampVolume * leftPanAmp;
		}
		m_sampleFrame = std::min(static_cast<float>(m_region->sample()->size()), m_sampleFrame + freqRatio);
		m_frameCount++;
	}


	// Deactive the voice if it has been released and the release has finished
	if (m_released && static_cast<f_cnt_t>(m_frameCount - m_releaseFrame) > ampegReleaseFrames)
	{
		m_active = false;
	}

	// If loop_mode is one_shot, no noteOff signal will ever come to release it, so we need to manually deactivate when we reach the end of the sample
	if (m_region->m_loop_mode == SfzOpcodeState::LoopMode::OneShot && m_sampleFrame >= m_region->sample()->size())
	{
		m_active = false; // TODO should this forcefully decative or just release?
	}

	return true;
}


void SfzRegionPlayState::processTrigger(const SfzTrigger& trigger)
{
	if (m_released) { return; } // If we already released, don't do anything

	if (trigger.type() == SfzTrigger::Type::NoteOff)
	{
		if (m_region->m_loop_mode == SfzOpcodeState::LoopMode::OneShot) { return; } // If one_shot looping is enabled, the whole sample will play regardless of if the note is released

		if (trigger.key() == m_trigger.key())
		{
			m_released = true;
			m_releaseFrame = m_frameCount; // testing
		}
	}
}

} // namespace lmms