/*
 * SfzSampler.h - Declaration of SfzSampler class, Simple SFZ instrument loader/editor
 *
 * Copyright (c) 2023 Daniel Kauss Serna <daniel.kauss.serna@gmail.com>
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

#ifndef LMMS_SFZSAMPLER_H
#define LMMS_SFZSAMPLER_H

#include "AutomatableModel.h"
#include "ComboBoxModel.h"
#include "Instrument.h"
#include "Note.h"
#include "Sample.h"
#include "SfzSamplerView.h"
#include "SfzParser.h"
#include "SfzRegion.h"
#include "SfzRegionPlayState.h"
#include "SfzGlobalState.h"
#include "SfzControlsConfig.h"
#include "SfzSamplePool.h"

namespace lmms {

class SfzSampler : public Instrument
{
	Q_OBJECT

public:
	SfzSampler(InstrumentTrack* instrumentTrack);

	void playNote(NotePlayHandle* handle, SampleFrame* workingBuffer) override;
	void deleteNotePluginData(NotePlayHandle* handle) override;

	void play(SampleFrame* workingBuffer) override;
	bool handleMidiEvent(const MidiEvent& event, const TimePos& time = TimePos(), f_cnt_t offset = 0) override;

	void saveSettings(QDomDocument& document, QDomElement& element) override;
	void loadSettings(const QDomElement& element) override;

	void loadFile(const QString& filePath) override;

	QString nodeName() const override;
	gui::PluginView* instantiateView(QWidget* parent) override;

signals:
	void fileLoaded();

private:
	void processTrigger(const SfzTrigger& trigger);

	InstrumentTrack* m_parentTrack;

	QString m_sfzFilePath = "";

	//! Holds information about the total number of notes active, last switch keys pressed, etc
	SfzGlobalState m_sfzGlobalState;

	//! Holds information about midi CC default values, labels, which CC's are actually used, etc
	SfzControlsConfig m_controlsConfig;

	std::vector<SfzRegion> m_sfzRegions;

	//! So that the regions don't accidentally load the same sample multiple times, we store all the sames in one place and the regions ask it to load each sample/retrieve a pointer if it's already been loaded
	SfzSamplePool m_samplePool;

	// The GUI needs models to connect to midi CC knobs, so a bunch of dummy models are defined here.
	// Ideally it would be better to reuse the midi CC models for the instrument track
	std::array<FloatModel, SfzOpcodeState::NumMidiCCs> m_ccModels;

	//! Unfortunately, LMMS by default has the velocity of NoteOff events be 0. However, SFZ expects them to match the velocity of the corresponding NoteOn event.
	//! To account for this, we store the velocity of the last NoteOn event for each key, and use that value when hanlding NoteOff events.
	std::array<int, 128> m_previousNoteOnVelocity = {};

	friend class gui::SfzSamplerView;
};
} // namespace lmms
#endif // LMMS_SFZSAMPLER_H
