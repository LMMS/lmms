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

static int zoh_process (SRC_PRIVATE *psrc, SRC_DATA *data) ;
static void zoh_reset (SRC_PRIVATE *psrc) ;

/*========================================================================================
*/

#define	ZOH_MAGIC_MARKER	MAKE_MAGIC ('s', 'r', 'c', 'z', 'o', 'h')

typedef struct
{	int		zoh_magic_marker ;
	int		channels ;
	long	in_count, in_used ;
	long	out_count, out_gen ;
	float	last_value [1] ;
} ZOH_DATA ;

/*----------------------------------------------------------------------------------------
*/

static int
zoh_process (SRC_PRIVATE *psrc, SRC_DATA *data)
{	ZOH_DATA 	*zoh ;
	double		src_ratio, input_index ;
	int			ch ;

	if (psrc->private_data == NULL)
		return SRC_ERR_NO_PRIVATE ;

	zoh = (ZOH_DATA*) psrc->private_data ;

	zoh->in_count = data->input_frames * zoh->channels ;
	zoh->out_count = data->output_frames * zoh->channels ;
	zoh->in_used = zoh->out_gen = 0 ;

	src_ratio = psrc->last_ratio ;
	input_index = psrc->last_position ;

	/* Calculate samples before first sample in input array. */
	while (input_index < 1.0 && zoh->out_gen < zoh->out_count)
	{
		if (zoh->in_used + zoh->channels * input_index >= zoh->in_count)
			break ;

		if (fabs (psrc->last_ratio - data->src_ratio) > SRC_MIN_RATIO_DIFF)
			src_ratio = psrc->last_ratio + zoh->out_gen * (data->src_ratio - psrc->last_ratio) / (zoh->out_count - 1) ;

		for (ch = 0 ; ch < zoh->channels ; ch++)
		{	data->data_out [zoh->out_gen] = zoh->last_value [ch] ;
			zoh->out_gen ++ ;
			} ;

		/* Figure out the next index. */
		input_index += 1.0 / src_ratio ;
		} ;

	zoh->in_used += zoh->channels * lrint (floor (input_index)) ;
	input_index -= floor (input_index) ;

	/* Main processing loop. */
	while (zoh->out_gen < zoh->out_count && zoh->in_used + zoh->channels * input_index <= zoh->in_count)
	{
		if (fabs (psrc->last_ratio - data->src_ratio) > SRC_MIN_RATIO_DIFF)
			src_ratio = psrc->last_ratio + zoh->out_gen * (data->src_ratio - psrc->last_ratio) / (zoh->out_count - 1) ;

		for (ch = 0 ; ch < zoh->channels ; ch++)
		{	data->data_out [zoh->out_gen] = data->data_in [zoh->in_used - zoh->channels + ch] ;
			zoh->out_gen ++ ;
			} ;

		/* Figure out the next index. */
		input_index += 1.0 / src_ratio ;

		zoh->in_used += zoh->channels * lrint (floor (input_index)) ;
		input_index -= floor (input_index) ;
		} ;

	if (zoh->in_used > zoh->in_count)
	{	input_index += zoh->in_used - zoh->in_count ;
		zoh->in_used = zoh->in_count ;
		} ;

	psrc->last_position = input_index ;

	if (zoh->in_used > 0)
		for (ch = 0 ; ch < zoh->channels ; ch++)
			zoh->last_value [ch] = data->data_in [zoh->in_used - zoh->channels + ch] ;

	/* Save current ratio rather then target ratio. */
	psrc->last_ratio = src_ratio ;

	data->input_frames_used = zoh->in_used / zoh->channels ;
	data->output_frames_gen = zoh->out_gen / zoh->channels ;

	return SRC_ERR_NO_ERROR ;
} /* zoh_process */

/*------------------------------------------------------------------------------
*/

const char*
zoh_get_name (int src_enum)
{
	if (src_enum == SRC_ZERO_ORDER_HOLD)
		return "ZOH Interpolator" ;

	return NULL ;
} /* zoh_get_name */

const char*
zoh_get_description (int src_enum)
{
	if (src_enum == SRC_ZERO_ORDER_HOLD)
		return "Zero order hold interpolator, very fast, poor quality." ;

	return NULL ;
} /* zoh_get_descrition */

int
zoh_set_converter (SRC_PRIVATE *psrc, int src_enum)
{	ZOH_DATA *zoh = NULL ;

	if (src_enum != SRC_ZERO_ORDER_HOLD)
		return SRC_ERR_BAD_CONVERTER ;

	if (psrc->private_data != NULL)
	{	zoh = (ZOH_DATA*) psrc->private_data ;
		if (zoh->zoh_magic_marker != ZOH_MAGIC_MARKER)
		{	free (psrc->private_data) ;
			psrc->private_data = NULL ;
			} ;
		} ;

	if (psrc->private_data == NULL)
	{	zoh = calloc (1, sizeof (*zoh) + psrc->channels * sizeof (float)) ;
		if (zoh == NULL)
			return SRC_ERR_MALLOC_FAILED ;
		psrc->private_data = zoh ;
		} ;

	zoh->zoh_magic_marker = ZOH_MAGIC_MARKER ;
	zoh->channels = psrc->channels ;

	psrc->process = zoh_process ;
	psrc->reset = zoh_reset ;

	zoh_reset (psrc) ;

	return SRC_ERR_NO_ERROR ;
} /* zoh_set_converter */

/*===================================================================================
*/

static void
zoh_reset (SRC_PRIVATE *psrc)
{	ZOH_DATA *zoh ;

	zoh = (ZOH_DATA*) psrc->private_data ;
	if (zoh == NULL)
		return ;

	zoh->channels = psrc->channels ;
	memset (zoh->last_value, 0, sizeof (zoh->last_value [0]) * zoh->channels) ;

	return ;
} /* zoh_reset */
/*
** Do not edit or modify anything in this comment block.
** The arch-tag line is a file identity tag for the GNU Arch 
** revision control system.
**
** arch-tag: 808e62f8-2e4a-44a6-840f-180a3e41af01
*/

