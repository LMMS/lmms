/*
 * fft_helpers.cpp - some functions around FFT analysis
 *
 * Copyright (c) 2008-2012 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2019 Martin Pavelek <he29.HS/at/gmail.com>
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


#include "fft_helpers.h"

#include <iostream>

#include <cmath>
#include <vector>
#include "lmms_constants.h"

namespace lmms
{


/* Returns biggest value from abs_spectrum[spec_size] array.
 *
 * return -1 on error, otherwise the maximum value
 */
float maximum(const float *abs_spectrum, unsigned int spec_size)
{
	if (abs_spectrum == nullptr) {return -1;}
	if (spec_size == 0) {return -1;}

	float maxi = 0;

	for (unsigned int i = 0; i < spec_size; i++)
	{
		if (abs_spectrum[i] > maxi) {maxi = abs_spectrum[i];}
	}
	return maxi;
}

float maximum(const std::vector<float> &abs_spectrum)
{
	return maximum(abs_spectrum.data(), abs_spectrum.size());
}


/* Normalize the array of absolute magnitudes to a 0..1 range.
 * Block size refers to FFT block size before any zero padding.
 *
 * return -1 on error, 0 on success
 */
int normalize(const float *abs_spectrum, float *norm_spectrum, unsigned int bin_count, unsigned int block_size)
{
	if (abs_spectrum == nullptr || norm_spectrum == nullptr) {return -1;}
	if (bin_count == 0 || block_size == 0) {return -1;}

	block_size /= 2;
	for (unsigned int i = 0; i < bin_count; i++)
	{
		norm_spectrum[i] = abs_spectrum[i] / block_size;
	}
	return 0;
}

int normalize(const std::vector<float> &abs_spectrum, std::vector<float> &norm_spectrum, unsigned int block_size)
{
	if (abs_spectrum.size() != norm_spectrum.size()) {return -1;}

	return normalize(abs_spectrum.data(), norm_spectrum.data(), abs_spectrum.size(), block_size);
}


/* Check if the spectrum contains any non-zero value.
 *
 * return 1 if spectrum contains any non-zero value
 * return 0 otherwise
 */
int notEmpty(const std::vector<float> &spectrum)
{
	for (float s : spectrum)
	{
		if (s != 0) {return 1;}
	}
	return 0;
}


/* Precompute an FFT window function for later real-time use.
 *
 * return -1 on error
 */
int precomputeWindow(float *window, unsigned int length, FFTWindow type, bool normalized)
{
	using namespace std::numbers;
	if (window == nullptr) {return -1;}

	float gain = 0;
	float a0;
	float a1;
	float a2;
	float a3;

	// constants taken from
	// https://en.wikipedia.org/wiki/Window_function#AList_of_window_functions
	switch (type)
	{
		default:
		case FFTWindow::Rectangular:
			for (unsigned int i = 0; i < length; i++) {window[i] = 1.0;}
			gain = 1;
			return 0;
		case FFTWindow::BlackmanHarris:	
			a0 = 0.35875f;
			a1 = 0.48829f;
			a2 = 0.14128f;
			a3 = 0.01168f;
			break;
		case FFTWindow::Hamming:
			a0 = 0.54f;
			a1 = 1.0 - a0;
			a2 = 0;
			a3 = 0;
			break;
		case FFTWindow::Hanning:
			a0 = 0.5;
			a1 = 1.0 - a0;
			a2 = 0;
			a3 = 0;
			break;
	}

	// common computation for cosine-sum based windows
	for (unsigned int i = 0; i < length; i++)
	{
		window[i] = (a0 - a1 * std::cos(2 * pi_v<float> * i / (static_cast<float>(length) - 1.0))
			+ a2 * std::cos(4 * pi_v<float> * i / (static_cast<float>(length) - 1.0))
			- a3 * std::cos(6 * pi_v<float> * i / (static_cast<float>(length) - 1.0)));
		gain += window[i];
	}

	// apply amplitude correction
	gain /= (float) length;
	for (unsigned int i = 0; i < length; i++) {window[i] /= gain;}

	return 0;
}


/* Compute absolute values of complex_buffer, save to absspec_buffer.
 * Take care that - compl_len is not bigger than complex_buffer!
 *                - absspec buffer is big enough!
 *
 * return 0 on success, else -1
 */
int absspec(const fftwf_complex *complex_buffer, float *absspec_buffer, unsigned int compl_length)
{
	if (complex_buffer == nullptr || absspec_buffer == nullptr) {return -1;}
	if (compl_length == 0) {return -1;}

	for (unsigned int i = 0; i < compl_length; i++)
	{
		absspec_buffer[i] = (float)sqrt(complex_buffer[i][0] * complex_buffer[i][0]
							+ complex_buffer[i][1] * complex_buffer[i][1]);
	}

	return 0;
}


/* TODO
 * Take care that - compl_len is not bigger than complex_buffer!
 *                - absspec buffer is big enough!
 *
 * return 0 on success, else -1
 */
int getPhase(const fftwf_complex *complex_buffer, float *phaseBufferOut, unsigned int compl_length)
{
	if (complex_buffer == nullptr || phaseBufferOut == nullptr) {return -1;}
	if (compl_length == 0) {return -1;}

	for (unsigned int i = 0; i < compl_length; i++)
	{
		phaseBufferOut[i] = std::atan(complex_buffer[i][1] / complex_buffer[i][0]);
	}

	return 0;
}


/* TODO
 * Take care that - compl_len is not bigger than complex_buffer!
 *                - absspec buffer is big enough!
 *
 * return 0 on success, else -1
 */
int inverseAbsspec(fftwf_complex *complex_buffer, const float *absspec_buffer, const float *phaseBufferIn,
	unsigned int sampleRateIn, unsigned int compl_length)
{
	if (complex_buffer == nullptr || absspec_buffer == nullptr || phaseBufferIn == nullptr) {return -1;}
	if (compl_length == 0) {return -1;}
	/*
	for a complex number $c$ of magnitude $m$ and phase offset $\theta$
	[
	c = me^{j(2\pi f + \theta)} = m\cos(2\pi f + \theta) + jm\sin(2\pi f + \theta)
	] 
	*/
	// 2 / pi
	float twoDivPi = 1.570796327f;
	for (unsigned int i = 0; i < compl_length; i++)
	{
		float curFreq = getFreq(i, sampleRateIn, compl_length);
		//std::cout << "inverseAbsspec i: " << i << " freq: " << curFreq << " amp: " << absspec_buffer[i] << " phase: " << phaseBufferIn[i] << std::endl;
		//complex_buffer[i][0] = absspec_buffer[i] / std::cos(twoDivPi * curFreq + phaseBufferIn[i]);
		//complex_buffer[i][1] = absspec_buffer[i] / std::sin(twoDivPi * curFreq + phaseBufferIn[i]);
		//complex_buffer[i][0] = std::pow(absspec_buffer[i] * 2.718281828f, twoDivPi * curFreq + phaseBufferIn[i]);
		//complex_buffer[i][1] = std::pow(absspec_buffer[i] * 2.718281828f, twoDivPi * curFreq + phaseBufferIn[i]);
		//complex_buffer[i][0] = absspec_buffer[i] * std::cos(twoDivPi * curFreq + phaseBufferIn[i]);
		//complex_buffer[i][1] = absspec_buffer[i] * std::sin(twoDivPi * curFreq + phaseBufferIn[i]);
		//complex_buffer[i][0] = absspec_buffer[i] * std::cos(phaseBufferIn[i]);
		//complex_buffer[i][1] = absspec_buffer[i] * std::sin(phaseBufferIn[i]);
		
		//complex_buffer[i][0] = absspec_buffer[i] * 2.718281828f;
		//complex_buffer[i][1] = absspec_buffer[i] * std::pow(2.718281828f, phaseBufferIn[i]);
		//complex_buffer[i][1] = absspec_buffer[i] * std::pow(2.718281828f, twoDivPi * curFreq + phaseBufferIn[i]);
		complex_buffer[i][0] = absspec_buffer[i] * std::acos(twoDivPi * curFreq / phaseBufferIn[i]);
		complex_buffer[i][1] = absspec_buffer[i] * std::asin(twoDivPi * curFreq / phaseBufferIn[i]);
	}
	return 0;
}
int inverseAbsspec(fftwf_complex *complex_buffer, const float *absspec_buffer, unsigned int sampleRateIn, unsigned int compl_length)
{
	if (complex_buffer == nullptr || absspec_buffer == nullptr) {return -1;}
	if (compl_length == 0) {return -1;}

	std::vector<float> phaseInput(compl_length);
	getPhase(complex_buffer, phaseInput.data(), compl_length);
	return inverseAbsspec(complex_buffer, absspec_buffer, phaseInput.data(), sampleRateIn, compl_length);
}


/* TODO
 * Take care that - compl_len is not bigger than complex_buffer!
 *                - absspec buffer is big enough!
 *
 * return 0 on success, else -1
 */
int getFreq(float *freqBufferOut, unsigned int sampleRateIn, unsigned int compl_length)
{
	if (freqBufferOut == nullptr) {return -1;}
	if (compl_length == 0) {return -1;}

	for (unsigned int i = 0; i < compl_length; i++)
	{
		freqBufferOut[i] = getFreq(i, sampleRateIn, compl_length);
	}

	return 0;
}
float getFreq(unsigned int locationIn, unsigned int sampleRateIn, unsigned int compl_length)
{
	return locationIn * sampleRateIn / static_cast<float>(compl_length) / 2.0f;
}


/* Build fewer subbands from many absolute spectrum values.
 * Take care that - compressedbands[] array num_new elements long
 *                - num_old > num_new
 *
 * return 0 on success, else -1
 */
int compressbands(const float *absspec_buffer, float *compressedband, int num_old, int num_new, int bottom, int top)
{
	if (absspec_buffer == nullptr || compressedband == nullptr) {return -1;}
	if (num_old < num_new) {return -1;}
	if (num_old <= 0 || num_new <= 0) {return -1;}
	if (bottom < 0) {bottom = 0;}
	if (top >= num_old) {top = num_old - 1;}

	int usefromold = num_old - (num_old - top) - bottom;

	float ratio = (float)usefromold / (float)num_new;

	// for each new subband
	for (int i = 0; i < num_new; i++)
	{
		compressedband[i] = 0;

		float j_min = (i * ratio) + bottom;

		if (j_min < 0) { j_min = static_cast<float>(bottom); }

		float j_max = j_min + ratio;

		for (float j = std::floor(j_min); j <= j_max; j++)
		{
			compressedband[i] += absspec_buffer[(int)j];
		}
	}

	return 0;
}


} // namespace lmms
