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

    $Id: tap_reflector.c,v 1.1 2004/06/18 20:12:41 tszilagyi Exp $
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <ladspa.h>
#include "tap_utils.h"


/* The Unique ID of the plugin: */

#define ID_MONO         2154

/* The port numbers for the plugin: */

#define FRAGMENT        0
#define DRYLEVEL        1
#define WETLEVEL        2
#define INPUT           3
#define OUTPUT          4

/* Total number of ports */


#define PORTCOUNT_MONO   5


/* minimum & maximum fragment length [ms] */
#define MIN_FRAGMENT_LEN 20
#define MAX_FRAGMENT_LEN 1600

/* in kHz */
#define MAX_SAMPLE_RATE 192


#define COS_TABLE_SIZE 1024
LADSPA_Data cos_table[COS_TABLE_SIZE];



/* The structure used to hold port connection information and state */

typedef struct {
	LADSPA_Data * fragment;
	LADSPA_Data * drylevel;
	LADSPA_Data * wetlevel;
	LADSPA_Data * input;
	LADSPA_Data * output;

	LADSPA_Data * ring0;
	unsigned long buflen0;
	unsigned long pos0;
	LADSPA_Data * ring1;
	unsigned long buflen1;
	unsigned long pos1;
	LADSPA_Data * delay1;
	unsigned long delay_buflen1;
	unsigned long delay_pos1;
	LADSPA_Data * ring2;
	unsigned long buflen2;
	unsigned long pos2;
	LADSPA_Data * delay2;
	unsigned long delay_buflen2;
	unsigned long delay_pos2;

	unsigned long fragment_pos;

	unsigned long sample_rate;
	LADSPA_Data run_adding_gain;
} Reflector;



/* Construct a new plugin instance. */
LADSPA_Handle 
instantiate_Reflector(const LADSPA_Descriptor * Descriptor,
		      unsigned long             sample_rate) {
  
        LADSPA_Handle * ptr;
	
	if ((ptr = malloc(sizeof(Reflector))) != NULL) {
		((Reflector *)ptr)->sample_rate = sample_rate;
		((Reflector *)ptr)->run_adding_gain = 1.0f;

		if ((((Reflector *)ptr)->ring0 = 
		     calloc(2 * MAX_FRAGMENT_LEN * MAX_SAMPLE_RATE, sizeof(LADSPA_Data))) == NULL)
			return NULL;
		((Reflector *)ptr)->buflen0 = 2 * MAX_FRAGMENT_LEN * sample_rate / 1000;
		((Reflector *)ptr)->pos0 = 0;

		if ((((Reflector *)ptr)->ring1 = 
		     calloc(2 * MAX_FRAGMENT_LEN * MAX_SAMPLE_RATE, sizeof(LADSPA_Data))) == NULL)
			return NULL;
		((Reflector *)ptr)->buflen1 = 2 * MAX_FRAGMENT_LEN * sample_rate / 1000;
		((Reflector *)ptr)->pos1 = 0;

		if ((((Reflector *)ptr)->delay1 = 
		     calloc(2 * MAX_FRAGMENT_LEN * MAX_SAMPLE_RATE, sizeof(LADSPA_Data))) == NULL)
			return NULL;
		((Reflector *)ptr)->delay_buflen1 = 2 * MAX_FRAGMENT_LEN * sample_rate / 3000;
		((Reflector *)ptr)->pos1 = 0;

		if ((((Reflector *)ptr)->ring2 = 
		     calloc(2 * MAX_FRAGMENT_LEN * MAX_SAMPLE_RATE, sizeof(LADSPA_Data))) == NULL)
			return NULL;
		((Reflector *)ptr)->buflen2 = 2 * MAX_FRAGMENT_LEN * sample_rate / 1000;
		((Reflector *)ptr)->pos2 = 0;

		if ((((Reflector *)ptr)->delay2 = 
		     calloc(2 * MAX_FRAGMENT_LEN * MAX_SAMPLE_RATE, sizeof(LADSPA_Data))) == NULL)
			return NULL;
		((Reflector *)ptr)->delay_buflen2 = 4 * MAX_FRAGMENT_LEN * sample_rate / 3000;
		((Reflector *)ptr)->pos2 = 0;

		return ptr;
	}
       	return NULL;
}


void
activate_Reflector(LADSPA_Handle Instance) {

	Reflector * ptr = (Reflector *)Instance;
	unsigned long i;

	for (i = 0; i < ptr->buflen0; i++)
		ptr->ring0[i] = 0.0f;
	ptr->pos0 = 0;
	for (i = 0; i < ptr->buflen1; i++)
		ptr->ring1[i] = 0.0f;
	ptr->pos1 = 0;
	for (i = 0; i < ptr->buflen2; i++)
		ptr->ring2[i] = 0.0f;
	ptr->pos2 = 0;

	for (i = 0; i < ptr->delay_buflen1; i++)
		ptr->delay1[i] = 0.0f;
	ptr->delay_pos1 = 0;
	for (i = 0; i < ptr->delay_buflen2; i++)
		ptr->delay2[i] = 0.0f;
	ptr->delay_pos2 = 0;

	ptr->fragment_pos = 0;
}





/* Connect a port to a data location. */
void 
connect_port_Reflector(LADSPA_Handle Instance,
		       unsigned long Port,
		       LADSPA_Data * DataLocation) {
	
	Reflector * ptr = (Reflector *)Instance;

	switch (Port) {
	case FRAGMENT:
		ptr->fragment = DataLocation;
		break;
	case DRYLEVEL:
		ptr->drylevel = DataLocation;
		break;
	case WETLEVEL:
		ptr->wetlevel = DataLocation;
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
run_Reflector(LADSPA_Handle Instance,
	      unsigned long SampleCount) {
  
	Reflector * ptr = (Reflector *)Instance;
	LADSPA_Data * input = ptr->input;
	LADSPA_Data * output = ptr->output;
	LADSPA_Data drylevel = db2lin(LIMIT(*(ptr->drylevel),-90.0f,20.0f));
	LADSPA_Data wetlevel = 0.333333f * db2lin(LIMIT(*(ptr->wetlevel),-90.0f,20.0f));
	LADSPA_Data fragment = LIMIT(*(ptr->fragment),(float)MIN_FRAGMENT_LEN,(float)MAX_FRAGMENT_LEN);

	unsigned long sample_index;
	unsigned long sample_count = SampleCount;

	LADSPA_Data in = 0.0f;
	LADSPA_Data in1 = 0.0f;
	LADSPA_Data in2 = 0.0f;
	LADSPA_Data out_0 = 0.0f;
	LADSPA_Data out_1 = 0.0f;
	LADSPA_Data out_2 = 0.0f;

	unsigned long fragment_pos1 = 0;
	unsigned long fragment_pos2 = 0;

	unsigned long arg_0 = 0;
	LADSPA_Data am_0 = 0.0f;
	unsigned long arg_1 = 0;
	LADSPA_Data am_1 = 0.0f;
	unsigned long arg_2 = 0;
	LADSPA_Data am_2 = 0.0f;

	ptr->buflen0 = 2 * fragment * ptr->sample_rate / 1000.0f;
	ptr->buflen1 = ptr->buflen0;
	ptr->buflen2 = ptr->buflen0;
	ptr->delay_buflen1 = ptr->buflen0 / 3;
	ptr->delay_buflen2 = 2 * ptr->buflen0 / 3;

	for (sample_index = 0; sample_index < sample_count; sample_index++) {

		in = *(input++);
		in1 = push_buffer(in, ptr->delay1, ptr->delay_buflen1, &(ptr->delay_pos1));
		in2 = push_buffer(in, ptr->delay2, ptr->delay_buflen2, &(ptr->delay_pos2));

		push_buffer(in2, ptr->ring0, ptr->buflen0, &(ptr->pos0));
		push_buffer(in1, ptr->ring1, ptr->buflen1, &(ptr->pos1));
		push_buffer(in, ptr->ring2, ptr->buflen2, &(ptr->pos2));

		fragment_pos1 = (ptr->fragment_pos + ptr->buflen0 / 3) % ptr->buflen0;
		fragment_pos2 = (ptr->fragment_pos + 2 * ptr->buflen1 / 3) % ptr->buflen1;

		out_0 = read_buffer(ptr->ring0, ptr->buflen0, ptr->pos0,
				    ptr->buflen0 - ptr->fragment_pos - 1);
		out_1 = read_buffer(ptr->ring1, ptr->buflen1, ptr->pos1,
				    ptr->buflen1 - fragment_pos1 - 1);
		out_2 = read_buffer(ptr->ring2, ptr->buflen2, ptr->pos2,
				    ptr->buflen2 - fragment_pos2 - 1);

		ptr->fragment_pos += 2;
		if (ptr->fragment_pos >= ptr->buflen0)
			ptr->fragment_pos = 0;

		arg_0 = (float)ptr->fragment_pos / (float)ptr->buflen0 * COS_TABLE_SIZE;
		am_0 = 1.0f - cos_table[arg_0];
		arg_1 = (float)fragment_pos1 / (float)ptr->buflen1 * COS_TABLE_SIZE;
		am_1 = 1.0f - cos_table[arg_1];
		arg_2 = (float)fragment_pos2 / (float)ptr->buflen2 * COS_TABLE_SIZE;
		am_2 = 1.0f - cos_table[arg_2];

		*(output++) = drylevel * in + wetlevel *
			(am_0 * out_0 + am_1 * out_1 + am_2 * out_2);
	}
}



void
set_run_adding_gain_Reflector(LADSPA_Handle Instance, LADSPA_Data gain) {

	Reflector * ptr = (Reflector *)Instance;

	ptr->run_adding_gain = gain;
}



void 
run_adding_Reflector(LADSPA_Handle Instance,
		     unsigned long SampleCount) {
  
	Reflector * ptr = (Reflector *)Instance;
	LADSPA_Data * input = ptr->input;
	LADSPA_Data * output = ptr->output;
	LADSPA_Data drylevel = db2lin(LIMIT(*(ptr->drylevel),-90.0f,20.0f));
	LADSPA_Data wetlevel = 0.333333f * db2lin(LIMIT(*(ptr->wetlevel),-90.0f,20.0f));
	LADSPA_Data fragment = LIMIT(*(ptr->fragment),(float)MIN_FRAGMENT_LEN,(float)MAX_FRAGMENT_LEN);

	unsigned long sample_index;
	unsigned long sample_count = SampleCount;

	LADSPA_Data in = 0.0f;
	LADSPA_Data in1 = 0.0f;
	LADSPA_Data in2 = 0.0f;
	LADSPA_Data out_0 = 0.0f;
	LADSPA_Data out_1 = 0.0f;
	LADSPA_Data out_2 = 0.0f;

	unsigned long fragment_pos1 = 0;
	unsigned long fragment_pos2 = 0;

	unsigned long arg_0 = 0;
	LADSPA_Data am_0 = 0.0f;
	unsigned long arg_1 = 0;
	LADSPA_Data am_1 = 0.0f;
	unsigned long arg_2 = 0;
	LADSPA_Data am_2 = 0.0f;

	ptr->buflen0 = 2 * fragment * ptr->sample_rate / 1000.0f;
	ptr->buflen1 = ptr->buflen0;
	ptr->buflen2 = ptr->buflen0;
	ptr->delay_buflen1 = ptr->buflen0 / 3;
	ptr->delay_buflen2 = 2 * ptr->buflen0 / 3;

	for (sample_index = 0; sample_index < sample_count; sample_index++) {

		in = *(input++);
		in1 = push_buffer(in, ptr->delay1, ptr->delay_buflen1, &(ptr->delay_pos1));
		in2 = push_buffer(in, ptr->delay2, ptr->delay_buflen2, &(ptr->delay_pos2));

		push_buffer(in2, ptr->ring0, ptr->buflen0, &(ptr->pos0));
		push_buffer(in1, ptr->ring1, ptr->buflen1, &(ptr->pos1));
		push_buffer(in, ptr->ring2, ptr->buflen2, &(ptr->pos2));

		fragment_pos1 = (ptr->fragment_pos + ptr->buflen0 / 3) % ptr->buflen0;
		fragment_pos2 = (ptr->fragment_pos + 2 * ptr->buflen1 / 3) % ptr->buflen1;

		out_0 = read_buffer(ptr->ring0, ptr->buflen0, ptr->pos0,
				    ptr->buflen0 - ptr->fragment_pos - 1);
		out_1 = read_buffer(ptr->ring1, ptr->buflen1, ptr->pos1,
				    ptr->buflen1 - fragment_pos1 - 1);
		out_2 = read_buffer(ptr->ring2, ptr->buflen2, ptr->pos2,
				    ptr->buflen2 - fragment_pos2 - 1);

		ptr->fragment_pos += 2;
		if (ptr->fragment_pos >= ptr->buflen0)
			ptr->fragment_pos = 0;

		arg_0 = (float)ptr->fragment_pos / (float)ptr->buflen0 * COS_TABLE_SIZE;
		am_0 = 1.0f - cos_table[arg_0];
		arg_1 = (float)fragment_pos1 / (float)ptr->buflen1 * COS_TABLE_SIZE;
		am_1 = 1.0f - cos_table[arg_1];
		arg_2 = (float)fragment_pos2 / (float)ptr->buflen2 * COS_TABLE_SIZE;
		am_2 = 1.0f - cos_table[arg_2];

		*(output++) += ptr->run_adding_gain *
			(drylevel * in + wetlevel * (am_0 * out_0 + am_1 * out_1 + am_2 * out_2));
	}
}




/* Throw away a Reflector effect instance. */
void 
cleanup_Reflector(LADSPA_Handle Instance) {

  	Reflector * ptr = (Reflector *)Instance;
	free(ptr->ring0);
	free(ptr->ring1);
	free(ptr->ring2);
	free(ptr->delay1);
	free(ptr->delay2);
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
	mono_descriptor->Label = strdup("tap_reflector");
	mono_descriptor->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
	mono_descriptor->Name = strdup("TAP Reflector");
	mono_descriptor->Maker = strdup("Tom Szilagyi");
	mono_descriptor->Copyright = strdup("GPL");
	mono_descriptor->PortCount = PORTCOUNT_MONO;

	if ((port_descriptors =
	     (LADSPA_PortDescriptor *)calloc(PORTCOUNT_MONO, sizeof(LADSPA_PortDescriptor))) == NULL)
		exit(1);

	mono_descriptor->PortDescriptors = (const LADSPA_PortDescriptor *)port_descriptors;
	port_descriptors[FRAGMENT] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[DRYLEVEL] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[WETLEVEL] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[INPUT] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
	port_descriptors[OUTPUT] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;

	if ((port_names = 
	     (char **)calloc(PORTCOUNT_MONO, sizeof(char *))) == NULL)
		exit(1);

	mono_descriptor->PortNames = (const char **)port_names;
	port_names[FRAGMENT] = strdup("Fragment Length [ms]");
	port_names[DRYLEVEL] = strdup("Dry Level [dB]");
	port_names[WETLEVEL] = strdup("Wet Level [dB]");
	port_names[INPUT] = strdup("Input");
	port_names[OUTPUT] = strdup("Output");

	if ((port_range_hints = 
	     ((LADSPA_PortRangeHint *)calloc(PORTCOUNT_MONO, sizeof(LADSPA_PortRangeHint)))) == NULL)
		exit(1);

	mono_descriptor->PortRangeHints	= (const LADSPA_PortRangeHint *)port_range_hints;
	port_range_hints[FRAGMENT].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_LOW);
	port_range_hints[DRYLEVEL].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_MINIMUM);
	port_range_hints[WETLEVEL].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[FRAGMENT].LowerBound = (float)MIN_FRAGMENT_LEN;
	port_range_hints[FRAGMENT].UpperBound = (float)MAX_FRAGMENT_LEN;
	port_range_hints[DRYLEVEL].LowerBound = -90.0f;
	port_range_hints[DRYLEVEL].UpperBound = 20.0f;
	port_range_hints[WETLEVEL].LowerBound = -90.0f;
	port_range_hints[WETLEVEL].UpperBound = 20.0f;
	port_range_hints[INPUT].HintDescriptor = 0;
	port_range_hints[OUTPUT].HintDescriptor = 0;
	mono_descriptor->instantiate = instantiate_Reflector;
	mono_descriptor->connect_port = connect_port_Reflector;
	mono_descriptor->activate = activate_Reflector;
	mono_descriptor->run = run_Reflector;
	mono_descriptor->run_adding = run_adding_Reflector;
	mono_descriptor->set_run_adding_gain = set_run_adding_gain_Reflector;
	mono_descriptor->deactivate = NULL;
	mono_descriptor->cleanup = cleanup_Reflector;
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
