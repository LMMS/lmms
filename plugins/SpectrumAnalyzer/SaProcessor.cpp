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
#include "lmms_math.h"
#include <cmath>
#ifdef SA_DEBUG
	#include <chrono>
	#include <iomanip>
	#include <iostream>
#endif
#include <QMutexLocker>

#include "fft_helpers.h"
#include "lmms_constants.h"
#include "LocklessRingBuffer.h"
#include "SaControls.h"

#include <cassert>
#include <limits>

namespace lmms
{


SaProcessor::SaProcessor(const SaControls *controls) :
	m_controls(controls),
	m_terminate(false),
	m_inBlockSize(FFT_BLOCK_SIZES[0]),
	m_fftBlockSize(FFT_BLOCK_SIZES[0]),
	m_sampleRate(Engine::audioEngine()->outputSampleRate()),
	m_framesFilledUp(0),
	m_spectrumActive(false),
	m_waterfallActive(false),
	m_waterfallNotEmpty(0),
	m_reallocating(false)
{
	m_fftWindow.resize(m_inBlockSize, 1.0);
	precomputeWindow(m_fftWindow.data(), m_inBlockSize, FFTWindow::BlackmanHarris);

	m_bufferL.resize(m_inBlockSize, 0);
	m_bufferR.resize(m_inBlockSize, 0);
	m_filteredBufferL.resize(m_fftBlockSize, 0);
	m_filteredBufferR.resize(m_fftBlockSize, 0);
	m_spectrumL = (fftwf_complex *) fftwf_malloc(binCount() * sizeof (fftwf_complex));
	m_spectrumR = (fftwf_complex *) fftwf_malloc(binCount() * sizeof (fftwf_complex));
	m_fftPlanL = fftwf_plan_dft_r2c_1d(m_fftBlockSize, m_filteredBufferL.data(), m_spectrumL, FFTW_MEASURE);
	m_fftPlanR = fftwf_plan_dft_r2c_1d(m_fftBlockSize, m_filteredBufferR.data(), m_spectrumR, FFTW_MEASURE);

	m_absSpectrumL.resize(binCount(), 0);
	m_absSpectrumR.resize(binCount(), 0);
	m_normSpectrumL.resize(binCount(), 0);
	m_normSpectrumR.resize(binCount(), 0);

	m_waterfallHeight = 100;	// a small safe value
	m_history_work.resize(waterfallWidth() * m_waterfallHeight * sizeof qRgb(0,0,0), 0);
	m_history.resize(waterfallWidth() * m_waterfallHeight * sizeof qRgb(0,0,0), 0);
}


SaProcessor::~SaProcessor()
{
	if (m_fftPlanL != nullptr) {fftwf_destroy_plan(m_fftPlanL);}
	if (m_fftPlanR != nullptr) {fftwf_destroy_plan(m_fftPlanR);}
	if (m_spectrumL != nullptr) {fftwf_free(m_spectrumL);}
	if (m_spectrumR != nullptr) {fftwf_free(m_spectrumR);}

	m_fftPlanL = nullptr;
	m_fftPlanR = nullptr;
	m_spectrumL = nullptr;
	m_spectrumR = nullptr;
}


// Load data from audio thread ringbuffer and run FFT analysis if buffer is full enough.
void SaProcessor::analyze(LocklessRingBuffer<SampleFrame> &ring_buffer)
{
	LocklessRingBufferReader<SampleFrame> reader(ring_buffer);

	// Processing thread loop
	while (!m_terminate)
	{
		// If there is nothing to read, wait for notification from the writing side.
		if (reader.empty()) {reader.waitForData();}

		// skip waterfall render if processing can't keep up with input
		bool overload = ring_buffer.free() < ring_buffer.capacity() / 2;

		auto in_buffer = reader.read_max(ring_buffer.capacity() / 4);
		std::size_t frame_count = in_buffer.size();

		// Process received data only if any view is visible and not paused.
		// Also, to prevent a momentary GUI freeze under high load (due to lock
		// starvation), skip analysis when buffer reallocation is requested.
		if ((m_spectrumActive || m_waterfallActive) && !m_controls->m_pauseModel.value() && !m_reallocating)
		{
			const bool stereo = m_controls->m_stereoModel.value();
			fpp_t in_frame = 0;
			while (in_frame < frame_count)
			{
				// Lock data access to prevent reallocation from changing
				// buffers and control variables.
				QMutexLocker data_lock(&m_dataAccess);

				// Fill sample buffers and check for zero input.
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
				if (m_framesFilledUp < m_inBlockSize) {break;}

				// Print performance analysis once per 2 seconds if debug is enabled
				#ifdef SA_DEBUG
					unsigned int total_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
					if (total_time - m_last_dump_time > 2000000000)
					{
						std::cout << "FFT analysis: " << std::fixed << std::setprecision(2)
							<< m_sum_execution / m_dump_count << " ms avg / "
							<< m_max_execution << " ms peak, executing "
							<< m_dump_count << " times per second ("
							<< m_sum_execution / 20.0 << " % CPU usage)." << std::endl;
						m_last_dump_time = total_time;
						m_sum_execution = m_max_execution = m_dump_count = 0;
					}
				#endif

				// update sample rate
				m_sampleRate = Engine::audioEngine()->outputSampleRate();

				// apply FFT window
				for (unsigned int i = 0; i < m_inBlockSize; i++)
				{
					m_filteredBufferL[i] = m_bufferL[i] * m_fftWindow[i];
					m_filteredBufferR[i] = m_bufferR[i] * m_fftWindow[i];
				}

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
					auto pixel = (QRgb*)m_history_work.data();
					std::copy(pixel,
							  pixel + waterfallWidth() * m_waterfallHeight - waterfallWidth(),
							  pixel + waterfallWidth());
					memset(pixel, 0, waterfallWidth() * sizeof (QRgb));

					// add newest result on top
					float accL = 0;	// accumulators for merging multiple bins
					float accR = 0;
					for (unsigned int i = 0; i < binCount(); i++)
					{
						// fill line with red color to indicate lost data if CPU cannot keep up
						if (overload && i < waterfallWidth())
						{
							pixel[i] = qRgb(42, 0, 0);
							continue;
						}

						// Every frequency bin spans a frequency range that must be
						// partially or fully mapped to a pixel. Any inconsistency
						// may be seen in the spectrogram as dark or white lines --
						// play white noise to confirm your change did not break it.
						float band_start = freqToXPixel(binToFreq(i) - binBandwidth() / 2.0, waterfallWidth());
						float band_end = freqToXPixel(binToFreq(i + 1) - binBandwidth() / 2.0, waterfallWidth());
						if (m_controls->m_logXModel.value())
						{
							// Logarithmic scale
							if (band_end - band_start > 1.0)
							{
								// band spans multiple pixels: draw all pixels it covers
								for (auto target = static_cast<std::size_t>(std::max(band_start, 0.f));
									 target < band_end && target < waterfallWidth(); target++)
								{
									pixel[target] = makePixel(m_normSpectrumL[i], m_normSpectrumR[i]);
								}
								// save remaining portion of the band for the following band / pixel
								// (in case the next band uses sub-pixel drawing)
								accL = (band_end - (int)band_end) * m_normSpectrumL[i];
								accR = (band_end - (int)band_end) * m_normSpectrumR[i];
							}
							else
							{
								// sub-pixel drawing; add contribution of current band
								int target = static_cast<int>(band_start);
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

									if (target >= 0 && static_cast<std::size_t>(target) < waterfallWidth()) {
										pixel[target] = makePixel(accL, accR);
									}

									// save remaining portion of the band for the following band / pixel
									accL = (band_end - (int)band_end) * m_normSpectrumL[i];
									accR = (band_end - (int)band_end) * m_normSpectrumR[i];
								}
							}
						}
						else
						{
							// Linear: always draws one or more pixels per band
							for (auto target = static_cast<std::size_t>(std::max(band_start, 0.f));
								 target < band_end && target < waterfallWidth(); target++)
							{
								pixel[target] = makePixel(m_normSpectrumL[i], m_normSpectrumR[i]);
							}
						}
					}

					// Copy work buffer to result buffer. Done only if requested, so
					// that time isn't wasted on updating faster than display FPS.
					// (The copy is about as expensive as the movement.)
					if (m_flipRequest)
					{
						m_history = m_history_work;
						m_flipRequest = false;
					}
				}
				// clean up before checking for more data from input buffer
				const unsigned int overlaps = m_controls->m_windowOverlapModel.value();
				if (overlaps == 1)	// Discard buffer, each sample used only once
				{
					m_framesFilledUp = 0;
				}
				else
				{
					// Drop only a part of the buffer from the beginning, so that new
					// data can be added to the end. This means the older samples will
					// be analyzed again, but in a different position in the window,
					// making short transient signals show up better in the waterfall.
					const unsigned int drop = m_inBlockSize / overlaps;
					std::move(m_bufferL.begin() + drop, m_bufferL.end(), m_bufferL.begin());
					std::move(m_bufferR.begin() + drop, m_bufferR.end(), m_bufferR.begin());
					m_framesFilledUp -= drop;
				}

				#ifdef SA_DEBUG
					// measure overall FFT processing speed
					total_time = std::chrono::high_resolution_clock::now().time_since_epoch().count() - total_time;
					m_dump_count++;
					m_sum_execution += total_time / 1000000.0;
					if (total_time / 1000000.0 > m_max_execution) {m_max_execution = total_time / 1000000.0;}
				#endif
			}	// frame filler and processing
		}	// process if active
	}	// thread loop end
}


// Produce a spectrogram pixel from normalized spectrum data.
// Values over 1.0 will cause the color components to overflow: this is left
// intentionally untreated as it clearly indicates which frequency is clipping.
// Gamma correction is applied to make small values more visible and to make
// a linear gradient actually appear roughly linear. The correction should be
// around 0.42 to 0.45 for sRGB displays (or lower for bigger visibility boost).
QRgb SaProcessor::makePixel(float left, float right) const
{
	const float gamma_correction = m_controls->m_waterfallGammaModel.value();
	if (m_controls->m_stereoModel.value())
	{
		float ampL = std::pow(left, gamma_correction);
		float ampR = std::pow(right, gamma_correction);
		return qRgb(m_controls->m_colorL.red() * ampL + m_controls->m_colorR.red() * ampR,
					m_controls->m_colorL.green() * ampL + m_controls->m_colorR.green() * ampR,
					m_controls->m_colorL.blue() * ampL + m_controls->m_colorR.blue() * ampR);
	}
	else
	{
		float ampL = std::pow(left, gamma_correction);
		// make mono color brighter to compensate for the fact it is not summed
		return qRgb(m_controls->m_colorMonoW.red() * ampL,
					m_controls->m_colorMonoW.green() * ampL,
					m_controls->m_colorMonoW.blue() * ampL);
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
	m_zeroPadFactor = m_controls->m_zeroPaddingModel.value();

	// get new block sizes and bin count based on selected index
	const unsigned int new_size_index = m_controls->m_blockSizeModel.value();

	const unsigned int new_in_size = new_size_index < FFT_BLOCK_SIZES.size()
		? FFT_BLOCK_SIZES[new_size_index]
		: FFT_BLOCK_SIZES.back();

	const unsigned int new_fft_size = (new_size_index + m_zeroPadFactor < FFT_BLOCK_SIZES.size())
		? FFT_BLOCK_SIZES[new_size_index + m_zeroPadFactor]
		: FFT_BLOCK_SIZES.back();

	const unsigned int new_bins = new_fft_size / 2 + 1;

	// Use m_reallocating to tell analyze() to avoid asking for the lock. This
	// is needed because under heavy load the FFT thread requests data lock so
	// often that this routine could end up waiting even for several seconds.
	m_reallocating = true;

	// Lock data shared with SaSpectrumView and SaWaterfallView.
	// Reallocation lock must be acquired first to avoid deadlock (a view class
	// may already have it and request the "stronger" data lock on top of that).
	QMutexLocker reloc_lock(&m_reallocationAccess);
	QMutexLocker data_lock(&m_dataAccess);

	// destroy old FFT plan and free the result buffer
	if (m_fftPlanL != nullptr) {fftwf_destroy_plan(m_fftPlanL);}
	if (m_fftPlanR != nullptr) {fftwf_destroy_plan(m_fftPlanR);}
	if (m_spectrumL != nullptr) {fftwf_free(m_spectrumL);}
	if (m_spectrumR != nullptr) {fftwf_free(m_spectrumR);}

	// allocate new space, create new plan and resize containers
	m_fftWindow.resize(new_in_size, 1.0);
	precomputeWindow(m_fftWindow.data(), new_in_size, (FFTWindow) m_controls->m_windowModel.value());
	m_bufferL.resize(new_in_size, 0);
	m_bufferR.resize(new_in_size, 0);
	m_filteredBufferL.resize(new_fft_size, 0);
	m_filteredBufferR.resize(new_fft_size, 0);
	m_spectrumL = (fftwf_complex *) fftwf_malloc(new_bins * sizeof (fftwf_complex));
	m_spectrumR = (fftwf_complex *) fftwf_malloc(new_bins * sizeof (fftwf_complex));
	m_fftPlanL = fftwf_plan_dft_r2c_1d(new_fft_size, m_filteredBufferL.data(), m_spectrumL, FFTW_MEASURE);
	m_fftPlanR = fftwf_plan_dft_r2c_1d(new_fft_size, m_filteredBufferR.data(), m_spectrumR, FFTW_MEASURE);

	if (m_fftPlanL == nullptr || m_fftPlanR == nullptr)
	{
		#ifdef SA_DEBUG
			std::cerr << "Analyzer: failed to create new FFT plan!" << std::endl;
		#endif
	}
	m_absSpectrumL.resize(new_bins, 0);
	m_absSpectrumR.resize(new_bins, 0);
	m_normSpectrumL.resize(new_bins, 0);
	m_normSpectrumR.resize(new_bins, 0);

	m_waterfallHeight = m_controls->m_waterfallHeightModel.value();
	m_history_work.resize((new_bins < m_waterfallMaxWidth ? new_bins : m_waterfallMaxWidth)
							* m_waterfallHeight
							* sizeof qRgb(0,0,0), 0);
	m_history.resize((new_bins < m_waterfallMaxWidth ? new_bins : m_waterfallMaxWidth)
						* m_waterfallHeight
						* sizeof qRgb(0,0,0), 0);

	// done; publish new sizes and clean up
	m_inBlockSize = new_in_size;
	m_fftBlockSize = new_fft_size;

	data_lock.unlock();
	reloc_lock.unlock();
	m_reallocating = false;

	clear();
}


// Precompute a new FFT window based on currently selected type.
void SaProcessor::rebuildWindow()
{
	// computation is done in fft_helpers
	QMutexLocker lock(&m_dataAccess);
	precomputeWindow(m_fftWindow.data(), m_inBlockSize, (FFTWindow) m_controls->m_windowModel.value());
}


// Clear all data buffers and replace contents with zeros.
// Note: may take a few milliseconds, do not call in a loop!
void SaProcessor::clear()
{
	const unsigned int overlaps = m_controls->m_windowOverlapModel.value();
	QMutexLocker lock(&m_dataAccess);
	// If there is any window overlap, leave space only for the new samples
	// and treat the rest at initialized with zeros. Prevents missing
	// transients at the start of the very first block.
	m_framesFilledUp = m_inBlockSize - m_inBlockSize / overlaps;
	std::fill(m_bufferL.begin(), m_bufferL.end(), 0);
	std::fill(m_bufferR.begin(), m_bufferR.end(), 0);
	std::fill(m_filteredBufferL.begin(), m_filteredBufferL.end(), 0);
	std::fill(m_filteredBufferR.begin(), m_filteredBufferR.end(), 0);
	std::fill(m_absSpectrumL.begin(), m_absSpectrumL.end(), 0);
	std::fill(m_absSpectrumR.begin(), m_absSpectrumR.end(), 0);
	std::fill(m_normSpectrumL.begin(), m_normSpectrumL.end(), 0);
	std::fill(m_normSpectrumR.begin(), m_normSpectrumR.end(), 0);
	std::fill(m_history_work.begin(), m_history_work.end(), 0);
	std::fill(m_history.begin(), m_history.end(), 0);
}

// Clear only history work buffer. Used to flush old data when waterfall
// is shown after a period of inactivity.
void SaProcessor::clearHistory()
{
	QMutexLocker lock(&m_dataAccess);
	std::fill(m_history_work.begin(), m_history_work.end(), 0);
}

// Check if result buffers contain any non-zero values
bool SaProcessor::spectrumNotEmpty()
{
	QMutexLocker lock(&m_reallocationAccess);
	return notEmpty(m_normSpectrumL) || notEmpty(m_normSpectrumR);
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


// Return the final width of waterfall display buffer.
// Normally the waterfall width equals the number of frequency bins, but the
// FFT transform can easily produce more bins than can be reasonably useful for
// currently used display resolutions. This function limits width of the final
// image to a given size, which is then used during waterfall render and display.
unsigned int SaProcessor::waterfallWidth() const
{
	return binCount() < m_waterfallMaxWidth ? binCount() : m_waterfallMaxWidth;
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
	switch (static_cast<FrequencyRange>(m_controls->m_freqRangeModel.value()))
	{
		case FrequencyRange::Audible: return FRANGE_AUDIBLE_START;
		case FrequencyRange::Bass: return FRANGE_BASS_START;
		case FrequencyRange::Mids: return FRANGE_MIDS_START;
		case FrequencyRange::High: return FRANGE_HIGH_START;
		default:
		case FrequencyRange::Full: return linear ? 0 : LOWEST_LOG_FREQ;
	}
}


float SaProcessor::getFreqRangeMax() const
{
	switch (static_cast<FrequencyRange>(m_controls->m_freqRangeModel.value()))
	{
		case FrequencyRange::Audible: return FRANGE_AUDIBLE_END;
		case FrequencyRange::Bass: return FRANGE_BASS_END;
		case FrequencyRange::Mids: return FRANGE_MIDS_END;
		case FrequencyRange::High: return FRANGE_HIGH_END;
		default:
		case FrequencyRange::Full: return getNyquistFreq();
	}
}


// Map frequency to pixel x position on a display of given width.
// NOTE: Results of this function may be cached by SaSpectrumView. If you use
// a new function call or variable that can affect results of this function,
// make sure to also add it as a trigger for cache update in SaSpectrumView.
float SaProcessor::freqToXPixel(float freq, unsigned int width) const
{
	if (m_controls->m_logXModel.value())
	{
		if (freq <= 1) {return 0;}
		float min = std::log10(getFreqRangeMin());
		float range = std::log10(getFreqRangeMax()) - min;
		return (std::log10(freq) - min) / range * width;
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
		float min = std::log10(getFreqRangeMin());
		float max = std::log10(getFreqRangeMax());
		float range = max - min;
		return fastPow10f(min + x / width * range);
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
	switch (static_cast<AmplitudeRange>(m_controls->m_ampRangeModel.value()))
	{
		case AmplitudeRange::Extended: return ARANGE_EXTENDED_START;
		case AmplitudeRange::Silent: return ARANGE_SILENT_START;
		case AmplitudeRange::Loud: return ARANGE_LOUD_START;
		default:
		case AmplitudeRange::Audible: return ARANGE_AUDIBLE_START;
	}
}


float SaProcessor::getAmpRangeMax() const
{
	switch (static_cast<AmplitudeRange>(m_controls->m_ampRangeModel.value()))
	{
		case AmplitudeRange::Extended: return ARANGE_EXTENDED_END;
		case AmplitudeRange::Silent: return ARANGE_SILENT_END;
		case AmplitudeRange::Loud: return ARANGE_LOUD_END;
		default:
		case AmplitudeRange::Audible: return ARANGE_AUDIBLE_END;
	}
}


// Map linear amplitude to pixel y position on a display of given height.
// Note that display coordinates are flipped: amplitude grows from [height] to zero.
float SaProcessor::ampToYPixel(float amplitude, unsigned int height) const
{
	if (m_controls->m_logYModel.value())
	{
		// logarithmic scale: convert linear amplitude to dB (relative to 1.0)
		assert (amplitude >= 0);
		float amplitude_dB = 10 * std::log10(std::max(amplitude, std::numeric_limits<float>::min()));
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
		float max = fastPow10f(getAmpRangeMax() / 10);
		float range = fastPow10f(getAmpRangeMin() / 10) - max;
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
		float max = fastPow10f(getAmpRangeMax() / 10);
		float range = fastPow10f(getAmpRangeMin() / 10) - max;
		return max + range * (y / height);
	}
}


} // namespace lmms
