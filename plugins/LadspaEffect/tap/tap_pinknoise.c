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

    $Id: tap_pinknoise.c,v 1.2 2004/08/13 18:34:31 tszilagyi Exp $
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include <ladspa.h>
#include "tap_utils.h"

/* The Unique ID of the plugin: */
#define ID_MONO         2155


/* The port numbers for the plugin: */
#define HURST  0
#define SIGNAL 1
#define NOISE  2
#define INPUT  3
#define OUTPUT 4


/* Total number of ports */
#define PORTCOUNT_MONO   5


#define NOISE_LEN  1024


/* The structure used to hold port connection information and state */
typedef struct {
	LADSPA_Data * hurst;
	LADSPA_Data * signal;
	LADSPA_Data * noise;
	LADSPA_Data * input;
	LADSPA_Data * output;

	LADSPA_Data * ring;
	unsigned long buflen;
	unsigned long pos;

	unsigned long sample_rate;
	LADSPA_Data run_adding_gain;
} Pinknoise;



/* generate fractal pattern using Midpoint Displacement Method
 * v: buffer of floats to output fractal pattern to
 * N: length of v, MUST be integer power of 2 (ie 128, 256, ...)
 * H: Hurst constant, between 0 and 0.9999 (fractal dimension)
 */
void
fractal(LADSPA_Data * v, int N, float H) {

	int l = N;
	int k;
	float r = 2.0f * H*H + 0.3f;
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
instantiate_Pinknoise(const LADSPA_Descriptor * Descriptor,
		      unsigned long             SampleRate) {
	
	LADSPA_Handle * ptr;
	
	if ((ptr = malloc(sizeof(Pinknoise))) != NULL) {
	        ((Pinknoise *)ptr)->sample_rate = SampleRate;
	        ((Pinknoise *)ptr)->run_adding_gain = 1.0;

                if ((((Pinknoise *)ptr)->ring =
                     calloc(NOISE_LEN, sizeof(LADSPA_Data))) == NULL)
                        return NULL;
                ((Pinknoise *)ptr)->buflen = NOISE_LEN;
                ((Pinknoise *)ptr)->pos = 0;

		return ptr;
	}
	
	return NULL;
}


/* Connect a port to a data location. */
void 
connect_port_Pinknoise(LADSPA_Handle Instance,
		       unsigned long Port,
		       LADSPA_Data * data) {
	
	Pinknoise * ptr;
	
	ptr = (Pinknoise *)Instance;
	switch (Port) {
	case HURST:
		ptr->hurst = data;
		break;
	case SIGNAL:
		ptr->signal = data;
		break;
	case NOISE:
		ptr->noise = data;
		break;
	case INPUT:
		ptr->input = data;
		break;
	case OUTPUT:
		ptr->output = data;
		break;
	}
}



void 
run_Pinknoise(LADSPA_Handle Instance,
	      unsigned long SampleCount) {
  
	Pinknoise * ptr = (Pinknoise *)Instance;

	LADSPA_Data * input = ptr->input;
	LADSPA_Data * output = ptr->output;
	LADSPA_Data hurst = LIMIT(*(ptr->hurst), 0.0f, 1.0f);
	LADSPA_Data signal = db2lin(LIMIT(*(ptr->signal), -90.0f, 20.0f));
	LADSPA_Data noise = db2lin(LIMIT(*(ptr->noise), -90.0f, 20.0f));
	unsigned long sample_index;
	
  	for (sample_index = 0; sample_index < SampleCount; sample_index++) {

		if (!ptr->pos)
			fractal(ptr->ring, NOISE_LEN, hurst);

		*(output++) = signal * *(input++) +
			noise * push_buffer(0.0f, ptr->ring, ptr->buflen, &(ptr->pos));
	}
}



void
set_run_adding_gain_Pinknoise(LADSPA_Handle Instance, LADSPA_Data gain) {

	Pinknoise * ptr;

	ptr = (Pinknoise *)Instance;

	ptr->run_adding_gain = gain;
}



void 
run_adding_Pinknoise(LADSPA_Handle Instance,
		   unsigned long SampleCount) {
  
	Pinknoise * ptr = (Pinknoise *)Instance;

	LADSPA_Data * input = ptr->input;
	LADSPA_Data * output = ptr->output;
	LADSPA_Data hurst = LIMIT(*(ptr->hurst), 0.0f, 1.0f);
	LADSPA_Data signal = db2lin(LIMIT(*(ptr->signal), -90.0f, 20.0f));
	LADSPA_Data noise = db2lin(LIMIT(*(ptr->noise), -90.0f, 20.0f));
	unsigned long sample_index;
	
  	for (sample_index = 0; sample_index < SampleCount; sample_index++) {

		if (!ptr->pos)
			fractal(ptr->ring, NOISE_LEN, hurst);

		*(output++) += ptr->run_adding_gain * (signal * *(input++) +
						       noise * push_buffer(0.0f, ptr->ring,
									   ptr->buflen, &(ptr->pos)));
	}
}




/* Throw away a Pinknoise effect instance. */
void 
cleanup_Pinknoise(LADSPA_Handle Instance) {
        Pinknoise * ptr = (Pinknoise *)Instance;
        free(ptr->ring);
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

	/* initialize RNG */
//	srand(time(0));

	mono_descriptor->UniqueID = ID_MONO;
	mono_descriptor->Label = strdup("tap_pinknoise");
	mono_descriptor->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
	mono_descriptor->Name = strdup("TAP Pink/Fractal Noise");
	mono_descriptor->Maker = strdup("Tom Szilagyi");
	mono_descriptor->Copyright = strdup("GPL");
	mono_descriptor->PortCount = PORTCOUNT_MONO;

	if ((port_descriptors =
	     (LADSPA_PortDescriptor *)calloc(PORTCOUNT_MONO, sizeof(LADSPA_PortDescriptor))) == NULL)
		exit(1);

	mono_descriptor->PortDescriptors = (const LADSPA_PortDescriptor *)port_descriptors;
	port_descriptors[HURST] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[SIGNAL] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[NOISE] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[INPUT] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
	port_descriptors[OUTPUT] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;

	if ((port_names = 
	     (char **)calloc(PORTCOUNT_MONO, sizeof(char *))) == NULL)
		exit(1);

	mono_descriptor->PortNames = (const char **)port_names;
	port_names[HURST] = strdup("Fractal Dimension");
	port_names[SIGNAL] = strdup("Signal Level [dB]");
	port_names[NOISE] = strdup("Noise Level [dB]");
	port_names[INPUT] = strdup("Input");
	port_names[OUTPUT] = strdup("Output");

	if ((port_range_hints = 
	     ((LADSPA_PortRangeHint *)calloc(PORTCOUNT_MONO, sizeof(LADSPA_PortRangeHint)))) == NULL)
		exit(1);

	mono_descriptor->PortRangeHints	= (const LADSPA_PortRangeHint *)port_range_hints;
	port_range_hints[HURST].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_MIDDLE);
	port_range_hints[SIGNAL].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[NOISE].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_MINIMUM);
	port_range_hints[HURST].LowerBound = 0.0f;
	port_range_hints[HURST].UpperBound = 1.0f;
	port_range_hints[SIGNAL].LowerBound = -90.0f;
	port_range_hints[SIGNAL].UpperBound = 20.0f;
	port_range_hints[NOISE].LowerBound = -90.0f;
	port_range_hints[NOISE].UpperBound = 20.0f;
	port_range_hints[INPUT].HintDescriptor = 0;
	port_range_hints[OUTPUT].HintDescriptor = 0;
	mono_descriptor->instantiate = instantiate_Pinknoise;
	mono_descriptor->connect_port = connect_port_Pinknoise;
	mono_descriptor->activate = NULL;
	mono_descriptor->run = run_Pinknoise;
	mono_descriptor->run_adding = run_adding_Pinknoise;
	mono_descriptor->set_run_adding_gain = set_run_adding_gain_Pinknoise;
	mono_descriptor->deactivate = NULL;
	mono_descriptor->cleanup = cleanup_Pinknoise;
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
