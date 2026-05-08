/*
 * SfzPlayer.h - Simple SFZ instrument player
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

#ifndef LMMS_SFZPLAYER_H
#define LMMS_SFZPLAYER_H

#include "AutomatableModel.h"
#include "ComboBoxModel.h"
#include "Instrument.h"
#include "Note.h"
#include "Sample.h"
#include "SfzPlayerView.h"
#include "SfzParser.h"
#include "SfzRegion.h"
#include "SfzRegionPlayState.h"
#include "SfzGlobalState.h"
#include "SfzControlsConfig.h"
#include "SfzSamplePool.h"
#include "SfzRegionManager.h"

#include <thread>

namespace lmms {

class SfzPlayer : public Instrument
{
	Q_OBJECT

public:
	SfzPlayer(InstrumentTrack* instrumentTrack);
	~SfzPlayer();

	void playNote(NotePlayHandle* handle, SampleFrame* workingBuffer) override;
	void deleteNotePluginData(NotePlayHandle* handle) override;

	void play(SampleFrame* workingBuffer) override;
	bool handleMidiEvent(const MidiEvent& event, const TimePos& time = TimePos(), f_cnt_t offset = 0) override;

	void saveSettings(QDomDocument& document, QDomElement& element) override;
	void loadSettings(const QDomElement& element) override;

	void loadFile(const QString& filePath) override;
	//! This extra function for loading files is needed so that we can choose only to reset the CC knobs when the user is loading a new file, not a preset/project
	void loadSfzFile(const QString& filePath, const bool resetCCKnobs);

	QString nodeName() const override;
	gui::PluginView* instantiateView(QWidget* parent) override;

signals:
	void fileLoaded();

private:
	void processTrigger(const SfzTrigger& trigger);

	//! Holds a list of all the SfzRegion objects, which have the configurations for each of the samples/regions: what keys/velocities/etc trigger it, the volume, filter, envelopes, etc
	//! This object helps map triggers to lists of potentially matching regions, so that we don't have to loop over all the regions, checking every single one whether all conditions are matched before spawning a voice.
	SfzRegionManager* m_regionManager = nullptr;

	static constexpr int MAX_ACTIVE_SOUNDS = 128;
	//! Array to store all active (and inactive) sound play-states across all regions
	std::array<SfzRegionPlayState, MAX_ACTIVE_SOUNDS> m_voices;
	//! Maximum active index in the play state array
	//! By always spawning new sounds at the lowest open index and keeping track of the maximum index which contains an actice sound,
	//! you only need to loop through the first n elements and ignore the rest since you know they are inactive (Thanks to Lost Robot for the idea)
	size_t m_maxActiveIndex = 0;
	//! Helper function to figure out what the maximum active index is, in the event the maximum index deactivated
	void recalculateMaxActiveIndex();

	//! So that the regions don't accidentally load the same sample multiple times, we store all the sames in one place and the regions ask it to load each sample/retrieve a pointer if it's already been loaded
	SfzSamplePool* m_samplePool = nullptr;

	//! Holds information about the total number of notes active, last switch keys pressed, etc
	SfzGlobalState m_sfzGlobalState;

	//! Holds information about midi CC default values, labels, which CC's are actually used, etc
	SfzControlsConfig m_controlsConfig;

	//! Unfortunately, LMMS by default has the velocity of NoteOff events be 0. However, SFZ expects them to match the velocity of the corresponding NoteOn event.
	//! To account for this, we store the velocity of the last NoteOn event for each key, and use that value when hanlding NoteOff events.
	std::array<int, 128> m_previousNoteOnVelocity = {};

	InstrumentTrack* m_parentTrack;

	//! The path to the currently loaded SFZ file
	QString m_sfzFilePath = "";

	//! Helper function for printing debug info/passing status info to the GUI
	void setStatusInfo(const QString& text);
	QString m_statusText = "";


	//! Helper thread for loading sample files so that the main thread isn't blocked
	std::thread m_sampleLoadingThread;
	//! The function which the sample loading thread uses to load all the samples
	void sampleLoadingThreadFunction(const QDir& parentDirectory);

	//! Unfortunately, dealing with multiple threads gets complicated.
	//! There is the possibility that the audio thread could access the region/sample data while the main thread is loading a new SFZ file and the samples
	//! We need some way to swap out the current region/samples with the new data without breaking real-time safety.
	//! To do this, essentially we have temporary buffers which the main thread works on while loading the files. When it's done, it sets a flag
	//! which tells the audio thread that it can swap out the temporary objects for the real objects, and continue processing the audio.

	//! When a new SFZ file is being loaded and the regions/samples need to be swapped out, the main thread sets this flag to let the
	//! audio thread know to move the data from the temporary variables into the real region/sample stores. The audio thread will set this back to false when it has finished swapping
	std::atomic<bool> m_newSfzDataReady = false;
	std::atomic<bool> m_justSwappedData = false; // And another flag so the main thread knows if it should be waiting for enough buffers to pass before loaidng another sfz file
	//! Also have a flag for whether the sample loading thread is active or not, so that we don't accidentally try to start up a new sample loading thread while samples are already being loaded.
	std::atomic<bool> m_currentlyLoadingSamples = false;
	//! Temporary variables for the region and sample data, which the audio thread will swap with the real ones when m_newSfzDataReady is true
	SfzRegionManager* m_tempRegionManager = nullptr;
	SfzSamplePool* m_tempSamplePool = nullptr;
	//! Counts the number of buffers which have been processed. This is used for determining how long it has been since the audio thread has swapped out the region/sample data
	//! when loading a new SFZ file (Thanks to Lost Robot for the idea)
	std::atomic<size_t> m_bufferCounter = 0;
	//! The main thread will also save the buffer counter when m_newSfzDataReady was set so that it will know if enough buffers have passed that the audio thread can be guaranteed to have swapped the data already.
	//! If the user tries to load another SFZ file within one or two buffers of a previous SFZ file being loaded, it will refuse, because the audio thread may still be swapping the data from the temporary objects
	size_t m_bufferCounterWhenDataReady = 0;
	//! Also another flag just for the main thread so that it can remember whether the midi CC knobs should be reset to the SFZ file's defaults, since it has to wait until the audio thread has finished the swap.
	bool m_resetCCKnobs = false;

	//! A helper function for the audio thread to handle swapping the data
	void audioThreadHandleNewSfzData();
	//! A helper function for the main thread to delete the old data and update things like the CC knobs after the audio thread has swapped the data.
private slots:
	void mainThreadUpdateAfterDataSwap();

	friend class gui::SfzPlayerView;
};

} // namespace lmms

#endif // LMMS_SFZPLAYER_H
