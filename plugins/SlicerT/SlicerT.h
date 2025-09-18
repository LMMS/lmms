/*
 * SlicerT.h - declaration of class SlicerT
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

#ifndef LMMS_SLICERT_H
#define LMMS_SLICERT_H

#include <stdexcept>

#include "AutomatableModel.h"
#include "ComboBoxModel.h"
#include "Instrument.h"
#include "Note.h"
#include "Sample.h"

namespace lmms {

class InstrumentTrack;
namespace gui {
class SlicerTView;
class SlicerTWaveform;
}

class PlaybackState
{
public:
	explicit PlaybackState(float startFrame)
		: m_currentNoteDone(startFrame)
		, m_resamplingState(src_new(SRC_LINEAR, DEFAULT_CHANNELS, nullptr))
	{
		if (!m_resamplingState) { throw std::runtime_error{"Failed to create sample rate converter object"}; }
	}
	~PlaybackState() noexcept { src_delete(m_resamplingState); }

	float noteDone() const { return m_currentNoteDone; }
	void setNoteDone(float newNoteDone) { m_currentNoteDone = newNoteDone; }

	SRC_STATE* resamplingState() const { return m_resamplingState; }

private:
	float m_currentNoteDone;
	SRC_STATE* m_resamplingState;
};

class SlicerT : public Instrument
{
	Q_OBJECT

public slots:
	void updateFile(QString file);
	void updateSlices();

signals:
	void isPlaying(float current, float start, float end);

public:
	SlicerT(InstrumentTrack* instrumentTrack);

	void playNote(NotePlayHandle* handle, SampleFrame* workingBuffer) override;
	void deleteNotePluginData(NotePlayHandle* handle) override;

	void saveSettings(QDomDocument& document, QDomElement& element) override;
	void loadSettings(const QDomElement& element) override;

	void findSlices();
	void findBPM();

	QString getSampleName() { return m_originalSample.sampleFile(); }

	QString nodeName() const override;
	gui::PluginView* instantiateView(QWidget* parent) override;

	std::vector<Note> getMidi();

private:
	FloatModel m_noteThreshold;
	FloatModel m_fadeOutFrames;
	IntModel m_originalBPM;
	ComboBoxModel m_sliceSnap;
	BoolModel m_enableSync;

	Sample m_originalSample;

	std::vector<float> m_slicePoints;

	InstrumentTrack* m_parentTrack;

	friend class gui::SlicerTView;
	friend class gui::SlicerTWaveform;
};
} // namespace lmms
#endif // LMMS_SLICERT_H
