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

static int linear_process (SRC_PRIVATE *psrc, SRC_DATA *data) ;
static void linear_reset (SRC_PRIVATE *psrc) ;

/*========================================================================================
*/

#define	LINEAR_MAGIC_MARKER	MAKE_MAGIC ('l', 'i', 'n', 'e', 'a', 'r')

#define	SRC_DEBUG	0

typedef struct
{	int		linear_magic_marker ;
	int		channels ;
	long	in_count, in_used ;
	long	out_count, out_gen ;
	float	last_value [1] ;
} LINEAR_DATA ;

/*----------------------------------------------------------------------------------------
*/

static int
linear_process (SRC_PRIVATE *psrc, SRC_DATA *data)
{	LINEAR_DATA *linear ;
	double		src_ratio, input_index ;
	int			ch ;

	if (psrc->private_data == NULL)
		return SRC_ERR_NO_PRIVATE ;

	linear = (LINEAR_DATA*) psrc->private_data ;

	linear->in_count = data->input_frames * linear->channels ;
	linear->out_count = data->output_frames * linear->channels ;
	linear->in_used = linear->out_gen = 0 ;

	src_ratio = psrc->last_ratio ;
	input_index = psrc->last_position ;

	/* Calculate samples before first sample in input array. */
	while (input_index < 1.0 && linear->out_gen < linear->out_count)
	{
		if (linear->in_used + linear->channels * input_index > linear->in_count)
			break ;

		if (fabs (psrc->last_ratio - data->src_ratio) > SRC_MIN_RATIO_DIFF)
			src_ratio = psrc->last_ratio + linear->out_gen * (data->src_ratio - psrc->last_ratio) / (linear->out_count - 1) ;

		for (ch = 0 ; ch < linear->channels ; ch++)
		{	data->data_out [linear->out_gen] = linear->last_value [ch] + input_index *
										(data->data_in [ch] - linear->last_value [ch]) ;
			linear->out_gen ++ ;
			} ;

		/* Figure out the next index. */
		input_index += 1.0 / src_ratio ;
		} ;

	linear->in_used += linear->channels * lrint (floor (input_index)) ;
	input_index -= floor (input_index) ;

	/* Main processing loop. */
	while (linear->out_gen < linear->out_count && linear->in_used + linear->channels * input_index <= linear->in_count)
	{
		if (fabs (psrc->last_ratio - data->src_ratio) > SRC_MIN_RATIO_DIFF)
			src_ratio = psrc->last_ratio + linear->out_gen * (data->src_ratio - psrc->last_ratio) / (linear->out_count - 1) ;

		if (SRC_DEBUG && linear->in_used < linear->channels && input_index < 1.0)
		{	printf ("Whoops!!!!   in_used : %ld     channels : %d     input_index : %f\n", linear->in_used, linear->channels, input_index) ;
			exit (1) ;
			} ;

		for (ch = 0 ; ch < linear->channels ; ch++)
		{	data->data_out [linear->out_gen] = data->data_in [linear->in_used - linear->channels + ch] + input_index *
						(data->data_in [linear->in_used + ch] - data->data_in [linear->in_used - linear->channels + ch]) ;
			linear->out_gen ++ ;
			} ;

		/* Figure out the next index. */
		input_index += 1.0 / src_ratio ;

		linear->in_used += linear->channels * lrint (floor (input_index)) ;
		input_index -= floor (input_index) ;
		} ;

	if (linear->in_used > linear->in_count)
	{	input_index += linear->in_used - linear->in_count ;
		linear->in_used = linear->in_count ;
		} ;

	psrc->last_position = input_index ;

	if (linear->in_used > 0)
		for (ch = 0 ; ch < linear->channels ; ch++)
			linear->last_value [ch] = data->data_in [linear->in_used - linear->channels + ch] ;

	/* Save current ratio rather then target ratio. */
	psrc->last_ratio = src_ratio ;

	data->input_frames_used = linear->in_used / linear->channels ;
	data->output_frames_gen = linear->out_gen / linear->channels ;

	return SRC_ERR_NO_ERROR ;
} /* linear_process */

/*------------------------------------------------------------------------------
*/

const char*
linear_get_name (int src_enum)
{
	if (src_enum == SRC_LINEAR)
		return "Linear Interpolator" ;

	return NULL ;
} /* linear_get_name */

const char*
linear_get_description (int src_enum)
{
	if (src_enum == SRC_LINEAR)
		return "Linear interpolator, very fast, poor quality." ;

	return NULL ;
} /* linear_get_descrition */

int
linear_set_converter (SRC_PRIVATE *psrc, int src_enum)
{	LINEAR_DATA *linear = NULL ;

	if (src_enum != SRC_LINEAR)
		return SRC_ERR_BAD_CONVERTER ;

	if (psrc->private_data != NULL)
	{	linear = (LINEAR_DATA*) psrc->private_data ;
		if (linear->linear_magic_marker != LINEAR_MAGIC_MARKER)
		{	free (psrc->private_data) ;
			psrc->private_data = NULL ;
			} ;
		} ;

	if (psrc->private_data == NULL)
	{	linear = calloc (1, sizeof (*linear) + psrc->channels * sizeof (float)) ;
		if (linear == NULL)
			return SRC_ERR_MALLOC_FAILED ;
		psrc->private_data = linear ;
		} ;

	linear->linear_magic_marker = LINEAR_MAGIC_MARKER ;
	linear->channels = psrc->channels ;

	psrc->process = linear_process ;
	psrc->reset = linear_reset ;

	linear_reset (psrc) ;

	return SRC_ERR_NO_ERROR ;
} /* linear_set_converter */

/*===================================================================================
*/

static void
linear_reset (SRC_PRIVATE *psrc)
{	LINEAR_DATA *linear = NULL ;

	linear = (LINEAR_DATA*) psrc->private_data ;
	if (linear == NULL)
		return ;

	memset (linear->last_value, 0, sizeof (linear->last_value [0]) * linear->channels) ;
} /* linear_reset */
/*
** Do not edit or modify anything in this comment block.
** The arch-tag line is a file identity tag for the GNU Arch 
** revision control system.
**
** arch-tag: 7eac3103-3d84-45d3-8bbd-f409c2b2d1a9
*/

