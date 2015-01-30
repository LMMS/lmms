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

    $Id: tap_pitch.c,v 1.2 2004/02/21 17:33:36 tszilagyi Exp $
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <ladspa.h>
#include "tap_utils.h"


/* The Unique ID of the plugin: */

#define ID_MONO         2150

/* The port numbers for the plugin: */

#define SEMITONE        0
#define RATE            1
#define DRYLEVEL        2
#define WETLEVEL        3
#define LATENCY         4
#define INPUT           5
#define OUTPUT          6

/* Total number of ports */


#define PORTCOUNT_MONO   7


/* depth of phase mod (yes, this is a magic number) */
#define PM_DEPTH 3681.0f


/* another magic number, derived from the above one */
#define PM_BUFLEN 16027


/* frequency of the modulation signal (Hz) */
#define PM_FREQ 6.0f


#define COS_TABLE_SIZE 1024
LADSPA_Data cos_table[COS_TABLE_SIZE];


/* \sqrt{12}{2} used for key frequency computing */
#define ROOT_12_2  1.059463094f


/* The structure used to hold port connection information and state */

typedef struct {
	LADSPA_Data * rate;
	LADSPA_Data * semitone;
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
} Pitch;



/* Construct a new plugin instance. */
LADSPA_Handle 
instantiate_Pitch(const LADSPA_Descriptor * Descriptor,
		  unsigned long             sample_rate) {
  
        LADSPA_Handle * ptr;
	
	if ((ptr = malloc(sizeof(Pitch))) != NULL) {
		((Pitch *)ptr)->sample_rate = sample_rate;
		((Pitch *)ptr)->run_adding_gain = 1.0f;

		if ((((Pitch *)ptr)->ringbuffer = 
		     calloc(2 * PM_BUFLEN, sizeof(LADSPA_Data))) == NULL)
			return NULL;
		((Pitch *)ptr)->buflen = 2 * PM_BUFLEN * sample_rate / 192000;
		((Pitch *)ptr)->pos = 0;

		return ptr;
	}
       	return NULL;
}


void
activate_Pitch(LADSPA_Handle Instance) {

	Pitch * ptr = (Pitch *)Instance;
	unsigned long i;

	for (i = 0; i < ptr->buflen; i++)
		ptr->ringbuffer[i] = 0.0f;

	ptr->phase = 0.0f;
}





/* Connect a port to a data location. */
void 
connect_port_Pitch(LADSPA_Handle Instance,
		     unsigned long Port,
		     LADSPA_Data * DataLocation) {
	
	Pitch * ptr = (Pitch *)Instance;

	switch (Port) {
	case RATE:
		ptr->rate = DataLocation;
		break;
	case SEMITONE:
		ptr->semitone = DataLocation;
		break;
	case DRYLEVEL:
		ptr->drylevel = DataLocation;
		break;
	case WETLEVEL:
		ptr->wetlevel = DataLocation;
		break;
	case LATENCY:
		ptr->latency = DataLocation;
		*(ptr->latency) = ptr->buflen / 2; /* IS THIS LEGAL? */
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
run_Pitch(LADSPA_Handle Instance,
	    unsigned long SampleCount) {
  
	Pitch * ptr = (Pitch *)Instance;
	LADSPA_Data * input = ptr->input;
	LADSPA_Data * output = ptr->output;
	LADSPA_Data drylevel = db2lin(LIMIT(*(ptr->drylevel),-90.0f,20.0f));
	LADSPA_Data wetlevel = 0.333333f * db2lin(LIMIT(*(ptr->wetlevel),-90.0f,20.0f));
	LADSPA_Data buflen = ptr->buflen / 2.0f;
	LADSPA_Data semitone = LIMIT(*(ptr->semitone),-12.0f,12.0f);
	LADSPA_Data rate; 
	LADSPA_Data r;
	LADSPA_Data depth;

	unsigned long sample_index;
	unsigned long sample_count = SampleCount;
	
	LADSPA_Data in = 0.0f;
	LADSPA_Data sign = 1.0f;
	LADSPA_Data phase_0 = 0.0f;
	LADSPA_Data phase_am_0 = 0.0f;
	LADSPA_Data phase_1 = 0.0f;
	LADSPA_Data phase_am_1 = 0.0f;
	LADSPA_Data phase_2 = 0.0f;
	LADSPA_Data phase_am_2 = 0.0f;
	LADSPA_Data fpos_0 = 0.0f, fpos_1 = 0.0f, fpos_2 = 0.0f;
	LADSPA_Data n_0 = 0.0f, n_1 = 0.0f, n_2 = 0.0f;
	LADSPA_Data rem_0 = 0.0f, rem_1 = 0.0f, rem_2 = 0.0f;
	LADSPA_Data sa_0, sb_0, sa_1, sb_1, sa_2, sb_2;


	if (semitone == 0.0f)
		rate = LIMIT(*(ptr->rate),-50.0f,100.0f);
	else
		rate = 100.0f * (powf(ROOT_12_2,semitone) - 1.0f);
	
	r = -1.0f * ABS(rate);
	depth = buflen * LIMIT(ABS(r) / 100.0f, 0.0f, 1.0f);
	

	if (rate > 0.0f)
		sign = -1.0f;

	for (sample_index = 0; sample_index < sample_count; sample_index++) {

		in = *(input++);

		phase_0 = COS_TABLE_SIZE * PM_FREQ * sample_index / ptr->sample_rate + ptr->phase;
		while (phase_0 >= COS_TABLE_SIZE)
		        phase_0 -= COS_TABLE_SIZE;
		phase_am_0 = phase_0 + COS_TABLE_SIZE/2;
		while (phase_am_0 >= COS_TABLE_SIZE)
			phase_am_0 -= COS_TABLE_SIZE;

		phase_1 = phase_0 + COS_TABLE_SIZE/3.0f;
		while (phase_1 >= COS_TABLE_SIZE)
		        phase_1 -= COS_TABLE_SIZE;
		phase_am_1 = phase_1 + COS_TABLE_SIZE/2;
		while (phase_am_1 >= COS_TABLE_SIZE)
			phase_am_1 -= COS_TABLE_SIZE;

		phase_2 = phase_0 + 2.0f*COS_TABLE_SIZE/3.0f;
		while (phase_2 >= COS_TABLE_SIZE)
		        phase_2 -= COS_TABLE_SIZE;
		phase_am_2 = phase_2 + COS_TABLE_SIZE/2;
		while (phase_am_2 >= COS_TABLE_SIZE)
			phase_am_2 -= COS_TABLE_SIZE;

		push_buffer(in, ptr->ringbuffer, ptr->buflen, &(ptr->pos));

		fpos_0 = depth * (1.0f - sign * (2.0f * phase_0 / COS_TABLE_SIZE - 1.0f));
		n_0 = floorf(fpos_0);
		rem_0 = fpos_0 - n_0;

		fpos_1 = depth * (1.0f - sign * (2.0f * phase_1 / COS_TABLE_SIZE - 1.0f));
		n_1 = floorf(fpos_1);
		rem_1 = fpos_1 - n_1;

		fpos_2 = depth * (1.0f - sign * (2.0f * phase_2 / COS_TABLE_SIZE - 1.0f));
		n_2 = floorf(fpos_2);
		rem_2 = fpos_2 - n_2;

		sa_0 = read_buffer(ptr->ringbuffer, ptr->buflen, ptr->pos, (unsigned long) n_0);
		sb_0 = read_buffer(ptr->ringbuffer, ptr->buflen, ptr->pos, (unsigned long) n_0 + 1);

		sa_1 = read_buffer(ptr->ringbuffer, ptr->buflen, ptr->pos, (unsigned long) n_1);
		sb_1 = read_buffer(ptr->ringbuffer, ptr->buflen, ptr->pos, (unsigned long) n_1 + 1);

		sa_2 = read_buffer(ptr->ringbuffer, ptr->buflen, ptr->pos, (unsigned long) n_2);
		sb_2 = read_buffer(ptr->ringbuffer, ptr->buflen, ptr->pos, (unsigned long) n_2 + 1);

		*(output++) = 
			wetlevel *
			((1.0f + cos_table[(unsigned long) phase_am_0]) *
			 ((1 - rem_0) * sa_0 + rem_0 * sb_0) +
			 (1.0f + cos_table[(unsigned long) phase_am_1]) *
			 ((1 - rem_1) * sa_1 + rem_1 * sb_1) +
			 (1.0f + cos_table[(unsigned long) phase_am_2]) *
			 ((1 - rem_2) * sa_2 + rem_2 * sb_2)) +
			drylevel *
			read_buffer(ptr->ringbuffer, ptr->buflen, ptr->pos, (unsigned long) depth);

	}

	ptr->phase += COS_TABLE_SIZE * PM_FREQ * sample_index / ptr->sample_rate;
	while (ptr->phase >= COS_TABLE_SIZE)
		ptr->phase -= COS_TABLE_SIZE;

	*(ptr->latency) = buflen - (unsigned long) depth;
}



void
set_run_adding_gain_Pitch(LADSPA_Handle Instance, LADSPA_Data gain) {

	Pitch * ptr = (Pitch *)Instance;

	ptr->run_adding_gain = gain;
}



void 
run_adding_Pitch(LADSPA_Handle Instance,
                 unsigned long SampleCount) {
  
	Pitch * ptr = (Pitch *)Instance;
	LADSPA_Data * input = ptr->input;
	LADSPA_Data * output = ptr->output;
	LADSPA_Data drylevel = db2lin(LIMIT(*(ptr->drylevel),-90.0f,20.0f));
	LADSPA_Data wetlevel = 0.333333f * db2lin(LIMIT(*(ptr->wetlevel),-90.0f,20.0f));
	LADSPA_Data buflen = ptr->buflen / 2.0f;
	LADSPA_Data semitone = LIMIT(*(ptr->semitone),-12.0f,12.0f);
	LADSPA_Data rate; 
	LADSPA_Data r;
	LADSPA_Data depth;

	unsigned long sample_index;
	unsigned long sample_count = SampleCount;
	
	LADSPA_Data in = 0.0f;
	LADSPA_Data sign = 1.0f;
	LADSPA_Data phase_0 = 0.0f;
	LADSPA_Data phase_am_0 = 0.0f;
	LADSPA_Data phase_1 = 0.0f;
	LADSPA_Data phase_am_1 = 0.0f;
	LADSPA_Data phase_2 = 0.0f;
	LADSPA_Data phase_am_2 = 0.0f;
	LADSPA_Data fpos_0 = 0.0f, fpos_1 = 0.0f, fpos_2 = 0.0f;
	LADSPA_Data n_0 = 0.0f, n_1 = 0.0f, n_2 = 0.0f;
	LADSPA_Data rem_0 = 0.0f, rem_1 = 0.0f, rem_2 = 0.0f;
	LADSPA_Data sa_0, sb_0, sa_1, sb_1, sa_2, sb_2;


	if (semitone == 0.0f)
		rate = LIMIT(*(ptr->rate),-50.0f,100.0f);
	else
		rate = 100.0f * (powf(ROOT_12_2,semitone) - 1.0f);
	
	r = -1.0f * ABS(rate);
	depth = buflen * LIMIT(ABS(r) / 100.0f, 0.0f, 1.0f);
	

	if (rate > 0.0f)
		sign = -1.0f;

	for (sample_index = 0; sample_index < sample_count; sample_index++) {

		in = *(input++);

		phase_0 = COS_TABLE_SIZE * PM_FREQ * sample_index / ptr->sample_rate + ptr->phase;
		while (phase_0 >= COS_TABLE_SIZE)
		        phase_0 -= COS_TABLE_SIZE;
		phase_am_0 = phase_0 + COS_TABLE_SIZE/2;
		while (phase_am_0 >= COS_TABLE_SIZE)
			phase_am_0 -= COS_TABLE_SIZE;

		phase_1 = phase_0 + COS_TABLE_SIZE/3.0f;
		while (phase_1 >= COS_TABLE_SIZE)
		        phase_1 -= COS_TABLE_SIZE;
		phase_am_1 = phase_1 + COS_TABLE_SIZE/2;
		while (phase_am_1 >= COS_TABLE_SIZE)
			phase_am_1 -= COS_TABLE_SIZE;

		phase_2 = phase_0 + 2.0f*COS_TABLE_SIZE/3.0f;
		while (phase_2 >= COS_TABLE_SIZE)
		        phase_2 -= COS_TABLE_SIZE;
		phase_am_2 = phase_2 + COS_TABLE_SIZE/2;
		while (phase_am_2 >= COS_TABLE_SIZE)
			phase_am_2 -= COS_TABLE_SIZE;

		push_buffer(in, ptr->ringbuffer, ptr->buflen, &(ptr->pos));

		fpos_0 = depth * (1.0f - sign * (2.0f * phase_0 / COS_TABLE_SIZE - 1.0f));
		n_0 = floorf(fpos_0);
		rem_0 = fpos_0 - n_0;

		fpos_1 = depth * (1.0f - sign * (2.0f * phase_1 / COS_TABLE_SIZE - 1.0f));
		n_1 = floorf(fpos_1);
		rem_1 = fpos_1 - n_1;

		fpos_2 = depth * (1.0f - sign * (2.0f * phase_2 / COS_TABLE_SIZE - 1.0f));
		n_2 = floorf(fpos_2);
		rem_2 = fpos_2 - n_2;

		sa_0 = read_buffer(ptr->ringbuffer, ptr->buflen, ptr->pos, (unsigned long) n_0);
		sb_0 = read_buffer(ptr->ringbuffer, ptr->buflen, ptr->pos, (unsigned long) n_0 + 1);

		sa_1 = read_buffer(ptr->ringbuffer, ptr->buflen, ptr->pos, (unsigned long) n_1);
		sb_1 = read_buffer(ptr->ringbuffer, ptr->buflen, ptr->pos, (unsigned long) n_1 + 1);

		sa_2 = read_buffer(ptr->ringbuffer, ptr->buflen, ptr->pos, (unsigned long) n_2);
		sb_2 = read_buffer(ptr->ringbuffer, ptr->buflen, ptr->pos, (unsigned long) n_2 + 1);

		*(output++) += ptr->run_adding_gain *
			wetlevel *
			((1.0f + cos_table[(unsigned long) phase_am_0]) *
			 ((1 - rem_0) * sa_0 + rem_0 * sb_0) +
			 (1.0f + cos_table[(unsigned long) phase_am_1]) *
			 ((1 - rem_1) * sa_1 + rem_1 * sb_1) +
			 (1.0f + cos_table[(unsigned long) phase_am_2]) *
			 ((1 - rem_2) * sa_2 + rem_2 * sb_2)) +
			drylevel *
			read_buffer(ptr->ringbuffer, ptr->buflen, ptr->pos, (unsigned long) depth);

	}

	ptr->phase += COS_TABLE_SIZE * PM_FREQ * sample_index / ptr->sample_rate;
	while (ptr->phase >= COS_TABLE_SIZE)
		ptr->phase -= COS_TABLE_SIZE;

	*(ptr->latency) = buflen - (unsigned long) depth;
}



/* Throw away a Pitch effect instance. */
void 
cleanup_Pitch(LADSPA_Handle Instance) {

  	Pitch * ptr = (Pitch *)Instance;
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
	mono_descriptor->Label = strdup("tap_pitch");
	mono_descriptor->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
	mono_descriptor->Name = strdup("TAP Pitch Shifter");
	mono_descriptor->Maker = strdup("Tom Szilagyi");
	mono_descriptor->Copyright = strdup("GPL");
	mono_descriptor->PortCount = PORTCOUNT_MONO;

	if ((port_descriptors =
	     (LADSPA_PortDescriptor *)calloc(PORTCOUNT_MONO, sizeof(LADSPA_PortDescriptor))) == NULL)
		exit(1);

	mono_descriptor->PortDescriptors = (const LADSPA_PortDescriptor *)port_descriptors;
	port_descriptors[RATE] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[SEMITONE] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[DRYLEVEL] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[WETLEVEL] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[LATENCY] = LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL;
	port_descriptors[INPUT] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
	port_descriptors[OUTPUT] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;

	if ((port_names = 
	     (char **)calloc(PORTCOUNT_MONO, sizeof(char *))) == NULL)
		exit(1);

	mono_descriptor->PortNames = (const char **)port_names;
	port_names[SEMITONE] = strdup("Semitone Shift");
	port_names[RATE] = strdup("Rate Shift [%]");
	port_names[DRYLEVEL] = strdup("Dry Level [dB]");
	port_names[WETLEVEL] = strdup("Wet Level [dB]");
	port_names[LATENCY] = strdup("latency");
	port_names[INPUT] = strdup("Input");
	port_names[OUTPUT] = strdup("Output");

	if ((port_range_hints = 
	     ((LADSPA_PortRangeHint *)calloc(PORTCOUNT_MONO, sizeof(LADSPA_PortRangeHint)))) == NULL)
		exit(1);

	mono_descriptor->PortRangeHints	= (const LADSPA_PortRangeHint *)port_range_hints;
	port_range_hints[RATE].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[SEMITONE].HintDescriptor = 
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
	port_range_hints[RATE].LowerBound = -50.0f;
	port_range_hints[RATE].UpperBound = 100.0f;
	port_range_hints[SEMITONE].LowerBound = -12.0f;
	port_range_hints[SEMITONE].UpperBound = 12.0f;
	port_range_hints[DRYLEVEL].LowerBound = -90.0f;
	port_range_hints[DRYLEVEL].UpperBound = 20.0f;
	port_range_hints[WETLEVEL].LowerBound = -90.0f;
	port_range_hints[WETLEVEL].UpperBound = 20.0f;
	port_range_hints[LATENCY].LowerBound = 0;
	port_range_hints[LATENCY].UpperBound = PM_BUFLEN;
	port_range_hints[INPUT].HintDescriptor = 0;
	port_range_hints[OUTPUT].HintDescriptor = 0;
	mono_descriptor->instantiate = instantiate_Pitch;
	mono_descriptor->connect_port = connect_port_Pitch;
	mono_descriptor->activate = activate_Pitch;
	mono_descriptor->run = run_Pitch;
	mono_descriptor->run_adding = run_adding_Pitch;
	mono_descriptor->set_run_adding_gain = set_run_adding_gain_Pitch;
	mono_descriptor->deactivate = NULL;
	mono_descriptor->cleanup = cleanup_Pitch;
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
