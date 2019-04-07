/* SaProcessor.cpp - implementation of SaProcessor class.
 *
 * Copyright (c) 2019 Martin Pavelek <he29/dot/HS/at/gmail/dot/com>
 *
 * Based partially on Eq plugin code,
 * Copyright (c) 2014-2017, David French <dave/dot/french3/at/googlemail/dot/com>
 *
 * This file is part of LMMS - https://lmms.io
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#include "SaProcessor.h"

#include <algorithm>
#include <cmath>
#include <iostream>

#include "lmms_math.h"

SaProcessor::SaProcessor(SaControls *controls) :
	m_controls(controls),
	m_inBlockSize(FFT_BLOCK_SIZES[0]),
	m_fftBlockSize(FFT_BLOCK_SIZES[0]),
	m_sampleRate(Engine::mixer()->processingSampleRate()),
	m_windowType(BLACKMAN_HARRIS),
	m_framesFilledUp(0),
	m_active(false),
	m_destroyed(false)
{
	m_fftWindow.resize(m_inBlockSize, 1.0);
	precomputeWindow(m_fftWindow.data(), m_inBlockSize, (FFT_WINDOWS) m_windowType);

	m_bufferL.resize(m_fftBlockSize, 0);
	m_bufferR.resize(m_fftBlockSize, 0);
	m_spectrumL = (fftwf_complex *) fftwf_malloc(binCount() * sizeof (fftwf_complex));
	m_spectrumR = (fftwf_complex *) fftwf_malloc(binCount() * sizeof (fftwf_complex));
	m_fftPlanL = fftwf_plan_dft_r2c_1d(m_fftBlockSize, m_bufferL.data(), m_spectrumL, FFTW_MEASURE);
	m_fftPlanR = fftwf_plan_dft_r2c_1d(m_fftBlockSize, m_bufferR.data(), m_spectrumR, FFTW_MEASURE);

	m_absSpectrumL.resize(binCount(), 0);
	m_absSpectrumR.resize(binCount(), 0);
	m_normSpectrumL.resize(binCount(), 0);
	m_normSpectrumR.resize(binCount(), 0);

	m_history.resize(binCount() * m_waterfallHeight * sizeof qRgb(0,0,0), 0);

	clear();
}


SaProcessor::~SaProcessor()
{
	m_destroyed = true;

	if (m_fftPlanL != NULL) {fftwf_destroy_plan(m_fftPlanL);}
	if (m_fftPlanR != NULL) {fftwf_destroy_plan(m_fftPlanR);}
	if (m_spectrumL != NULL) {fftwf_free(m_spectrumL);}
	if (m_spectrumR != NULL) {fftwf_free(m_spectrumR);}

	m_fftPlanL = NULL;
	m_fftPlanR = NULL;
	m_spectrumL = NULL;
	m_spectrumR = NULL;
}


void SaProcessor::analyse(sampleFrame *in_buffer, const fpp_t frame_count) {
	#ifdef SA_DEBUG
		int start_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	#endif
	// only analyse if any view is visible and not paused
	if ((m_active || m_waterfallActive) && !m_controls->m_pauseModel.value())
	{
		const bool stereo = m_controls->m_stereoModel.value();

		// process data
		fpp_t frame = 0;
		while (frame < frame_count){
			// fill sample buffers
			for (; frame < frame_count && m_framesFilledUp < m_inBlockSize; frame++, m_framesFilledUp++)
			{
				if (stereo) {
					m_bufferL[m_framesFilledUp] = in_buffer[frame][0];
					m_bufferR[m_framesFilledUp] = in_buffer[frame][1];
				} else {
					m_bufferL[m_framesFilledUp] = (in_buffer[frame][0] + in_buffer[frame][1]) * 0.5;
					m_bufferR[m_framesFilledUp] = (in_buffer[frame][0] + in_buffer[frame][1]) * 0.5;
				}
			}
	
			// run analysis if buffers contain enough data
			if (m_framesFilledUp < m_inBlockSize) {
				return;
			}
	
			// update sample rate
			m_sampleRate = Engine::mixer()->processingSampleRate();
	
			// apply FFT window
			for (int i = 0; i < m_inBlockSize; i++) {
				m_bufferL[i] = m_bufferL[i] * m_fftWindow[i];
				m_bufferR[i] = m_bufferR[i] * m_fftWindow[i];
			}
	
			// lock data shared with SaSpectrumView and SaWaterfallView
			m_dataAccess.lock();

			// analyse spectrum of the left channel
			fftwf_execute(m_fftPlanL);
			absspec(m_spectrumL, m_absSpectrumL.data(), binCount());
			normalize(m_absSpectrumL, m_normSpectrumL, m_inBlockSize);
	
			// repeat analysis for right channel only if stereo processing is enabled
			if (stereo) {
				fftwf_execute(m_fftPlanR);
				absspec(m_spectrumR, m_absSpectrumR.data(), binCount());
				normalize(m_absSpectrumR, m_normSpectrumR, m_inBlockSize);
			} else {
				std::fill(m_absSpectrumR.begin(), m_absSpectrumR.end(), 0);
				std::fill(m_normSpectrumR.begin(), m_normSpectrumR.end(), 0);
			}
	
			// move waterfall history one line down and add newest result on top
			if (m_waterfallActive) {
				QRgb *pixel = (QRgb *)m_history.data();
				std::copy(	pixel,
							pixel + binCount() * m_waterfallHeight - binCount(),
							pixel + binCount());
				memset(pixel, 0, binCount() * sizeof (QRgb));
		
				int target;
				float accL = 0;
				float accR = 0;
	
				for (int i = 0; i < binCount(); i++) {
					float band_start = freqToXPixel(binToFreq(i) - binBandwidth() / 2.0, binCount() -1);
					float band_end = freqToXPixel(binToFreq(i + 1) - binBandwidth() / 2.0, binCount() -1);
					if (m_controls->m_logXModel.value()) {
						// Logarithmic
						if (band_end - band_start > 1.0) {
							// draw all pixels covered by this band
							for (target = band_start; target < band_end; target++) {
								if (target >= 0 && target < binCount()) {
									pixel[target] = makePixel(m_normSpectrumL[i], m_normSpectrumR[i]);
								}
							}
							accL = (band_end - (int)band_end) * m_normSpectrumL[i];
							accR = (band_end - (int)band_end) * m_normSpectrumR[i];
						} else {
							// sub-pixel drawing; add contribution of current band
							target = band_start;
							if ((int)band_start == (int)band_end) {
								accL += (band_end - band_start) * m_normSpectrumL[i];
								accR += (band_end - band_start) * m_normSpectrumR[i];
							} else {
								// make sure contribution is split correctly on pixel boundary
								accL += ((int)band_end - band_start) * m_normSpectrumL[i];
								accR += ((int)band_end - band_start) * m_normSpectrumR[i];
	
								if (target >= 0 && target < binCount()) {
									pixel[target] = makePixel(accL, accR);
								}
								// save remaining portion of the band for the following band / pixel
								accL = (band_end - (int)band_end) * m_normSpectrumL[i];
								accR = (band_end - (int)band_end) * m_normSpectrumR[i];
							}
						}
					} else {
						// Linear: always draws one or more pixels per band
						for (target = band_start; target < band_end; target++) {
							if (target >= 0 && target < binCount()) {
								pixel[target] = makePixel(m_normSpectrumL[i], m_normSpectrumR[i]);
							}
						}
					}
				}
			}

			m_dataAccess.unlock();

			#ifdef SA_DEBUG
				start_time = std::chrono::high_resolution_clock::now().time_since_epoch().count() - start_time;
				std::cout << "Processed " << m_framesFilledUp << " samples in " << start_time / 1000000.0 << " ms" << std::endl;
			#endif
			m_framesFilledUp = 0;
		}
	}
}


QRgb SaProcessor::makePixel(float left, float right) {
	// apply gamma correction to make small values more visible
	// (should be around 0.42 to 0.45 for sRGB displays)
	if (m_controls->m_stereoModel.value()) {
		float ampL = powf(left, 0.42);
		float ampR = powf(right, 0.42);
		return qRgb(m_controls->m_colorL.red() * ampL + m_controls->m_colorR.red() * ampR,
					m_controls->m_colorL.green() * ampL + m_controls->m_colorR.green() * ampR,
					m_controls->m_colorL.blue() * ampL + m_controls->m_colorR.blue() * ampR);
	} else {
		float ampL = powf(left, 0.42);
		return qRgb(m_controls->m_colorMono.lighter().red() * ampL,
					m_controls->m_colorMono.lighter().green() * ampL,
					m_controls->m_colorMono.lighter().blue() * ampL);
	}
}


int SaProcessor::getSampleRate() const {
	return m_sampleRate;
}


bool SaProcessor::getActive() const {
	return m_active;
}


void SaProcessor::setActive(bool active) {
	m_active = active;
}


void SaProcessor::setWaterfallActive(bool active) {
	m_waterfallActive = active;
}


void SaProcessor::reallocateBuffers() {
	if (m_destroyed) {return;}

	int new_in_size, new_fft_size;
	int new_bins;

	int new_size_index = m_controls->m_blockSizeModel.value();

	// get new block sizes and bin count based on selected index
	if (new_size_index < FFT_BLOCK_SIZES.size()){
		new_in_size = FFT_BLOCK_SIZES[new_size_index];
	} else {
		new_in_size = FFT_BLOCK_SIZES.back();
	}
	if (new_size_index + m_zeroPadFactor < FFT_BLOCK_SIZES.size()){
		new_fft_size = FFT_BLOCK_SIZES[new_size_index + m_zeroPadFactor];
	} else {
		new_fft_size = FFT_BLOCK_SIZES.back();
	}

	new_bins = new_fft_size / 2 +1;

	// lock data shared with SaSpectrumView and SaWaterfallView
	m_dataAccess.lock();

	// destroy old FFT plan and free the result buffer
	if (m_fftPlanL != NULL) {fftwf_destroy_plan(m_fftPlanL);}
	if (m_fftPlanR != NULL) {fftwf_destroy_plan(m_fftPlanR);}
	if (m_spectrumL != NULL) {fftwf_free(m_spectrumL);}
	if (m_spectrumR != NULL) {fftwf_free(m_spectrumR);}

	// allocate new space and create new plan
	m_fftWindow.resize(new_in_size, 1.0);
	precomputeWindow(m_fftWindow.data(), new_in_size, (FFT_WINDOWS) m_controls->m_windowModel.value());
	m_bufferL.resize(new_fft_size, 0);
	m_bufferR.resize(new_fft_size, 0);
	m_spectrumL = (fftwf_complex *) fftwf_malloc(new_bins * sizeof (fftwf_complex));
	m_spectrumR = (fftwf_complex *) fftwf_malloc(new_bins * sizeof (fftwf_complex));
	m_fftPlanL = fftwf_plan_dft_r2c_1d(new_fft_size, m_bufferL.data(), m_spectrumL, FFTW_MEASURE);
	m_fftPlanR = fftwf_plan_dft_r2c_1d(new_fft_size, m_bufferR.data(), m_spectrumR, FFTW_MEASURE);

	if (m_fftPlanL == NULL || m_fftPlanR == NULL) {
		std::cerr << "Failed to create new FFT plan!" << std::endl;
	}
	m_absSpectrumL.resize(new_bins, 0);
	m_absSpectrumR.resize(new_bins, 0);
	m_normSpectrumL.resize(new_bins, 0);
	m_normSpectrumR.resize(new_bins, 0);

	m_history.resize(new_bins * m_waterfallHeight * sizeof qRgb(0,0,0), 0);

	m_inBlockSize = new_in_size;
	m_fftBlockSize = new_fft_size;

	m_dataAccess.unlock();
	clear();
}


// Call function from fft_helpers to make a new window based on currently set controls.
void SaProcessor::rebuildWindow() {
	if (m_destroyed) {return;}
	m_dataAccess.lock();
	precomputeWindow(m_fftWindow.data(), m_inBlockSize, (FFT_WINDOWS) m_controls->m_windowModel.value());
	m_windowType = m_controls->m_windowModel.value();
	m_dataAccess.unlock();
}


// Clear all data buffers and replace contents with zeros.
// Note: may take a few milliseconds, do not call in a loop!
void SaProcessor::clear() {
	m_dataAccess.lock();
	m_framesFilledUp = 0;
	std::fill(m_bufferL.begin(), m_bufferL.end(), 0);
	std::fill(m_bufferR.begin(), m_bufferR.end(), 0);
	std::fill(m_absSpectrumL.begin(), m_absSpectrumL.end(), 0);
	std::fill(m_absSpectrumR.begin(), m_absSpectrumR.end(), 0);
	std::fill(m_normSpectrumL.begin(), m_normSpectrumL.end(), 0);
	std::fill(m_normSpectrumR.begin(), m_normSpectrumR.end(), 0);
	std::fill(m_history.begin(), m_history.end(), 0);
	m_dataAccess.unlock();
}


// --------------------------------------
// Frequency conversion helpers
//
float SaProcessor::binToFreq(int index) {
	return (index * getSampleRate() / 2.0) / binCount();
}


float SaProcessor::binBandwidth() {
	return (getSampleRate() / 2.0) / binCount();
}


float SaProcessor::getFreqRangeMin(bool linear) {
	switch (m_controls->m_freqRangeModel.value()) {
		case FRANGE_AUDIBLE: return FRANGE_AUDIBLE_START;
		case FRANGE_BASS: return FRANGE_BASS_START;
		case FRANGE_MIDS: return FRANGE_MIDS_START;
		case FRANGE_HIGH: return FRANGE_HIGH_START;
		default:
		case FRANGE_FULL: return !linear ? LOWEST_LOG_FREQ : 0;
	}
}


float SaProcessor::getFreqRangeMax() {
	switch (m_controls->m_freqRangeModel.value()) {
		case FRANGE_AUDIBLE: return FRANGE_AUDIBLE_END;
		case FRANGE_BASS: return FRANGE_BASS_END;
		case FRANGE_MIDS: return FRANGE_MIDS_END;
		case FRANGE_HIGH: return FRANGE_HIGH_END;
		default:
		case FRANGE_FULL: return getSampleRate() / 2;
	}
}


float SaProcessor::freqToXPixel(float freq, int width) {
	if (m_controls->m_logXModel.value()) {
		if (freq <= 1) {return 0;}
		float min = log10(getFreqRangeMin());
		float range = log10(getFreqRangeMax()) - min;
		return (log10(freq) - min) / range * width;
	} else {
		float min = getFreqRangeMin();
		float range = getFreqRangeMax() - min;
		return (freq - min) / range * width;
	}
}


float SaProcessor::xPixelToFreq(float x, int width) {
	if (m_controls->m_logXModel.value()) {
		float min = log10(getFreqRangeMin());
		float max = log10(getFreqRangeMax());
		float range = max - min;
		return pow(10, min + x / width * range);
	} else {
		float min = getFreqRangeMin();
		float range = getFreqRangeMax() - min;
		return min + x / width * range;
	}
}


// --------------------------------------
// Amplitude conversion helpers
//
float SaProcessor::ampToYPixel(float amplitude, int height) {
	if (m_controls->m_logYModel.value()){
		if (10 * log10(amplitude) < getAmpRangeMin()){
			return height;
		} else {
			float max = getAmpRangeMax();
			float range = getAmpRangeMin() - max;
			return (10 * log10(amplitude) - max) / range * height;
		}
	} else {
		float max = pow(10, getAmpRangeMax() / 10);
		float range = pow(10, getAmpRangeMin() / 10) - max;
		return (amplitude - max) / range * height;
	}
}


float SaProcessor::yPixelToAmp(float y, int height) {
	if (m_controls->m_logYModel.value()){
		float max = getAmpRangeMax();
		float range = getAmpRangeMin() - max;
		return max + range * (y / height);
	} else {
		float max = pow(10, getAmpRangeMax() / 10);
		float range = pow(10, getAmpRangeMin() / 10) - max;
		return max + range * (y / height);
	}
}


float SaProcessor::getAmpRangeMin() {
	switch (m_controls->m_ampRangeModel.value()) {
		case ARANGE_EXTENDED: return ARANGE_EXTENDED_START;
		case ARANGE_AUDIBLE: return ARANGE_AUDIBLE_START;
		case ARANGE_NOISE: return ARANGE_NOISE_START;
		default:
		case ARANGE_DEFAULT: return ARANGE_DEFAULT_START;
	}
}


float SaProcessor::getAmpRangeMax() {
	switch (m_controls->m_ampRangeModel.value()) {
		case ARANGE_EXTENDED: return ARANGE_EXTENDED_END;
		case ARANGE_AUDIBLE: return ARANGE_AUDIBLE_END;
		case ARANGE_NOISE: return ARANGE_NOISE_END;
		default:
		case ARANGE_DEFAULT: return ARANGE_DEFAULT_END;
	}
}

