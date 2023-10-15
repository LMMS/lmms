/*
 * PhaseVocoder.h - declaration of the PhaseVocoder class
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
#ifndef LMMS_PHASEVOCODER_H
#define LMMS_PHASEVOCODER_H

#include <QMutex>
#include <array>
#include <fftw3.h>
#include <math.h>
#include <vector>

#include "lmms_export.h"

namespace lmms {

/**
	Dynamically timeshifts one audio channel by a changable ratio
	Allows access to the timeshifted data in a threadsafe and realtime maner 
*/
class LMMS_EXPORT PhaseVocoder
{
public:
	PhaseVocoder();
	~PhaseVocoder();

	//! Loads a new sample, and precomputes the analysis cache
	void loadData(std::vector<float> originalData, int sampleRate, float newRatio);
	//! Change the output timeshift ratio
	void setScaleRatio(float newRatio) { updateParams(newRatio); }

	//! Copy a number of frames from a startpoint into an out buffer. 
	//! This is NOT relative to the original sample
	void getFrames(std::vector<float>& outData, int start, int frames);

	//! Get total number of frames for the timeshifted sample, NOT the original
	int frames() { return m_processedBuffer.size(); }
	//! Get the current scaleRatio
	float scaleRatio() { return m_scaleRatio; }

	// timeshift config
	static const int s_windowSize = 512;
	static const int s_overSampling = 32;

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
	std::array<fftwf_complex, s_windowSize> m_FFTSpectrum;
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
} // namespace lmms
#endif // LMMS_PHASEVOCODER_H
