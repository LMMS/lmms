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

#include <algorithm>
#include <fftw3.h>

#include "AutomatableModel.h"
#include "Instrument.h"
#include "InstrumentView.h"
#include "Note.h"
#include "PhaseVocoder.h"
#include "SampleBuffer.h"
#include "SlicerTView.h"
#include "lmms_basics.h"

namespace lmms {

class PlayBackState
{
public:
	PlayBackState(float startFrame)
		: currentNote(startFrame)
	{
		resamplingState = src_new(SRC_LINEAR, 2, nullptr);
	}
	~PlayBackState() { src_delete(resamplingState); }
	float getNoteDone() { return currentNote; }
	void setNoteDone(float newDone) { currentNote = newDone; }
	SRC_STATE* getResampleState() { return resamplingState; }

private:
	float currentNote; // these are all absoute floats
	SRC_STATE* resamplingState;
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

	void playNote(NotePlayHandle* handle, sampleFrame* workingBuffer) override;

	void saveSettings(QDomDocument& document, QDomElement& element) override;
	void loadSettings(const QDomElement& element) override;

	void findSlices();
	void findBPM();

	QString nodeName() const override;
	gui::PluginView* instantiateView(QWidget* parent) override;

	std::vector<Note> getMidi();

private:
	// models
	FloatModel m_noteThreshold;
	FloatModel m_fadeOutFrames;
	IntModel m_originalBPM;
	ComboBoxModel m_sliceSnap;
	BoolModel m_enableSync;

	// sample buffers
	SampleBuffer m_originalSample;

	std::vector<float> m_slicePoints;

	InstrumentTrack* m_parentTrack;

	friend class gui::SlicerTView;
	friend class gui::SlicerTWaveform;
};
} // namespace lmms
#endif // LMMS_SLICERT_H
