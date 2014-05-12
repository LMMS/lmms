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

    $Id: tap_sigmoid.c,v 1.3 2005/08/30 11:19:14 tszilagyi Exp $
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <ladspa.h>
#include "tap_utils.h"


/* The Unique ID of the plugin: */

#define ID_MONO         2157

/* The port numbers for the plugin: */

#define PREGAIN         0
#define POSTGAIN        1
#define INPUT           2
#define OUTPUT          3

/* Total number of ports */
#define PORTCOUNT_MONO   4


/* The closer this is to 1.0, the slower the input parameter
   interpolation will be. */
#define INTERP 0.99f


/* The structure used to hold port connection information and state */

typedef struct {
	LADSPA_Data * pregain;
	LADSPA_Data * postgain;
	LADSPA_Data * input;
	LADSPA_Data * output;

	LADSPA_Data pregain_i;
	LADSPA_Data postgain_i;

	unsigned long sample_rate;
	LADSPA_Data run_adding_gain;
} Sigmoid;


/* Construct a new plugin instance. */
LADSPA_Handle 
instantiate_Sigmoid(const LADSPA_Descriptor * Descriptor,
		    unsigned long             sample_rate) {
  
        LADSPA_Handle * ptr;
	
	if ((ptr = malloc(sizeof(Sigmoid))) != NULL) {
		((Sigmoid *)ptr)->sample_rate = sample_rate;
		((Sigmoid *)ptr)->run_adding_gain = 1.0f;

		return ptr;
	}
       	return NULL;
}


/* Connect a port to a data location. */
void 
connect_port_Sigmoid(LADSPA_Handle Instance,
		       unsigned long Port,
		       LADSPA_Data * DataLocation) {
	
	Sigmoid * ptr = (Sigmoid *)Instance;

	switch (Port) {
	case PREGAIN:
		ptr->pregain = DataLocation;
		ptr->pregain_i = db2lin(LIMIT(*DataLocation,-90.0f,20.0f));
		break;
	case POSTGAIN:
		ptr->postgain = DataLocation;
		ptr->postgain_i = db2lin(LIMIT(*DataLocation,-90.0f,20.0f));
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
run_Sigmoid(LADSPA_Handle Instance,
	    unsigned long SampleCount) {
  
	Sigmoid * ptr = (Sigmoid *)Instance;
	LADSPA_Data * input = ptr->input;
	LADSPA_Data * output = ptr->output;
	LADSPA_Data pregain = db2lin(LIMIT(*(ptr->pregain),-90.0f,20.0f));
	LADSPA_Data postgain = db2lin(LIMIT(*(ptr->postgain),-90.0f,20.0f));
	LADSPA_Data pregain_i = ptr->pregain_i;
	LADSPA_Data postgain_i = ptr->postgain_i;

	unsigned long sample_index;
	unsigned long sample_count = SampleCount;

	LADSPA_Data in = 0.0f;
	LADSPA_Data out = 0.0f;

	if ((pregain_i != pregain) || (postgain_i != postgain))	{

		for (sample_index = 0; sample_index < sample_count; sample_index++) {

			pregain_i = pregain_i * INTERP + pregain * (1.0f - INTERP);
			postgain_i = postgain_i * INTERP + postgain * (1.0f - INTERP);

			in = *(input++) * pregain_i;
		
			out = 2.0f / (1.0f + exp(-5.0*in)) - 1.0f;

			*(output++) = out * postgain_i;
		}

		ptr->pregain_i = pregain_i;
		ptr->postgain_i = postgain_i;

	} else {
		for (sample_index = 0; sample_index < sample_count; sample_index++) {

			in = *(input++) * pregain_i;
		
			out = 2.0f / (1.0f + exp(-5.0*in)) - 1.0f;

			*(output++) = out * postgain_i;
		}

		ptr->pregain_i = pregain_i;
		ptr->postgain_i = postgain_i;
	}
}


void
set_run_adding_gain_Sigmoid(LADSPA_Handle Instance, LADSPA_Data gain) {

	Sigmoid * ptr = (Sigmoid *)Instance;

	ptr->run_adding_gain = gain;
}


void 
run_adding_Sigmoid(LADSPA_Handle Instance,
		   unsigned long SampleCount) {
  
	Sigmoid * ptr = (Sigmoid *)Instance;
	LADSPA_Data * input = ptr->input;
	LADSPA_Data * output = ptr->output;
	LADSPA_Data pregain = db2lin(LIMIT(*(ptr->pregain),-90.0f,20.0f));
	LADSPA_Data postgain = db2lin(LIMIT(*(ptr->postgain),-90.0f,20.0f));
	LADSPA_Data pregain_i = ptr->pregain_i;
	LADSPA_Data postgain_i = ptr->postgain_i;

	unsigned long sample_index;
	unsigned long sample_count = SampleCount;

	LADSPA_Data in = 0.0f;
	LADSPA_Data out = 0.0f;


	if ((pregain_i != pregain) || (postgain_i != postgain))	{

		for (sample_index = 0; sample_index < sample_count; sample_index++) {

			pregain_i = pregain_i * INTERP + pregain * (1.0f - INTERP);
			postgain_i = postgain_i * INTERP + postgain * (1.0f - INTERP);

			in = *(input++) * pregain_i;
		
			out = 2.0f / (1.0f + exp(-5.0*in)) - 1.0f;

			*(output++) = out * postgain_i * ptr->run_adding_gain;
		}

		ptr->pregain_i = pregain_i;
		ptr->postgain_i = postgain_i;

	} else {
		for (sample_index = 0; sample_index < sample_count; sample_index++) {

			in = *(input++) * pregain_i;
		
			out = 2.0f / (1.0f + exp(-5.0*in)) - 1.0f;

			*(output++) = out * postgain_i * ptr->run_adding_gain;
		}
	}		
}


/* Throw away a Sigmoid effect instance. */
void 
cleanup_Sigmoid(LADSPA_Handle Instance) {

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
	mono_descriptor->Label = strdup("tap_sigmoid");
	mono_descriptor->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
	mono_descriptor->Name = strdup("TAP Sigmoid Booster");
	mono_descriptor->Maker = strdup("Tom Szilagyi");
	mono_descriptor->Copyright = strdup("GPL");
	mono_descriptor->PortCount = PORTCOUNT_MONO;

	if ((port_descriptors =
	     (LADSPA_PortDescriptor *)calloc(PORTCOUNT_MONO, sizeof(LADSPA_PortDescriptor))) == NULL)
		exit(1);

	mono_descriptor->PortDescriptors = (const LADSPA_PortDescriptor *)port_descriptors;
	port_descriptors[PREGAIN] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[POSTGAIN] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[INPUT] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
	port_descriptors[OUTPUT] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;

	if ((port_names = 
	     (char **)calloc(PORTCOUNT_MONO, sizeof(char *))) == NULL)
		exit(1);

	mono_descriptor->PortNames = (const char **)port_names;
	port_names[PREGAIN] = strdup("Pre Gain [dB]");
	port_names[POSTGAIN] = strdup("Post Gain [dB]");
	port_names[INPUT] = strdup("Input");
	port_names[OUTPUT] = strdup("Output");

	if ((port_range_hints = 
	     ((LADSPA_PortRangeHint *)calloc(PORTCOUNT_MONO, sizeof(LADSPA_PortRangeHint)))) == NULL)
		exit(1);

	mono_descriptor->PortRangeHints	= (const LADSPA_PortRangeHint *)port_range_hints;
	port_range_hints[PREGAIN].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[POSTGAIN].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[PREGAIN].LowerBound = -90.0f;
	port_range_hints[PREGAIN].UpperBound = 20.0f;
	port_range_hints[POSTGAIN].LowerBound = -90.0f;
	port_range_hints[POSTGAIN].UpperBound = 20.0f;
	port_range_hints[INPUT].HintDescriptor = 0;
	port_range_hints[OUTPUT].HintDescriptor = 0;
	mono_descriptor->instantiate = instantiate_Sigmoid;
	mono_descriptor->connect_port = connect_port_Sigmoid;
	mono_descriptor->activate = NULL;
	mono_descriptor->run = run_Sigmoid;
	mono_descriptor->run_adding = run_adding_Sigmoid;
	mono_descriptor->set_run_adding_gain = set_run_adding_gain_Sigmoid;
	mono_descriptor->deactivate = NULL;
	mono_descriptor->cleanup = cleanup_Sigmoid;
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
