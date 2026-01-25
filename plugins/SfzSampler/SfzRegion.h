/*
 * SfzRegion.h - Wrapper class for SfzOpcodeState which handles the actual sample, along with helper functions for triggers
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

#ifndef LMMS_SFZ_REGION_H
#define LMMS_SFZ_REGION_H

#include "SfzOpcodeState.h"
#include "SfzGlobalState.h"
#include "SfzTrigger.h"
#include "SfzRegionPlayState.h"
#include "SfzSampleBuffer.h"
#include "SfzSamplePool.h"

#include <QDir>

namespace lmms
{

class SfzRegion : public SfzOpcodeState
{
public:
	SfzRegion(SfzOpcodeState opcodeState);

	//! Returns true if the trigger event matches all of the requirements defined by the opcodes of this region
	//! For example, if lokey=24 and hikey=29, and the trigger is a NoteOn event on key 26, then it will return true
	//! If the trigger does not fall in the key range or velocity range or any other conditions are not met (including global state conditions, 
	//! such as which switch key was last pressed) this will return false.
	//! TODO this method also increments the round robin counter, if applicable. Ideally I feel like this method should be const, but I'm not sure how to best organize it.
	bool triggerConditionsMet(const SfzGlobalState& globalState, const SfzTrigger& trigger);

	//! Updates cached CC modulations, and helps update keyswitch states
	void processTrigger(SfzGlobalState& globalState, const SfzTrigger& trigger);

	//! Load the sample file given by the `sample` opcode into m_sample.
	//! The sample path is treated as relative to the path to the sfz file, so the parent directory is also needed
	//! Returns true if successful
	bool initializeSample(const QDir& parentDirectory, SfzSamplePool& samplePool);

	const SfzSampleBuffer* sample() const { return m_sample; }

private:
	//! Pointer to sample object to be played. The sample file path is defined in the `sample` opcode, but the data needs to be loaded first
	//! The actual sample objects are stored in a shared pool, SfzSamplePool, so that if multiple of the same sample are loaded, they don't waste memory.
	const SfzSampleBuffer* m_sample = nullptr;

	//! In order to do round robin, the region needs to keep track of how many notes it has played in its lifetime. Or rather, the number of notes it *would* have played if it weren't restricted to only play a note when the round-robin counter hit the right numbers.
	int m_roundRobinCount = 0;

	//! Store the current total midi CC modulation amounts for the different targets, just so that we don't
	// have to recalculate them every buffer, instead only when a trigger occurs.
	float m_amplitude_totalCC = 0.0f;

	float m_ampeg_delay_totalCC = 0.0f;
	float m_ampeg_attack_totalCC = 0.0f;
	float m_ampeg_hold_totalCC = 0.0f;
	float m_ampeg_decay_totalCC = 0.0f;
	float m_ampeg_sustain_totalCC = 0.0f;
	float m_ampeg_release_totalCC = 0.0f;

	float m_gain_totalCC = 0.0f;

	//! When checking whether all the current CC values fall between loccN and hiccN, it's useful to only check the ones where lo/hiccN is actually defined, not all 128
	//! This vector stores a list of which CC numbers have lo/hiccN opcodes in this region
	std::vector<int> m_lohiccDefinedCCNumbers;

	//! Helper function to calculate the total modulation of all midi CC controllers on a parameter. Essentially it just multiplies the modulation amounts by the current CC values and adds it all up.
	float totalCCModulation(const std::array<float, SfzOpcodeState::NumMidiCCs>& ccModulationAmounts, const SfzGlobalState& globalState) const;
	void recalculateTotalCCModulation(const SfzGlobalState& globalState);

	friend class SfzRegionPlayState; // TODO this was just to make it easy but...?
};


} // namespace lmms


#endif // LMMS_SFZ_REGION_H