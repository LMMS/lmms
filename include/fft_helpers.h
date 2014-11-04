/*
 * fft_helpers.h - some functions around FFT analysis
 *
 * Copyright (c) 2008-2012 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of LMMS - http://lmms.io
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


#ifndef _FFT_HELPERS_H
#define _FFT_HELPERS_H

#include "lmmsconfig.h"
#include "export.h"

#include <fftw3.h>

const int FFT_BUFFER_SIZE = 2048;

enum WINDOWS
{
        KAISER=1,
        RECTANGLE,
        HANNING,
        HAMMING
};

/* returns biggest value from abs_spectrum[spec_size] array
 *
 *    returns -1 on error
 */
float EXPORT maximum( float * _abs_spectrum, unsigned int _spec_size );

/* apply hanning or hamming window to channel
 *
 *    returns -1 on error
 */
int EXPORT hanming( float * _timebuffer, int _length, WINDOWS _type );

/* compute absolute values of complex_buffer, save to absspec_buffer
 * take care that - compl_len is not bigger than complex_buffer!
 *                - absspec buffer is big enough!
 *
 *    returns 0 on success, else -1
 */
int EXPORT absspec( fftwf_complex * _complex_buffer, float * _absspec_buffer,
							int _compl_length );

/* build fewer subbands from many absolute spectrum values
 * take care that - compressedbands[] array num_new elements long
 *                - num_old > num_new
 *                       
 *    returns 0 on success, else -1
 */
int EXPORT compressbands( float * _absspec_buffer, float * _compressedband,
			int _num_old, int _num_new, int _bottom, int _top );


int EXPORT calc13octaveband31( float * _absspec_buffer, float * _subbands,
				int _num_spec, float _max_frequency );

/* compute power of finite time sequence
 * take care num_values is length of timesignal[]
 *
 *    returns power on success, else -1
 */
float EXPORT signalpower(float *timesignal, int num_values);

#endif
