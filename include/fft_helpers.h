/*
 * fft_helpers.h - some functions around FFT analysis
 *
 * Copyright (c) 2008-2012 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef FFT_HELPERS_H
#define FFT_HELPERS_H

#include "lmms_export.h"

#include <fftw3.h>

const int FFT_BUFFER_SIZE = 2048;

enum WINDOWS
{
        RECTANGULAR = 0,
		BLACKMAN_HARRIS,
        HAMMING,
        HANNING,
		KAISER
};

/* returns biggest value from abs_spectrum[spec_size] array
 *
 *    returns -1 on error
 */
float LMMS_EXPORT maximum( float * _abs_spectrum, unsigned int _spec_size );		// NOTE: used

/* Normalizes the abs_spectrum array of absolute values to a 0..1 range
 * based on supplied energy and stores it in the norm_spectrum array.
 *
 *	returns -1 on error
*/
int LMMS_EXPORT normalize(float *abs_spectrum, float energy, float *norm_spectrum, unsigned int block_size);

/* Precompute a window function for later real-time use.
 *
 *	returns -1 on error
 */
int LMMS_EXPORT precomputeWindow(float * window, int length, WINDOWS type);

/* compute absolute values of complex_buffer, save to absspec_buffer
 * take care that - compl_len is not bigger than complex_buffer!
 *                - absspec buffer is big enough!
 *
 *    returns 0 on success, else -1
 */
int LMMS_EXPORT absspec( fftwf_complex * _complex_buffer, float * _absspec_buffer,	// NOTE: used
							int _compl_length );

/* build fewer subbands from many absolute spectrum values
 * take care that - compressedbands[] array num_new elements long
 *                - num_old > num_new
 *
 *    returns 0 on success, else -1
 */
int LMMS_EXPORT compressbands( float * _absspec_buffer, float * _compressedband,	// NOTE: used
			int _num_old, int _num_new, int _bottom, int _top );


int LMMS_EXPORT calc13octaveband31( float * _absspec_buffer, float * _subbands,		// NOTE: unused
				int _num_spec, float _max_frequency );

/* compute power of finite time sequence
 * take care num_values is length of timesignal[]
 *
 *    returns power on success, else -1
 */
float LMMS_EXPORT signalpower(float *timesignal, int num_values);					// NOTE: unused

#endif
