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

#include <fftw3.h>

#include "AutomatableModel.h"
#include "Instrument.h"
#include "InstrumentView.h"
#include "Note.h"
#include "SampleBuffer.h"
#include "SlicerTView.h"

namespace lmms {

// takes one audio-channel and timeshifts it
class PhaseVocoder
{
public:
	PhaseVocoder();
	~PhaseVocoder();

	void loadData(std::vector<float> originalData, int sampleRate, float newRatio);
	void setScaleRatio(float newRatio) { updateParams(newRatio); }

	void getFrames(std::vector<float>& outData, int start, int frames);

	int frames() { return m_processedBuffer.size(); }
	float scaleRatio() { return m_scaleRatio; }

	// timeshift config
	static constexpr int s_windowSize = 512;
	static constexpr int s_overSampling = 32;

private:
	QMutex m_dataLock;
	// original data
	std::vector<float> m_originalBuffer;
	int m_originalSampleRate = 0;

	float m_scaleRatio = -1; // to force on first load

	// output data
	std::vector<float> m_processedBuffer; // final output
	std::vector<bool> m_processedWindows; // marks a window processed

	// depending on scaleRatio
	int m_stepSize = 0;
	int m_numWindows = 0;
	float m_outStepSize = 0;
	float m_freqPerBin = 0;
	float m_expectedPhaseIn = 0;
	float m_expectedPhaseOut = 0;

	// buffers
	std::vector<fftwf_complex> m_FFTSpectrum;
	std::vector<float> m_FFTInput;
	std::vector<float> m_IFFTReconstruction;
	std::vector<float> m_allMagnitudes;
	std::vector<float> m_allFrequencies;
	std::vector<float> m_processedFreq;
	std::vector<float> m_processedMagn;
	std::vector<float> m_lastPhase;
	std::vector<float> m_sumPhase;

	// cache
	std::vector<float> m_freqCache;
	std::vector<float> m_magCache;

	// fftw plans
	fftwf_plan m_fftPlan;
	fftwf_plan m_ifftPlan;

	void updateParams(float newRatio);
	void generateWindow(int windowNum, bool useCache);
};

// simple helper class that handles the different audio channels
class DynamicPlaybackBuffer
{
public:
	DynamicPlaybackBuffer()
		: m_leftChannel()
		, m_rightChannel()
	{}

	void loadSample(const sampleFrame* outData, int frames, int sampleRate, float newRatio)
	{
		std::vector<float> leftData(frames, 0);
		std::vector<float> rightData(frames, 0);
		for (int i = 0; i < frames; i++)
		{
			leftData[i] = outData[i][0];
			rightData[i] = outData[i][1];
		}
		m_leftChannel.loadData(leftData, sampleRate, newRatio);
		m_rightChannel.loadData(rightData, sampleRate, newRatio);
	}

	void getFrames(sampleFrame* outData, int startFrame, int frames)
	{
		std::vector<float> leftOut(frames, 0); // not a huge performance issue
		std::vector<float> rightOut(frames, 0);

		m_leftChannel.getFrames(leftOut, startFrame, frames);
		m_rightChannel.getFrames(rightOut, startFrame, frames);

		for (int i = 0; i < frames; i++)
		{
			outData[i][0] = leftOut[i];
			outData[i][1] = rightOut[i];
		}
	}

	int frames() { return m_leftChannel.frames(); }
	float scaleRatio() { return m_leftChannel.scaleRatio(); }

	void setScaleRatio(float newRatio)
	{
		m_leftChannel.setScaleRatio(newRatio);
		m_rightChannel.setScaleRatio(newRatio);
	}

private:
	PhaseVocoder m_leftChannel;
	PhaseVocoder m_rightChannel;
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
	~SlicerT() override = default;

	void playNote(NotePlayHandle* handle, sampleFrame* workingBuffer) override;

	void saveSettings(QDomDocument& document, QDomElement& element) override;
	void loadSettings(const QDomElement& element) override;

	void findSlices();
	void findBPM();

	QString nodeName() const override;
	gui::PluginView* instantiateView(QWidget* parent) override;

	void writeToMidi(std::vector<Note>* outClip);

private:
	// models
	FloatModel m_noteThreshold;
	FloatModel m_fadeOutFrames;
	IntModel m_originalBPM;
	ComboBoxModel m_sliceSnap;
	BoolModel m_enableSync;

	// sample buffers
	SampleBuffer m_originalSample;
	DynamicPlaybackBuffer m_phaseVocoder;

	std::vector<int> m_slicePoints;

	InstrumentTrack* m_parentTrack;

	friend class gui::SlicerTView;
	friend class gui::SlicerTWaveform;
};
} // namespace lmms
#endif // LMMS_SLICERT_H
