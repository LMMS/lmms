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
#include <QMutexLocker>

#include "lmms_math.h"


SaProcessor::SaProcessor(SaControls *controls) :
	m_controls(controls),
	m_inBlockSize(FFT_BLOCK_SIZES[0]),
	m_fftBlockSize(FFT_BLOCK_SIZES[0]),
	m_sampleRate(Engine::mixer()->processingSampleRate()),
	m_framesFilledUp(0),
	m_spectrumActive(false),
	m_waterfallActive(false),
	m_waterfallNotEmpty(0),
	m_reallocating(false)
{
	m_fftWindow.resize(m_inBlockSize, 1.0);
	precomputeWindow(m_fftWindow.data(), m_inBlockSize, BLACKMAN_HARRIS);

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
	if (m_fftPlanL != NULL) {fftwf_destroy_plan(m_fftPlanL);}
	if (m_fftPlanR != NULL) {fftwf_destroy_plan(m_fftPlanR);}
	if (m_spectrumL != NULL) {fftwf_free(m_spectrumL);}
	if (m_spectrumR != NULL) {fftwf_free(m_spectrumR);}

	m_fftPlanL = NULL;
	m_fftPlanR = NULL;
	m_spectrumL = NULL;
	m_spectrumR = NULL;
}


// Load a batch of data from LMMS; run FFT analysis if buffer is full enough.
void SaProcessor::analyse(sampleFrame *in_buffer, const fpp_t frame_count)
{
	#ifdef SA_DEBUG
		int start_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	#endif
	// only take in data if any view is visible and not paused
	if ((m_spectrumActive || m_waterfallActive) && !m_controls->m_pauseModel.value())
	{
		const bool stereo = m_controls->m_stereoModel.value();
		fpp_t in_frame = 0;
		while (in_frame < frame_count)
		{
			// fill sample buffers and check for zero input
			bool block_empty = true;
			for (; in_frame < frame_count && m_framesFilledUp < m_inBlockSize; in_frame++, m_framesFilledUp++)
			{
				if (stereo)
				{
					m_bufferL[m_framesFilledUp] = in_buffer[in_frame][0];
					m_bufferR[m_framesFilledUp] = in_buffer[in_frame][1];
				}
				else
				{
					m_bufferL[m_framesFilledUp] =
					m_bufferR[m_framesFilledUp] = (in_buffer[in_frame][0] + in_buffer[in_frame][1]) * 0.5f;
				}
				if (in_buffer[in_frame][0] != 0.f || in_buffer[in_frame][1] != 0.f)
				{
					block_empty = false;
				}
			}
	
			// Run analysis only if buffers contain enough data.
			// Also, to prevent audio interruption and a momentary GUI freeze,
			// skip analysis if buffers are being reallocated.
			if (m_framesFilledUp < m_inBlockSize || m_reallocating) {return;}

			// update sample rate
			m_sampleRate = Engine::mixer()->processingSampleRate();
	
			// apply FFT window
			for (unsigned int i = 0; i < m_inBlockSize; i++)
			{
				m_bufferL[i] = m_bufferL[i] * m_fftWindow[i];
				m_bufferR[i] = m_bufferR[i] * m_fftWindow[i];
			}
	
			// lock data shared with SaSpectrumView and SaWaterfallView
			QMutexLocker lock(&m_dataAccess);

			// Run FFT on left channel, convert the result to absolute magnitude
			// spectrum and normalize it.
			fftwf_execute(m_fftPlanL);
			absspec(m_spectrumL, m_absSpectrumL.data(), binCount());
			normalize(m_absSpectrumL, m_normSpectrumL, m_inBlockSize);
	
			// repeat analysis for right channel if stereo processing is enabled
			if (stereo)
			{
				fftwf_execute(m_fftPlanR);
				absspec(m_spectrumR, m_absSpectrumR.data(), binCount());
				normalize(m_absSpectrumR, m_normSpectrumR, m_inBlockSize);
			}

			// count empty lines so that empty history does not have to update
			if (block_empty && m_waterfallNotEmpty)
			{
				m_waterfallNotEmpty -= 1;
			}
			else if (!block_empty)
			{
				m_waterfallNotEmpty = m_waterfallHeight + 2;
			}

			if (m_waterfallActive && m_waterfallNotEmpty)
			{
				// move waterfall history one line down and clear the top line
				QRgb *pixel = (QRgb *)m_history.data();
				std::copy(pixel,
						  pixel + binCount() * m_waterfallHeight - binCount(),
						  pixel + binCount());
				memset(pixel, 0, binCount() * sizeof (QRgb));

				// add newest result on top
				int target;		// pixel being constructed
				float accL = 0;	// accumulators for merging multiple bins
				float accR = 0;
	
				for (unsigned int i = 0; i < binCount(); i++)
				{
					// Every frequency bin spans a frequency range that must be
					// partially or fully mapped to a pixel. Any inconsistency
					// may be seen in the spectrogram as dark or white lines --
					// play white noise to confirm your change did not break it.
					float band_start = freqToXPixel(binToFreq(i) - binBandwidth() / 2.0, binCount());
					float band_end = freqToXPixel(binToFreq(i + 1) - binBandwidth() / 2.0, binCount());
					if (m_controls->m_logXModel.value())
					{
						// Logarithmic scale
						if (band_end - band_start > 1.0)
						{
							// band spans multiple pixels: draw all pixels it covers
							for (target = (int)band_start; target < (int)band_end; target++)
							{
								if (target >= 0 && target < binCount())
								{
									pixel[target] = makePixel(m_normSpectrumL[i], m_normSpectrumR[i]);
								}
							}
							// save remaining portion of the band for the following band / pixel
							// (in case the next band uses sub-pixel drawing)
							accL = (band_end - (int)band_end) * m_normSpectrumL[i];
							accR = (band_end - (int)band_end) * m_normSpectrumR[i];
						}
						else
						{
							// sub-pixel drawing; add contribution of current band
							target = (int)band_start;
							if ((int)band_start == (int)band_end)
							{
								// band ends within current target pixel, accumulate
								accL += (band_end - band_start) * m_normSpectrumL[i];
								accR += (band_end - band_start) * m_normSpectrumR[i];
							}
							else
							{
								// Band ends in the next pixel -- finalize the current pixel.
								// Make sure contribution is split correctly on pixel boundary.
								accL += ((int)band_end - band_start) * m_normSpectrumL[i];
								accR += ((int)band_end - band_start) * m_normSpectrumR[i];
	
								if (target >= 0 && target < binCount()) {pixel[target] = makePixel(accL, accR);}

								// save remaining portion of the band for the following band / pixel
								accL = (band_end - (int)band_end) * m_normSpectrumL[i];
								accR = (band_end - (int)band_end) * m_normSpectrumR[i];
							}
						}
					}
					else
					{
						// Linear: always draws one or more pixels per band
						for (target = (int)band_start; target < band_end; target++)
						{
							if (target >= 0 && target < binCount())
							{
								pixel[target] = makePixel(m_normSpectrumL[i], m_normSpectrumR[i]);
							}
						}
					}
				}
			}
			#ifdef SA_DEBUG
				// report FFT processing speed
				start_time = std::chrono::high_resolution_clock::now().time_since_epoch().count() - start_time;
				std::cout << "Processed " << m_framesFilledUp << " samples in " << start_time / 1000000.0 << " ms" << std::endl;
			#endif

			// clean up before checking for more data from input buffer
			m_framesFilledUp = 0;
		}
	}
}


// Produce a spectrogram pixel from normalized spectrum data.
// Values over 1.0 will cause the color components to overflow: this is left
// intentionally untreated as it clearly indicates which frequency is clipping.
// Gamma correction is applied to make small values more visible and to make
// a linear gradient actually appear roughly linear. The correction should be
// around 0.42 to 0.45 for sRGB displays (or lower for bigger visibility boost).
QRgb SaProcessor::makePixel(float left, float right, float gamma_correction) const
{
	if (m_controls->m_stereoModel.value())
	{
		float ampL = pow(left, gamma_correction);
		float ampR = pow(right, gamma_correction);
		return qRgb(m_controls->m_colorL.red() * ampL + m_controls->m_colorR.red() * ampR,
					m_controls->m_colorL.green() * ampL + m_controls->m_colorR.green() * ampR,
					m_controls->m_colorL.blue() * ampL + m_controls->m_colorR.blue() * ampR);
	}
	else
	{
		float ampL = pow(left, gamma_correction);
		// make mono color brighter to compensate for the fact it is not summed
		return qRgb(m_controls->m_colorMono.lighter().red() * ampL,
					m_controls->m_colorMono.lighter().green() * ampL,
					m_controls->m_colorMono.lighter().blue() * ampL);
	}
}



// Inform the processor whether any display widgets actually need it.
void SaProcessor::setSpectrumActive(bool active)
{
	m_spectrumActive = active;
}

void SaProcessor::setWaterfallActive(bool active)
{
	m_waterfallActive = active;
}


// Reallocate data buffers according to newly set block size.
void SaProcessor::reallocateBuffers()
{
	unsigned int new_size_index = m_controls->m_blockSizeModel.value();
	unsigned int new_in_size, new_fft_size;
	unsigned int new_bins;

	// get new block sizes and bin count based on selected index
	if (new_size_index < FFT_BLOCK_SIZES.size())
	{
		new_in_size = FFT_BLOCK_SIZES[new_size_index];
	}
	else
	{
		new_in_size = FFT_BLOCK_SIZES.back();
	}
	if (new_size_index + m_zeroPadFactor < FFT_BLOCK_SIZES.size())
	{
		new_fft_size = FFT_BLOCK_SIZES[new_size_index + m_zeroPadFactor];
	}
	else
	{
		new_fft_size = FFT_BLOCK_SIZES.back();
	}

	new_bins = new_fft_size / 2 +1;

	// Lock data shared with SaSpectrumView and SaWaterfallView.
	// The m_reallocating is here to tell analyse() to avoid asking for the
	// lock, since fftw3 can take a while to find the fastest FFT algorithm
	// for given machine, which would produce interruption in the audio stream.
	m_reallocating = true;
	QMutexLocker lock(&m_dataAccess);

	// destroy old FFT plan and free the result buffer
	if (m_fftPlanL != NULL) {fftwf_destroy_plan(m_fftPlanL);}
	if (m_fftPlanR != NULL) {fftwf_destroy_plan(m_fftPlanR);}
	if (m_spectrumL != NULL) {fftwf_free(m_spectrumL);}
	if (m_spectrumR != NULL) {fftwf_free(m_spectrumR);}

	// allocate new space, create new plan and resize containers
	m_fftWindow.resize(new_in_size, 1.0);
	precomputeWindow(m_fftWindow.data(), new_in_size, (FFT_WINDOWS) m_controls->m_windowModel.value());
	m_bufferL.resize(new_fft_size, 0);
	m_bufferR.resize(new_fft_size, 0);
	m_spectrumL = (fftwf_complex *) fftwf_malloc(new_bins * sizeof (fftwf_complex));
	m_spectrumR = (fftwf_complex *) fftwf_malloc(new_bins * sizeof (fftwf_complex));
	m_fftPlanL = fftwf_plan_dft_r2c_1d(new_fft_size, m_bufferL.data(), m_spectrumL, FFTW_MEASURE);
	m_fftPlanR = fftwf_plan_dft_r2c_1d(new_fft_size, m_bufferR.data(), m_spectrumR, FFTW_MEASURE);

	if (m_fftPlanL == NULL || m_fftPlanR == NULL)
	{
		std::cerr << "Failed to create new FFT plan!" << std::endl;
	}
	m_absSpectrumL.resize(new_bins, 0);
	m_absSpectrumR.resize(new_bins, 0);
	m_normSpectrumL.resize(new_bins, 0);
	m_normSpectrumR.resize(new_bins, 0);

	m_history.resize(new_bins * m_waterfallHeight * sizeof qRgb(0,0,0), 0);

	// done; publish new sizes and clean up
	m_inBlockSize = new_in_size;
	m_fftBlockSize = new_fft_size;

	lock.unlock();
	m_reallocating = false;
	clear();
}


// Precompute a new FFT window based on currently selected type.
void SaProcessor::rebuildWindow()
{
	// computation is done in fft_helpers
	QMutexLocker lock(&m_dataAccess);
	precomputeWindow(m_fftWindow.data(), m_inBlockSize, (FFT_WINDOWS) m_controls->m_windowModel.value());
}


// Clear all data buffers and replace contents with zeros.
// Note: may take a few milliseconds, do not call in a loop!
void SaProcessor::clear()
{
	QMutexLocker lock(&m_dataAccess);
	m_framesFilledUp = 0;
	std::fill(m_bufferL.begin(), m_bufferL.end(), 0);
	std::fill(m_bufferR.begin(), m_bufferR.end(), 0);
	std::fill(m_absSpectrumL.begin(), m_absSpectrumL.end(), 0);
	std::fill(m_absSpectrumR.begin(), m_absSpectrumR.end(), 0);
	std::fill(m_normSpectrumL.begin(), m_normSpectrumL.end(), 0);
	std::fill(m_normSpectrumR.begin(), m_normSpectrumR.end(), 0);
	std::fill(m_history.begin(), m_history.end(), 0);
}


// --------------------------------------
// Frequency conversion helpers
//

// Get sample rate value that is valid for currently stored results.
unsigned int SaProcessor::getSampleRate() const
{
	return m_sampleRate;
}


// Maximum frequency of a sampled signal is equal to half of its sample rate.
float SaProcessor::getNyquistFreq() const
{
	return getSampleRate() / 2.0f;
}


// FFTW automatically discards upper half of the symmetric FFT output, so
// the useful bin count is the transform size divided by 2, plus zero.
unsigned int SaProcessor::binCount() const
{
	return m_fftBlockSize / 2 + 1;
}


// Return the center frequency of given frequency bin.
float SaProcessor::binToFreq(unsigned int bin_index) const
{
	return getNyquistFreq() * bin_index / binCount();
}


// Return width of the frequency range that falls into one bin.
// The binCount is lowered by one since half of the first and last bin is
// actually outside the frequency range.
float SaProcessor::binBandwidth() const
{
	return getNyquistFreq() / (binCount() - 1);
}


float SaProcessor::getFreqRangeMin(bool linear) const
{
	switch (m_controls->m_freqRangeModel.value())
	{
		case FRANGE_AUDIBLE: return FRANGE_AUDIBLE_START;
		case FRANGE_BASS: return FRANGE_BASS_START;
		case FRANGE_MIDS: return FRANGE_MIDS_START;
		case FRANGE_HIGH: return FRANGE_HIGH_START;
		default:
		case FRANGE_FULL: return linear ? 0 : LOWEST_LOG_FREQ;
	}
}


float SaProcessor::getFreqRangeMax() const
{
	switch (m_controls->m_freqRangeModel.value())
	{
		case FRANGE_AUDIBLE: return FRANGE_AUDIBLE_END;
		case FRANGE_BASS: return FRANGE_BASS_END;
		case FRANGE_MIDS: return FRANGE_MIDS_END;
		case FRANGE_HIGH: return FRANGE_HIGH_END;
		default:
		case FRANGE_FULL: return getNyquistFreq();
	}
}


// Map frequency to pixel x position on a display of given width.
float SaProcessor::freqToXPixel(float freq, unsigned int width) const
{
	if (m_controls->m_logXModel.value())
	{
		if (freq <= 1) {return 0;}
		float min = log10(getFreqRangeMin());
		float range = log10(getFreqRangeMax()) - min;
		return (log10(freq) - min) / range * width;
	}
	else
	{
		float min = getFreqRangeMin();
		float range = getFreqRangeMax() - min;
		return (freq - min) / range * width;
	}
}


// Map pixel x position on display of given width back to frequency.
float SaProcessor::xPixelToFreq(float x, unsigned int width) const
{
	if (m_controls->m_logXModel.value())
	{
		float min = log10(getFreqRangeMin());
		float max = log10(getFreqRangeMax());
		float range = max - min;
		return pow(10, min + x / width * range);
	}
	else
	{
		float min = getFreqRangeMin();
		float range = getFreqRangeMax() - min;
		return min + x / width * range;
	}
}


// --------------------------------------
// Amplitude conversion helpers
//
float SaProcessor::getAmpRangeMin(bool linear) const
{
	// return very low limit to make sure zero gets included at linear grid
	if (linear) {return -900;}
	switch (m_controls->m_ampRangeModel.value())
	{
		case ARANGE_EXTENDED: return ARANGE_EXTENDED_START;
		case ARANGE_AUDIBLE: return ARANGE_AUDIBLE_START;
		case ARANGE_NOISE: return ARANGE_NOISE_START;
		default:
		case ARANGE_DEFAULT: return ARANGE_DEFAULT_START;
	}
}


float SaProcessor::getAmpRangeMax() const
{
	switch (m_controls->m_ampRangeModel.value())
	{
		case ARANGE_EXTENDED: return ARANGE_EXTENDED_END;
		case ARANGE_AUDIBLE: return ARANGE_AUDIBLE_END;
		case ARANGE_NOISE: return ARANGE_NOISE_END;
		default:
		case ARANGE_DEFAULT: return ARANGE_DEFAULT_END;
	}
}


// Map linear amplitude to pixel y position on a display of given height.
// Note that display coordinates are flipped: amplitude grows from [height] to zero.
float SaProcessor::ampToYPixel(float amplitude, unsigned int height) const
{
	if (m_controls->m_logYModel.value())
	{
		// logarithmic scale: convert linear amplitude to dB (relative to 1.0)
		float amplitude_dB = 10 * log10(amplitude);
		if (amplitude_dB < getAmpRangeMin())
		{
			return height;
		}
		else
		{
			float max = getAmpRangeMax();
			float range = getAmpRangeMin() - max;
			return (amplitude_dB - max) / range * height;
		}
	}
	else
	{
		// linear scale: convert returned ranges from dB to linear scale
		float max = pow(10, getAmpRangeMax() / 10);
		float range = pow(10, getAmpRangeMin() / 10) - max;
		return (amplitude - max) / range * height;
	}
}


// Map pixel y position on display of given height back to amplitude.
// Note that display coordinates are flipped: amplitude grows from [height] to zero.
// Also note that in logarithmic Y mode the returned amplitude is in dB, not linear.
float SaProcessor::yPixelToAmp(float y, unsigned int height) const
{
	if (m_controls->m_logYModel.value())
	{
		float max = getAmpRangeMax();
		float range = getAmpRangeMin() - max;
		return max + range * (y / height);
	}
	else
	{
		// linear scale: convert returned ranges from dB to linear scale
		float max = pow(10, getAmpRangeMax() / 10);
		float range = pow(10, getAmpRangeMin() / 10) - max;
		return max + range * (y / height);
	}
}

