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

    $Id: tap_dynamics_st.c,v 1.2 2004/06/15 14:50:55 tszilagyi Exp $
*/


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include <ladspa.h>
#include "tap_utils.h"


/* ***** VERY IMPORTANT! *****
 *
 * If you enable this, the plugin will use float arithmetics in DSP
 * calculations.  This usually yields lower average CPU usage, but
 * occasionaly may result in high CPU peaks which cause trouble to you
 * and your JACK server.  The default is to use fixpoint arithmetics
 * (with the following #define commented out).  But (depending on the
 * processor on which you run the code) you may find floating point
 * mode usable.
 */
/*#define DYN_CALC_FLOAT*/


typedef signed int sample;

/* coefficient for float to sample (signed int) conversion */
/* this allows for about 60 dB headroom above 0dB, if 0 dB is equivalent to 1.0f */
/* As 2^31 equals more than 180 dB, about 120 dB dynamics remains below 0 dB */
#define F2S 2147483


#ifdef DYN_CALC_FLOAT
typedef LADSPA_Data dyn_t;
typedef float rms_t;
#else
typedef sample dyn_t;
typedef int64_t rms_t;
#endif



/* The Unique ID of the plugin: */

#define ID_STEREO         2153

/* The port numbers for the plugin: */

#define ATTACK          0
#define RELEASE         1
#define OFFSGAIN        2
#define MUGAIN          3
#define RMSENV_L        4
#define RMSENV_R        5
#define MODGAIN_L       6
#define MODGAIN_R       7
#define STEREO          8
#define MODE            9
#define INPUT_L         10
#define INPUT_R         11
#define OUTPUT_L        12
#define OUTPUT_R        13


/* Total number of ports */

#define PORTCOUNT_STEREO   14


#define TABSIZE 256
#define RMSSIZE 64


typedef struct {
        rms_t        buffer[RMSSIZE];
        unsigned int pos;
        rms_t        sum;
} rms_env;


/* max. number of breakpoints on in/out dB graph */
#define MAX_POINTS 20

typedef struct {
	LADSPA_Data x;
	LADSPA_Data y;
} GRAPH_POINT;

typedef struct {
	unsigned long num_points;
	GRAPH_POINT points[MAX_POINTS];
} DYNAMICS_DATA;

#include "tap_dynamics_presets.h"


/* The structure used to hold port connection information and state */

typedef struct {
	LADSPA_Data * attack;
	LADSPA_Data * release;
	LADSPA_Data * offsgain;
	LADSPA_Data * mugain;
	LADSPA_Data * rmsenv_L;
	LADSPA_Data * rmsenv_R;
	LADSPA_Data * modgain_L;
	LADSPA_Data * modgain_R;
	LADSPA_Data * stereo;
	LADSPA_Data * mode;
	LADSPA_Data * input_L;
	LADSPA_Data * output_L;
	LADSPA_Data * input_R;
	LADSPA_Data * output_R;
	unsigned long sample_rate;

	float * as;
	unsigned long count;
	dyn_t amp_L;
	dyn_t amp_R;
	dyn_t env_L;
	dyn_t env_R;
	float gain_L;
	float gain_R;
	float gain_out_L;
	float gain_out_R;
	rms_env * rms_L;
	rms_env * rms_R;
	rms_t sum_L;
	rms_t sum_R;

	DYNAMICS_DATA graph;

	LADSPA_Data run_adding_gain;
} Dynamics;



/* RMS envelope stuff, grabbed without a second thought from Steve Harris's swh-plugins, util/rms.c */
/* Adapted, though, to be able to use fixed-point arithmetics as well. */

rms_env *
rms_env_new(void) {

        rms_env * new = (rms_env *)calloc(1, sizeof(rms_env));

        return new;
}

void
rms_env_reset(rms_env *r) {

        unsigned int i;

        for (i = 0; i < RMSSIZE; i++) {
                r->buffer[i] = 0.0f;
        }
        r->pos = 0;
        r->sum = 0.0f;
}

inline static
dyn_t
rms_env_process(rms_env *r, const rms_t x) {

        r->sum -= r->buffer[r->pos];
        r->sum += x;
        r->buffer[r->pos] = x;
        r->pos = (r->pos + 1) & (RMSSIZE - 1);

#ifdef DYN_CALC_FLOAT
        return sqrt(r->sum / (float)RMSSIZE);
#else
        return sqrt(r->sum / RMSSIZE);
#endif
}



inline
LADSPA_Data
get_table_gain(int mode, LADSPA_Data level) {

	LADSPA_Data x1 = -80.0f;
	LADSPA_Data y1 = -80.0f;
	LADSPA_Data x2 = 0.0f;
	LADSPA_Data y2 = 0.0f;
	unsigned int i = 0;

	if (level <= -80.0f)
		return get_table_gain(mode, -79.9f);

	while (i < dyn_data[mode].num_points && dyn_data[mode].points[i].x < level) {
		x1 = dyn_data[mode].points[i].x;
		y1 = dyn_data[mode].points[i].y;
		i++;
	}
	if (i < dyn_data[mode].num_points) {
		x2 = dyn_data[mode].points[i].x;
		y2 = dyn_data[mode].points[i].y;
	} else
		return 0.0f;

	return y1 + ((level - x1) * (y2 - y1) / (x2 - x1)) - level;
}


/* Construct a new plugin instance. */
LADSPA_Handle 
instantiate_Dynamics(const LADSPA_Descriptor * Descriptor, unsigned long sample_rate) {
	
	LADSPA_Handle * ptr;

	float * as = NULL;
	unsigned int count = 0;
	dyn_t amp_L = 0.0f;
	dyn_t amp_R = 0.0f;
	dyn_t env_L = 0.0f;
	dyn_t env_R = 0.0f;
	float gain_L = 0.0f;
	float gain_R = 0.0f;
	float gain_out_L = 0.0f;
	float gain_out_R = 0.0f;
	rms_env * rms_L = NULL;
	rms_env * rms_R = NULL;
	rms_t sum_L = 0.0f;
	rms_t sum_R = 0.0f;
	int i;
	
	if ((ptr = malloc(sizeof(Dynamics))) == NULL)
		return NULL;

	((Dynamics *)ptr)->sample_rate = sample_rate;
	((Dynamics *)ptr)->run_adding_gain = 1.0;

        if ((rms_L = rms_env_new()) == NULL)
		return NULL;
        if ((rms_R = rms_env_new()) == NULL)
		return NULL;

        if ((as = malloc(TABSIZE * sizeof(float))) == NULL)
		return NULL;

        as[0] = 1.0f;
        for (i = 1; i < TABSIZE; i++) {
		as[i] = expf(-1.0f / (sample_rate * (float)i / (float)TABSIZE));
        }

        ((Dynamics *)ptr)->as = as;
        ((Dynamics *)ptr)->count = count;
        ((Dynamics *)ptr)->amp_L = amp_L;
        ((Dynamics *)ptr)->amp_R = amp_R;
        ((Dynamics *)ptr)->env_L = env_L;
        ((Dynamics *)ptr)->env_R = env_R;
        ((Dynamics *)ptr)->gain_L = gain_L;
        ((Dynamics *)ptr)->gain_R = gain_R;
        ((Dynamics *)ptr)->gain_out_L = gain_out_L;
        ((Dynamics *)ptr)->gain_out_R = gain_out_R;
        ((Dynamics *)ptr)->rms_L = rms_L;
        ((Dynamics *)ptr)->rms_R = rms_R;
        ((Dynamics *)ptr)->sum_L = sum_L;
        ((Dynamics *)ptr)->sum_R = sum_R;

	return ptr;
}



/* Connect a port to a data location. */
void 
connect_port_Dynamics(LADSPA_Handle Instance,
		     unsigned long Port,
		     LADSPA_Data * DataLocation) {
	
	Dynamics * ptr = (Dynamics *)Instance;

	switch (Port) {
	case ATTACK:
		ptr->attack = DataLocation;
		break;
	case RELEASE:
		ptr->release = DataLocation;
		break;
	case OFFSGAIN:
		ptr->offsgain = DataLocation;
		break;
	case MUGAIN:
		ptr->mugain = DataLocation;
		break;
	case RMSENV_L:
		ptr->rmsenv_L = DataLocation;
		*(ptr->rmsenv_L) = -60.0f;
		break;
	case RMSENV_R:
		ptr->rmsenv_R = DataLocation;
		*(ptr->rmsenv_R) = -60.0f;
		break;
	case MODGAIN_L:
		ptr->modgain_L = DataLocation;
		*(ptr->modgain_L) = 0.0f;
		break;
	case MODGAIN_R:
		ptr->modgain_R = DataLocation;
		*(ptr->modgain_R) = 0.0f;
		break;
	case STEREO:
		ptr->stereo = DataLocation;
		break;
	case MODE:
		ptr->mode = DataLocation;
		break;
	case INPUT_L:
		ptr->input_L = DataLocation;
		break;
	case OUTPUT_L:
		ptr->output_L = DataLocation;
		break;
	case INPUT_R:
		ptr->input_R = DataLocation;
		break;
	case OUTPUT_R:
		ptr->output_R = DataLocation;
		break;
	}
}



void 
run_Dynamics(LADSPA_Handle Instance,
	     unsigned long sample_count) {

	Dynamics * ptr = (Dynamics *)Instance;
	LADSPA_Data * input_L = ptr->input_L;
	LADSPA_Data * output_L = ptr->output_L;
	LADSPA_Data * input_R = ptr->input_R;
	LADSPA_Data * output_R = ptr->output_R;
        const float attack = LIMIT(*(ptr->attack), 4.0f, 500.0f);
        const float release = LIMIT(*(ptr->release), 4.0f, 1000.0f);
        const float offsgain = LIMIT(*(ptr->offsgain), -20.0f, 20.0f);
        const float mugain = db2lin(LIMIT(*(ptr->mugain), -20.0f, 20.0f));
	const int stereo = LIMIT(*(ptr->stereo), 0, 2);
	const int mode = LIMIT(*(ptr->mode), 0, NUM_MODES-1);
	unsigned long sample_index;

        dyn_t amp_L = ptr->amp_L;
        dyn_t amp_R = ptr->amp_R;
        dyn_t env_L = ptr->env_L;
        dyn_t env_R = ptr->env_R;
        float * as = ptr->as;
        unsigned int count = ptr->count;
        float gain_L = ptr->gain_L;
        float gain_R = ptr->gain_R;
        float gain_out_L = ptr->gain_out_L;
        float gain_out_R = ptr->gain_out_R;
        rms_env * rms_L = ptr->rms_L;
        rms_env * rms_R = ptr->rms_R;
        rms_t sum_L = ptr->sum_L;
        rms_t sum_R = ptr->sum_R;

        const float ga = as[(unsigned int)(attack * 0.001f * (LADSPA_Data)(TABSIZE-1))];
        const float gr = as[(unsigned int)(release * 0.001f * (LADSPA_Data)(TABSIZE-1))];
        const float ef_a = ga * 0.25f;
        const float ef_ai = 1.0f - ef_a;

	float level_L = 0.0f;
	float level_R = 0.0f;
	float adjust_L = 0.0f;
	float adjust_R = 0.0f;

        for (sample_index = 0; sample_index < sample_count; sample_index++) {

#ifdef DYN_CALC_FLOAT
                sum_L += input_L[sample_index] * input_L[sample_index];
                sum_R += input_R[sample_index] * input_R[sample_index];

                if (amp_L > env_L) {
                        env_L = env_L * ga + amp_L * (1.0f - ga);
                } else {
                        env_L = env_L * gr + amp_L * (1.0f - gr);
                }
                if (amp_R > env_R) {
                        env_R = env_R * ga + amp_R * (1.0f - ga);
                } else {
                        env_R = env_R * gr + amp_R * (1.0f - gr);
                }
#else
		sum_L += (rms_t)(input_L[sample_index] * F2S) * (rms_t)(input_L[sample_index] * F2S);
		sum_R += (rms_t)(input_R[sample_index] * F2S) * (rms_t)(input_R[sample_index] * F2S);

		if (amp_L) {
			if (amp_L > env_L) {
				env_L = (double)env_L * ga + (double)amp_L * (1.0f - ga);
			} else {
				env_L = (double)env_L * gr + (double)amp_L * (1.0f - gr);
			}
		} else
			env_L = 0;

		if (amp_R) {
			if (amp_R > env_R) {
				env_R = (double)env_R * ga + (double)amp_R * (1.0f - ga);
			} else {
				env_R = (double)env_R * gr + (double)amp_R * (1.0f - gr);
			}
		} else
			env_R = 0;
#endif

		if (count++ % 4 == 3) {
#ifdef DYN_CALC_FLOAT
			amp_L = rms_env_process(rms_L, sum_L * 0.25f);
			amp_R = rms_env_process(rms_R, sum_R * 0.25f);
#else
			if (sum_L)
				amp_L = rms_env_process(rms_L, sum_L * 0.25f);
			else
				amp_L = 0;

			if (sum_R)
				amp_R = rms_env_process(rms_R, sum_R * 0.25f);
			else
				amp_R = 0;
#endif


#ifdef DYN_CALC_FLOAT
			if (isnan(amp_L))
				amp_L = 0.0f;
			if (isnan(amp_R))
				amp_R = 0.0f;
#endif
			sum_L = sum_R = 0;

			/* set gain_out according to the difference between
			   the envelope volume level (env) and the corresponding
			   output level (from graph) */
#ifdef DYN_CALC_FLOAT
			level_L = 20 * log10f(2 * env_L);
			level_R = 20 * log10f(2 * env_R);
#else
                        level_L = 20 * log10f(2 * (double)env_L / (double)F2S);
                        level_R = 20 * log10f(2 * (double)env_R / (double)F2S);
#endif
			adjust_L = get_table_gain(mode, level_L + offsgain);
			adjust_R = get_table_gain(mode, level_R + offsgain);

			/* set gains according to stereo mode */
			switch (stereo) {
			case 0:
				gain_out_L = db2lin(adjust_L);
				gain_out_R = db2lin(adjust_R);
				break;
			case 1:
				adjust_L = adjust_R = (adjust_L + adjust_R) / 2.0f;
				gain_out_L = gain_out_R = db2lin(adjust_L);
				break;
			case 2:
				adjust_L = adjust_R = (adjust_L > adjust_R) ? adjust_L : adjust_R;
				gain_out_L = gain_out_R = db2lin(adjust_L);
				break;
			}

		}
		gain_L = gain_L * ef_a + gain_out_L * ef_ai;
		gain_R = gain_R * ef_a + gain_out_R * ef_ai;
		output_L[sample_index] = input_L[sample_index] * gain_L * mugain;
		output_R[sample_index] = input_R[sample_index] * gain_R * mugain;
        }
        ptr->sum_L = sum_L;
        ptr->sum_R = sum_R;
        ptr->amp_L = amp_L;
        ptr->amp_R = amp_R;
        ptr->gain_L = gain_L;
        ptr->gain_R = gain_R;
        ptr->gain_out_L = gain_out_L;
        ptr->gain_out_R = gain_out_R;
        ptr->env_L = env_L;
        ptr->env_R = env_R;
        ptr->count = count;

	*(ptr->rmsenv_L) = LIMIT(level_L, -60.0f, 20.0f);
	*(ptr->rmsenv_R) = LIMIT(level_R, -60.0f, 20.0f);
	*(ptr->modgain_L) = LIMIT(adjust_L, -60.0f, 20.0f);
	*(ptr->modgain_R) = LIMIT(adjust_R, -60.0f, 20.0f);
}



void
set_run_adding_gain_Dynamics(LADSPA_Handle Instance, LADSPA_Data gain) {

	Dynamics * ptr = (Dynamics *)Instance;

	ptr->run_adding_gain = gain;
}



void 
run_adding_Dynamics(LADSPA_Handle Instance,
		    unsigned long sample_count) {

	Dynamics * ptr = (Dynamics *)Instance;
	LADSPA_Data * input_L = ptr->input_L;
	LADSPA_Data * output_L = ptr->output_L;
	LADSPA_Data * input_R = ptr->input_R;
	LADSPA_Data * output_R = ptr->output_R;
        const float attack = LIMIT(*(ptr->attack), 4.0f, 500.0f);
        const float release = LIMIT(*(ptr->release), 4.0f, 1000.0f);
        const float offsgain = LIMIT(*(ptr->offsgain), -20.0f, 20.0f);
        const float mugain = db2lin(LIMIT(*(ptr->mugain), -20.0f, 20.0f));
	const int stereo = LIMIT(*(ptr->stereo), 0, 2);
	const int mode = LIMIT(*(ptr->mode), 0, NUM_MODES-1);
	unsigned long sample_index;

        dyn_t amp_L = ptr->amp_L;
        dyn_t amp_R = ptr->amp_R;
        dyn_t env_L = ptr->env_L;
        dyn_t env_R = ptr->env_R;
        float * as = ptr->as;
        unsigned int count = ptr->count;
        float gain_L = ptr->gain_L;
        float gain_R = ptr->gain_R;
        float gain_out_L = ptr->gain_out_L;
        float gain_out_R = ptr->gain_out_R;
        rms_env * rms_L = ptr->rms_L;
        rms_env * rms_R = ptr->rms_R;
        rms_t sum_L = ptr->sum_L;
        rms_t sum_R = ptr->sum_R;

        const float ga = as[(unsigned int)(attack * 0.001f * (LADSPA_Data)(TABSIZE-1))];
        const float gr = as[(unsigned int)(release * 0.001f * (LADSPA_Data)(TABSIZE-1))];
        const float ef_a = ga * 0.25f;
        const float ef_ai = 1.0f - ef_a;

	float level_L = 0.0f;
	float level_R = 0.0f;
	float adjust_L = 0.0f;
	float adjust_R = 0.0f;

        for (sample_index = 0; sample_index < sample_count; sample_index++) {

#ifdef DYN_CALC_FLOAT
                sum_L += input_L[sample_index] * input_L[sample_index];
                sum_R += input_R[sample_index] * input_R[sample_index];

                if (amp_L > env_L) {
                        env_L = env_L * ga + amp_L * (1.0f - ga);
                } else {
                        env_L = env_L * gr + amp_L * (1.0f - gr);
                }
                if (amp_R > env_R) {
                        env_R = env_R * ga + amp_R * (1.0f - ga);
                } else {
                        env_R = env_R * gr + amp_R * (1.0f - gr);
                }
#else
		sum_L += (rms_t)(input_L[sample_index] * F2S) * (rms_t)(input_L[sample_index] * F2S);
		sum_R += (rms_t)(input_R[sample_index] * F2S) * (rms_t)(input_R[sample_index] * F2S);

		if (amp_L) {
			if (amp_L > env_L) {
				env_L = (double)env_L * ga + (double)amp_L * (1.0f - ga);
			} else {
				env_L = (double)env_L * gr + (double)amp_L * (1.0f - gr);
			}
		} else
			env_L = 0;

		if (amp_R) {
			if (amp_R > env_R) {
				env_R = (double)env_R * ga + (double)amp_R * (1.0f - ga);
			} else {
				env_R = (double)env_R * gr + (double)amp_R * (1.0f - gr);
			}
		} else
			env_R = 0;
#endif

		if (count++ % 4 == 3) {
#ifdef DYN_CALC_FLOAT
			amp_L = rms_env_process(rms_L, sum_L * 0.25f);
			amp_R = rms_env_process(rms_R, sum_R * 0.25f);
#else
			if (sum_L)
				amp_L = rms_env_process(rms_L, sum_L * 0.25f);
			else
				amp_L = 0;

			if (sum_R)
				amp_R = rms_env_process(rms_R, sum_R * 0.25f);
			else
				amp_R = 0;
#endif


#ifdef DYN_CALC_FLOAT
			if (isnan(amp_L))
				amp_L = 0.0f;
			if (isnan(amp_R))
				amp_R = 0.0f;
#endif
			sum_L = sum_R = 0;

			/* set gain_out according to the difference between
			   the envelope volume level (env) and the corresponding
			   output level (from graph) */
#ifdef DYN_CALC_FLOAT
			level_L = 20 * log10f(2 * env_L);
			level_R = 20 * log10f(2 * env_R);
#else
                        level_L = 20 * log10f(2 * (double)env_L / (double)F2S);
                        level_R = 20 * log10f(2 * (double)env_R / (double)F2S);
#endif
			adjust_L = get_table_gain(mode, level_L + offsgain);
			adjust_R = get_table_gain(mode, level_R + offsgain);

			/* set gains according to stereo mode */
			switch (stereo) {
			case 0:
				gain_out_L = db2lin(adjust_L);
				gain_out_R = db2lin(adjust_R);
				break;
			case 1:
				adjust_L = adjust_R = (adjust_L + adjust_R) / 2.0f;
				gain_out_L = gain_out_R = db2lin(adjust_L);
				break;
			case 2:
				adjust_L = adjust_R = (adjust_L > adjust_R) ? adjust_L : adjust_R;
				gain_out_L = gain_out_R = db2lin(adjust_L);
				break;
			}

		}
		gain_L = gain_L * ef_a + gain_out_L * ef_ai;
		gain_R = gain_R * ef_a + gain_out_R * ef_ai;
		output_L[sample_index] += ptr->run_adding_gain * input_L[sample_index] * gain_L * mugain;
		output_R[sample_index] += ptr->run_adding_gain * input_R[sample_index] * gain_R * mugain;
        }
        ptr->sum_L = sum_L;
        ptr->sum_R = sum_R;
        ptr->amp_L = amp_L;
        ptr->amp_R = amp_R;
        ptr->gain_L = gain_L;
        ptr->gain_R = gain_R;
        ptr->gain_out_L = gain_out_L;
        ptr->gain_out_R = gain_out_R;
        ptr->env_L = env_L;
        ptr->env_R = env_R;
        ptr->count = count;

	*(ptr->rmsenv_L) = LIMIT(level_L, -60.0f, 20.0f);
	*(ptr->rmsenv_R) = LIMIT(level_R, -60.0f, 20.0f);
	*(ptr->modgain_L) = LIMIT(adjust_L, -60.0f, 20.0f);
	*(ptr->modgain_R) = LIMIT(adjust_R, -60.0f, 20.0f);
}




/* Throw away a Dynamics effect instance. */
void 
cleanup_Dynamics(LADSPA_Handle Instance) {

	Dynamics * ptr = (Dynamics *)Instance;

	free(ptr->rms_L);
	free(ptr->rms_R);
	free(ptr->as);
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
	stereo_descriptor->Label = strdup("tap_dynamics_st");
	stereo_descriptor->Properties = 0;
	stereo_descriptor->Name = strdup("TAP Dynamics (St)");
	stereo_descriptor->Maker = strdup("Tom Szilagyi");
	stereo_descriptor->Copyright = strdup("GPL");
	stereo_descriptor->PortCount = PORTCOUNT_STEREO;

	if ((port_descriptors =
	     (LADSPA_PortDescriptor *)calloc(PORTCOUNT_STEREO, sizeof(LADSPA_PortDescriptor))) == NULL)
		exit(1);

	stereo_descriptor->PortDescriptors = (const LADSPA_PortDescriptor *)port_descriptors;
	port_descriptors[ATTACK] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[RELEASE] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[OFFSGAIN] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[MUGAIN] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[STEREO] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[MODE] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[RMSENV_L] = LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL;
	port_descriptors[RMSENV_R] = LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL;
	port_descriptors[MODGAIN_L] = LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL;
	port_descriptors[MODGAIN_R] = LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL;
	port_descriptors[INPUT_L] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
	port_descriptors[INPUT_R] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
	port_descriptors[OUTPUT_L] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
	port_descriptors[OUTPUT_R] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;

	if ((port_names = 
	     (char **)calloc(PORTCOUNT_STEREO, sizeof(char *))) == NULL)
		exit(1);

	stereo_descriptor->PortNames = (const char **)port_names;
	port_names[ATTACK] = strdup("Attack [ms]");
	port_names[RELEASE] = strdup("Release [ms]");
	port_names[OFFSGAIN] = strdup("Offset Gain [dB]");
	port_names[MUGAIN] = strdup("Makeup Gain [dB]");
	port_names[STEREO] = strdup("Stereo Mode");
	port_names[MODE] = strdup("Function");
	port_names[RMSENV_L] = strdup("Envelope Volume (L) [dB]");
	port_names[RMSENV_R] = strdup("Envelope Volume (R) [dB]");
	port_names[MODGAIN_L] = strdup("Gain Adjustment (L) [dB]");
	port_names[MODGAIN_R] = strdup("Gain Adjustment (R) [dB]");
	port_names[INPUT_L] = strdup("Input Left");
	port_names[INPUT_R] = strdup("Input Right");
	port_names[OUTPUT_L] = strdup("Output Left");
	port_names[OUTPUT_R] = strdup("Output Right");

	if ((port_range_hints = 
	     ((LADSPA_PortRangeHint *)calloc(PORTCOUNT_STEREO, sizeof(LADSPA_PortRangeHint)))) == NULL)
		exit(1);

	stereo_descriptor->PortRangeHints = (const LADSPA_PortRangeHint *)port_range_hints;
	port_range_hints[ATTACK].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_LOW);
	port_range_hints[RELEASE].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_MIDDLE);
	port_range_hints[OFFSGAIN].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[MUGAIN].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[RMSENV_L].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[RMSENV_R].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[MODGAIN_L].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[MODGAIN_R].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[STEREO].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_INTEGER |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[MODE].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_INTEGER |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[ATTACK].LowerBound = 4.0f;
	port_range_hints[ATTACK].UpperBound = 500.0f;
	port_range_hints[RELEASE].LowerBound = 4.0f;
	port_range_hints[RELEASE].UpperBound = 1000.0f;
	port_range_hints[OFFSGAIN].LowerBound = -20.0f;
	port_range_hints[OFFSGAIN].UpperBound = 20.0f;
	port_range_hints[MUGAIN].LowerBound = -20.0f;
	port_range_hints[MUGAIN].UpperBound = 20.0f;
	port_range_hints[RMSENV_L].LowerBound = -60.0f;
	port_range_hints[RMSENV_L].UpperBound = 20.0f;
	port_range_hints[RMSENV_R].LowerBound = -60.0f;
	port_range_hints[RMSENV_R].UpperBound = 20.0f;
	port_range_hints[MODGAIN_L].LowerBound = -60.0f;
	port_range_hints[MODGAIN_L].UpperBound = 20.0f;
	port_range_hints[MODGAIN_R].LowerBound = -60.0f;
	port_range_hints[MODGAIN_R].UpperBound = 20.0f;
	port_range_hints[STEREO].LowerBound = 0;
	port_range_hints[STEREO].UpperBound = 2.1f;
	port_range_hints[MODE].LowerBound = 0;
	port_range_hints[MODE].UpperBound = NUM_MODES - 0.9f;
	port_range_hints[INPUT_L].HintDescriptor = 0;
	port_range_hints[INPUT_R].HintDescriptor = 0;
	port_range_hints[OUTPUT_L].HintDescriptor = 0;
	port_range_hints[OUTPUT_R].HintDescriptor = 0;
	stereo_descriptor->instantiate = instantiate_Dynamics;
	stereo_descriptor->connect_port = connect_port_Dynamics;
	stereo_descriptor->activate = NULL;
	stereo_descriptor->run = run_Dynamics;
	stereo_descriptor->run_adding = run_adding_Dynamics;
	stereo_descriptor->set_run_adding_gain = set_run_adding_gain_Dynamics;
	stereo_descriptor->deactivate = NULL;
	stereo_descriptor->cleanup = cleanup_Dynamics;
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
