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
#include "SfzRegionManager.h"

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

	//! Holds a list of all the SfzRegion objects, which have the configurations for each of the samples/regions: what keys/velocities/etc trigger it, the volume, filter, envelopes, etc
	//! This object helps map triggers to lists of potentially matching regions, so that we don't have to loop over all the regions, checking every single one whether all conditions are matched before spawning a voice.
	SfzRegionManager m_regionManager;

	static constexpr int MAX_ACTIVE_SOUNDS = 128;
	//! Array to store all active (and inactive) sound play-states across all regions
	std::array<SfzRegionPlayState, MAX_ACTIVE_SOUNDS> m_voices;
	//! Maximum active index in the play state array
	//! By always spawning new sounds at the lowest open index and keeping track of the maximum index which contains an actice sound,
	//! you only need to loop through the first n elements and ignore the rest since you know they are inactive (Thanks to Lost Robot for the idea)
	size_t m_maxActiveIndex = 0;
	//! Helper function to figure out what the maximum active index is, in the event the maximum index deacticated
	void recalculateMaxActiveIndex();

	//! So that the regions don't accidentally load the same sample multiple times, we store all the sames in one place and the regions ask it to load each sample/retrieve a pointer if it's already been loaded
	SfzSamplePool m_samplePool;

	//! Holds information about the total number of notes active, last switch keys pressed, etc
	SfzGlobalState m_sfzGlobalState;

	//! Holds information about midi CC default values, labels, which CC's are actually used, etc
	SfzControlsConfig m_controlsConfig;

	//! Unfortunately, LMMS by default has the velocity of NoteOff events be 0. However, SFZ expects them to match the velocity of the corresponding NoteOn event.
	//! To account for this, we store the velocity of the last NoteOn event for each key, and use that value when hanlding NoteOff events.
	std::array<int, 128> m_previousNoteOnVelocity = {};

	InstrumentTrack* m_parentTrack;

	QString m_sfzFilePath = "";

	friend class gui::SfzSamplerView;
};
} // namespace lmms
#endif // LMMS_SFZSAMPLER_H
