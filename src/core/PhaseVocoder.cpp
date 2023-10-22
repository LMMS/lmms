/*
 * PhaseVocoder.cpp - Implementation of the PhaseVocoder class
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
#include "PhaseVocoder.h"

#include <algorithm>
#include <cstdio>
#include <fftw3.h>
#include <math.h>
#include <typeinfo>

#include "lmms_basics.h"
#include "lmms_constants.h"

namespace lmms {

PhaseVocoder::PhaseVocoder()
	: m_FFTInput(s_windowSize, 0)
	, m_IFFTReconstruction(s_windowSize, 0)
	, m_allMagnitudes(s_windowSize, 0)
	, m_allFrequencies(s_windowSize, 0)
	, m_processedFreq(s_windowSize, 0)
	, m_processedMagn(s_windowSize, 0)
{
	m_fftPlan = fftwf_plan_dft_r2c_1d(s_windowSize, m_FFTInput.data(), m_FFTSpectrum.data(), FFTW_MEASURE);
	m_ifftPlan = fftwf_plan_dft_c2r_1d(s_windowSize, m_FFTSpectrum.data(), m_IFFTReconstruction.data(), FFTW_MEASURE);
}

PhaseVocoder::~PhaseVocoder()
{
	fftwf_destroy_plan(m_fftPlan);
	fftwf_destroy_plan(m_ifftPlan);
}

void PhaseVocoder::loadData(const std::vector<float>& originalData, int sampleRate, float newRatio)
{
	m_dataLock.lock();

	m_originalBuffer = std::move(originalData);
	m_originalSampleRate = sampleRate;
	m_scaleRatio = -1; // force update, kinda hacky

	m_dataLock.unlock(); // stupid, but QRecursiveMutex is too expensive to have in updateParas and getFrames
	updateParams(newRatio);
	m_dataLock.lock();

	// set buffer sizes
	m_processedWindows.resize(m_numWindows, false);
	m_lastPhase.resize(m_numWindows * s_windowSize, 0);
	m_sumPhase.resize((m_numWindows + 1) * s_windowSize, 0);
	m_freqCache.resize(m_numWindows * s_windowSize, 0);
	m_magCache.resize(m_numWindows * s_windowSize, 0);

	// clear phase buffers
	std::fill(m_lastPhase.begin(), m_lastPhase.end(), 0);
	std::fill(m_sumPhase.begin(), m_sumPhase.end(), 0);

	// maybe limit this to a set amount of windows to reduce initial lag spikes
	for (int i = 0; i < m_numWindows; i++)
	{
		if (!m_processedWindows.at(i))
		{
			generateWindow(i, false); // first pass, no cache
			m_processedWindows.at(i) = true;
		}
	}

	m_dataLock.unlock();
}

void PhaseVocoder::getFrames(std::vector<float>& outData, int start, int frames)
{
	if (m_originalBuffer.size() < 2048) { return; }
	m_dataLock.lock();

	if (typeInfo<float>::isEqual(m_scaleRatio, 1.0f))
	{ // directly copy original data
		std::copy_n(m_originalBuffer.data() + start, frames, outData.data());
		m_dataLock.unlock();
		return;
	}

	int windowMargin = s_overSampling / 2; // numbers of windows before full quality
	int startWindow = static_cast<float>(start) / m_outStepSize - windowMargin;
	int endWindow = static_cast<float>((start + frames)) / m_outStepSize + windowMargin;

	startWindow = std::clamp(startWindow, 0, m_numWindows - 1);
	endWindow = std::clamp(endWindow, 0, m_numWindows - 1);

	// discard previous phaseSum if not processed
	if (!m_processedWindows[startWindow])
	{
		std::fill_n(m_sumPhase.data() + startWindow * s_windowSize, s_windowSize, 0);
	}

	// this encompases the minimum windows needed to get full quality,
	// which must be computed
	for (int i = startWindow; i < endWindow; i++)
	{
		if (!m_processedWindows.at(i))
		{
			generateWindow(i, true); // theses should use the cache
			m_processedWindows.at(i) = true;
		}
	}

	for (int i = 0; i < frames; i++)
	{
		outData.at(i) = m_processedBuffer[start + i];
	}

	m_dataLock.unlock();
}

// adjust pv params buffers to a new scale ratio
void PhaseVocoder::updateParams(float newRatio)
{
	if (m_originalBuffer.size() < 2048) { return; }
	if (newRatio == m_scaleRatio) { return; } // nothing changed
	m_dataLock.lock();

	m_scaleRatio = newRatio;
	m_stepSize = static_cast<float>(s_windowSize) / s_overSampling;
	m_numWindows = static_cast<float>(m_originalBuffer.size()) / m_stepSize - s_overSampling - 1;
	m_outStepSize = m_scaleRatio * m_stepSize; // float, else inaccurate
	m_freqPerBin = m_originalSampleRate / s_windowSize;
	m_expectedPhaseIn = 2. * F_PI * m_stepSize / s_windowSize;
	m_expectedPhaseOut = 2. * F_PI * m_outStepSize / s_windowSize;

	m_processedBuffer.resize(m_numWindows * m_outStepSize + s_windowSize, 0);

	// very slow :(
	std::fill(m_processedWindows.begin(), m_processedWindows.end(), false);
	std::fill(m_processedBuffer.begin(), m_processedBuffer.end(), 0);

	m_dataLock.unlock();
}

// time shifts one window from originalBuffer and writes to m_processedBuffer
// resources:
// http://blogs.zynaptiq.com/bernsee/pitch-shifting-using-the-ft/
// https://sethares.engr.wisc.edu/vocoders/phasevocoder.html
// https://dsp.stackexchange.com/questions/40101/audio-time-stretching-without-pitch-shifting/40367#40367
// https://www.guitarpitchshifter.com/
// https://en.wikipedia.org/wiki/Window_function
void PhaseVocoder::generateWindow(int windowNum, bool useCache)
{
	// declare vars
	float real, imag, phase, magnitude, freq, deltaPhase;
	int windowStart = static_cast<float>(windowNum) * m_stepSize;
	int windowIndex = static_cast<float>(windowNum) * s_windowSize;

	if (!useCache)
	{ // normal stuff
		std::copy_n(m_originalBuffer.data() + windowStart, s_windowSize, m_FFTInput.data());

		// FFT
		fftwf_execute(m_fftPlan);

		// analysis step
		for (int j = 0; j < s_windowSize / 2; j++) // only process nyquistic frequency
		{
			real = m_FFTSpectrum.at(j)[0];
			imag = m_FFTSpectrum.at(j)[1];

			magnitude = 2. * std::sqrt(real * real + imag * imag);
			phase = std::atan2(imag, real);

			// calculate difference in phase with prev window
			freq = phase;
			freq = phase - m_lastPhase.at(std::max(0, windowIndex + j - s_windowSize)); // subtract prev pahse to get phase
																					 // diference
			m_lastPhase.at(windowIndex + j) = phase;

			freq -= m_expectedPhaseIn * j; // subtract expected phase
			// at this point, freq is the difference in phase
			// between the last phase, having removed the expected phase at this point in the sample

			// this puts freq in 0-2pi. Since the phase difference is proportional to the deviation in bin frequency,
			// with this we can better estimate the true frequency
			freq = std::fmod(freq + F_PI, -2.0f * F_PI) + F_PI;

			// convert phase difference into bin freq mulitplier
			freq = freq * s_overSampling / (2. * F_PI);

			// add to the expected freq the change in freq calculated from the phase diff
			freq = m_freqPerBin * j + m_freqPerBin * freq;

			m_allMagnitudes.at(j) = magnitude;
			m_allFrequencies.at(j) = freq;
		}
		// write cache
		std::copy_n(m_allFrequencies.data(), s_windowSize, m_freqCache.data() + windowIndex);
		std::copy_n(m_allMagnitudes.data(), s_windowSize, m_magCache.data() + windowIndex);
	}
	else
	{
		// read cache
		std::copy_n(m_freqCache.data() + windowIndex, s_windowSize, m_allFrequencies.data());
		std::copy_n(m_magCache.data() + windowIndex, s_windowSize, m_allMagnitudes.data());
	}

	// synthesis, all the operations are the reverse of the analysis
	for (int j = 0; j < s_windowSize / 2; j++)
	{
		magnitude = m_allMagnitudes.at(j);
		freq = m_allFrequencies.at(j);

		// difference to bin freq mulitplier
		deltaPhase = freq - m_freqPerBin * j;

		// convert to phase difference
		deltaPhase /= m_freqPerBin;

		// difference in phase
		deltaPhase = 2. * F_PI * deltaPhase / s_overSampling;

		// add the expected phase
		deltaPhase += m_expectedPhaseOut * j;

		// sum this phase to the total, to keep track of the out phase along the sample
		m_sumPhase.at(windowIndex + j) += deltaPhase;
		deltaPhase = m_sumPhase.at(windowIndex + j); // final bin phase

		m_sumPhase.at(windowIndex + j + s_windowSize) = deltaPhase; // copy to the next

		m_FFTSpectrum.at(j)[0] = magnitude * std::cos(deltaPhase);
		m_FFTSpectrum.at(j)[1] = magnitude * std::sin(deltaPhase);
	}

	// inverse fft
	fftwf_execute(m_ifftPlan);

	// windowing
	for (int j = 0; j < s_windowSize; j++)
	{
		float outIndex = windowNum * m_outStepSize + j;

		// blackman-harris window
		float a0 = 0.35875f;
		float a1 = 0.48829f;
		float a2 = 0.14128f;
		float a3 = 0.01168f;

		float piN2 = 2.0f * F_PI * j;
		float window
			= a0 - (a1 * std::cos(piN2 / s_windowSize)) + (a2 * std::cos(2.0f * piN2 / s_windowSize)) - (a3 * std::cos(3.0f * piN2));

		// inverse fft magnitudes are windowsSize times bigger
		m_processedBuffer.at(outIndex) += window * (m_IFFTReconstruction.at(j) / s_windowSize / s_overSampling);
	}
}
} // namespace lmms
