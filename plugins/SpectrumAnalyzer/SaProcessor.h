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

#include <mutex>
#include <QImage>
#include <vector>

#include "fft_helpers.h"
#include "SaControls.h"

class SaProcessor {
public:
	SaProcessor(SaControls *controls);
	virtual ~SaProcessor();

	void analyse(sampleFrame *buf, const fpp_t frames);

	void setActive(bool active);
	bool getActive() const;

	void reallocateBuffers();
	void rebuildWindow();
	void clear();

	float binToFreq(int index);
	float binBandwidth();

	float freqToXPixel(float frequency, int width);
	float xPixelToFreq(float x, int width);

	float ampToYPixel(float amplitude, int height);
	float yPixelToAmp(float y, int height);

	int getSampleRate() const;
	float getFreqRangeMin(bool linear = false);
	float getFreqRangeMax();
	float getAmpRangeMin();
	float getAmpRangeMax();

	std::mutex m_dataAccess;

private:
	SaControls *m_controls;

	unsigned int m_blockSize;		// size of FFT input block
	unsigned int m_sampleRate;
	unsigned int m_windowType;

	unsigned int binCount() {return m_blockSize / 2 + 1;}	// number of output frequency bins

	unsigned int m_framesFilledUp;
	std::vector<float> m_fftWindow;
	std::vector<float> m_bufferL;
	std::vector<float> m_bufferR;
	fftwf_plan m_fftPlanL;
	fftwf_plan m_fftPlanR;
	fftwf_complex * m_spectrumL;
	fftwf_complex * m_spectrumR;
	std::vector<float> m_absSpectrumL;
	std::vector<float> m_absSpectrumR;
	std::vector<float> m_normSpectrumL;
	std::vector<float> m_normSpectrumR;

	std::vector<uchar> m_history;

	bool m_active;
	bool m_destroyed;

	QRgb makePixel(float left, float right);

	friend class SaSpectrumView;
	friend class SaWaterfallView;
};
#endif // SAPROCESSOR_H

