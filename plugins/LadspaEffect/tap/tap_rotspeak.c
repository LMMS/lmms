/*                                                     -*- linux-c -*-
    Copyright (C) 2004 Tom Szilagyi

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    $Id: tap_rotspeak.c,v 1.3 2004/02/21 17:33:36 tszilagyi Exp $
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <ladspa.h>
#include "tap_utils.h"

/* The Unique ID of the plugin: */

#define ID_STEREO         2149

/* The port numbers for the plugin: */

#define BASSFREQ        0
#define HORNFREQ        1
#define STWIDTH         2
#define HRBAL           3
#define LATENCY         4
#define INPUT_L         5
#define INPUT_R         6
#define OUTPUT_L        7
#define OUTPUT_R        8


/* Total number of ports */

#define PORTCOUNT_STEREO   9

/*
 * This has to be bigger than 0.3f * sample_rate / (2*PI) for any sample rate.
 * At 192 kHz 9168 is needed so this should be enough.
 */
#define PM_DEPTH 9200

/* maximum phase mod freq */
#define PM_FREQ 30.0f


/* splitting input signals into low and high freq components */
#define SPLIT_FREQ 1000.0f
#define SPLIT_BW 1.0f


/* approx. sound velocity in air [m/s] */
#define C_AIR 340.0f

/* coefficient between rotating frequency and pitch mod depth (aka. Doppler effect) */
#define FREQ_PITCH 1.6f

/* cosine table for fast computations */
LADSPA_Data cos_table[1024];


/* The structure used to hold port connection information and state */

typedef struct {
	LADSPA_Data * hornfreq;
	LADSPA_Data * bassfreq;
	LADSPA_Data * stwidth;
	LADSPA_Data * hrbal;
	LADSPA_Data * latency;
	LADSPA_Data * input_L;
	LADSPA_Data * input_R;
	LADSPA_Data * output_L;
	LADSPA_Data * output_R;

        LADSPA_Data * ringbuffer_h_L;
        unsigned long buflen_h_L;
        unsigned long pos_h_L;
        LADSPA_Data * ringbuffer_h_R;
        unsigned long buflen_h_R;
        unsigned long pos_h_R;

        LADSPA_Data * ringbuffer_b_L;
        unsigned long buflen_b_L;
        unsigned long pos_b_L;
        LADSPA_Data * ringbuffer_b_R;
        unsigned long buflen_b_R;
        unsigned long pos_b_R;

	biquad * eq_filter_L;
	biquad * lp_filter_L;
	biquad * hp_filter_L;

	biquad * eq_filter_R;
	biquad * lp_filter_R;
	biquad * hp_filter_R;

	unsigned long sample_rate;
	LADSPA_Data phase_h;
	LADSPA_Data phase_b;

	LADSPA_Data run_adding_gain;
} RotSpkr;


void cleanup_RotSpkr(LADSPA_Handle Instance);

/* Construct a new plugin instance. */
LADSPA_Handle
instantiate_RotSpkr(const LADSPA_Descriptor * Descriptor,
		    unsigned long             sample_rate) {

	LADSPA_Handle * ptr;

	if ((ptr = calloc(1, sizeof(RotSpkr))) != NULL) {
		RotSpkr* rotSpeak = (RotSpkr*)ptr;
		rotSpeak->sample_rate = sample_rate;
		rotSpeak->run_adding_gain = 1.0;

		if ((rotSpeak->ringbuffer_h_L =
		     calloc(2 * PM_DEPTH, sizeof(LADSPA_Data))) == NULL)
		{
			cleanup_RotSpkr((LADSPA_Handle)rotSpeak);
			return NULL;
		}
		if ((rotSpeak->ringbuffer_h_R =
		     calloc(2 * PM_DEPTH, sizeof(LADSPA_Data))) == NULL)
		{
			cleanup_RotSpkr((LADSPA_Handle)rotSpeak);
			return NULL;
		}
		rotSpeak->buflen_h_L = ceil(0.3f * sample_rate / M_PI);
		rotSpeak->buflen_h_R = ceil(0.3f * sample_rate / M_PI);
		rotSpeak->pos_h_L = 0;
		rotSpeak->pos_h_R = 0;

		if ((rotSpeak->ringbuffer_b_L =
		     calloc(2 * PM_DEPTH, sizeof(LADSPA_Data))) == NULL)
		{
			cleanup_RotSpkr((LADSPA_Handle)rotSpeak);
			return NULL;
		}
		if ((rotSpeak->ringbuffer_b_R =
		     calloc(2 * PM_DEPTH, sizeof(LADSPA_Data))) == NULL)
		{
			cleanup_RotSpkr((LADSPA_Handle)rotSpeak);
			return NULL;
		}
		rotSpeak->buflen_b_L = ceil(0.3f * sample_rate / M_PI);
		rotSpeak->buflen_b_R = ceil(0.3f * sample_rate / M_PI);
		rotSpeak->pos_b_L = 0;
		rotSpeak->pos_b_R = 0;

		if ((rotSpeak->eq_filter_L = calloc(1, sizeof(biquad))) == NULL)
		{
			cleanup_RotSpkr((LADSPA_Handle)rotSpeak);
			return NULL;
		}
		if ((rotSpeak->lp_filter_L = calloc(1, sizeof(biquad))) == NULL)
		{
			cleanup_RotSpkr((LADSPA_Handle)rotSpeak);
			return NULL;
		}
		if ((rotSpeak->hp_filter_L = calloc(1, sizeof(biquad))) == NULL)
		{
			cleanup_RotSpkr((LADSPA_Handle)rotSpeak);
			return NULL;
		}

		if ((rotSpeak->eq_filter_R = calloc(1, sizeof(biquad))) == NULL)
		{
			cleanup_RotSpkr((LADSPA_Handle)rotSpeak);
			return NULL;
		}
		if ((rotSpeak->lp_filter_R = calloc(1, sizeof(biquad))) == NULL)
		{
			cleanup_RotSpkr((LADSPA_Handle)rotSpeak);
			return NULL;
		}
		if ((rotSpeak->hp_filter_R = calloc(1, sizeof(biquad))) == NULL)
		{
			cleanup_RotSpkr((LADSPA_Handle)rotSpeak);
			return NULL;
		}

		return ptr;
	}

	return NULL;
}


void
activate_RotSpkr(LADSPA_Handle Instance) {

	int i;
	RotSpkr * ptr;

	ptr = (RotSpkr *)Instance;

	for (i = 0; i < 2 * PM_DEPTH; i++) {
		ptr->ringbuffer_h_L[i] = 0.0f;
		ptr->ringbuffer_h_R[i] = 0.0f;
		ptr->ringbuffer_b_L[i] = 0.0f;
		ptr->ringbuffer_b_R[i] = 0.0f;
	}

	ptr->phase_h = 0.0f;
	ptr->phase_b = 0.0f;

	biquad_init(ptr->eq_filter_L);
	biquad_init(ptr->lp_filter_L);
	biquad_init(ptr->hp_filter_L);
	biquad_init(ptr->eq_filter_R);
	biquad_init(ptr->lp_filter_R);
	biquad_init(ptr->hp_filter_R);

	eq_set_params(ptr->eq_filter_L, SPLIT_FREQ, +8.0f, SPLIT_BW, ptr->sample_rate);
	eq_set_params(ptr->eq_filter_R, SPLIT_FREQ, +8.0f, SPLIT_BW, ptr->sample_rate);
	lp_set_params(ptr->lp_filter_L, SPLIT_FREQ, SPLIT_BW, ptr->sample_rate);
	lp_set_params(ptr->lp_filter_R, SPLIT_FREQ, SPLIT_BW, ptr->sample_rate);
	hp_set_params(ptr->hp_filter_L, SPLIT_FREQ, SPLIT_BW, ptr->sample_rate);
	hp_set_params(ptr->hp_filter_R, SPLIT_FREQ, SPLIT_BW, ptr->sample_rate);
}



/* Connect a port to a data location. */
void
connect_port_RotSpkr(LADSPA_Handle Instance,
		     unsigned long Port,
		     LADSPA_Data * DataLocation) {

	RotSpkr * ptr;

	ptr = (RotSpkr *)Instance;
	switch (Port) {
	case HORNFREQ:
		ptr->hornfreq = DataLocation;
		break;
	case BASSFREQ:
		ptr->bassfreq = DataLocation;
		break;
	case STWIDTH:
		ptr->stwidth = DataLocation;
		break;
	case HRBAL:
		ptr->hrbal = DataLocation;
		break;
	case LATENCY:
		ptr->latency = DataLocation;
                *(ptr->latency) = ptr->buflen_h_L / 2;  /* IS THIS LEGAL? YES, ONLY IF DataLocation points to valid memory location on stack/heap*/
		break;
	case INPUT_L:
		ptr->input_L = DataLocation;
		break;
	case INPUT_R:
		ptr->input_R = DataLocation;
		break;
	case OUTPUT_L:
		ptr->output_L = DataLocation;
		break;
	case OUTPUT_R:
		ptr->output_R = DataLocation;
		break;
	}
}



void
run_RotSpkr(LADSPA_Handle Instance,
	    unsigned long SampleCount) {

	RotSpkr * ptr = (RotSpkr *)Instance;

	LADSPA_Data * input_L = ptr->input_L;
	LADSPA_Data * input_R = ptr->input_R;
	LADSPA_Data * output_L = ptr->output_L;
	LADSPA_Data * output_R = ptr->output_R;
	LADSPA_Data freq_h = LIMIT(*(ptr->hornfreq),0.0f,PM_FREQ);
	LADSPA_Data freq_b = LIMIT(*(ptr->bassfreq),0.0f,PM_FREQ);
	LADSPA_Data stwidth = LIMIT(*(ptr->stwidth),0.0f,100.0f);
	LADSPA_Data hrbal = LIMIT(*(ptr->hrbal),0.0f,1.0f);
	LADSPA_Data pmdepth_h =
		LIMIT(1.0f/(1.0f+FREQ_PITCH*freq_h/C_AIR) * ptr->sample_rate
		      / 200.0f / M_PI / freq_h, 0, ptr->buflen_h_L / 2);
	LADSPA_Data pmdepth_b =
		LIMIT(1.0f/(1.0f+FREQ_PITCH*freq_b/C_AIR) * ptr->sample_rate
		      / 200.0f / M_PI / freq_b, 0, ptr->buflen_b_L / 2);
	unsigned long sample_index;

	LADSPA_Data in_L = 0.0f, in_R = 0.0f;
	LADSPA_Data lo_L = 0.0f, lo_R = 0.0f;
	LADSPA_Data hi_L = 0.0f, hi_R = 0.0f;

	LADSPA_Data phase_h_L = 0.0f, phase_b_L = 0.0f;
	LADSPA_Data phase_h_R = 0.0f, phase_b_R = 0.0f;
	LADSPA_Data phase_pm_h_L = 0.0f, phase_pm_b_L = 0.0f;
	LADSPA_Data phase_pm_h_R = 0.0f, phase_pm_b_R = 0.0f;
	LADSPA_Data pm_h_L = 0.0f, pm_b_L = 0.0f;
	LADSPA_Data pm_h_R = 0.0f, pm_b_R = 0.0f;

	LADSPA_Data fpos_h_L = 0.0f, fpos_b_L = 0.0f, fpos_h_R = 0.0f, fpos_b_R = 0.0f;
	LADSPA_Data n_h_L = 0.0f, n_b_L = 0.0f, n_h_R = 0.0f, n_b_R = 0.0f;
	LADSPA_Data rem_h_L = 0.0f, rem_b_L = 0.0f, rem_h_R = 0.0f, rem_b_R = 0.0f;
	LADSPA_Data sa_h_L = 0.0f, sa_b_L = 0.0f, sb_h_L = 0.0f, sb_b_L = 0.0f;
	LADSPA_Data sa_h_R = 0.0f, sa_b_R = 0.0f, sb_h_R = 0.0f, sb_b_R = 0.0f;
	

	for (sample_index = 0; sample_index < SampleCount; sample_index++) {

		in_L = *(input_L++);
		in_R = *(input_R++);

		in_L = biquad_run(ptr->eq_filter_L, in_L);
		in_R = biquad_run(ptr->eq_filter_R, in_R);
		lo_L = biquad_run(ptr->lp_filter_L, in_L);
		lo_R = biquad_run(ptr->lp_filter_R, in_R);
		hi_L = biquad_run(ptr->hp_filter_L, in_L);
		hi_R = biquad_run(ptr->hp_filter_R, in_R);
	

		phase_h_L = 1024.0f * freq_h * sample_index / ptr->sample_rate + ptr->phase_h;
		while (phase_h_L >= 1024.0f)
		        phase_h_L -= 1024.0f;  
		phase_pm_h_L = phase_h_L + 256.0f;
		while (phase_pm_h_L >= 1024.0f)
			phase_pm_h_L -= 1024.0f;
 		phase_h_R = phase_h_L + 512.0f;
		while (phase_h_R >= 1024.0f)
		        phase_h_R -= 1024.0f;
		phase_pm_h_R = phase_h_R + 256.0f;
		while (phase_pm_h_R >= 1024.0f)
			phase_pm_h_R -= 1024.0f;

		phase_b_L = 1024.0f * freq_b * sample_index / ptr->sample_rate + ptr->phase_b;
		while (phase_b_L >= 1024.0f)
		        phase_b_L -= 1024.0f;  
		phase_pm_b_L = phase_b_L + 256.0f;
		while (phase_pm_b_L >= 1024.0f)
			phase_pm_b_L -= 1024.0f;
 		phase_b_R = phase_b_L + 512.0f;
		while (phase_b_R >= 1024.0f)
		        phase_b_R -= 1024.0f;
		phase_pm_b_R = phase_b_R + 256.0f;
		while (phase_pm_b_R >= 1024.0f)
			phase_pm_b_R -= 1024.0f;

                push_buffer(hi_L, ptr->ringbuffer_h_L, ptr->buflen_h_L, &(ptr->pos_h_L));
                push_buffer(hi_R, ptr->ringbuffer_h_R, ptr->buflen_h_R, &(ptr->pos_h_R));
                push_buffer(lo_L, ptr->ringbuffer_b_L, ptr->buflen_b_L, &(ptr->pos_b_L));
                push_buffer(lo_R, ptr->ringbuffer_b_R, ptr->buflen_b_R, &(ptr->pos_b_R));

                fpos_h_L = pmdepth_h * (1.0f - cos_table[(unsigned long) phase_pm_h_L]);
                n_h_L = floorf(fpos_h_L);
                rem_h_L = fpos_h_L - n_h_L;
                sa_h_L = read_buffer(ptr->ringbuffer_h_L,
				     ptr->buflen_h_L, ptr->pos_h_L, (unsigned long) n_h_L);
                sb_h_L = read_buffer(ptr->ringbuffer_h_L,
				     ptr->buflen_h_L, ptr->pos_h_L, (unsigned long) n_h_L + 1);
                pm_h_L = (1 - rem_h_L) * sa_h_L + rem_h_L * sb_h_L;

                fpos_h_R = pmdepth_h * (1.0f - cos_table[(unsigned long) phase_pm_h_R]);
                n_h_R = floorf(fpos_h_R);
                rem_h_R = fpos_h_R - n_h_R;
                sa_h_R = read_buffer(ptr->ringbuffer_h_R,
				     ptr->buflen_h_R, ptr->pos_h_R, (unsigned long) n_h_R);
                sb_h_R = read_buffer(ptr->ringbuffer_h_R,
				     ptr->buflen_h_R, ptr->pos_h_R, (unsigned long) n_h_R + 1);
                pm_h_R = (1 - rem_h_R) * sa_h_R + rem_h_R * sb_h_R;


                fpos_b_L = pmdepth_b * (1.0f - cos_table[(unsigned long) phase_pm_b_L]);
                n_b_L = floorf(fpos_b_L);
                rem_b_L = fpos_b_L - n_b_L;
                sa_b_L = read_buffer(ptr->ringbuffer_b_L,
				     ptr->buflen_b_L, ptr->pos_b_L, (unsigned long) n_b_L);
                sb_b_L = read_buffer(ptr->ringbuffer_b_L,
				     ptr->buflen_b_L, ptr->pos_b_L, (unsigned long) n_b_L + 1);
                pm_b_L = (1 - rem_b_L) * sa_b_L + rem_b_L * sb_b_L;

                fpos_b_R = pmdepth_b * (1.0f - cos_table[(unsigned long) phase_pm_b_R]);
                n_b_R = floorf(fpos_b_R);
                rem_b_R = fpos_b_R - n_b_R;
                sa_b_R = read_buffer(ptr->ringbuffer_b_R,
				     ptr->buflen_b_R, ptr->pos_b_R, (unsigned long) n_b_R);
                sb_b_R = read_buffer(ptr->ringbuffer_b_R,
				     ptr->buflen_b_R, ptr->pos_b_R, (unsigned long) n_b_R + 1);
                pm_b_R = (1 - rem_b_R) * sa_b_R + rem_b_R * sb_b_R;


		*(output_L++) =
			hrbal * pm_h_L * (1.0f + 0.5f * stwidth/100.0f *
					  cos_table[(unsigned long) phase_h_L]) +
			(1.0f - hrbal) * pm_b_L * (1.0f + 0.5f * stwidth/100.0f *
						   cos_table[(unsigned long) phase_b_L]);

		*(output_R++) =
			hrbal * pm_h_R * (1.0f + 0.5f * stwidth/100.0f *
					  cos_table[(unsigned long) phase_h_R]) +
			(1.0f - hrbal) * pm_b_R * (1.0f + 0.5f * stwidth/100.0f *
						   cos_table[(unsigned long) phase_b_R]);
	}

        ptr->phase_h += 1024.0f * freq_h * sample_index / ptr->sample_rate;
	while (ptr->phase_h >= 1024.0f)
		ptr->phase_h -= 1024.0f;
        ptr->phase_b += 1024.0f * freq_b * sample_index / ptr->sample_rate;
	while (ptr->phase_b >= 1024.0f)
		ptr->phase_b -= 1024.0f;

	*(ptr->latency) = ptr->buflen_h_L / 2;
}


void
set_run_adding_gain_RotSpkr(LADSPA_Handle Instance, LADSPA_Data gain) {

	RotSpkr * ptr = (RotSpkr *)Instance;

	ptr->run_adding_gain = gain;
}



void 
run_adding_RotSpkr(LADSPA_Handle Instance,
		   unsigned long SampleCount) {
  
	RotSpkr * ptr = (RotSpkr *)Instance;

	LADSPA_Data * input_L = ptr->input_L;
	LADSPA_Data * input_R = ptr->input_R;
	LADSPA_Data * output_L = ptr->output_L;
	LADSPA_Data * output_R = ptr->output_R;
	LADSPA_Data freq_h = LIMIT(*(ptr->hornfreq),0.0f,PM_FREQ);
	LADSPA_Data freq_b = LIMIT(*(ptr->bassfreq),0.0f,PM_FREQ);
	LADSPA_Data stwidth = LIMIT(*(ptr->stwidth),0.0f,100.0f);
	LADSPA_Data hrbal = LIMIT(*(ptr->hrbal),0.0f,1.0f);
	LADSPA_Data pmdepth_h =
		LIMIT(1.0f/(1.0f+FREQ_PITCH*freq_h/C_AIR) * ptr->sample_rate
		      / 200.0f / M_PI / freq_h, 0, ptr->buflen_h_L / 2);
	LADSPA_Data pmdepth_b =
		LIMIT(1.0f/(1.0f+FREQ_PITCH*freq_b/C_AIR) * ptr->sample_rate
		      / 200.0f / M_PI / freq_b, 0, ptr->buflen_b_L / 2);
	unsigned long sample_index;

	LADSPA_Data in_L = 0.0f, in_R = 0.0f;
	LADSPA_Data lo_L = 0.0f, lo_R = 0.0f;
	LADSPA_Data hi_L = 0.0f, hi_R = 0.0f;

	LADSPA_Data phase_h_L = 0.0f, phase_b_L = 0.0f;
	LADSPA_Data phase_h_R = 0.0f, phase_b_R = 0.0f;
	LADSPA_Data phase_pm_h_L = 0.0f, phase_pm_b_L = 0.0f;
	LADSPA_Data phase_pm_h_R = 0.0f, phase_pm_b_R = 0.0f;
	LADSPA_Data pm_h_L = 0.0f, pm_b_L = 0.0f;
	LADSPA_Data pm_h_R = 0.0f, pm_b_R = 0.0f;

	LADSPA_Data fpos_h_L = 0.0f, fpos_b_L = 0.0f, fpos_h_R = 0.0f, fpos_b_R = 0.0f;
	LADSPA_Data n_h_L = 0.0f, n_b_L = 0.0f, n_h_R = 0.0f, n_b_R = 0.0f;
	LADSPA_Data rem_h_L = 0.0f, rem_b_L = 0.0f, rem_h_R = 0.0f, rem_b_R = 0.0f;
	LADSPA_Data sa_h_L = 0.0f, sa_b_L = 0.0f, sb_h_L = 0.0f, sb_b_L = 0.0f;
	LADSPA_Data sa_h_R = 0.0f, sa_b_R = 0.0f, sb_h_R = 0.0f, sb_b_R = 0.0f;
	

	for (sample_index = 0; sample_index < SampleCount; sample_index++) {

		in_L = *(input_L++);
		in_R = *(input_R++);

		in_L = biquad_run(ptr->eq_filter_L, in_L);
		in_R = biquad_run(ptr->eq_filter_R, in_R);
		lo_L = biquad_run(ptr->lp_filter_L, in_L);
		lo_R = biquad_run(ptr->lp_filter_R, in_R);
		hi_L = biquad_run(ptr->hp_filter_L, in_L);
		hi_R = biquad_run(ptr->hp_filter_R, in_R);
	

		phase_h_L = 1024.0f * freq_h * sample_index / ptr->sample_rate + ptr->phase_h;
		while (phase_h_L >= 1024.0f)
		        phase_h_L -= 1024.0f;  
		phase_pm_h_L = phase_h_L + 256.0f;
		while (phase_pm_h_L >= 1024.0f)
			phase_pm_h_L -= 1024.0f;
 		phase_h_R = phase_h_L + 512.0f;
		while (phase_h_R >= 1024.0f)
		        phase_h_R -= 1024.0f;
		phase_pm_h_R = phase_h_R + 256.0f;
		while (phase_pm_h_R >= 1024.0f)
			phase_pm_h_R -= 1024.0f;

		phase_b_L = 1024.0f * freq_b * sample_index / ptr->sample_rate + ptr->phase_b;
		while (phase_b_L >= 1024.0f)
		        phase_b_L -= 1024.0f;  
		phase_pm_b_L = phase_b_L + 256.0f;
		while (phase_pm_b_L >= 1024.0f)
			phase_pm_b_L -= 1024.0f;
 		phase_b_R = phase_b_L + 512.0f;
		while (phase_b_R >= 1024.0f)
		        phase_b_R -= 1024.0f;
		phase_pm_b_R = phase_b_R + 256.0f;
		while (phase_pm_b_R >= 1024.0f)
			phase_pm_b_R -= 1024.0f;

                push_buffer(hi_L, ptr->ringbuffer_h_L, ptr->buflen_h_L, &(ptr->pos_h_L));
                push_buffer(hi_R, ptr->ringbuffer_h_R, ptr->buflen_h_R, &(ptr->pos_h_R));
                push_buffer(lo_L, ptr->ringbuffer_b_L, ptr->buflen_b_L, &(ptr->pos_b_L));
                push_buffer(lo_R, ptr->ringbuffer_b_R, ptr->buflen_b_R, &(ptr->pos_b_R));

                fpos_h_L = pmdepth_h * (1.0f - cos_table[(unsigned long) phase_pm_h_L]);
                n_h_L = floorf(fpos_h_L);
                rem_h_L = fpos_h_L - n_h_L;
                sa_h_L = read_buffer(ptr->ringbuffer_h_L,
				     ptr->buflen_h_L, ptr->pos_h_L, (unsigned long) n_h_L);
                sb_h_L = read_buffer(ptr->ringbuffer_h_L,
				     ptr->buflen_h_L, ptr->pos_h_L, (unsigned long) n_h_L + 1);
                pm_h_L = (1 - rem_h_L) * sa_h_L + rem_h_L * sb_h_L;

                fpos_h_R = pmdepth_h * (1.0f - cos_table[(unsigned long) phase_pm_h_R]);
                n_h_R = floorf(fpos_h_R);
                rem_h_R = fpos_h_R - n_h_R;
                sa_h_R = read_buffer(ptr->ringbuffer_h_R,
				     ptr->buflen_h_R, ptr->pos_h_R, (unsigned long) n_h_R);
                sb_h_R = read_buffer(ptr->ringbuffer_h_R,
				     ptr->buflen_h_R, ptr->pos_h_R, (unsigned long) n_h_R + 1);
                pm_h_R = (1 - rem_h_R) * sa_h_R + rem_h_R * sb_h_R;


                fpos_b_L = pmdepth_b * (1.0f - cos_table[(unsigned long) phase_pm_b_L]);
                n_b_L = floorf(fpos_b_L);
                rem_b_L = fpos_b_L - n_b_L;
                sa_b_L = read_buffer(ptr->ringbuffer_b_L,
				     ptr->buflen_b_L, ptr->pos_b_L, (unsigned long) n_b_L);
                sb_b_L = read_buffer(ptr->ringbuffer_b_L,
				     ptr->buflen_b_L, ptr->pos_b_L, (unsigned long) n_b_L + 1);
                pm_b_L = (1 - rem_b_L) * sa_b_L + rem_b_L * sb_b_L;

                fpos_b_R = pmdepth_b * (1.0f - cos_table[(unsigned long) phase_pm_b_R]);
                n_b_R = floorf(fpos_b_R);
                rem_b_R = fpos_b_R - n_b_R;
                sa_b_R = read_buffer(ptr->ringbuffer_b_R,
				     ptr->buflen_b_R, ptr->pos_b_R, (unsigned long) n_b_R);
                sb_b_R = read_buffer(ptr->ringbuffer_b_R,
				     ptr->buflen_b_R, ptr->pos_b_R, (unsigned long) n_b_R + 1);
                pm_b_R = (1 - rem_b_R) * sa_b_R + rem_b_R * sb_b_R;


		*(output_L++) += ptr->run_adding_gain *
			hrbal * pm_h_L * (1.0f + 0.5f * stwidth/100.0f *
					  cos_table[(unsigned long) phase_h_L]) +
			(1.0f - hrbal) * pm_b_L * (1.0f + 0.5f * stwidth/100.0f *
						   cos_table[(unsigned long) phase_b_L]);

		*(output_R++) += ptr->run_adding_gain *
			hrbal * pm_h_R * (1.0f + 0.5f * stwidth/100.0f *
					  cos_table[(unsigned long) phase_h_R]) +
			(1.0f - hrbal) * pm_b_R * (1.0f + 0.5f * stwidth/100.0f *
						   cos_table[(unsigned long) phase_b_R]);
	}

        ptr->phase_h += 1024.0f * freq_h * sample_index / ptr->sample_rate;
	while (ptr->phase_h >= 1024.0f)
		ptr->phase_h -= 1024.0f;
        ptr->phase_b += 1024.0f * freq_b * sample_index / ptr->sample_rate;
	while (ptr->phase_b >= 1024.0f)
		ptr->phase_b -= 1024.0f;

	*(ptr->latency) = ptr->buflen_h_L / 2;
}



/*
   Throw away an RotSpkr effect instance.
   This function should be called only when RotSpkr was allocated with calloc.
 */
void
cleanup_RotSpkr(LADSPA_Handle Instance) {

	RotSpkr * ptr = (RotSpkr *)Instance;
	if (!ptr)
		return;
	if (ptr->ringbuffer_h_L)
		free(ptr->ringbuffer_h_L);
	if (ptr->ringbuffer_h_R)
		free(ptr->ringbuffer_h_R);
	if (ptr->ringbuffer_b_L)
		free(ptr->ringbuffer_b_L);
	if (ptr->ringbuffer_b_R)
		free(ptr->ringbuffer_b_R);
	if (ptr->eq_filter_L)
		free(ptr->eq_filter_L);
	if (ptr->eq_filter_R)
		free(ptr->eq_filter_R);
	if (ptr->lp_filter_L)
		free(ptr->lp_filter_L);
	if (ptr->lp_filter_R)
		free(ptr->lp_filter_R);
	if (ptr->hp_filter_L)
		free(ptr->hp_filter_L);
	if (ptr->hp_filter_R)
		free(ptr->hp_filter_R);
	if (Instance)
		free(Instance);
}



LADSPA_Descriptor * stereo_descriptor = NULL;



/* __attribute__((constructor)) tap_init() is called automatically when the plugin library is first
   loaded. */
void 
__attribute__((constructor)) tap_init() {
	
	int i;
	char ** port_names;
	LADSPA_PortDescriptor * port_descriptors;
	LADSPA_PortRangeHint * port_range_hints;
	
	if ((stereo_descriptor = 
	     (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor))) == NULL)
		exit(1);
	
	for (i = 0; i < 1024; i++)
		cos_table[i] = cosf(i * M_PI / 512.0f);


	stereo_descriptor->UniqueID = ID_STEREO;
	stereo_descriptor->Label = strdup("tap_rotspeak");
	stereo_descriptor->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
	stereo_descriptor->Name = strdup("TAP Rotary Speaker");
	stereo_descriptor->Maker = strdup("Tom Szilagyi");
	stereo_descriptor->Copyright = strdup("GPL");
	stereo_descriptor->PortCount = PORTCOUNT_STEREO;

	if ((port_descriptors =
	     (LADSPA_PortDescriptor *)calloc(PORTCOUNT_STEREO, sizeof(LADSPA_PortDescriptor))) == NULL)
		exit(1);

	stereo_descriptor->PortDescriptors = (const LADSPA_PortDescriptor *)port_descriptors;
	port_descriptors[HORNFREQ] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[BASSFREQ] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[STWIDTH] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[HRBAL] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[LATENCY] = LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL;
	port_descriptors[INPUT_L] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
	port_descriptors[INPUT_R] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
	port_descriptors[OUTPUT_L] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
	port_descriptors[OUTPUT_R] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;

	if ((port_names = 
	     (char **)calloc(PORTCOUNT_STEREO, sizeof(char *))) == NULL)
		exit(1);

	stereo_descriptor->PortNames = (const char **)port_names;
	port_names[HORNFREQ] = strdup("Horn Frequency [Hz]");
	port_names[BASSFREQ] = strdup("Rotor Frequency [Hz]");
	port_names[STWIDTH] = strdup("Mic Distance [%]");
	port_names[HRBAL] = strdup("Rotor/Horn Mix");
	port_names[LATENCY] = strdup("latency");
	port_names[INPUT_L] = strdup("Input L");
	port_names[INPUT_R] = strdup("Input R");
	port_names[OUTPUT_L] = strdup("Output L");
	port_names[OUTPUT_R] = strdup("Output R");

	if ((port_range_hints = 
	     ((LADSPA_PortRangeHint *)calloc(PORTCOUNT_STEREO, sizeof(LADSPA_PortRangeHint)))) == NULL)
		exit(1);

	stereo_descriptor->PortRangeHints	= (const LADSPA_PortRangeHint *)port_range_hints;
	port_range_hints[HORNFREQ].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[BASSFREQ].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[STWIDTH].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_LOW);
	port_range_hints[HRBAL].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_MIDDLE);
	port_range_hints[LATENCY].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_MAXIMUM);
	port_range_hints[HORNFREQ].LowerBound = 0;
	port_range_hints[HORNFREQ].UpperBound = PM_FREQ;
	port_range_hints[BASSFREQ].LowerBound = 0;
	port_range_hints[BASSFREQ].UpperBound = PM_FREQ;
	port_range_hints[STWIDTH].LowerBound = 0;
	port_range_hints[STWIDTH].UpperBound = 100.0f;
	port_range_hints[HRBAL].LowerBound = 0;
	port_range_hints[HRBAL].UpperBound = 1.0f;
	port_range_hints[LATENCY].LowerBound = 0;
	port_range_hints[LATENCY].UpperBound = PM_DEPTH;
	port_range_hints[INPUT_L].HintDescriptor = 0;
	port_range_hints[INPUT_R].HintDescriptor = 0;
	port_range_hints[OUTPUT_L].HintDescriptor = 0;
	port_range_hints[OUTPUT_R].HintDescriptor = 0;
	stereo_descriptor->instantiate = instantiate_RotSpkr;
	stereo_descriptor->connect_port = connect_port_RotSpkr;
	stereo_descriptor->activate = activate_RotSpkr;
	stereo_descriptor->run = run_RotSpkr;
	stereo_descriptor->run_adding = run_adding_RotSpkr;
	stereo_descriptor->set_run_adding_gain = set_run_adding_gain_RotSpkr;
	stereo_descriptor->deactivate = NULL;
	stereo_descriptor->cleanup = cleanup_RotSpkr;
}


void
delete_descriptor(LADSPA_Descriptor * descriptor) {
	unsigned long index;
	if (descriptor) {
		free((char *)descriptor->Label);
		free((char *)descriptor->Name);
		free((char *)descriptor->Maker);
		free((char *)descriptor->Copyright);
		free((LADSPA_PortDescriptor *)descriptor->PortDescriptors);
		for (index = 0; index < descriptor->PortCount; index++)
			free((char *)(descriptor->PortNames[index]));
		free((char **)descriptor->PortNames);
		free((LADSPA_PortRangeHint *)descriptor->PortRangeHints);
		free(descriptor);
	}
}


/* __attribute__((destructor)) tap_fini() is called automatically when the library is unloaded. */
void
__attribute__((destructor)) tap_fini() {
	delete_descriptor(stereo_descriptor);
}


/* Return a descriptor of the requested plugin type. */
const LADSPA_Descriptor * 
ladspa_descriptor(unsigned long Index) {

	switch (Index) {
	case 0:
		return stereo_descriptor;
	default:
		return NULL;
	}
}
