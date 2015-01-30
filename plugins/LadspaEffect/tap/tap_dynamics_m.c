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

    $Id: tap_dynamics_m.c,v 1.2 2004/06/15 14:50:55 tszilagyi Exp $
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
#define F2S 2147483


#ifdef DYN_CALC_FLOAT
typedef LADSPA_Data dyn_t;
typedef float rms_t;
#else
typedef sample dyn_t;
typedef int64_t rms_t;
#endif



/* The Unique ID of the plugin: */

#define ID_MONO         2152

/* The port numbers for the plugin: */

#define ATTACK          0
#define RELEASE         1
#define OFFSGAIN        2
#define MUGAIN          3
#define RMSENV          4
#define MODGAIN         5
#define MODE            6
#define INPUT           7
#define OUTPUT          8


/* Total number of ports */

#define PORTCOUNT_MONO   9


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
	LADSPA_Data * rmsenv;
	LADSPA_Data * modgain;
	LADSPA_Data * mode;
	LADSPA_Data * input;
	LADSPA_Data * output;
	unsigned long sample_rate;

	float * as;
	unsigned long count;
	dyn_t amp;
	dyn_t env;
	float gain;
	float gain_out;
	rms_env * rms;
	rms_t sum;

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
        dyn_t amp = 0.0f;
	dyn_t env = 0.0f;
	float gain = 0.0f;
	float gain_out = 0.0f;
	rms_env * rms = NULL;
	rms_t sum = 0;
	int i;
	
	if ((ptr = malloc(sizeof(Dynamics))) == NULL)
		return NULL;

	((Dynamics *)ptr)->sample_rate = sample_rate;
	((Dynamics *)ptr)->run_adding_gain = 1.0;

        if ((rms = rms_env_new()) == NULL)
		return NULL;

        if ((as = malloc(TABSIZE * sizeof(float))) == NULL)
		return NULL;

        as[0] = 1.0f;
        for (i = 1; i < TABSIZE; i++) {
		as[i] = expf(-1.0f / (sample_rate * (float)i / (float)TABSIZE));
        }

        ((Dynamics *)ptr)->as = as;
        ((Dynamics *)ptr)->count = count;
        ((Dynamics *)ptr)->amp = amp;
        ((Dynamics *)ptr)->env = env;
        ((Dynamics *)ptr)->gain = gain;
        ((Dynamics *)ptr)->gain_out = gain_out;
        ((Dynamics *)ptr)->rms = rms;
        ((Dynamics *)ptr)->sum = sum;

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
	case RMSENV:
		ptr->rmsenv = DataLocation;
		*(ptr->rmsenv) = -60.0f;
		break;
	case MODGAIN:
		ptr->modgain = DataLocation;
		*(ptr->modgain) = 0.0f;
		break;
	case MODE:
		ptr->mode = DataLocation;
		break;
	case INPUT:
		ptr->input = DataLocation;
		break;
	case OUTPUT:
		ptr->output = DataLocation;
		break;
	}
}



void 
run_Dynamics(LADSPA_Handle Instance,
	     unsigned long sample_count) {

	Dynamics * ptr = (Dynamics *)Instance;
	LADSPA_Data * input = ptr->input;
	LADSPA_Data * output = ptr->output;
        const float attack = LIMIT(*(ptr->attack), 4.0f, 500.0f);
        const float release = LIMIT(*(ptr->release), 4.0f, 1000.0f);
        const float offsgain = LIMIT(*(ptr->offsgain), -20.0f, 20.0f);
        const float mugain = db2lin(LIMIT(*(ptr->mugain), -20.0f, 20.0f));
	const int mode = LIMIT(*(ptr->mode), 0, NUM_MODES-1);
	unsigned long sample_index;

        dyn_t amp = ptr->amp;
        dyn_t env = ptr->env;
        float * as = ptr->as;
        unsigned int count = ptr->count;
        float gain = ptr->gain;
        float gain_out = ptr->gain_out;
        rms_env * rms = ptr->rms;
        rms_t sum = ptr->sum;

        const float ga = as[(unsigned int)(attack * 0.001f * (float)(TABSIZE-1))];
        const float gr = as[(unsigned int)(release * 0.001f * (float)(TABSIZE-1))];
        const float ef_a = ga * 0.25f;
        const float ef_ai = 1.0f - ef_a;

	float level = 0.0f;
	float adjust = 0.0f;

        for (sample_index = 0; sample_index < sample_count; sample_index++) {

#ifdef DYN_CALC_FLOAT
		sum += input[sample_index] * input[sample_index];
		if (amp > env) {
			env = env * ga + amp * (1.0f - ga);
		} else {
			env = env * gr + amp * (1.0f - gr);
		}
#else
		sum += (rms_t)(input[sample_index] * F2S * input[sample_index] * F2S);
		if (amp) {
			if (amp > env) {
				env = (double)env * ga + (double)amp * (1.0f - ga);
			} else {
				env = (double)env * gr + (double)amp * (1.0f - gr);
			}
		} else
			env = 0;
#endif

		if (count++ % 4 == 3) {
#ifdef DYN_CALC_FLOAT
			amp = rms_env_process(rms, sum / 4);
#else
			if (sum)
				amp = rms_env_process(rms, sum / 4);
			else
				amp = 0;
#endif

#ifdef DYN_CALC_FLOAT
			if (isnan(amp))
				amp = 0.0f;
#endif
			sum = 0;

			/* set gain_out according to the difference between
			   the envelope volume level (env) and the corresponding
			   output level (from graph) */
#ifdef DYN_CALC_FLOAT
			level = 20 * log10f(2 * env);
#else
			level = 20 * log10f(2 * (double)env / (double)F2S);
#endif
			adjust = get_table_gain(mode, level + offsgain);
			gain_out = db2lin(adjust);

		}
		gain = gain * ef_a + gain_out * ef_ai;
		output[sample_index] = input[sample_index] * gain * mugain;
        }
        ptr->sum = sum;
        ptr->amp = amp;
        ptr->gain = gain;
        ptr->gain_out = gain_out;
        ptr->env = env;
        ptr->count = count;

	*(ptr->rmsenv) = LIMIT(level, -60.0f, 20.0f);
	*(ptr->modgain) = LIMIT(adjust, -60.0f, 20.0f);
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
	LADSPA_Data * input = ptr->input;
	LADSPA_Data * output = ptr->output;
        const float attack = LIMIT(*(ptr->attack), 4.0f, 500.0f);
        const float release = LIMIT(*(ptr->release), 4.0f, 1000.0f);
        const float offsgain = LIMIT(*(ptr->offsgain), -20.0f, 20.0f);
        const float mugain = db2lin(LIMIT(*(ptr->mugain), -20.0f, 20.0f));
	const int mode = LIMIT(*(ptr->mode), 0, NUM_MODES-1);
	unsigned long sample_index;

        dyn_t amp = ptr->amp;
        dyn_t env = ptr->env;
        float * as = ptr->as;
        unsigned int count = ptr->count;
        float gain = ptr->gain;
        float gain_out = ptr->gain_out;
        rms_env * rms = ptr->rms;
        rms_t sum = ptr->sum;

        const float ga = as[(unsigned int)(attack * 0.001f * (float)(TABSIZE-1))];
        const float gr = as[(unsigned int)(release * 0.001f * (float)(TABSIZE-1))];
        const float ef_a = ga * 0.25f;
        const float ef_ai = 1.0f - ef_a;

	float level = 0.0f;
	float adjust = 0.0f;

        for (sample_index = 0; sample_index < sample_count; sample_index++) {

#ifdef DYN_CALC_FLOAT
		sum += input[sample_index] * input[sample_index];
		if (amp > env) {
			env = env * ga + amp * (1.0f - ga);
		} else {
			env = env * gr + amp * (1.0f - gr);
		}
#else
		sum += (rms_t)(input[sample_index] * F2S * input[sample_index] * F2S);
		if (amp) {
			if (amp > env) {
				env = (double)env * ga + (double)amp * (1.0f - ga);
			} else {
				env = (double)env * gr + (double)amp * (1.0f - gr);
			}
		} else
			env = 0;
#endif

		if (count++ % 4 == 3) {
#ifdef DYN_CALC_FLOAT
			amp = rms_env_process(rms, sum / 4);
#else
			if (sum)
				amp = rms_env_process(rms, sum / 4);
			else
				amp = 0;
#endif

#ifdef DYN_CALC_FLOAT
			if (isnan(amp))
				amp = 0.0f;
#endif
			sum = 0;

			/* set gain_out according to the difference between
			   the envelope volume level (env) and the corresponding
			   output level (from graph) */
#ifdef DYN_CALC_FLOAT
			level = 20 * log10f(2 * env);
#else
			level = 20 * log10f(2 * (double)env / (double)F2S);
#endif
			adjust = get_table_gain(mode, level + offsgain);
			gain_out = db2lin(adjust);

		}
		gain = gain * ef_a + gain_out * ef_ai;
		output[sample_index] += ptr->run_adding_gain * input[sample_index] * gain * mugain;
        }
        ptr->sum = sum;
        ptr->amp = amp;
        ptr->gain = gain;
        ptr->gain_out = gain_out;
        ptr->env = env;
        ptr->count = count;

	*(ptr->rmsenv) = LIMIT(level, -60.0f, 20.0f);
	*(ptr->modgain) = LIMIT(adjust, -60.0f, 20.0f);
}




/* Throw away a Dynamics effect instance. */
void 
cleanup_Dynamics(LADSPA_Handle Instance) {

	Dynamics * ptr = (Dynamics *)Instance;

	free(ptr->rms);
	free(ptr->as);
	free(Instance);
}



LADSPA_Descriptor * mono_descriptor = NULL;



/* __attribute__((constructor)) tap_init() is called automatically when the plugin library is first
   loaded. */
void 
__attribute__((constructor)) tap_init() {
	
	char ** port_names;
	LADSPA_PortDescriptor * port_descriptors;
	LADSPA_PortRangeHint * port_range_hints;
	
	if ((mono_descriptor = 
	     (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor))) == NULL)
		exit(1);
	
	mono_descriptor->UniqueID = ID_MONO;
	mono_descriptor->Label = strdup("tap_dynamics_m");
	mono_descriptor->Properties = 0;
	mono_descriptor->Name = strdup("TAP Dynamics (M)");
	mono_descriptor->Maker = strdup("Tom Szilagyi");
	mono_descriptor->Copyright = strdup("GPL");
	mono_descriptor->PortCount = PORTCOUNT_MONO;

	if ((port_descriptors =
	     (LADSPA_PortDescriptor *)calloc(PORTCOUNT_MONO, sizeof(LADSPA_PortDescriptor))) == NULL)
		exit(1);

	mono_descriptor->PortDescriptors = (const LADSPA_PortDescriptor *)port_descriptors;
	port_descriptors[ATTACK] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[RELEASE] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[OFFSGAIN] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[MUGAIN] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[MODE] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[RMSENV] = LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL;
	port_descriptors[MODGAIN] = LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL;
	port_descriptors[INPUT] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
	port_descriptors[OUTPUT] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;

	if ((port_names = 
	     (char **)calloc(PORTCOUNT_MONO, sizeof(char *))) == NULL)
		exit(1);

	mono_descriptor->PortNames = (const char **)port_names;
	port_names[ATTACK] = strdup("Attack [ms]");
	port_names[RELEASE] = strdup("Release [ms]");
	port_names[OFFSGAIN] = strdup("Offset Gain [dB]");
	port_names[MUGAIN] = strdup("Makeup Gain [dB]");
	port_names[MODE] = strdup("Function");
	port_names[RMSENV] = strdup("Envelope Volume [dB]");
	port_names[MODGAIN] = strdup("Gain Adjustment [dB]");
	port_names[INPUT] = strdup("Input");
	port_names[OUTPUT] = strdup("Output");

	if ((port_range_hints = 
	     ((LADSPA_PortRangeHint *)calloc(PORTCOUNT_MONO, sizeof(LADSPA_PortRangeHint)))) == NULL)
		exit(1);

	mono_descriptor->PortRangeHints = (const LADSPA_PortRangeHint *)port_range_hints;
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
	port_range_hints[RMSENV].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[MODGAIN].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
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
	port_range_hints[RMSENV].LowerBound = -60.0f;
	port_range_hints[RMSENV].UpperBound = 20.0f;
	port_range_hints[MODGAIN].LowerBound = -60.0f;
	port_range_hints[MODGAIN].UpperBound = 20.0f;
	port_range_hints[MODE].LowerBound = 0;
	port_range_hints[MODE].UpperBound = NUM_MODES - 0.9f;
	port_range_hints[INPUT].HintDescriptor = 0;
	port_range_hints[OUTPUT].HintDescriptor = 0;
	mono_descriptor->instantiate = instantiate_Dynamics;
	mono_descriptor->connect_port = connect_port_Dynamics;
	mono_descriptor->activate = NULL;
	mono_descriptor->run = run_Dynamics;
	mono_descriptor->run_adding = run_adding_Dynamics;
	mono_descriptor->set_run_adding_gain = set_run_adding_gain_Dynamics;
	mono_descriptor->deactivate = NULL;
	mono_descriptor->cleanup = cleanup_Dynamics;
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
	delete_descriptor(mono_descriptor);
}


/* Return a descriptor of the requested plugin type. */
const LADSPA_Descriptor * 
ladspa_descriptor(unsigned long Index) {

	switch (Index) {
	case 0:
		return mono_descriptor;
	default:
		return NULL;
	}
}
