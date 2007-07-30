/*
** Copyright (C) 2002-2004 Erik de Castro Lopo <erikd@mega-nerd.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "float_cast.h"
#include "common.h"

#define	SINC_MAGIC_MARKER	MAKE_MAGIC (' ', 's', 'i', 'n', 'c', ' ')

#define	ARRAY_LEN(x)		((int) (sizeof (x) / sizeof ((x) [0])))

/*========================================================================================
**	Macros for handling the index into the array for the filter.
**	Double precision floating point is not accurate enough so use a 64 bit
**	fixed point value instead. SHIFT_BITS (current value of 48) is the number
**	of bits to the right of the decimal point.
**	The rest of the macros are for retrieving the fractional and integer parts
**	and for converting floats and ints to the fixed point format or from the
**	fixed point type back to integers and floats.
*/

#define MAKE_INCREMENT_T(x) 	((increment_t) (x))

#define	SHIFT_BITS				16
#define	FP_ONE					((double) (((increment_t) 1) << SHIFT_BITS))

#define	DOUBLE_TO_FP(x)			(lrint ((x) * FP_ONE))
#define	INT_TO_FP(x)			(((increment_t) (x)) << SHIFT_BITS)

#define	FP_FRACTION_PART(x)		((x) & ((((increment_t) 1) << SHIFT_BITS) - 1))
#define	FP_INTEGER_PART(x)		((x) & (((increment_t) -1) << SHIFT_BITS))

#define	FP_TO_INT(x)			(((x) >> SHIFT_BITS))
#define	FP_TO_DOUBLE(x)			(FP_FRACTION_PART (x) / FP_ONE)

/*========================================================================================
*/

typedef int32_t increment_t ;
typedef float	coeff_t ;

enum
{
	STATE_BUFFER_START	= 101,
	STATE_DATA_CONTINUE	= 102,
	STATE_BUFFER_END	= 103,
	STATE_FINISHED
} ;

typedef struct
{	int		sinc_magic_marker ;

	int		channels ;
	long	in_count, in_used ;
	long	out_count, out_gen ;

	int		coeff_half_len, index_inc ;
	int		has_diffs ;

	double	src_ratio, input_index ;

	int		coeff_len ;
	coeff_t const	*coeffs ;

	int		b_current, b_end, b_real_end, b_len ;
	float	buffer [1] ;
} SINC_FILTER ;

static int sinc_process (SRC_PRIVATE *psrc, SRC_DATA *data) ;

static double calc_output (SINC_FILTER *filter, increment_t increment, increment_t start_filter_index, int ch) ;

static void prepare_data (SINC_FILTER *filter, SRC_DATA *data, int half_filter_chan_len) ;

static void sinc_reset (SRC_PRIVATE *psrc) ;

static coeff_t const high_qual_coeffs [] =
{
#include "high_qual_coeffs.h"
} ; /* high_qual_coeffs */

static coeff_t const mid_qual_coeffs [] =
{
#include "mid_qual_coeffs.h"
} ; /* mid_qual_coeffs */

static coeff_t const fastest_coeffs [] =
{
#include "fastest_coeffs.h"
} ; /* fastest_coeffs */

/*----------------------------------------------------------------------------------------
*/

const char*
sinc_get_name (int src_enum)
{
	switch (src_enum)
	{	case SRC_SINC_BEST_QUALITY :
			return "Best Sinc Interpolator" ;

		case SRC_SINC_MEDIUM_QUALITY :
			return "Medium Sinc Interpolator" ;

		case SRC_SINC_FASTEST :
			return "Fastest Sinc Interpolator" ;
		} ;

	return NULL ;
} /* sinc_get_descrition */

const char*
sinc_get_description (int src_enum)
{
	switch (src_enum)
	{	case SRC_SINC_BEST_QUALITY :
			return "Band limited sinc interpolation, best quality, 97dB SNR, 96% BW." ;

		case SRC_SINC_MEDIUM_QUALITY :
			return "Band limited sinc interpolation, medium quality, 97dB SNR, 90% BW." ;

		case SRC_SINC_FASTEST :
			return "Band limited sinc interpolation, fastest, 97dB SNR, 80% BW." ;
		} ;

	return NULL ;
} /* sinc_get_descrition */

int
sinc_set_converter (SRC_PRIVATE *psrc, int src_enum)
{	SINC_FILTER *filter, temp_filter ;
	int count, bits ;

	/* Quick sanity check. */
	if (SHIFT_BITS >= sizeof (increment_t) * 8 - 1)
		return SRC_ERR_SHIFT_BITS ;

	if (psrc->private_data != NULL)
	{	filter = (SINC_FILTER*) psrc->private_data ;
		if (filter->sinc_magic_marker != SINC_MAGIC_MARKER)
		{	free (psrc->private_data) ;
			psrc->private_data = NULL ;
			} ;
		} ;

	memset (&temp_filter, 0, sizeof (temp_filter)) ;

	temp_filter.sinc_magic_marker = SINC_MAGIC_MARKER ;
	temp_filter.channels = psrc->channels ;

	psrc->process = sinc_process ;
	psrc->reset = sinc_reset ;

	switch (src_enum)
	{	case SRC_SINC_BEST_QUALITY :
				temp_filter.coeffs = high_qual_coeffs ;
				temp_filter.coeff_half_len = ARRAY_LEN (high_qual_coeffs) - 1 ;
				temp_filter.index_inc = 128 ;
				temp_filter.has_diffs = SRC_FALSE ;
				temp_filter.coeff_len = ARRAY_LEN (high_qual_coeffs) ;
				break ;

		case SRC_SINC_MEDIUM_QUALITY :
				temp_filter.coeffs = mid_qual_coeffs ;
				temp_filter.coeff_half_len = ARRAY_LEN (mid_qual_coeffs) - 1 ;
				temp_filter.index_inc = 128 ;
				temp_filter.has_diffs = SRC_FALSE ;
				temp_filter.coeff_len = ARRAY_LEN (mid_qual_coeffs) ;
				break ;

		case SRC_SINC_FASTEST :
				temp_filter.coeffs = fastest_coeffs ;
				temp_filter.coeff_half_len = ARRAY_LEN (fastest_coeffs) - 1 ;
				temp_filter.index_inc = 128 ;
				temp_filter.has_diffs = SRC_FALSE ;
				temp_filter.coeff_len = ARRAY_LEN (fastest_coeffs) ;
				break ;

		default :
				return SRC_ERR_BAD_CONVERTER ;
		} ;

	/*
	** FIXME : This needs to be looked at more closely to see if there is
	** a better way. Need to look at prepare_data () at the same time.
	*/

	temp_filter.b_len = 1000 + 2 * lrint (0.5 + temp_filter.coeff_len / (temp_filter.index_inc * 1.0) * SRC_MAX_RATIO) ;
	temp_filter.b_len *= temp_filter.channels ;

	if ((filter = calloc (1, sizeof (SINC_FILTER) + sizeof (filter->buffer [0]) * (temp_filter.b_len + temp_filter.channels))) == NULL)
		return SRC_ERR_MALLOC_FAILED ;

	*filter = temp_filter ;
	memset (&temp_filter, 0xEE, sizeof (temp_filter)) ;

	psrc->private_data = filter ;

	sinc_reset (psrc) ;

	count = filter->coeff_half_len ;
	for (bits = 0 ; (1 << bits) < count ; bits++)
		count |= (1 << bits) ;

	if (bits + SHIFT_BITS - 1 >= (int) (sizeof (increment_t) * 8))
		return SRC_ERR_FILTER_LEN ;

	return SRC_ERR_NO_ERROR ;
} /* sinc_set_converter */

static void
sinc_reset (SRC_PRIVATE *psrc)
{	SINC_FILTER *filter ;

	filter = (SINC_FILTER*) psrc->private_data ;
	if (filter == NULL)
		return ;

	filter->b_current = filter->b_end = 0 ;
	filter->b_real_end = -1 ;

	filter->src_ratio = filter->input_index = 0.0 ;

	memset (filter->buffer, 0, filter->b_len * sizeof (filter->buffer [0])) ;

	/* Set this for a sanity check */
	memset (filter->buffer + filter->b_len, 0xAA, filter->channels * sizeof (filter->buffer [0])) ;
} /* sinc_reset */

/*========================================================================================
**	Beware all ye who dare pass this point. There be dragons here.
*/

static int
sinc_process (SRC_PRIVATE *psrc, SRC_DATA *data)
{	SINC_FILTER *filter ;
	double		input_index, src_ratio, count, float_increment, terminate, rem ;
	increment_t	increment, start_filter_index ;
	int			half_filter_chan_len, samples_in_hand, ch ;

	if (psrc->private_data == NULL)
		return SRC_ERR_NO_PRIVATE ;

	filter = (SINC_FILTER*) psrc->private_data ;

	/* If there is not a problem, this will be optimised out. */
	if (sizeof (filter->buffer [0]) != sizeof (data->data_in [0]))
		return SRC_ERR_SIZE_INCOMPATIBILITY ;

	filter->in_count = data->input_frames * filter->channels ;
	filter->out_count = data->output_frames * filter->channels ;
	filter->in_used = filter->out_gen = 0 ;

	src_ratio = psrc->last_ratio ;

	/* Check the sample rate ratio wrt the buffer len. */
	count = (filter->coeff_half_len + 2.0) / filter->index_inc ;
	if (MIN (psrc->last_ratio, data->src_ratio) < 1.0)
		count /= MIN (psrc->last_ratio, data->src_ratio) ;

	/* Maximum coefficientson either side of center point. */
	half_filter_chan_len = filter->channels * (lrint (count) + 1) ;

	input_index = psrc->last_position ;
	float_increment = filter->index_inc ;

	rem = fmod (input_index, 1.0) ;
	filter->b_current = (filter->b_current + filter->channels * lrint (input_index - rem)) % filter->b_len ;
	input_index = rem ;

	terminate = 1.0 / src_ratio + 1e-20 ;

	/* Main processing loop. */
	while (filter->out_gen < filter->out_count)
	{
		/* Need to reload buffer? */
		samples_in_hand = (filter->b_end - filter->b_current + filter->b_len) % filter->b_len ;

		if (samples_in_hand <= half_filter_chan_len)
		{	prepare_data (filter, data, half_filter_chan_len) ;

			samples_in_hand = (filter->b_end - filter->b_current + filter->b_len) % filter->b_len ;
			if (samples_in_hand <= half_filter_chan_len)
				break ;
			} ;

		/* This is the termination condition. */
		if (filter->b_real_end >= 0)
		{	if (filter->b_current + input_index + terminate >= filter->b_real_end)
				break ;
			} ;

		if (fabs (psrc->last_ratio - data->src_ratio) > 1e-10)
			src_ratio = psrc->last_ratio + filter->out_gen * (data->src_ratio - psrc->last_ratio) / (filter->out_count - 1) ;

		float_increment = filter->index_inc * 1.0 ;
		if (src_ratio < 1.0)
			float_increment = filter->index_inc * src_ratio ;

		increment = DOUBLE_TO_FP (float_increment) ;

		start_filter_index = DOUBLE_TO_FP (input_index * float_increment) ;

		for (ch = 0 ; ch < filter->channels ; ch++)
		{	data->data_out [filter->out_gen] = (float_increment / filter->index_inc) *
											calc_output (filter, increment, start_filter_index, ch) ;
			filter->out_gen ++ ;
			} ;

		/* Figure out the next index. */
		input_index += 1.0 / src_ratio ;
		rem = fmod (input_index, 1.0) ;

		filter->b_current = (filter->b_current + filter->channels * lrint (input_index - rem)) % filter->b_len ;
		input_index = rem ;
		} ;

	psrc->last_position = input_index ;

	/* Save current ratio rather then target ratio. */
	psrc->last_ratio = src_ratio ;

	data->input_frames_used = filter->in_used / filter->channels ;
	data->output_frames_gen = filter->out_gen / filter->channels ;

	return SRC_ERR_NO_ERROR ;
} /* sinc_process */

/*----------------------------------------------------------------------------------------
*/

static void
prepare_data (SINC_FILTER *filter, SRC_DATA *data, int half_filter_chan_len)
{	int len = 0 ;

	if (filter->b_real_end >= 0)
		return ;	/* This doesn't make sense, so return. */

	if (filter->b_current == 0)
	{	/* Initial state. Set up zeros at the start of the buffer and
		** then load new data after that.
		*/
		len = filter->b_len - 2 * half_filter_chan_len ;

		filter->b_current = filter->b_end = half_filter_chan_len ;
		}
	else if (filter->b_end + half_filter_chan_len + filter->channels < filter->b_len)
	{	/*  Load data at current end position. */
		len = MAX (filter->b_len - filter->b_current - half_filter_chan_len, 0) ;
		}
	else
	{	/* Move data at end of buffer back to the start of the buffer. */
		len = filter->b_end - filter->b_current ;
		memmove (filter->buffer, filter->buffer + filter->b_current - half_filter_chan_len,
						(half_filter_chan_len + len) * sizeof (filter->buffer [0])) ;

		filter->b_current = half_filter_chan_len ;
		filter->b_end = filter->b_current + len ;

		/* Now load data at current end of buffer. */
		len = MAX (filter->b_len - filter->b_current - half_filter_chan_len, 0) ;
		} ;

	len = MIN (filter->in_count - filter->in_used, len) ;
	len -= (len % filter->channels) ;

	memcpy (filter->buffer + filter->b_end, data->data_in + filter->in_used,
						len * sizeof (filter->buffer [0])) ;

	filter->b_end += len ;
	filter->in_used += len ;

	if (filter->in_used == filter->in_count &&
			filter->b_end - filter->b_current < 2 * half_filter_chan_len && data->end_of_input)
	{	/* Handle the case where all data in the current buffer has been
		** consumed and this is the last buffer.
		*/

		if (filter->b_len - filter->b_end < half_filter_chan_len + 5)
		{	/* If necessary, move data down to the start of the buffer. */
			len = filter->b_end - filter->b_current ;
			memmove (filter->buffer, filter->buffer + filter->b_current - half_filter_chan_len,
							(half_filter_chan_len + len) * sizeof (filter->buffer [0])) ;

			filter->b_current = half_filter_chan_len ;
			filter->b_end = filter->b_current + len ;
			} ;

		filter->b_real_end = filter->b_end ;
		len = half_filter_chan_len + 5 ;

		memset (filter->buffer + filter->b_end, 0, len * sizeof (filter->buffer [0])) ;
		filter->b_end += len ;
		} ;

	return ;
} /* prepare_data */


static double
calc_output (SINC_FILTER *filter, increment_t increment, increment_t start_filter_index, int ch)
{	double		fraction, left, right, icoeff ;
	increment_t	filter_index, max_filter_index ;
	int			data_index, coeff_count, indx ;

	/* Convert input parameters into fixed point. */
	max_filter_index = INT_TO_FP (filter->coeff_half_len) ;

	/* First apply the left half of the filter. */
	filter_index = start_filter_index ;
	coeff_count = (max_filter_index - filter_index) / increment ;
	filter_index = filter_index + coeff_count * increment ;
	data_index = filter->b_current - filter->channels * coeff_count ;

	left = 0.0 ;
	do
	{	fraction = FP_TO_DOUBLE (filter_index) ;
		indx = FP_TO_INT (filter_index) ;

		icoeff = filter->coeffs [indx] + fraction * (filter->coeffs [indx + 1] - filter->coeffs [indx]) ;

		left += icoeff * filter->buffer [data_index + ch] ;

		filter_index -= increment ;
		data_index = data_index + filter->channels ;
		}
	while (filter_index >= MAKE_INCREMENT_T (0)) ;

	/* Now apply the right half of the filter. */
	filter_index = increment - start_filter_index ;
	coeff_count = (max_filter_index - filter_index) / increment ;
	filter_index = filter_index + coeff_count * increment ;
	data_index = filter->b_current + filter->channels * (1 + coeff_count) ;

	right = 0.0 ;
	do
	{	fraction = FP_TO_DOUBLE (filter_index) ;
		indx = FP_TO_INT (filter_index) ;

		icoeff = filter->coeffs [indx] + fraction * (filter->coeffs [indx + 1] - filter->coeffs [indx]) ;

		right += icoeff * filter->buffer [data_index + ch] ;

		filter_index -= increment ;
		data_index = data_index - filter->channels ;
		}
	while (filter_index > MAKE_INCREMENT_T (0)) ;

	return (left + right) ;
} /* calc_output */

/*
** Do not edit or modify anything in this comment block.
** The arch-tag line is a file identity tag for the GNU Arch 
** revision control system.
**
** arch-tag: db8efe06-2fbd-487e-be8f-bfc01e68c19f
*/

