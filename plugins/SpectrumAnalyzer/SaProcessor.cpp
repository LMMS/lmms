/* SaProcessor.cpp - implementation of SaProcessor class.
*
* Copyright (c) 2014-2017, David French <dave/dot/french3/at/googlemail/dot/com>
* Copyright (c) 2019 Martin Pavelek <he29/dot/HS/at/gmail/dot/com>
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
#ifdef DEBUG
	#include <iostream>
#endif

#include "lmms_math.h"

SaProcessor::SaProcessor(SaControls *controls) :
	m_controls(controls),
	m_blockSizeIndex(4),
	m_blockSize(FFT_BLOCK_SIZES[4]),
	m_sampleRate(Engine::mixer()->processingSampleRate()),
	m_windowType(BLACKMAN_HARRIS),
	m_framesFilledUp(0),
	m_active(false),
	m_inProgress(false)
{
	m_fftWindow.resize(m_blockSize, 1.0);
	precomputeWindow(m_fftWindow.data(), m_blockSize, (FFT_WINDOWS) m_windowType);

	m_bufferL.resize(m_blockSize, 0);
	m_bufferR.resize(m_blockSize, 0);
	m_spectrumL = (fftwf_complex *) fftwf_malloc(binCount() * sizeof (fftwf_complex));
	m_spectrumR = (fftwf_complex *) fftwf_malloc(binCount() * sizeof (fftwf_complex));
	m_fftPlanL = fftwf_plan_dft_r2c_1d(m_blockSize, m_bufferL.data(), m_spectrumL, FFTW_MEASURE);
	m_fftPlanR = fftwf_plan_dft_r2c_1d(m_blockSize, m_bufferR.data(), m_spectrumR, FFTW_MEASURE);

	m_absSpectrumL.resize(binCount(), 0);
	m_absSpectrumR.resize(binCount(), 0);
	m_normSpectrumL.resize(binCount(), 0);
	m_normSpectrumR.resize(binCount(), 0);

	m_history.resize(binCount() * WATERFALL_HEIGHT * sizeof qRgb(0,0,0), 0);

	clear();
}


SaProcessor::~SaProcessor()
{
	fftwf_destroy_plan(m_fftPlanL);
	fftwf_destroy_plan(m_fftPlanR);
	fftwf_free(m_spectrumL);
	fftwf_free(m_spectrumR);
}


void SaProcessor::analyse(sampleFrame *in_buffer, const fpp_t frame_count)
{
	#ifdef DEBUG
		int start_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	#endif
	// only analyse if the view is visible and not paused
	if (m_active && !m_controls->m_pauseModel.value())
	{
		m_inProgress = true;
		const bool stereo = m_controls->m_stereoModel.value();

		// check if FFT buffers need to be reallocated while it is safe to do so
		if (m_blockSizeIndex != m_controls->m_blockSizeModel.value()){
			reallocateBuffers(m_controls->m_blockSizeModel.value());
		}
		if (m_windowType != m_controls->m_windowModel.value()) {
			precomputeWindow(m_fftWindow.data(), m_blockSize, (FFT_WINDOWS) m_controls->m_windowModel.value());
			m_windowType = m_controls->m_windowModel.value();
		}

		// process data
		fpp_t frame = 0;
		while (frame < frame_count){
			// fill sample buffers
			for (; frame < frame_count && m_framesFilledUp < m_blockSize; frame++, m_framesFilledUp++)
			{
				if (stereo) {	//FIXME: predelat na case (dat do SaControls) a pridat MonoRMS
					m_bufferL[m_framesFilledUp] = in_buffer[frame][0];
					m_bufferR[m_framesFilledUp] = in_buffer[frame][1];
				} else {
					m_bufferL[m_framesFilledUp] = (in_buffer[frame][0] + in_buffer[frame][1]) * 0.5;
					m_bufferR[m_framesFilledUp] = (in_buffer[frame][0] + in_buffer[frame][1]) * 0.5;
				}
			}
	
			// run analysis if buffers contain enough data
			if (m_framesFilledUp < m_blockSize) {
				m_inProgress = false;
				return;
			}
	
			// update sample rate
			m_sampleRate = Engine::mixer()->processingSampleRate();
	
			// apply FFT window
			for (int i = 0; i < m_blockSize; i++) {
				m_bufferL[i] = m_bufferL[i] * m_fftWindow[i];
				m_bufferR[i] = m_bufferR[i] * m_fftWindow[i];
			}
	
			// lock data shared with SaSpectrumView and SaWaterfallView
			m_dataAccess.lock();

			// analyse spectrum of the left channel
			fftwf_execute(m_fftPlanL);
			absspec(m_spectrumL, m_absSpectrumL.data(), binCount());
			normalize(m_absSpectrumL, m_normSpectrumL);
	
			// repeat analysis for right channel only if stereo processing is enabled
			if (stereo) {
				fftwf_execute(m_fftPlanR);
				absspec(m_spectrumR, m_absSpectrumR.data(), binCount());
				normalize(m_absSpectrumR, m_normSpectrumR);
			} else {
				std::fill(m_absSpectrumR.begin(), m_absSpectrumR.end(), 0);
				std::fill(m_normSpectrumR.begin(), m_normSpectrumR.end(), 0);
			}
	
			// move waterfall history one line down and add newest result on top
			QRgb *pixel = (QRgb *)m_history.data();
			std::copy(	pixel,
						pixel + binCount() * WATERFALL_HEIGHT - binCount(),
						pixel + binCount());
			memset(pixel, 0, binCount() * sizeof (QRgb));
	
			int target;
			float accL = 0;
			float accR = 0;

			for (int i = 0; i < binCount(); i++) {
				if (m_controls->m_logXModel.value()) {
					// Logarithmic
					float band_start = freqToXPixel(binToFreq(i) - binBandwidth() / 2.0, binCount() -1);
					float band_end = freqToXPixel(binToFreq(i + 1) - binBandwidth() / 2.0, binCount() -1);

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
					// Linear: simple 1:1 assignment
					pixel[i] = makePixel(m_normSpectrumL[i], m_normSpectrumR[i]);
				}
			}

			m_dataAccess.unlock();

			#ifdef DEBUG
				start_time = std::chrono::high_resolution_clock::now().time_since_epoch().count() - start_time;
				std::cout << "Processed " << m_framesFilledUp << " samples in " << start_time / 1000000.0 << " ms" << std::endl;
			#endif
			m_framesFilledUp = 0;
		}

		m_inProgress = false;
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


float SaProcessor::binToFreq(int index)
{
	return (index * getSampleRate() / 2.0) / binCount();
}


float SaProcessor::binBandwidth()
{
	return (getSampleRate() / 2.0) / binCount();
}


float SaProcessor::getFreqRangeMin()
{
	switch (m_controls->m_freqRangeModel.value()) {
		case FRANGE_AUDIBLE: return FRANGE_AUDIBLE_START;
		case FRANGE_BASS: return FRANGE_BASS_START;
		case FRANGE_MIDS: return FRANGE_MIDS_START;
		case FRANGE_HIGH: return FRANGE_HIGH_START;
		default:
		case FRANGE_FULL: return m_controls->m_logXModel.value() ? LOWEST_LOG_FREQ : 0;
	}
}


float SaProcessor::getFreqRangeMax()
{
	switch (m_controls->m_freqRangeModel.value()) {
		case FRANGE_AUDIBLE: return FRANGE_AUDIBLE_END;
		case FRANGE_BASS: return FRANGE_BASS_END;
		case FRANGE_MIDS: return FRANGE_MIDS_END;
		case FRANGE_HIGH: return FRANGE_HIGH_END;
		default:
		case FRANGE_FULL: return getSampleRate() / 2;
	}
}


float SaProcessor::freqToXPixel(float freq, int width)
{
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


float SaProcessor::xPixelToFreq(float x, int width)
{
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


float SaProcessor::ampToYPixel(float amplitude, int height)
{
	if (m_controls->m_logYModel.value()){
		if (log10(amplitude) < LOWEST_LOG_AMP){
			return height;
		} else {
			return height * log10(amplitude) / LOWEST_LOG_AMP;
		}
	} else {
		return height - height * amplitude;
	}
}


float SaProcessor::yPixelToAmp(float y, int height)
{
	if (m_controls->m_logYModel.value()){
		return 10 * LOWEST_LOG_AMP * (y / height);
	} else {
		return 1 - y / height;
	}
}


int SaProcessor::getSampleRate() const
{
	return m_sampleRate;
}


bool SaProcessor::getActive() const
{
	return m_active;
}

void SaProcessor::setActive(bool active)
{
	m_active = active;
}

bool SaProcessor::getInProgress()
{
	return m_inProgress;
}


void SaProcessor::reallocateBuffers(int new_size_index)
{
	int new_size;
	int new_bins;

	// lock data shared with SaSpectrumView and SaWaterfallView
	m_dataAccess.lock();

	// get new block size and bin count based on selected index
	if (new_size_index == m_blockSizeIndex || new_size_index < 0) {return;}

	if (new_size_index < FFT_BLOCK_SIZES.size()){
		new_size = FFT_BLOCK_SIZES[new_size_index];
	} else {
		new_size = FFT_BLOCK_SIZES.back();
	}

	new_bins = new_size / 2 +1;

	// destroy old FFT plan and free the result buffer
	fftwf_destroy_plan(m_fftPlanL);
	fftwf_destroy_plan(m_fftPlanR);
	fftwf_free(m_spectrumL);
	fftwf_free(m_spectrumR);

	// allocate new space and create new plan
	m_fftWindow.resize(new_size, 1.0);
	precomputeWindow(m_fftWindow.data(), new_size, (FFT_WINDOWS) m_controls->m_windowModel.value());
	m_bufferL.resize(new_size, 0);
	m_bufferR.resize(new_size, 0);
	m_spectrumL = (fftwf_complex *) fftwf_malloc(new_bins * sizeof (fftwf_complex));
	m_spectrumR = (fftwf_complex *) fftwf_malloc(new_bins * sizeof (fftwf_complex));
	m_fftPlanL = fftwf_plan_dft_r2c_1d(new_size, m_bufferL.data(), m_spectrumL, FFTW_MEASURE);
	m_fftPlanR = fftwf_plan_dft_r2c_1d(new_size, m_bufferR.data(), m_spectrumR, FFTW_MEASURE);

	if (m_fftPlanL == NULL || m_fftPlanR == NULL)
		std::cerr << "Failed to create new FFT plan!" << std::endl;

	m_absSpectrumL.resize(new_bins, 0);
	m_absSpectrumR.resize(new_bins, 0);
	m_normSpectrumL.resize(new_bins, 0);
	m_normSpectrumR.resize(new_bins, 0);

	m_history.resize(new_bins * WATERFALL_HEIGHT * sizeof qRgb(0,0,0), 0);

	m_blockSize = new_size;
	m_blockSizeIndex = new_size_index;

	clear();
	m_dataAccess.unlock();

}


void SaProcessor::clear()
{
	m_framesFilledUp = 0;
	std::fill(m_bufferL.begin(), m_bufferL.end(), 0);
	std::fill(m_bufferR.begin(), m_bufferR.end(), 0);
	std::fill(m_absSpectrumL.begin(), m_absSpectrumL.end(), 0);
	std::fill(m_absSpectrumR.begin(), m_absSpectrumR.end(), 0);
	std::fill(m_normSpectrumL.begin(), m_normSpectrumL.end(), 0);
	std::fill(m_normSpectrumR.begin(), m_normSpectrumR.end(), 0);
	std::fill(m_history.begin(), m_history.end(), 0);
}

