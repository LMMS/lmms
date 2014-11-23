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

    $Id: tap_doubler.c,v 1.4 2004/08/13 18:34:31 tszilagyi Exp $
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include <ladspa.h>
#include "tap_utils.h"


/* The Unique ID of the plugin: */

#define ID_STEREO       2156

/* The port numbers for the plugin: */

#define TIME            0
#define PITCH           1
#define DRYLEVEL        2
#define DRYPOSL         3
#define DRYPOSR         4
#define WETLEVEL        5
#define WETPOSL         6
#define WETPOSR         7
#define INPUT_L         8
#define INPUT_R         9
#define OUTPUT_L       10
#define OUTPUT_R       11

/* Total number of ports */


#define PORTCOUNT_STEREO 12


/* Number of pink noise samples to be generated at once */
#define NOISE_LEN 1024

/*
 * Largest buffer length needed (at 192 kHz).
 */
#define BUFLEN 11520



/* The structure used to hold port connection information and state */

typedef struct {
	LADSPA_Data * time;
	LADSPA_Data * pitch;
	LADSPA_Data * drylevel;
	LADSPA_Data * dryposl;
	LADSPA_Data * dryposr;
	LADSPA_Data * wetlevel;
	LADSPA_Data * wetposl;
	LADSPA_Data * wetposr;
	LADSPA_Data * input_L;
	LADSPA_Data * input_R;
	LADSPA_Data * output_L;
	LADSPA_Data * output_R;

	LADSPA_Data old_time;
	LADSPA_Data old_pitch;

	LADSPA_Data * ring_L;
	unsigned long buflen_L;
	unsigned long pos_L;

	LADSPA_Data * ring_R;
	unsigned long buflen_R;
	unsigned long pos_R;

	LADSPA_Data * ring_pnoise;
	unsigned long buflen_pnoise;
	unsigned long pos_pnoise;

	LADSPA_Data * ring_dnoise;
	unsigned long buflen_dnoise;
	unsigned long pos_dnoise;

	float delay;
	float d_delay;
	float p_delay;
	unsigned long n_delay;

	float pitchmod;
	float d_pitch;
	float p_pitch;
	unsigned long n_pitch;

	unsigned long p_stretch;
	unsigned long d_stretch;

	unsigned long sample_rate;
	LADSPA_Data run_adding_gain;
} Doubler;


/* generate fractal pattern using Midpoint Displacement Method
 * v: buffer of floats to output fractal pattern to
 * N: length of v, MUST be integer power of 2 (ie 128, 256, ...)
 * H: Hurst constant, between 0 and 0.9999 (fractal dimension)
 */
void
fractal(LADSPA_Data * v, int N, float H) {

        int l = N;
        int k;
        float r = 1.0f;
        int c;

        v[0] = 0;
        while (l > 1) {
                k = N / l;
                for (c = 0; c < k; c++) {
                        v[c*l + l/2] = (v[c*l] + v[((c+1) * l) % N]) / 2.0f +
                                2.0f * r * (rand() - (float)RAND_MAX/2.0f) / (float)RAND_MAX;
                        v[c*l + l/2] = LIMIT(v[c*l + l/2], -1.0f, 1.0f);
                }
                l /= 2;
                r /= powf(2, H);
        }
}



/* Construct a new plugin instance. */
LADSPA_Handle 
instantiate_Doubler(const LADSPA_Descriptor * Descriptor,
		    unsigned long             sample_rate) {
  
        LADSPA_Handle * ptr;
	
	if ((ptr = malloc(sizeof(Doubler))) != NULL) {
		((Doubler *)ptr)->sample_rate = sample_rate;
		((Doubler *)ptr)->run_adding_gain = 1.0f;

		if ((((Doubler *)ptr)->ring_L = 
		     calloc(BUFLEN * sample_rate / 192000, sizeof(LADSPA_Data))) == NULL)
			return NULL;
		((Doubler *)ptr)->buflen_L = BUFLEN * sample_rate / 192000;
		((Doubler *)ptr)->pos_L = 0;

		if ((((Doubler *)ptr)->ring_R = 
		     calloc(BUFLEN * sample_rate / 192000, sizeof(LADSPA_Data))) == NULL)
			return NULL;
		((Doubler *)ptr)->buflen_R = BUFLEN * sample_rate / 192000;
		((Doubler *)ptr)->pos_R = 0;

		if ((((Doubler *)ptr)->ring_pnoise = 
		     calloc(NOISE_LEN, sizeof(LADSPA_Data))) == NULL)
			return NULL;
		((Doubler *)ptr)->buflen_pnoise = NOISE_LEN;
		((Doubler *)ptr)->pos_pnoise = 0;

		if ((((Doubler *)ptr)->ring_dnoise = 
		     calloc(NOISE_LEN, sizeof(LADSPA_Data))) == NULL)
			return NULL;
		((Doubler *)ptr)->buflen_dnoise = NOISE_LEN;
		((Doubler *)ptr)->pos_dnoise = 0;

		((Doubler *)ptr)->d_stretch = sample_rate / 10;
		((Doubler *)ptr)->p_stretch = sample_rate / 1000;

		((Doubler *)ptr)->delay = 0.0f;
		((Doubler *)ptr)->d_delay = 0.0f;
		((Doubler *)ptr)->p_delay = 0.0f;
		((Doubler *)ptr)->n_delay = ((Doubler *)ptr)->d_stretch;

		((Doubler *)ptr)->pitchmod = 0.0f;
		((Doubler *)ptr)->d_pitch = 0.0f;
		((Doubler *)ptr)->p_pitch = 0.0f;
		((Doubler *)ptr)->n_pitch = ((Doubler *)ptr)->p_stretch;

		return ptr;
	}
       	return NULL;
}


void
activate_Doubler(LADSPA_Handle Instance) {

	Doubler * ptr = (Doubler *)Instance;
	unsigned long i;

	for (i = 0; i < BUFLEN * ptr->sample_rate / 192000; i++) {
		ptr->ring_L[i] = 0.0f;
		ptr->ring_R[i] = 0.0f;
	}

	ptr->old_time = -1.0f;
	ptr->old_pitch = -1.0f;
}




/* Connect a port to a data location. */
void 
connect_port_Doubler(LADSPA_Handle Instance,
		     unsigned long Port,
		     LADSPA_Data * data) {
	
	Doubler * ptr = (Doubler *)Instance;

	switch (Port) {
	case TIME:
		ptr->time = data;
		break;
	case PITCH:
		ptr->pitch = data;
		break;
	case DRYLEVEL:
		ptr->drylevel = data;
		break;
	case DRYPOSL:
		ptr->dryposl = data;
		break;
	case DRYPOSR:
		ptr->dryposr = data;
		break;
	case WETLEVEL:
		ptr->wetlevel = data;
		break;
	case WETPOSL:
		ptr->wetposl = data;
		break;
	case WETPOSR:
		ptr->wetposr = data;
		break;
	case INPUT_L:
		ptr->input_L = data;
		break;
	case INPUT_R:
		ptr->input_R = data;
		break;
	case OUTPUT_L:
		ptr->output_L = data;
		break;
	case OUTPUT_R:
		ptr->output_R = data;
		break;
	}
}



void 
run_Doubler(LADSPA_Handle Instance,
	    unsigned long SampleCount) {
  
	Doubler * ptr = (Doubler *)Instance;

	LADSPA_Data pitch = LIMIT(*(ptr->pitch),0.0f,1.0f) + 0.75f;
	LADSPA_Data depth = LIMIT(((1.0f - LIMIT(*(ptr->pitch),0.0f,1.0f)) * 1.75f + 0.25f) *
				  ptr->sample_rate / 6000.0f / M_PI,
				  0, ptr->buflen_L / 2);
	LADSPA_Data time = LIMIT(*(ptr->time), 0.0f, 1.0f) + 0.5f;
	LADSPA_Data drylevel = db2lin(LIMIT(*(ptr->drylevel),-90.0f,20.0f));
	LADSPA_Data wetlevel = db2lin(LIMIT(*(ptr->wetlevel),-90.0f,20.0f));
	LADSPA_Data dryposl = 1.0f - LIMIT(*(ptr->dryposl), 0.0f, 1.0f);
	LADSPA_Data dryposr = LIMIT(*(ptr->dryposr), 0.0f, 1.0f);
	LADSPA_Data wetposl = 1.0f - LIMIT(*(ptr->wetposl), 0.0f, 1.0f);
	LADSPA_Data wetposr = LIMIT(*(ptr->wetposr), 0.0f, 1.0f);
	LADSPA_Data * input_L = ptr->input_L;
	LADSPA_Data * input_R = ptr->input_R;
	LADSPA_Data * output_L = ptr->output_L;
	LADSPA_Data * output_R = ptr->output_R;

	unsigned long sample_index;
	unsigned long sample_count = SampleCount;

	LADSPA_Data in_L = 0.0f;
	LADSPA_Data in_R = 0.0f;
	LADSPA_Data out_L = 0.0f;
	LADSPA_Data out_R = 0.0f;

	LADSPA_Data fpos = 0.0f;
	LADSPA_Data n = 0.0f;
	LADSPA_Data rem = 0.0f;
	LADSPA_Data s_a_L, s_a_R, s_b_L, s_b_R;
	LADSPA_Data prev_p_pitch = 0.0f;
	LADSPA_Data prev_p_delay = 0.0f;
	LADSPA_Data delay;

	LADSPA_Data drystream_L = 0.0f;
	LADSPA_Data drystream_R = 0.0f;
	LADSPA_Data wetstream_L = 0.0f;
	LADSPA_Data wetstream_R = 0.0f;

	if (ptr->old_pitch != pitch) {
		ptr->pitchmod = ptr->p_pitch;
		prev_p_pitch = ptr->p_pitch;
		fractal(ptr->ring_pnoise, NOISE_LEN, pitch);
		ptr->pos_pnoise = 0;
		ptr->p_pitch = push_buffer(0.0f, ptr->ring_pnoise,
					   ptr->buflen_pnoise, &(ptr->pos_pnoise));
		ptr->d_pitch = (ptr->p_pitch - prev_p_pitch) / (float)(ptr->p_stretch);
		ptr->n_pitch = 0;

		ptr->old_pitch = pitch;
	}

	if (ptr->old_time != time) {
		ptr->delay = ptr->p_delay;
		prev_p_delay = ptr->p_delay;
		fractal(ptr->ring_dnoise, NOISE_LEN, time);
		ptr->pos_dnoise = 0;
		ptr->p_delay = push_buffer(0.0f, ptr->ring_dnoise,
					   ptr->buflen_dnoise, &(ptr->pos_dnoise));
		ptr->d_delay = (ptr->p_delay - prev_p_delay) / (float)(ptr->d_stretch);
		ptr->n_delay = 0;

		ptr->old_time = time;
	}


	for (sample_index = 0; sample_index < sample_count; sample_index++) {

		in_L = *(input_L++);
		in_R = *(input_R++);

		push_buffer(in_L, ptr->ring_L, ptr->buflen_L, &(ptr->pos_L));
		push_buffer(in_R, ptr->ring_R, ptr->buflen_R, &(ptr->pos_R));

		if (ptr->n_pitch < ptr->p_stretch) {
			ptr->pitchmod += ptr->d_pitch;
			ptr->n_pitch++;
		} else {
			ptr->pitchmod = ptr->p_pitch;
			prev_p_pitch = ptr->p_pitch;
			if (!ptr->pos_pnoise) {
				fractal(ptr->ring_pnoise, NOISE_LEN, pitch);
			}
			ptr->p_pitch = push_buffer(0.0f, ptr->ring_pnoise,
						   ptr->buflen_pnoise, &(ptr->pos_pnoise));
			ptr->d_pitch = (ptr->p_pitch - prev_p_pitch) / (float)(ptr->p_stretch);
			ptr->n_pitch = 0;
		}

		if (ptr->n_delay < ptr->d_stretch) {
			ptr->delay += ptr->d_delay;
			ptr->n_delay++;
		} else {
			ptr->delay = ptr->p_delay;
			prev_p_delay = ptr->p_delay;
			if (!ptr->pos_dnoise) {
				fractal(ptr->ring_dnoise, NOISE_LEN, time);
			}
			ptr->p_delay = push_buffer(0.0f, ptr->ring_dnoise,
						   ptr->buflen_dnoise, &(ptr->pos_dnoise));
			ptr->d_delay = (ptr->p_delay - prev_p_delay) / (float)(ptr->d_stretch);
			ptr->n_delay = 0;
		}

		delay = (12.5f * ptr->delay + 37.5f) * ptr->sample_rate / 1000.0f;
		fpos = ptr->buflen_L - depth * (1.0f - ptr->pitchmod) - delay - 1.0f;
		n = floorf(fpos);
		rem = fpos - n;

		s_a_L = read_buffer(ptr->ring_L, ptr->buflen_L,
				    ptr->pos_L, (unsigned long) n);
		s_b_L = read_buffer(ptr->ring_L, ptr->buflen_L,
				    ptr->pos_L, (unsigned long) n + 1);

		s_a_R = read_buffer(ptr->ring_R, ptr->buflen_R,
				    ptr->pos_R, (unsigned long) n);
		s_b_R = read_buffer(ptr->ring_R, ptr->buflen_R,
				    ptr->pos_R, (unsigned long) n + 1);

		drystream_L = drylevel * in_L;
		drystream_R = drylevel * in_R;
		wetstream_L = wetlevel * ((1 - rem) * s_a_L + rem * s_b_L);
		wetstream_R = wetlevel * ((1 - rem) * s_a_R + rem * s_b_R);

		out_L = dryposl * drystream_L + (1.0f - dryposr) * drystream_R +
			wetposl * wetstream_L + (1.0f - wetposr) * wetstream_R;
		out_R = (1.0f - dryposl) * drystream_L + dryposr * drystream_R +
			(1.0f - wetposl) * wetstream_L + wetposr * wetstream_R;

		*(output_L++) = out_L;
		*(output_R++) = out_R;
	}
}


void
set_run_adding_gain_Doubler(LADSPA_Handle Instance, LADSPA_Data gain) {

	Doubler * ptr = (Doubler *)Instance;

	ptr->run_adding_gain = gain;
}



void 
run_adding_Doubler(LADSPA_Handle Instance,
		   unsigned long SampleCount) {
  
	Doubler * ptr = (Doubler *)Instance;

	LADSPA_Data pitch = LIMIT(*(ptr->pitch),0.0f,1.0f) + 0.75f;
	LADSPA_Data depth = LIMIT(((1.0f - LIMIT(*(ptr->pitch),0.0f,1.0f)) * 1.75f + 0.25f) *
				  ptr->sample_rate / 6000.0f / M_PI,
				  0, ptr->buflen_L / 2);
	LADSPA_Data time = LIMIT(*(ptr->time), 0.0f, 1.0f) + 0.5f;
	LADSPA_Data drylevel = db2lin(LIMIT(*(ptr->drylevel),-90.0f,20.0f));
	LADSPA_Data wetlevel = db2lin(LIMIT(*(ptr->wetlevel),-90.0f,20.0f));
	LADSPA_Data dryposl = 1.0f - LIMIT(*(ptr->dryposl), 0.0f, 1.0f);
	LADSPA_Data dryposr = LIMIT(*(ptr->dryposr), 0.0f, 1.0f);
	LADSPA_Data wetposl = 1.0f - LIMIT(*(ptr->wetposl), 0.0f, 1.0f);
	LADSPA_Data wetposr = LIMIT(*(ptr->wetposr), 0.0f, 1.0f);
	LADSPA_Data * input_L = ptr->input_L;
	LADSPA_Data * input_R = ptr->input_R;
	LADSPA_Data * output_L = ptr->output_L;
	LADSPA_Data * output_R = ptr->output_R;

	unsigned long sample_index;
	unsigned long sample_count = SampleCount;

	LADSPA_Data in_L = 0.0f;
	LADSPA_Data in_R = 0.0f;
	LADSPA_Data out_L = 0.0f;
	LADSPA_Data out_R = 0.0f;

	LADSPA_Data fpos = 0.0f;
	LADSPA_Data n = 0.0f;
	LADSPA_Data rem = 0.0f;
	LADSPA_Data s_a_L, s_a_R, s_b_L, s_b_R;
	LADSPA_Data prev_p_pitch = 0.0f;
	LADSPA_Data prev_p_delay = 0.0f;
	LADSPA_Data delay;

	LADSPA_Data drystream_L = 0.0f;
	LADSPA_Data drystream_R = 0.0f;
	LADSPA_Data wetstream_L = 0.0f;
	LADSPA_Data wetstream_R = 0.0f;

	if (ptr->old_pitch != pitch) {
		ptr->pitchmod = ptr->p_pitch;
		prev_p_pitch = ptr->p_pitch;
		fractal(ptr->ring_pnoise, NOISE_LEN, pitch);
		ptr->pos_pnoise = 0;
		ptr->p_pitch = push_buffer(0.0f, ptr->ring_pnoise,
					   ptr->buflen_pnoise, &(ptr->pos_pnoise));
		ptr->d_pitch = (ptr->p_pitch - prev_p_pitch) / (float)(ptr->p_stretch);
		ptr->n_pitch = 0;

		ptr->old_pitch = pitch;
	}

	if (ptr->old_time != time) {
		ptr->delay = ptr->p_delay;
		prev_p_delay = ptr->p_delay;
		fractal(ptr->ring_dnoise, NOISE_LEN, time);
		ptr->pos_dnoise = 0;
		ptr->p_delay = push_buffer(0.0f, ptr->ring_dnoise,
					   ptr->buflen_dnoise, &(ptr->pos_dnoise));
		ptr->d_delay = (ptr->p_delay - prev_p_delay) / (float)(ptr->d_stretch);
		ptr->n_delay = 0;

		ptr->old_time = time;
	}


	for (sample_index = 0; sample_index < sample_count; sample_index++) {

		in_L = *(input_L++);
		in_R = *(input_R++);

		push_buffer(in_L, ptr->ring_L, ptr->buflen_L, &(ptr->pos_L));
		push_buffer(in_R, ptr->ring_R, ptr->buflen_R, &(ptr->pos_R));

		if (ptr->n_pitch < ptr->p_stretch) {
			ptr->pitchmod += ptr->d_pitch;
			ptr->n_pitch++;
		} else {
			ptr->pitchmod = ptr->p_pitch;
			prev_p_pitch = ptr->p_pitch;
			if (!ptr->pos_pnoise) {
				fractal(ptr->ring_pnoise, NOISE_LEN, pitch);
			}
			ptr->p_pitch = push_buffer(0.0f, ptr->ring_pnoise,
						   ptr->buflen_pnoise, &(ptr->pos_pnoise));
			ptr->d_pitch = (ptr->p_pitch - prev_p_pitch) / (float)(ptr->p_stretch);
			ptr->n_pitch = 0;
		}

		if (ptr->n_delay < ptr->d_stretch) {
			ptr->delay += ptr->d_delay;
			ptr->n_delay++;
		} else {
			ptr->delay = ptr->p_delay;
			prev_p_delay = ptr->p_delay;
			if (!ptr->pos_dnoise) {
				fractal(ptr->ring_dnoise, NOISE_LEN, time);
			}
			ptr->p_delay = push_buffer(0.0f, ptr->ring_dnoise,
						   ptr->buflen_dnoise, &(ptr->pos_dnoise));
			ptr->d_delay = (ptr->p_delay - prev_p_delay) / (float)(ptr->d_stretch);
			ptr->n_delay = 0;
		}

		delay = (12.5f * ptr->delay + 37.5f) * ptr->sample_rate / 1000.0f;
		fpos = ptr->buflen_L - depth * (1.0f - ptr->pitchmod) - delay - 1.0f;
		n = floorf(fpos);
		rem = fpos - n;

		s_a_L = read_buffer(ptr->ring_L, ptr->buflen_L,
				    ptr->pos_L, (unsigned long) n);
		s_b_L = read_buffer(ptr->ring_L, ptr->buflen_L,
				    ptr->pos_L, (unsigned long) n + 1);

		s_a_R = read_buffer(ptr->ring_R, ptr->buflen_R,
				    ptr->pos_R, (unsigned long) n);
		s_b_R = read_buffer(ptr->ring_R, ptr->buflen_R,
				    ptr->pos_R, (unsigned long) n + 1);

		drystream_L = drylevel * in_L;
		drystream_R = drylevel * in_R;
		wetstream_L = wetlevel * ((1 - rem) * s_a_L + rem * s_b_L);
		wetstream_R = wetlevel * ((1 - rem) * s_a_R + rem * s_b_R);

		out_L = dryposl * drystream_L + (1.0f - dryposr) * drystream_R +
			wetposl * wetstream_L + (1.0f - wetposr) * wetstream_R;
		out_R = (1.0f - dryposl) * drystream_L + dryposr * drystream_R +
			(1.0f - wetposl) * wetstream_L + wetposr * wetstream_R;

		*(output_L++) += ptr->run_adding_gain * out_L;
		*(output_R++) += ptr->run_adding_gain * out_R;
	}
}



/* Throw away a Doubler effect instance. */
void 
cleanup_Doubler(LADSPA_Handle Instance) {

  	Doubler * ptr = (Doubler *)Instance;
	free(ptr->ring_L);
	free(ptr->ring_R);
	free(ptr->ring_pnoise);
	free(ptr->ring_dnoise);
	free(Instance);
}



LADSPA_Descriptor * stereo_descriptor = NULL;



/* __attribute__((constructor)) tap_init() is called automatically when the plugin library is first
   loaded. */
void 
__attribute__((constructor)) tap_init() {
	
	char ** port_names;
	LADSPA_PortDescriptor * port_descriptors;
	LADSPA_PortRangeHint * port_range_hints;
	
	if ((stereo_descriptor = 
	     (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor))) == NULL)
		exit(1);

	stereo_descriptor->UniqueID = ID_STEREO;
	stereo_descriptor->Label = strdup("tap_doubler");
	stereo_descriptor->Properties = 0;
	stereo_descriptor->Name = strdup("TAP Fractal Doubler");
	stereo_descriptor->Maker = strdup("Tom Szilagyi");
	stereo_descriptor->Copyright = strdup("GPL");
	stereo_descriptor->PortCount = PORTCOUNT_STEREO;

	if ((port_descriptors =
	     (LADSPA_PortDescriptor *)calloc(PORTCOUNT_STEREO, sizeof(LADSPA_PortDescriptor))) == NULL)
		exit(1);

	stereo_descriptor->PortDescriptors = (const LADSPA_PortDescriptor *)port_descriptors;
	port_descriptors[TIME] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[PITCH] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[DRYLEVEL] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[DRYPOSL] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[DRYPOSR] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[WETLEVEL] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[WETPOSL] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[WETPOSR] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[INPUT_L] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
	port_descriptors[INPUT_R] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
	port_descriptors[OUTPUT_L] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
	port_descriptors[OUTPUT_R] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;

	if ((port_names = 
	     (char **)calloc(PORTCOUNT_STEREO, sizeof(char *))) == NULL)
		exit(1);

	stereo_descriptor->PortNames = (const char **)port_names;
	port_names[TIME] = strdup("Time Tracking");
	port_names[PITCH] = strdup("Pitch Tracking");
	port_names[DRYLEVEL] = strdup("Dry Level [dB]");
	port_names[DRYPOSL] = strdup("Dry Left Position");
	port_names[DRYPOSR] = strdup("Dry Right Position");
	port_names[WETLEVEL] = strdup("Wet Level [dB]");
	port_names[WETPOSL] = strdup("Wet Left Position");
	port_names[WETPOSR] = strdup("Wet Right Position");
	port_names[INPUT_L] = strdup("Input_L");
	port_names[INPUT_R] = strdup("Input_R");
	port_names[OUTPUT_L] = strdup("Output_L");
	port_names[OUTPUT_R] = strdup("Output_R");

	if ((port_range_hints = 
	     ((LADSPA_PortRangeHint *)calloc(PORTCOUNT_STEREO, sizeof(LADSPA_PortRangeHint)))) == NULL)
		exit(1);

	stereo_descriptor->PortRangeHints = (const LADSPA_PortRangeHint *)port_range_hints;
	port_range_hints[TIME].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_MIDDLE);
	port_range_hints[PITCH].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_MIDDLE);
	port_range_hints[DRYLEVEL].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[DRYPOSL].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_MINIMUM);
	port_range_hints[DRYPOSR].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_MAXIMUM);
	port_range_hints[WETLEVEL].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[WETPOSL].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_MINIMUM);
	port_range_hints[WETPOSR].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_MAXIMUM);
	port_range_hints[TIME].LowerBound = 0.0f;
	port_range_hints[TIME].UpperBound = 1.0f;
	port_range_hints[PITCH].LowerBound = 0.0f;
	port_range_hints[PITCH].UpperBound = 1.0f;
	port_range_hints[DRYLEVEL].LowerBound = -90.0f;
	port_range_hints[DRYLEVEL].UpperBound = +20.0f;
	port_range_hints[DRYPOSL].LowerBound = 0.0f;
	port_range_hints[DRYPOSL].UpperBound = 1.0f;
	port_range_hints[DRYPOSR].LowerBound = 0.0f;
	port_range_hints[DRYPOSR].UpperBound = 1.0f;
	port_range_hints[WETLEVEL].LowerBound = -90.0f;
	port_range_hints[WETLEVEL].UpperBound = +20.0f;
	port_range_hints[WETPOSL].LowerBound = 0.0f;
	port_range_hints[WETPOSL].UpperBound = 1.0f;
	port_range_hints[WETPOSR].LowerBound = 0.0f;
	port_range_hints[WETPOSR].UpperBound = 1.0f;
	port_range_hints[INPUT_L].HintDescriptor = 0;
	port_range_hints[INPUT_R].HintDescriptor = 0;
	port_range_hints[OUTPUT_L].HintDescriptor = 0;
	port_range_hints[OUTPUT_R].HintDescriptor = 0;
	stereo_descriptor->instantiate = instantiate_Doubler;
	stereo_descriptor->connect_port = connect_port_Doubler;
	stereo_descriptor->activate = activate_Doubler;
	stereo_descriptor->run = run_Doubler;
	stereo_descriptor->run_adding = run_adding_Doubler;
	stereo_descriptor->set_run_adding_gain = set_run_adding_gain_Doubler;
	stereo_descriptor->deactivate = NULL;
	stereo_descriptor->cleanup = cleanup_Doubler;
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
