/*                                                     -*- linux-c -*-
    Copyright (C) 2004 Tom Szilagyi
    
    Patches were received from:
        Alexander Koenig <alex@lisas.de>

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

    $Id: tap_autopan.c,v 1.6 2004/02/21 17:33:36 tszilagyi Exp $
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <ladspa.h>
#include "tap_utils.h"

/* The Unique ID of the plugin: */

#define ID_STEREO         2146

/* The port numbers for the plugin: */

#define CONTROL_FREQ    0
#define CONTROL_DEPTH   1
#define CONTROL_GAIN    2
#define INPUT_L         3
#define INPUT_R         4
#define OUTPUT_L        5
#define OUTPUT_R        6


/* Total number of ports */

#define PORTCOUNT_STEREO   7


/* cosine table for fast computations */
LADSPA_Data cos_table[1024];


/* The structure used to hold port connection information and state */

typedef struct {
	LADSPA_Data * freq;
	LADSPA_Data * depth;
	LADSPA_Data * gain;
	LADSPA_Data * input_L;
	LADSPA_Data * input_R;
	LADSPA_Data * output_L;
	LADSPA_Data * output_R;
	unsigned long SampleRate;
	LADSPA_Data Phase;
	LADSPA_Data run_adding_gain;
} AutoPan;



/* Construct a new plugin instance. */
LADSPA_Handle 
instantiate_AutoPan(const LADSPA_Descriptor * Descriptor,
		    unsigned long             SampleRate) {
	
	LADSPA_Handle * ptr;
	
	if ((ptr = malloc(sizeof(AutoPan))) != NULL) {
		((AutoPan *)ptr)->SampleRate = SampleRate;
		((AutoPan *)ptr)->run_adding_gain = 1.0;
		return ptr;
	}
	
	return NULL;
}

void
activate_AutoPan(LADSPA_Handle Instance) {

	AutoPan * ptr;

	ptr = (AutoPan *)Instance;
	ptr->Phase = 0.0f;
}



/* Connect a port to a data location. */
void 
connect_port_AutoPan(LADSPA_Handle Instance,
		     unsigned long Port,
		     LADSPA_Data * DataLocation) {
	
	AutoPan * ptr;
	
	ptr = (AutoPan *)Instance;
	switch (Port) {
	case CONTROL_FREQ:
		ptr->freq = DataLocation;
		break;
	case CONTROL_DEPTH:
		ptr->depth = DataLocation;
		break;
	case CONTROL_GAIN:
		ptr->gain = DataLocation;
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
run_AutoPan(LADSPA_Handle Instance,
	    unsigned long SampleCount) {
  
	AutoPan * ptr = (AutoPan *)Instance;

	LADSPA_Data * input_L = ptr->input_L;
	LADSPA_Data * input_R = ptr->input_R;
	LADSPA_Data * output_L = ptr->output_L;
	LADSPA_Data * output_R = ptr->output_R;
	LADSPA_Data freq = LIMIT(*(ptr->freq),0.0f,20.0f);
	LADSPA_Data depth = LIMIT(*(ptr->depth),0.0f,100.0f);
	LADSPA_Data gain = db2lin(LIMIT(*(ptr->gain),-70.0f,20.0f));
	unsigned long sample_index;
	LADSPA_Data phase_L = 0;
	LADSPA_Data phase_R = 0;
	
	for (sample_index = 0; sample_index < SampleCount; sample_index++) {
		phase_L = 1024.0f * freq * sample_index / ptr->SampleRate + ptr->Phase;
		while (phase_L >= 1024.0f)
		        phase_L -= 1024.0f;  
 		phase_R = phase_L + 512.0f;
		while (phase_R >= 1024.0f)
		        phase_R -= 1024.0f;  

		*(output_L++) = *(input_L++) * gain *
			(1 - 0.5*depth/100 + 0.5 * depth/100 * cos_table[(unsigned long) phase_L]);
		*(output_R++) = *(input_R++) * gain *
			(1 - 0.5*depth/100 + 0.5 * depth/100 * cos_table[(unsigned long) phase_R]);
	}
	ptr->Phase = phase_L;
	while (ptr->Phase >= 1024.0f)
		ptr->Phase -= 1024.0f;
}



void
set_run_adding_gain_AutoPan(LADSPA_Handle Instance, LADSPA_Data gain) {

	AutoPan * ptr;

	ptr = (AutoPan *)Instance;

	ptr->run_adding_gain = gain;
}



void 
run_adding_AutoPan(LADSPA_Handle Instance,
		   unsigned long SampleCount) {
  
	AutoPan * ptr = (AutoPan *)Instance;

	LADSPA_Data * input_L = ptr->input_L;
	LADSPA_Data * input_R = ptr->input_R;
	LADSPA_Data * output_L = ptr->output_L;
	LADSPA_Data * output_R = ptr->output_R;
	LADSPA_Data freq = LIMIT(*(ptr->freq),0.0f,20.0f);
	LADSPA_Data depth = LIMIT(*(ptr->depth),0.0f,100.0f);
	LADSPA_Data gain = db2lin(LIMIT(*(ptr->gain),-70.0f,20.0f));
	unsigned long sample_index;
	LADSPA_Data phase_L = 0;
	LADSPA_Data phase_R = 0;
	
	for (sample_index = 0; sample_index < SampleCount; sample_index++) {
		phase_L = 1024.0f * freq * sample_index / ptr->SampleRate + ptr->Phase;
		while (phase_L >= 1024.0f)
		        phase_L -= 1024.0f;  
 		phase_R = phase_L + 512.0f;
		while (phase_R >= 1024.0f)
		        phase_R -= 1024.0f;  

		*(output_L++) += *(input_L++) * gain * ptr->run_adding_gain *
			(1 - 0.5*depth/100 + 0.5 * depth/100 * cos_table[(unsigned long) phase_L]);
		*(output_R++) += *(input_R++) * gain * ptr->run_adding_gain *
			(1 - 0.5*depth/100 + 0.5 * depth/100 * cos_table[(unsigned long) phase_R]);
	}
	ptr->Phase = phase_L;
	while (ptr->Phase >= 1024.0f)
		ptr->Phase -= 1024.0f;
}




/* Throw away an AutoPan effect instance. */
void 
cleanup_AutoPan(LADSPA_Handle Instance) {
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
	
	for (i = 0; i < 1024; i++)
		cos_table[i] = cosf(i * M_PI / 512.0f);


	mono_descriptor->UniqueID = ID_STEREO;
	mono_descriptor->Label = strdup("tap_autopan");
	mono_descriptor->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
	mono_descriptor->Name = strdup("TAP AutoPanner");
	mono_descriptor->Maker = strdup("Tom Szilagyi");
	mono_descriptor->Copyright = strdup("GPL");
	mono_descriptor->PortCount = PORTCOUNT_STEREO;

	if ((port_descriptors =
	     (LADSPA_PortDescriptor *)calloc(PORTCOUNT_STEREO, sizeof(LADSPA_PortDescriptor))) == NULL)
		exit(1);

	mono_descriptor->PortDescriptors = (const LADSPA_PortDescriptor *)port_descriptors;
	port_descriptors[CONTROL_FREQ] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[CONTROL_DEPTH] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[CONTROL_GAIN] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[INPUT_L] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
	port_descriptors[INPUT_R] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
	port_descriptors[OUTPUT_L] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
	port_descriptors[OUTPUT_R] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;

	if ((port_names = 
	     (char **)calloc(PORTCOUNT_STEREO, sizeof(char *))) == NULL)
		exit(1);

	mono_descriptor->PortNames = (const char **)port_names;
	port_names[CONTROL_FREQ] = strdup("Frequency [Hz]");
	port_names[CONTROL_DEPTH] = strdup("Depth [%]");
	port_names[CONTROL_GAIN] = strdup("Gain [dB]");
	port_names[INPUT_L] = strdup("Input L");
	port_names[INPUT_R] = strdup("Input R");
	port_names[OUTPUT_L] = strdup("Output L");
	port_names[OUTPUT_R] = strdup("Output R");

	if ((port_range_hints = 
	     ((LADSPA_PortRangeHint *)calloc(PORTCOUNT_STEREO, sizeof(LADSPA_PortRangeHint)))) == NULL)
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
	port_range_hints[INPUT_L].HintDescriptor = 0;
	port_range_hints[INPUT_R].HintDescriptor = 0;
	port_range_hints[OUTPUT_L].HintDescriptor = 0;
	port_range_hints[OUTPUT_R].HintDescriptor = 0;
	mono_descriptor->instantiate = instantiate_AutoPan;
	mono_descriptor->connect_port = connect_port_AutoPan;
	mono_descriptor->activate = activate_AutoPan;
	mono_descriptor->run = run_AutoPan;
	mono_descriptor->run_adding = run_adding_AutoPan;
	mono_descriptor->set_run_adding_gain = set_run_adding_gain_AutoPan;
	mono_descriptor->deactivate = NULL;
	mono_descriptor->cleanup = cleanup_AutoPan;
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
