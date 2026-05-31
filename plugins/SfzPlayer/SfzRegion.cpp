/*
 * SfzRegion.cpp - Wrapper class for SfzOpcodeState which handles the actual sample, along with helper functions for triggers
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
	for (int i = 0; i < NumMidiCCs; ++i)
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
		const int triggerKey = trigger.key().value();
		const int triggerVelocity = trigger.velocity().value();

		// Ensure the key was pressed between the `lokey` and `hikey` opcodes
		// TODO this can be removed now that SfzRegionManager handles lookup tables for regions by key
		if (triggerKey > m_hikey || triggerKey < m_lokey) { return false; }

		// And had velocity between `lovel` and `hivel` opcodes
		if (triggerVelocity > m_hivel || triggerVelocity < m_lovel) { return false; }

		// If a keyswitch range was defined, ensure the last pressed valid switch key in that range matches the specified keyswitch for this region
		// TODO add unit tests
		if (m_sw_last != std::nullopt && globalState.lastSwitchKeyPressedInRange(m_sw_lokey, m_sw_hikey, m_sw_default.m_value /*Accessing m_value due to issues with implicit cast to std::optional*/) != m_sw_last) { return false; }
	}

	// If the region uses lorand/hirand, the current random value stored in the global state (updated every trigger) is compared with the range
	// Note: the upper bound is inclusive, so lorand=0.2 hirand=0.4 will be triggered by any rand value >0.2 or <=0.4
	if (globalState.rand() > m_hirand || globalState.rand() <= m_lorand) { return false; }

	// If midi CC ranges are defined, make sure the current CC values are within range
	// Only loop over the CC's which have lo/hiccN defined, instead of checking all 128 every time
	for (const int i : m_lohiccDefinedCCNumbers)
	{
		const int ccValue = globalState.midiCCValue(i);
		if (ccValue > m_hicc.at(i) || ccValue < m_locc.at(i)) { return false; }
	}

	// If all conditions up until now have passed, that means we're ready to play sound. However, if round-robin is set up, we only do it if it's our turn.
	m_roundRobinCount++; // TODO it would be nice if this function could be const and we didn't have to update this here. idk.
	if (m_roundRobinCount % m_seq_length != m_seq_position - 1 /*Minus 1 because the opcode is 1-indexed*/) { return false; } // Not our turn

	// If all the contitions passed, return true and spawn a sound
	return true;
}


void SfzRegion::recalculateTotalCCModulation(const SfzGlobalState& globalState)
{
	m_amplitude.updateCachedModulation(globalState.midiCCValues());
	m_volume.updateCachedModulation(globalState.midiCCValues());
	m_pan.updateCachedModulation(globalState.midiCCValues());
	m_delay.updateCachedModulation(globalState.midiCCValues());

	m_ampeg.updateCachedModulation(globalState.midiCCValues());
	m_pitcheg.updateCachedModulation(globalState.midiCCValues());

	m_amplfo.updateCachedModulation(globalState.midiCCValues());
	m_pitchlfo.updateCachedModulation(globalState.midiCCValues());
}


bool SfzRegion::initializeSample(const QDir& parentDirectory, SfzSamplePool& samplePool, bool* sampleInPool)
{
	if (m_sampleFile == std::nullopt)
	{
		// It's weird for a region not to have a sample defined. That's literally all regions do, play samples, right?
		qDebug() << "[SFZ Player] Warning: `sample` opcode not assigned";
		return false;
	}

	// There are some special sample keywords for simple wave shapes, such as *sine, *triangle, *silence, etc
	// If one of them is used, we don't load a sample file, instead leave m_sample as nullptr and set m_basicWaveShape
	// to let the region know that we are doing a procedurally generated wave
	if (m_sampleFile == "*sine") { m_basicWaveShape = SfzBasicWaves::Shape::Sine; }
	else if (m_sampleFile == "*saw") { m_basicWaveShape = SfzBasicWaves::Shape::Saw; }
	else if (m_sampleFile == "*square") { m_basicWaveShape = SfzBasicWaves::Shape::Square; }
	else if (m_sampleFile == "*triangle") { m_basicWaveShape = SfzBasicWaves::Shape::Triangle; }
	else if (m_sampleFile == "*tri") { m_basicWaveShape = SfzBasicWaves::Shape::Triangle; }
	else if (m_sampleFile == "*noise") { m_basicWaveShape = SfzBasicWaves::Shape::Noise; }
	else if (m_sampleFile == "*silence") { m_basicWaveShape = SfzBasicWaves::Shape::Silence; }
	else
	{
		// If it's not a basic wave, load the real sample file.
		QDir defaultDirectory = QDir(parentDirectory.absoluteFilePath(m_default_path.value_or(""))); // TODO
		QString path = defaultDirectory.absoluteFilePath(m_sampleFile); // TODO
		// The sample pool handles making sure the same sample isn't loaded twice, which would waste memory
		m_sample = samplePool.loadSample(path, sampleInPool); // sampleInPool is passed so that we can tell the SfzPlayer if it actually needed to load it from disk or whether it was previously loaded and could be retrieved.

		return m_sample != nullptr;
	}
	return true;
}


} // namespace lmms
