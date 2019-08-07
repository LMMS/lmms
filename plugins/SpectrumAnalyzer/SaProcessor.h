/* SaProcessor.h - declaration of SaProcessor class.
 *
 * Copyright (c) 2019 Martin Pavelek <he29/dot/HS/at/gmail/dot/com>
 *
 * Based partially on Eq plugin code,
 * Copyright (c) 2014 David French <dave/dot/french3/at/googlemail/dot/com>
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

#ifndef SAPROCESSOR_H
#define SAPROCESSOR_H

#include <QColor>
#include <QMutex>
#include <vector>

#include "fft_helpers.h"
#include "SaControls.h"


//! Receives audio data, runs FFT analysis and stores the result.
class SaProcessor
{
public:
	explicit SaProcessor(SaControls *controls);
	virtual ~SaProcessor();

	void analyse(sampleFrame *in_buffer, const fpp_t frame_count);

	// inform processor if any processing is actually required
	void setSpectrumActive(bool active);
	void setWaterfallActive(bool active);

	// configuration is taken from models in SaControls; some changes require
	// an exlicit update request (reallocation and window rebuild)
	void reallocateBuffers();
	void rebuildWindow();
	void clear();

	// information about results and unit conversion helpers
	float binToFreq(unsigned int bin_index) const;
	float binBandwidth() const;

	float freqToXPixel(float frequency, unsigned int width) const;
	float xPixelToFreq(float x, unsigned int width) const;

	float ampToYPixel(float amplitude, unsigned int height) const;
	float yPixelToAmp(float y, unsigned int height) const;

	unsigned int getSampleRate() const;
	float getNyquistFreq() const;

	float getFreqRangeMin(bool linear = false) const;
	float getFreqRangeMax() const;
	float getAmpRangeMin(bool linear = false) const;
	float getAmpRangeMax() const;

	// data access lock must be acquired by any friendly class that touches
	// the results, mainly to prevent unexpected mid-way reallocation
	QMutex m_dataAccess;

private:
	SaControls *m_controls;

	// currently valid configuration
	const unsigned int m_zeroPadFactor = 2;	//!< use n-steps bigger FFT for given block size
	unsigned int m_inBlockSize;				//!< size of input (time domain) data block
	unsigned int m_fftBlockSize;			//!< size of padded block for FFT processing
	unsigned int m_sampleRate;

	unsigned int binCount() const;			//!< size of output (frequency domain) data block

	// data buffers (roughly in the order of processing, from input to output)
	unsigned int m_framesFilledUp;
	std::vector<float> m_bufferL;			//!< time domain samples (left)
	std::vector<float> m_bufferR;			//!< time domain samples (right)
	std::vector<float> m_fftWindow;			//!< precomputed window function coefficients
	fftwf_plan m_fftPlanL;
	fftwf_plan m_fftPlanR;
	fftwf_complex *m_spectrumL;				//!< frequency domain samples (complex) (left)
	fftwf_complex *m_spectrumR;				//!< frequency domain samples (complex) (right)
	std::vector<float> m_absSpectrumL;		//!< frequency domain samples (absolute) (left)
	std::vector<float> m_absSpectrumR;		//!< frequency domain samples (absolute) (right)
	std::vector<float> m_normSpectrumL;		//!< frequency domain samples (normalized) (left)
	std::vector<float> m_normSpectrumR;     //!< frequency domain samples (normalized) (right)

	// spectrum history for waterfall: new normSpectrum lines are added on top
	std::vector<uchar> m_history;
	const unsigned int m_waterfallHeight = 200;	// Number of stored lines.
												// Note: high values may make it harder to see transients.

	// book keeping
	bool m_spectrumActive;
	bool m_waterfallActive;
	unsigned int m_waterfallNotEmpty;
	bool m_reallocating;

	// merge L and R channels and apply gamma correction to make a spectrogram pixel
	QRgb makePixel(float left, float right, float gamma_correction = 0.30) const;

	friend class SaSpectrumView;
	friend class SaWaterfallView;
};
#endif // SAPROCESSOR_H

