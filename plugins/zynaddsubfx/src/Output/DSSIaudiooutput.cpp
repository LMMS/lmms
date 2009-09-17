/*
  ZynAddSubFX - a software synthesizer

  DSSIaudiooutput.C - Audio functions for DSSI
  Copyright (C) 2002 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2 or later) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

*/

//this file contains code used from trivial_synth.c from
//the DSSI (published by Steve Harris under public domain) as a template
//the code is incomplete
#include <string.h>
#include "DSSIaudiooutput.h"

static LADSPA_Descriptor *tsLDescriptor = NULL;
static DSSI_Descriptor *tsDDescriptor = NULL;

typedef struct {
    LADSPA_Data *outl;
    LADSPA_Data *outr;
//    note_data data[MIDI_NOTES];
//    float omega[MIDI_NOTES];
} TS;


static void cleanupTS(LADSPA_Handle instance)
{
    free(instance);
}
static void connectPortTS(LADSPA_Handle instance, unsigned long port,
                          LADSPA_Data * data)
{
    TS *plugin;
    plugin = (TS *) instance;
    switch (port) {
    case 0:
        plugin->outl = data;
        break;
    case 1:
        plugin->outr = data;
        break;
    }
}

const LADSPA_Descriptor *ladspa_descriptor(unsigned long index)
{
    switch (index) {
    case 0:
        return tsLDescriptor;
    default:
        return NULL;
    }
}

const DSSI_Descriptor *dssi_descriptor(unsigned long index)
{
//    FILE *a=fopen("/tmp/zzzzz11z","w");
//    fprintf(a,"aaaaaaaaaaa  TEST\n");
//    fclose(a);
    switch (index) {
    case 0:
        return tsDDescriptor;
    default:
        return NULL;
    }
}

static LADSPA_Handle instantiateTS(const LADSPA_Descriptor * descriptor,
                                   unsigned long s_rate)
{

    TS *plugin_data = (TS *) malloc(sizeof(TS));
    /*    for (i=0; i<MIDI_NOTES; i++) {
    	    plugin_data->omega[i] = M_PI * 2.0 / (double)s_rate *
    				    pow(2.0, (i-69.0) / 12.0);
        }
    */
    return (LADSPA_Handle) plugin_data;
}

static void activateTS(LADSPA_Handle instance)
{
    TS *plugin_data = (TS *) instance;

//    for (i=0; i<MIDI_NOTES; i++) {
//	plugin_data->data[i].active = 0;
//    }
}


static void runTS(LADSPA_Handle instance, unsigned long sample_count,
                  snd_seq_event_t *events, unsigned long event_count)
{
    TS *plugin_data = (TS *) instance;
//    LADSPA_Data *const output = plugin_data->output;
//    LADSPA_Data freq = *(plugin_data->freq);
//    LADSPA_Data vol = *(plugin_data->vol);
//    note_data *data = plugin_data->data;
    unsigned long pos;
    unsigned long event_pos;
    unsigned long note;

    /*    if (freq < 1.0) {
    	freq = 440.0f;
        }
        if (vol < 0.000001) {
    	vol = 1.0f;
        }

        if (event_count > 0) {
    	printf("trivial_synth: have %ld events\n", event_count);
        }

        for (pos = 0, event_pos = 0; pos < sample_count; pos++) {

    	while (event_pos < event_count
    	       && pos == events[event_pos].time.tick) {

    	    printf("trivial_synth: event type %d\n", events[event_pos].type);

    	    if (events[event_pos].type == SND_SEQ_EVENT_NOTEON) {
    		data[events[event_pos].data.note.note].amp =
    		    events[event_pos].data.note.velocity / 512.0f;
    		data[events[event_pos].data.note.note].
    		    active = events[event_pos].data.note.velocity > 0;
    		data[events[event_pos].data.note.note].
    		    phase = 0.0;
    	    } else if (events[event_pos].type == SND_SEQ_EVENT_NOTEOFF) {
    		data[events[event_pos].data.note.note].
    		    active = 0;
    	    }
    	    event_pos++;
    	}

    	output[pos] = 0.0f;
    	for (note = 0; note < MIDI_NOTES; note++) {
    	    if (data[note].active) {
    		output[pos] += sin(data[note].phase) * data[note].amp * vol;
    		data[note].phase += plugin_data->omega[note] * freq;
    		if (data[note].phase > M_PI * 2.0) {
    		    data[note].phase -= M_PI * 2.0;
    		}
    	    }
    	}
        }
        */
}


static void runTSWrapper(LADSPA_Handle instance,
                         unsigned long sample_count)
{
    runTS(instance, sample_count, NULL, 0);
}

int getControllerTS(LADSPA_Handle instance, unsigned long port)
{
    return -1;
}

void _init()
{
    char **port_names;
    LADSPA_PortDescriptor *port_descriptors;
    LADSPA_PortRangeHint *port_range_hints;

    FILE *a=fopen("/tmp/zzzzzz","w");
    fprintf(a,"aaaaaaaaaaa  TEST\n");
    fclose(a);


    tsLDescriptor = (LADSPA_Descriptor *) malloc(sizeof(LADSPA_Descriptor));
    if (tsLDescriptor) {
        tsLDescriptor->UniqueID = 100;
        tsLDescriptor->Label = "ZASF";
        tsLDescriptor->Properties = 0;
        tsLDescriptor->Name = "ZynAddSubFX";
        tsLDescriptor->Maker = "Nasca Octavian Paul <zynaddsubfx@yahoo.com>";
        tsLDescriptor->Copyright = "GNU General Public License v.2";
        tsLDescriptor->PortCount = 2;

        port_descriptors = (LADSPA_PortDescriptor *)
                           calloc(tsLDescriptor->PortCount, sizeof
                                  (LADSPA_PortDescriptor));
        tsLDescriptor->PortDescriptors =
            (const LADSPA_PortDescriptor *) port_descriptors;

        port_range_hints = (LADSPA_PortRangeHint *)
                           calloc(tsLDescriptor->PortCount, sizeof
                                  (LADSPA_PortRangeHint));
        tsLDescriptor->PortRangeHints =
            (const LADSPA_PortRangeHint *) port_range_hints;

        port_names = (char **) calloc(tsLDescriptor->PortCount, sizeof(char *));
        tsLDescriptor->PortNames = (const char **) port_names;

        port_descriptors[0] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
        port_names[0] = "Output L";
        port_range_hints[0].HintDescriptor = 0;
        port_descriptors[1] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
        port_names[1] = "Output R";
        port_range_hints[1].HintDescriptor = 0;

        tsLDescriptor->activate = activateTS;
        tsLDescriptor->cleanup = cleanupTS;
        tsLDescriptor->connect_port = connectPortTS;
        tsLDescriptor->deactivate = NULL;
        tsLDescriptor->instantiate = instantiateTS;
        tsLDescriptor->run = runTSWrapper;
        tsLDescriptor->run_adding = NULL;
        tsLDescriptor->set_run_adding_gain = NULL;
    }

    tsDDescriptor = (DSSI_Descriptor *) malloc(sizeof(DSSI_Descriptor));
    if (tsDDescriptor) {
        tsDDescriptor->DSSI_API_Version = 1;
        tsDDescriptor->LADSPA_Plugin = tsLDescriptor;
        tsDDescriptor->configure = NULL;
        tsDDescriptor->get_program = NULL;
        tsDDescriptor->get_midi_controller_for_port = getControllerTS;
        tsDDescriptor->select_program = NULL;
        tsDDescriptor->run_synth = runTS;
        tsDDescriptor->run_synth_adding = NULL;
        tsDDescriptor->run_multiple_synths = NULL;
        tsDDescriptor->run_multiple_synths_adding = NULL;
    }

};

void _fini()
{
};







//the constructor and the destructor are defined in main.C
/*
void VSTSynth::process (float **inputs, float **outputs, long sampleframes){
    float *outl=outputs[0];
    float *outr=outputs[1];
    pthread_mutex_lock(&vmaster->mutex);
     vmaster->GetAudioOutSamples(sampleframes,(int) getSampleRate(),outl,outr);
    pthread_mutex_unlock(&vmaster->mutex);
};

void VSTSynth::processReplacing (float **inputs, float **outputs, long sampleframes){
    process(inputs,outputs,sampleframes);
};

long int VSTSynth::canDo(char *txt){
 if (strcmp(txt,"receiveVstEvents")==0) return (1);
 if (strcmp(txt,"receiveVstMidiEvent")==0) return (1);
 return(-1);
};

bool VSTSynth::getVendorString(char *txt){
	strcpy(txt,"Nasca O. Paul");
	return(true);
};

bool VSTSynth::getProductString(char *txt){
	strcpy(txt,"ZynAddSubFX");
	return(true);
};

void VSTSynth::resume(){
	wantEvents();
};

*/
