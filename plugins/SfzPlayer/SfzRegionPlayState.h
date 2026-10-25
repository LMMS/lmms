/*
 * SfzRegionPlayState.h - Handles generating audio for a single voice based on the configuration of its parent SfzRegion
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


#ifndef LMMS_SFZ_REGION_PLAY_STATE_H
#define LMMS_SFZ_REGION_PLAY_STATE_H

#include "SampleFrame.h"
#include "SfzTrigger.h"
#include "SfzGlobalState.h"
#include "SfzOpcodeState.h"
#include "SfzSampleBuffer.h"

#include "BasicFilters.h"

#include <memory>

namespace lmms
{

class SfzRegion;


class SfzRegionPlayState
{
public:
	// I wish `region` could be const, but it's faster to have the voice update the region's cached modulation values than looping over all the regions elsewhere in the plugin, so for now it will have to be non-const.
	SfzRegionPlayState(SfzRegion* region, const SfzTrigger& trigger, const SfzGlobalState* globalState);
	SfzRegionPlayState() = default; // The default constructor is needed to initialize arrays of the object

	//! Helper function to calculate the base pitch and amplitude so that it doesn't have to be done per-buffer.
	//! This is done once during the constructor, but it must also be done whenever a CC knob
	//! changes, since that could affect the base pitch/amp of the playback.
	void precomputeBaseValues();

	//! Generates the next buffer of audio from this sound. If m_active is false, this function does nothing.
	//! Returns true if any sound was generated, false if the buffer is left untouched
	bool play(SampleFrame* buffer, const f_cnt_t frames);

	//! Handle incoming event to decide whether to deactivate/release
	//! Also handles updating the parent region's precomputed modulation values whenever a midi CC event occurs
	void processTrigger(const SfzTrigger& trigger);

	//! Returns whether this voice is done playing or not. If m_active is false, this voice object is avaiable to be overwritten and used as a new voice.
	bool active() const { return m_active; }

	//! Helper function for calculating the envelope value at the current frame
	float envelopeGenerator(const f_cnt_t delay, const f_cnt_t attack, const f_cnt_t hold, const f_cnt_t decay, const float sustain, const f_cnt_t release) const;

	//! Helper function for calculating the lfo value at the current frame
	float lfoGenerator(const f_cnt_t delay, const f_cnt_t fade, const float freq) const;


private:
	//! Stores whether this object still represents a sound which exists.
	//! This will be true when the sound is active, but will become false once the release has ended or it has been forcefully deactivated.
	bool m_active = false;
	//! Stores whether the note has been released yet
	bool m_released = false;

	//! Stores the current sample rate for convenience
	float m_lmmsSampleRate = 0.0f;

	//! Cache the rate the sample should be played (based on pitch and sample rate, compared to lmms's sample rate)
	// rather than computing it per buffer/frame. This does not include the effect of the pitch env/lfo, since those are computed every frame
	float m_baseFreqRatio = 0.0f;
	//! Similarly cache the base amplitude. The amplitude envelope/lfo will be applied on top of this.
	float m_baseAmplitudeLeft = 1.0f;
	float m_baseAmplitudeRight = 1.0f;

	//! The number of frames since the start of the sound. This may be negative if the region has delay or the note starts partway through a buffer.
	int m_frameCount = 0;
	//! The frame at which the note was released, relative to the start of the note
	int m_releaseFrame = 0;

	//! Stores the current frame index being played in the region's sample. This is a decimal, since interpolation is done to change pitch/sample rate. Double is needed, since float did not provide enough precision and led to the pitch drifting up and down ever so slightly during playback.
	double m_sampleFrame = 0.0;

	//! Some SFZ's utilize filters (e.g, a lowpass which changes cutoff depending on velocity), so each voice needs to have a filter object
	// TODO this assumes 2 channels--is that okay?
	BasicFilters<2> m_filter = {44100}; // This just initializes it to something so this class can be default-initialized in an array, but this will be reinitialized later.

	//! The trigger event which caused this sound
	SfzTrigger m_trigger;
	
	//! The region this sound originated from
	SfzRegion* m_region = nullptr;

	//! A pointer to the global sfz player state. This variable is needed since pitchbending/microtuning requires the frequency
	//! of the key to be updated (potentially every frame), so we need some way to check the current frequency of the key this
	//! voice originated from every buffer
	const SfzGlobalState* m_globalState = nullptr;

	//! The sample object of the parent region
	const SfzSampleBuffer* m_sampleObject = nullptr;
};


} // namespace lmms


#endif // LMMS_SFZ_REGION_PLAY_STATE_H
