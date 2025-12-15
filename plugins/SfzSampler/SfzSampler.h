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
#include "SfzFormat.h"

namespace lmms {

class SfzSampler : public Instrument
{
	Q_OBJECT

public:
	SfzSampler(InstrumentTrack* instrumentTrack);
	~SfzSampler();

	//void playNote(NotePlayHandle* handle, SampleFrame* workingBuffer) override;
	//void deleteNotePluginData(NotePlayHandle* handle) override;

	void play(SampleFrame* workingBuffer) override;
	bool handleMidiEvent(const MidiEvent& event, const TimePos& time = TimePos(), f_cnt_t offset = 0) override;

	void saveSettings(QDomDocument& document, QDomElement& element) override;
	void loadSettings(const QDomElement& element) override;

	void loadFile(const QString& file) override;

	QString nodeName() const override;
	gui::PluginView* instantiateView(QWidget* parent) override;

private:

	Sample m_originalSample1;
	Sample m_originalSample2;

	SampleFrame* m_tempBuffer;

	std::vector<Sample> m_samples;

	SfzSettings m_sfz;

	InstrumentTrack* m_parentTrack;

	struct NoteState
	{
		bool pressed = false;
		int velocity = 0;
		int frameCounter = 0;
		int frameReleased = 0;
		int sampleIndex = -1;
		double pitchRatio = 1.0;
		int attackFrames = 0;
		int holdFrames = 0;
		int decayFrames = 0;
		float sustainLevel = 1.0f;
		int releaseFrames = 0;
		Sample::PlaybackState playbackState;
	};

	std::array<NoteState, 128> m_noteStates;

	friend class gui::SfzSamplerView;
};
} // namespace lmms
#endif // LMMS_SFZSAMPLER_H
