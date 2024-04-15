/*
 * fft_helpers.h - some functions around FFT analysis
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

#ifndef LMMS_FFT_HELPERS_H
#define LMMS_FFT_HELPERS_H

#include "lmms_export.h"

#include <vector>
#include <fftw3.h>

namespace lmms
{

// NOTE: FFT_BUFFER_SIZE should be considered deprecated!
// It is used by Eq plugin and some older code here, but this should be a user
// switchable parameter, not a constant. Use a value from FFT_BLOCK_SIZES
constexpr auto FFT_BUFFER_SIZE = std::size_t{2048};

// Allowed FFT block sizes. Ranging from barely useful to barely acceptable
// because of performance and latency reasons.
const std::vector<unsigned int> FFT_BLOCK_SIZES = {256, 512, 1024, 2048, 4096, 8192, 16384};

// List of FFT window functions supported by precomputeWindow()
enum class FFTWindow
{
	Rectangular = 0,
	BlackmanHarris,
	Hamming,
	Hanning
};


/**	Returns biggest value from abs_spectrum[spec_size] array.
 *
 *	@return -1 on error, 0 on success
 */
float LMMS_EXPORT maximum(const float *abs_spectrum, unsigned int spec_size);
float LMMS_EXPORT maximum(const std::vector<float> &abs_spectrum);


/** Normalize the abs_spectrum array of absolute values to a 0..1 range
 *	based on supplied energy and stores it in the norm_spectrum array.
 *
 *	@return -1 on error
 */
int LMMS_EXPORT normalize(const float *abs_spectrum, float *norm_spectrum, unsigned int bin_count, unsigned int block_size);
int LMMS_EXPORT normalize(const std::vector<float> &abs_spectrum, std::vector<float> &norm_spectrum, unsigned int block_size);


/**	Check if the spectrum contains any non-zero value.
 *
 *	@return 1 if spectrum contains any non-zero value
 *	@return 0 otherwise
 */
int LMMS_EXPORT notEmpty(const std::vector<float> &spectrum);


/**	Precompute a window function for later real-time use.
 *	Set normalized to false if you do not want to apply amplitude correction.
 *
 *	@return -1 on error
 */
int LMMS_EXPORT precomputeWindow(float *window, unsigned int length, FFTWindow type, bool normalized = true);


/**	Compute absolute values of complex_buffer, save to absspec_buffer.
 *	Take care that - compl_len is not bigger than complex_buffer!
 *				   - absspec buffer is big enough!
 *
 *	@return 0 on success, else -1
 */
int LMMS_EXPORT absspec(const fftwf_complex *complex_buffer, float *absspec_buffer,
						unsigned int compl_length);


/**	Build fewer subbands from many absolute spectrum values.
 *	Take care that - compressedbands[] array num_new elements long
 *				   - num_old > num_new
 *
 *	@return 0 on success, else -1
 */
int LMMS_EXPORT compressbands(const float * _absspec_buffer, float * _compressedband,
			int _num_old, int _num_new, int _bottom, int _top);


} // namespace lmms

#endif // LMMS_FFT_HELPERS_H
