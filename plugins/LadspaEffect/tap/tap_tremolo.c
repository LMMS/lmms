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

    $Id: tap_tremolo.c,v 1.6 2004/02/21 17:33:36 tszilagyi Exp $
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <ladspa.h>
#include "tap_utils.h"

/* The Unique ID of the plugin: */

#define ID_MONO         2144

/* The port numbers for the plugin: */

#define CONTROL_FREQ    0
#define CONTROL_DEPTH   1
#define CONTROL_GAIN    2
#define INPUT_0         3
#define OUTPUT_0        4


/* Total number of ports */

#define PORTCOUNT_MONO   5


/* cosine table for fast computations */
LADSPA_Data cos_table[1024];


/* The structure used to hold port connection information and state */

typedef struct {
	LADSPA_Data * Control_Freq;
	LADSPA_Data * Control_Depth;
	LADSPA_Data * Control_Gain;
	LADSPA_Data * InputBuffer_1;
	LADSPA_Data * OutputBuffer_1;
	unsigned long SampleRate;
	LADSPA_Data Phase;
	LADSPA_Data run_adding_gain;
} Tremolo;



/* Construct a new plugin instance. */
LADSPA_Handle 
instantiate_Tremolo(const LADSPA_Descriptor * Descriptor,
		    unsigned long             SampleRate) {
	
	LADSPA_Handle * ptr;
	
	if ((ptr = malloc(sizeof(Tremolo))) != NULL) {
	        ((Tremolo *)ptr)->SampleRate = SampleRate;
	        ((Tremolo *)ptr)->run_adding_gain = 1.0;
		return ptr;
	}
	
	return NULL;
}

void
activate_Tremolo(LADSPA_Handle Instance) {

	Tremolo * ptr;

	ptr = (Tremolo *)Instance;
	ptr->Phase = 0.0f;
}



/* Connect a port to a data location. */
void 
connect_port_Tremolo(LADSPA_Handle Instance,
		     unsigned long Port,
		     LADSPA_Data * DataLocation) {
	
	Tremolo * ptr;
	
	ptr = (Tremolo *)Instance;
	switch (Port) {
	case CONTROL_FREQ:
		ptr->Control_Freq = DataLocation;
		break;
	case CONTROL_DEPTH:
		ptr->Control_Depth = DataLocation;
		break;
	case CONTROL_GAIN:
		ptr->Control_Gain = DataLocation;
		break;
	case INPUT_0:
		ptr->InputBuffer_1 = DataLocation;
		break;
	case OUTPUT_0:
		ptr->OutputBuffer_1 = DataLocation;
		break;
	}
}



void 
run_Tremolo(LADSPA_Handle Instance,
	    unsigned long SampleCount) {
  
	LADSPA_Data * input;
	LADSPA_Data * output;
	LADSPA_Data freq;
	LADSPA_Data depth;
	LADSPA_Data gain;
	Tremolo * ptr;
	unsigned long sample_index;
	LADSPA_Data phase = 0.0f;
	
	ptr = (Tremolo *)Instance;
	
	input = ptr->InputBuffer_1;
	output = ptr->OutputBuffer_1;
	freq = LIMIT(*(ptr->Control_Freq),0.0f,20.0f);
	depth = LIMIT(*(ptr->Control_Depth),0.0f,100.0f);
	gain = db2lin(LIMIT(*(ptr->Control_Gain),-70.0f,20.0f));

  	for (sample_index = 0; sample_index < SampleCount; sample_index++) {
		phase = 1024.0f * freq * sample_index / ptr->SampleRate + ptr->Phase;

		while (phase >= 1024.0f)
			phase -= 1024.0f;

		*(output++) = *(input++) * gain *
			(1 - 0.5*depth/100 + 0.5 * depth/100 * cos_table[(unsigned long) phase]);
	}
	ptr->Phase = phase;
	while (ptr->Phase >= 1024.0f)
		ptr->Phase -= 1024.0f;
}



void
set_run_adding_gain_Tremolo(LADSPA_Handle Instance, LADSPA_Data gain) {

	Tremolo * ptr;

	ptr = (Tremolo *)Instance;

	ptr->run_adding_gain = gain;
}



void 
run_adding_Tremolo(LADSPA_Handle Instance,
		   unsigned long SampleCount) {
  
	LADSPA_Data * input;
	LADSPA_Data * output;
	LADSPA_Data freq;
	LADSPA_Data depth;
	LADSPA_Data gain;
	Tremolo * ptr;
	unsigned long sample_index;
	LADSPA_Data phase = 0.0f;
	
	ptr = (Tremolo *)Instance;
	
	input = ptr->InputBuffer_1;
	output = ptr->OutputBuffer_1;
	freq = LIMIT(*(ptr->Control_Freq),0.0f,20.0f);
	depth = LIMIT(*(ptr->Control_Depth),0.0f,100.0f);
	gain = db2lin(LIMIT(*(ptr->Control_Gain),-70.0f,20.0f));

  	for (sample_index = 0; sample_index < SampleCount; sample_index++) {
		phase = 1024.0f * freq * sample_index / ptr->SampleRate + ptr->Phase;

		while (phase >= 1024.0f)
			phase -= 1024.0f;

		*(output++) += *(input++) * ptr->run_adding_gain * gain *
			(1 - 0.5*depth/100 + 0.5 * depth/100 * cos_table[(unsigned long) phase]);
	}
	ptr->Phase = phase;
	while (ptr->Phase >= 1024.0f)
		ptr->Phase -= 1024.0f;
}




/* Throw away a Tremolo effect instance. */
void 
cleanup_Tremolo(LADSPA_Handle Instance) {
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
	int i;
	
	if ((mono_descriptor = 
	     (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor))) == NULL)
		exit(1);
	

	for (i = 0; i < 1024; i++)
		cos_table[i] = cosf(i * M_PI / 512.0f);


	mono_descriptor->UniqueID = ID_MONO;
	mono_descriptor->Label = strdup("tap_tremolo");
	mono_descriptor->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
	mono_descriptor->Name = strdup("TAP Tremolo");
	mono_descriptor->Maker = strdup("Tom Szilagyi");
	mono_descriptor->Copyright = strdup("GPL");
	mono_descriptor->PortCount = PORTCOUNT_MONO;

	if ((port_descriptors =
	     (LADSPA_PortDescriptor *)calloc(PORTCOUNT_MONO, sizeof(LADSPA_PortDescriptor))) == NULL)
		exit(1);

	mono_descriptor->PortDescriptors = (const LADSPA_PortDescriptor *)port_descriptors;
	port_descriptors[CONTROL_FREQ] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[CONTROL_DEPTH] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[CONTROL_GAIN] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[INPUT_0] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
	port_descriptors[OUTPUT_0] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;

	if ((port_names = 
	     (char **)calloc(PORTCOUNT_MONO, sizeof(char *))) == NULL)
		exit(1);

	mono_descriptor->PortNames = (const char **)port_names;
	port_names[CONTROL_FREQ] = strdup("Frequency [Hz]");
	port_names[CONTROL_DEPTH] = strdup("Depth [%]");
	port_names[CONTROL_GAIN] = strdup("Gain [dB]");
	port_names[INPUT_0] = strdup("Input_0");
	port_names[OUTPUT_0] = strdup("Output_0");

	if ((port_range_hints = 
	     ((LADSPA_PortRangeHint *)calloc(PORTCOUNT_MONO, sizeof(LADSPA_PortRangeHint)))) == NULL)
		exit(1);

	mono_descriptor->PortRangeHints	= (const LADSPA_PortRangeHint *)port_range_hints;
	port_range_hints[CONTROL_FREQ].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[CONTROL_DEPTH].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[CONTROL_GAIN].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[CONTROL_FREQ].LowerBound = 0;
	port_range_hints[CONTROL_FREQ].UpperBound = 20;
	port_range_hints[CONTROL_DEPTH].LowerBound = 0;
	port_range_hints[CONTROL_DEPTH].UpperBound = 100;
	port_range_hints[CONTROL_GAIN].LowerBound = -70;
	port_range_hints[CONTROL_GAIN].UpperBound = 20;
	port_range_hints[INPUT_0].HintDescriptor = 0;
	port_range_hints[OUTPUT_0].HintDescriptor = 0;
	mono_descriptor->instantiate = instantiate_Tremolo;
	mono_descriptor->connect_port = connect_port_Tremolo;
	mono_descriptor->activate = activate_Tremolo;
	mono_descriptor->run = run_Tremolo;
	mono_descriptor->run_adding = run_adding_Tremolo;
	mono_descriptor->set_run_adding_gain = set_run_adding_gain_Tremolo;
	mono_descriptor->deactivate = NULL;
	mono_descriptor->cleanup = cleanup_Tremolo;
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
