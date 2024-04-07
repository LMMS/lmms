/*
 * FFTFilter.h - FFTFilter effect for lmms
 *
 * Copyright (c) 2019 Martin Pavelek <he29/dot/HS/at/gmail/dot/com>
 * Copyright (c) 2024 Szeli1 </at/gmail/dot/com> TODO
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

#ifndef LMMS_FFTPROCESSOR_H
#define LMMS_FFTPROCESSOR_H

#include <fftw3.h>
#include <vector>
#include <atomic>
#include <mutex>
#include <thread>

#include "lmms_basics.h"
#include "fft_helpers.h"
#include "LocklessRingBuffer.h"

namespace lmms
{

template<class T>
class LocklessRingBuffer;

class FFTProcessor
{
public:

	FFTProcessor(unsigned int bufferSizeIn, bool isThreadedIn, FFTWindow FFTWindowIn);
	~FFTProcessor();

	//void analyze(LocklessRingBuffer<sampleFrame> &ringBufferIn);
	// if m_isThreaded and termilnate == false -> block, else start thread or run on current thread 
	void analyze(sampleFrame* sampleBufferIn, unsigned int sizeIn, unsigned int sampLocIn);
	//void analyze(std::vector<sampleFrame>* inputArrayIn, unsigned int readStartIn, bool isStereoIn);

	void threadedStartThread(LocklessRingBuffer<sampleFrame>* ringBufferIn, unsigned int sampLocIn, bool computeSpectrum, bool computeInverse);
	void threadedTerminate();

	// getters
	std::vector<float> getNormSpectrum();
	std::vector<float> getSample();
	bool getOutputSpectrumChanged();
	bool getOutputSamplesChanged();
	void setComplexMultiplier(std::vector<float>* complexMultiplierIn);

	void reverse(std::vector<float> processedDataIn);

	void rebuildWindow(FFTWindow FFTWindowIn);

	static unsigned int binCount(unsigned int blockSizeIn);


private:
	static void threadedAnalyze(std::atomic<bool>* terminateIn, fftwf_plan* planIn, fftwf_plan* inversePlanIn, fftwf_complex* complexIn, std::vector<float>* samplesIn, std::vector<float>* samplesOut,
		std::atomic<bool>* spectrumChangedOut, std::atomic<bool>* samplesChangedOut,
		LocklessRingBuffer<sampleFrame>* ringBufferIn, std::vector<float>* complexMultiplierIn, unsigned int sampLocIn, unsigned int blockSizeIn,
		std::vector<float>* spectrumOut, std::mutex* outputAccessIn);

	// Thread:
	// terminates the thread
	std::atomic<bool> m_terminate;
	std::atomic<bool> m_outputSpectrumChanged;
	std::atomic<bool> m_outputSamplesChanged;
	// it analyze gets ran on a different thread
	bool m_isThreaded;
	// worker thread if m_isThreaded is enabled
	std::thread m_FFTThread;
	std::mutex m_outputAccess;


	unsigned int m_frameFillLoc;

	// fft
	fftwf_complex* m_out;

	fftwf_plan m_plan;
	fftwf_plan m_iplan; //inverse

	std::atomic<unsigned int> m_blockSize;

	std::vector<float> m_samplesIn;
	std::vector<float> m_samplesOut;
	std::vector<float> m_normSpectrum;

	std::vector<float> m_complexMultiplier;

	std::vector<float> m_fftWindow;
	FFTWindow m_FFTWindowType;
};

}  // namespace lmms

#endif // LMMS_FFTPROCESSOR_H
