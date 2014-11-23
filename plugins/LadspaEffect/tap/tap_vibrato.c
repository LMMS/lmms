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

    $Id: tap_vibrato.c,v 1.3 2004/02/21 17:33:36 tszilagyi Exp $
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <ladspa.h>
#include "tap_utils.h"


/* The Unique ID of the plugin: */

#define ID_MONO         2148

/* The port numbers for the plugin: */

#define FREQ            0
#define DEPTH           1
#define DRYLEVEL        2
#define WETLEVEL        3
#define LATENCY         4
#define INPUT           5
#define OUTPUT          6

/* Total number of ports */


#define PORTCOUNT_MONO   7


/*
 * This has to be bigger than 0.2f * sample_rate / (2*PI) for any sample rate.
 * At 192 kHz 6238 is needed so this should be enough.
 */
#define PM_DEPTH 6300


#define PM_FREQ 30.0f


#define COS_TABLE_SIZE 1024
LADSPA_Data cos_table[COS_TABLE_SIZE];


/* The structure used to hold port connection information and state */

typedef struct {
	LADSPA_Data * depth;
	LADSPA_Data * freq;
	LADSPA_Data * drylevel;
	LADSPA_Data * wetlevel;
	LADSPA_Data * latency;
	LADSPA_Data * input;
	LADSPA_Data * output;

	LADSPA_Data * ringbuffer;
	unsigned long buflen;
	unsigned long pos;
	LADSPA_Data phase;

	unsigned long sample_rate;
	LADSPA_Data run_adding_gain;
} Vibrato;



/* Construct a new plugin instance. */
LADSPA_Handle
instantiate_Vibrato(const LADSPA_Descriptor * Descriptor,
		    unsigned long             sample_rate) {

	LADSPA_Handle * ptr;

	if ((ptr = malloc(sizeof(Vibrato))) != NULL) {
		((Vibrato *)ptr)->sample_rate = sample_rate;
		((Vibrato *)ptr)->run_adding_gain = 1.0f;

		if ((((Vibrato *)ptr)->ringbuffer =
		     calloc(2 * PM_DEPTH, sizeof(LADSPA_Data))) == NULL)
		{
			free(ptr);
			return NULL;
		}
		((Vibrato *)ptr)->buflen = ceil(0.2f * sample_rate / M_PI);
		((Vibrato *)ptr)->pos = 0;

		return ptr;
	}
       	return NULL;
}


void
activate_Vibrato(LADSPA_Handle Instance) {

	Vibrato * ptr = (Vibrato *)Instance;
	unsigned long i;

	for (i = 0; i < 2 * PM_DEPTH; i++)
		ptr->ringbuffer[i] = 0.0f;

	ptr->phase = 0.0f;
}





/* Connect a port to a data location. */
void 
connect_port_Vibrato(LADSPA_Handle Instance,
		     unsigned long Port,
		     LADSPA_Data * DataLocation) {
	
	Vibrato * ptr = (Vibrato *)Instance;

	switch (Port) {
	case DEPTH:
		ptr->depth = DataLocation;
		break;
	case FREQ:
		ptr->freq = DataLocation;
		break;
	case DRYLEVEL:
		ptr->drylevel = DataLocation;
		break;
	case WETLEVEL:
		ptr->wetlevel = DataLocation;
		break;
	case LATENCY:
		ptr->latency = DataLocation;
		*(ptr->latency) = ptr->buflen / 2;  /* IS THIS LEGAL? */
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
run_Vibrato(LADSPA_Handle Instance,
	    unsigned long SampleCount) {
  
	Vibrato * ptr = (Vibrato *)Instance;

	LADSPA_Data freq = LIMIT(*(ptr->freq),0.0f,PM_FREQ);
	LADSPA_Data depth = 
		LIMIT(LIMIT(*(ptr->depth),0.0f,20.0f) * ptr->sample_rate / 200.0f / M_PI / freq,
		      0, ptr->buflen / 2);
	LADSPA_Data drylevel = db2lin(LIMIT(*(ptr->drylevel),-90.0f,20.0f));
	LADSPA_Data wetlevel = db2lin(LIMIT(*(ptr->wetlevel),-90.0f,20.0f));
	LADSPA_Data * input = ptr->input;
	LADSPA_Data * output = ptr->output;

	unsigned long sample_index;
	unsigned long sample_count = SampleCount;

	LADSPA_Data in = 0.0f;
	LADSPA_Data phase = 0.0f;
	LADSPA_Data fpos = 0.0f;
	LADSPA_Data n = 0.0f;
	LADSPA_Data rem = 0.0f;
	LADSPA_Data s_a, s_b;


	if (freq == 0.0f)
		depth = 0.0f;

	for (sample_index = 0; sample_index < sample_count; sample_index++) {

		in = *(input++);

		phase = COS_TABLE_SIZE * freq * sample_index / ptr->sample_rate + ptr->phase;
		while (phase >= COS_TABLE_SIZE)
		        phase -= COS_TABLE_SIZE;

		push_buffer(in, ptr->ringbuffer, ptr->buflen, &(ptr->pos));

		fpos = depth * (1.0f - cos_table[(unsigned long) phase]);
		n = floorf(fpos);
		rem = fpos - n;

		s_a = read_buffer(ptr->ringbuffer, ptr->buflen, ptr->pos, (unsigned long) n);
		s_b = read_buffer(ptr->ringbuffer, ptr->buflen, ptr->pos, (unsigned long) n + 1);

		*(output++) = wetlevel * ((1 - rem) * s_a + rem * s_b) +
			drylevel * read_buffer(ptr->ringbuffer, ptr->buflen,
					       ptr->pos, ptr->buflen / 2);

	}

	ptr->phase += COS_TABLE_SIZE * freq * sample_index / ptr->sample_rate;
	while (ptr->phase >= COS_TABLE_SIZE)
		ptr->phase -= COS_TABLE_SIZE;

	*(ptr->latency) = ptr->buflen / 2;
}


void
set_run_adding_gain_Vibrato(LADSPA_Handle Instance, LADSPA_Data gain) {

	Vibrato * ptr = (Vibrato *)Instance;

	ptr->run_adding_gain = gain;
}



void 
run_adding_Vibrato(LADSPA_Handle Instance,
		   unsigned long SampleCount) {
  
	Vibrato * ptr = (Vibrato *)Instance;

	LADSPA_Data freq = LIMIT(*(ptr->freq),0.0f,PM_FREQ);
	LADSPA_Data depth = 
		LIMIT(LIMIT(*(ptr->depth),0.0f,20.0f) * ptr->sample_rate / 200.0f / M_PI / freq,
		      0, ptr->buflen / 2);
	LADSPA_Data drylevel = db2lin(LIMIT(*(ptr->drylevel),-90.0f,20.0f));
	LADSPA_Data wetlevel = db2lin(LIMIT(*(ptr->wetlevel),-90.0f,20.0f));
	LADSPA_Data * input = ptr->input;
	LADSPA_Data * output = ptr->output;

	unsigned long sample_index;
	unsigned long sample_count = SampleCount;

	LADSPA_Data in = 0.0f;
	LADSPA_Data phase = 0.0f;
	LADSPA_Data fpos = 0.0f;
	LADSPA_Data n = 0.0f;
	LADSPA_Data rem = 0.0f;
	LADSPA_Data s_a, s_b;


	if (freq == 0.0f)
		depth = 0.0f;

	for (sample_index = 0; sample_index < sample_count; sample_index++) {

		in = *(input++);

		phase = COS_TABLE_SIZE * freq * sample_index / ptr->sample_rate + ptr->phase;
		while (phase >= COS_TABLE_SIZE)
		        phase -= COS_TABLE_SIZE;

		push_buffer(in, ptr->ringbuffer, ptr->buflen, &(ptr->pos));

		fpos = depth * (1.0f - cos_table[(unsigned long) phase]);
		n = floorf(fpos);
		rem = fpos - n;

		s_a = read_buffer(ptr->ringbuffer, ptr->buflen, ptr->pos, (unsigned long) n);
		s_b = read_buffer(ptr->ringbuffer, ptr->buflen, ptr->pos, (unsigned long) n + 1);

		*(output++) += ptr->run_adding_gain * wetlevel * ((1 - rem) * s_a + rem * s_b) +
			drylevel * read_buffer(ptr->ringbuffer, ptr->buflen,
					       ptr->pos, ptr->buflen / 2);

	}

	ptr->phase += COS_TABLE_SIZE * freq * sample_index / ptr->sample_rate;
	while (ptr->phase >= COS_TABLE_SIZE)
		ptr->phase -= COS_TABLE_SIZE;

	*(ptr->latency) = ptr->buflen / 2;
}



/* Throw away a Vibrato effect instance. */
void 
cleanup_Vibrato(LADSPA_Handle Instance) {

  	Vibrato * ptr = (Vibrato *)Instance;
	free(ptr->ringbuffer);
	free(Instance);
}



LADSPA_Descriptor * mono_descriptor = NULL;



/* __attribute__((constructor)) tap_init() is called automatically when the plugin library is first
   loaded. */
void 
__attribute__((constructor)) tap_init() {
	
	int i;
	char ** port_names;
	LADSPA_PortDescriptor * port_descriptors;
	LADSPA_PortRangeHint * port_range_hints;
	
	if ((mono_descriptor = 
	     (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor))) == NULL)
		exit(1);
	
	for (i = 0; i < COS_TABLE_SIZE; i++)
		cos_table[i] = cosf(i * 2.0f * M_PI / COS_TABLE_SIZE);

	mono_descriptor->UniqueID = ID_MONO;
	mono_descriptor->Label = strdup("tap_vibrato");
	mono_descriptor->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
	mono_descriptor->Name = strdup("TAP Vibrato");
	mono_descriptor->Maker = strdup("Tom Szilagyi");
	mono_descriptor->Copyright = strdup("GPL");
	mono_descriptor->PortCount = PORTCOUNT_MONO;

	if ((port_descriptors =
	     (LADSPA_PortDescriptor *)calloc(PORTCOUNT_MONO, sizeof(LADSPA_PortDescriptor))) == NULL)
		exit(1);

	mono_descriptor->PortDescriptors = (const LADSPA_PortDescriptor *)port_descriptors;
	port_descriptors[DEPTH] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[FREQ] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[DRYLEVEL] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[WETLEVEL] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[LATENCY] = LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL;
	port_descriptors[INPUT] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
	port_descriptors[OUTPUT] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;

	if ((port_names = 
	     (char **)calloc(PORTCOUNT_MONO, sizeof(char *))) == NULL)
		exit(1);

	mono_descriptor->PortNames = (const char **)port_names;
	port_names[FREQ] = strdup("Frequency [Hz]");
	port_names[DEPTH] = strdup("Depth [%]");
	port_names[DRYLEVEL] = strdup("Dry Level [dB]");
	port_names[WETLEVEL] = strdup("Wet Level [dB]");
	port_names[LATENCY] = strdup("latency");
	port_names[INPUT] = strdup("Input");
	port_names[OUTPUT] = strdup("Output");

	if ((port_range_hints = 
	     ((LADSPA_PortRangeHint *)calloc(PORTCOUNT_MONO, sizeof(LADSPA_PortRangeHint)))) == NULL)
		exit(1);

	mono_descriptor->PortRangeHints	= (const LADSPA_PortRangeHint *)port_range_hints;
	port_range_hints[DEPTH].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[FREQ].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[DRYLEVEL].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_MINIMUM);
	port_range_hints[WETLEVEL].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[LATENCY].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_MAXIMUM);
	port_range_hints[DEPTH].LowerBound = 0;
	port_range_hints[DEPTH].UpperBound = 20.0f;
	port_range_hints[FREQ].LowerBound = 0;
	port_range_hints[FREQ].UpperBound = PM_FREQ;
	port_range_hints[DRYLEVEL].LowerBound = -90.0f;
	port_range_hints[DRYLEVEL].UpperBound = +20.0f;
	port_range_hints[WETLEVEL].LowerBound = -90.0f;
	port_range_hints[WETLEVEL].UpperBound = +20.0f;
	port_range_hints[LATENCY].LowerBound = 0;
	port_range_hints[LATENCY].UpperBound = PM_DEPTH;
	port_range_hints[INPUT].HintDescriptor = 0;
	port_range_hints[OUTPUT].HintDescriptor = 0;
	mono_descriptor->instantiate = instantiate_Vibrato;
	mono_descriptor->connect_port = connect_port_Vibrato;
	mono_descriptor->activate = activate_Vibrato;
	mono_descriptor->run = run_Vibrato;
	mono_descriptor->run_adding = run_adding_Vibrato;
	mono_descriptor->set_run_adding_gain = set_run_adding_gain_Vibrato;
	mono_descriptor->deactivate = NULL;
	mono_descriptor->cleanup = cleanup_Vibrato;
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
