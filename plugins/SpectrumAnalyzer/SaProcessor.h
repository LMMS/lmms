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

#include <QImage>
#include <QMutex>
#include <vector>

#include "fft_helpers.h"
#include "SaControls.h"


// Receives audio data, runs FFT analysis and stores the result.
class SaProcessor {
public:
	SaProcessor(SaControls *controls);
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
	float binToFreq(int index);
	float binBandwidth();

	float freqToXPixel(float frequency, int width);
	float xPixelToFreq(float x, int width);

	float ampToYPixel(float amplitude, int height);
	float yPixelToAmp(float y, int height);

	int getSampleRate() const;
	float getFreqRangeMin(bool linear = false);
	float getFreqRangeMax();
	float getAmpRangeMin(bool linear = false);
	float getAmpRangeMax();

	// data access lock must be acquired by any friendly class that touches
	// the results, mainly to prevent unexpected mid-way reallocation
	QMutex m_dataAccess;

private:
	SaControls *m_controls;

	// currently valid configuration
	const unsigned int m_zeroPadFactor = 2;	// use n-steps bigger FFT for given block
	unsigned int m_inBlockSize;				// size of input data block
	unsigned int m_fftBlockSize;			// size of padded block for FFT processing
	unsigned int m_sampleRate;

	unsigned int binCount() {return m_fftBlockSize / 2 + 1;}

	// data buffers (roughly in the order of processing, from input to output)
	unsigned int m_framesFilledUp;
	std::vector<float> m_bufferL;
	std::vector<float> m_bufferR;
	std::vector<float> m_fftWindow;
	fftwf_plan m_fftPlanL;
	fftwf_plan m_fftPlanR;
	fftwf_complex *m_spectrumL;
	fftwf_complex *m_spectrumR;
	std::vector<float> m_absSpectrumL;
	std::vector<float> m_absSpectrumR;
	std::vector<float> m_normSpectrumL;
	std::vector<float> m_normSpectrumR;

	// spectrum history for waterfall: new normSpectrum lines are added on top
	std::vector<uchar> m_history;
	const int m_waterfallHeight = 200;	// Number of stored lines.
										// Note: high values may make it harder to see transients.

	// book keeping
	bool m_spectrumActive;
	bool m_waterfallActive;
	bool m_destroyed;
	bool m_reallocating;

	// merge L and R channels and apply gamma correction to make a spectrogram pixel
	QRgb makePixel(float left, float right, float gamma_correction = 0.42);

	friend class SaSpectrumView;
	friend class SaWaterfallView;
};
#endif // SAPROCESSOR_H

