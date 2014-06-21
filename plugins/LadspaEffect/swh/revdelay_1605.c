#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include "config.h"
#endif

#ifdef ENABLE_NLS
#include <libintl.h>
#endif

#define         _ISOC9X_SOURCE  1
#define         _ISOC99_SOURCE  1
#define         __USE_ISOC99    1
#define         __USE_ISOC9X    1

#include <math.h>

#include "ladspa.h"

#ifdef WIN32
#define _WINDOWS_DLL_EXPORT_ __declspec(dllexport)
int bIsFirstTime = 1; 
void __attribute__((constructor)) swh_init(); // forward declaration
#else
#define _WINDOWS_DLL_EXPORT_ 
#endif


#include "ladspa-util.h"
#include <stdio.h>

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define CALC_DELAY(delaytime) \
  (f_clamp (delaytime * sample_rate, 1.f, (float)(buffer_size + 1)))

#define REVDELAY_IN                    0
#define REVDELAY_OUT                   1
#define REVDELAY_DELAY_TIME            2
#define REVDELAY_DRY_LEVEL             3
#define REVDELAY_WET_LEVEL             4
#define REVDELAY_FEEDBACK              5
#define REVDELAY_XFADE_SAMP            6

static LADSPA_Descriptor *revdelayDescriptor = NULL;

typedef struct {
	LADSPA_Data *in;
	LADSPA_Data *out;
	LADSPA_Data *delay_time;
	LADSPA_Data *dry_level;
	LADSPA_Data *wet_level;
	LADSPA_Data *feedback;
	LADSPA_Data *xfade_samp;
	LADSPA_Data *buffer;
	unsigned int buffer_size;
	LADSPA_Data  delay_samples;
	LADSPA_Data  last_delay_time;
	unsigned int sample_rate;
	long         write_phase;
	LADSPA_Data run_adding_gain;
} Revdelay;

_WINDOWS_DLL_EXPORT_
const LADSPA_Descriptor *ladspa_descriptor(unsigned long index) {

#ifdef WIN32
	if (bIsFirstTime) {
		swh_init();
		bIsFirstTime = 0;
	}
#endif
	switch (index) {
	case 0:
		return revdelayDescriptor;
	default:
		return NULL;
	}
}

static void activateRevdelay(LADSPA_Handle instance) {
	Revdelay *plugin_data = (Revdelay *)instance;
	LADSPA_Data *buffer = plugin_data->buffer;
	unsigned int buffer_size = plugin_data->buffer_size;
	LADSPA_Data delay_samples = plugin_data->delay_samples;
	LADSPA_Data last_delay_time = plugin_data->last_delay_time;
	unsigned int sample_rate = plugin_data->sample_rate;
	long write_phase = plugin_data->write_phase;
	unsigned int size;

	size = sample_rate * 5 * 2; /* 5 second maximum */
	  
	/* calloc sets the buffer to zero. */
	buffer = calloc(size, sizeof(LADSPA_Data));

	buffer_size = size;
	write_phase = 0;
	delay_samples = 0;
	plugin_data->buffer = buffer;
	plugin_data->buffer_size = buffer_size;
	plugin_data->delay_samples = delay_samples;
	plugin_data->last_delay_time = last_delay_time;
	plugin_data->sample_rate = sample_rate;
	plugin_data->write_phase = write_phase;

}

static void cleanupRevdelay(LADSPA_Handle instance) {
	Revdelay *plugin_data = (Revdelay *)instance;
	free(plugin_data->buffer);
	free(instance);
}

static void connectPortRevdelay(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Revdelay *plugin;

	plugin = (Revdelay *)instance;
	switch (port) {
	case REVDELAY_IN:
		plugin->in = data;
		break;
	case REVDELAY_OUT:
		plugin->out = data;
		break;
	case REVDELAY_DELAY_TIME:
		plugin->delay_time = data;
		break;
	case REVDELAY_DRY_LEVEL:
		plugin->dry_level = data;
		break;
	case REVDELAY_WET_LEVEL:
		plugin->wet_level = data;
		break;
	case REVDELAY_FEEDBACK:
		plugin->feedback = data;
		break;
	case REVDELAY_XFADE_SAMP:
		plugin->xfade_samp = data;
		break;
	}
}

static LADSPA_Handle instantiateRevdelay(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Revdelay *plugin_data = (Revdelay *)malloc(sizeof(Revdelay));
	LADSPA_Data *buffer = NULL;
	unsigned int buffer_size;
	LADSPA_Data delay_samples;
	LADSPA_Data last_delay_time;
	unsigned int sample_rate;
	long write_phase;

	sample_rate = s_rate;
	buffer_size = 0;
	delay_samples = 0;
	last_delay_time = 0;
	write_phase = 0;

	plugin_data->buffer = buffer;
	plugin_data->buffer_size = buffer_size;
	plugin_data->delay_samples = delay_samples;
	plugin_data->last_delay_time = last_delay_time;
	plugin_data->sample_rate = sample_rate;
	plugin_data->write_phase = write_phase;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runRevdelay(LADSPA_Handle instance, unsigned long sample_count) {
	Revdelay *plugin_data = (Revdelay *)instance;

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const in = plugin_data->in;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const out = plugin_data->out;

	/* Delay Time (s) (float value) */
	const LADSPA_Data delay_time = *(plugin_data->delay_time);

	/* Dry Level (dB) (float value) */
	const LADSPA_Data dry_level = *(plugin_data->dry_level);

	/* Wet Level (dB) (float value) */
	const LADSPA_Data wet_level = *(plugin_data->wet_level);

	/* Feedback (float value) */
	const LADSPA_Data feedback = *(plugin_data->feedback);

	/* Crossfade samples (float value) */
	const LADSPA_Data xfade_samp = *(plugin_data->xfade_samp);
	LADSPA_Data * buffer = plugin_data->buffer;
	unsigned int buffer_size = plugin_data->buffer_size;
	LADSPA_Data delay_samples = plugin_data->delay_samples;
	LADSPA_Data last_delay_time = plugin_data->last_delay_time;
	unsigned int sample_rate = plugin_data->sample_rate;
	long write_phase = plugin_data->write_phase;

	int i;
	unsigned long delay2;
	float dry = DB_CO(dry_level);
	float wet = DB_CO(wet_level);
	float fadescale;
	unsigned long xfadesamp = xfade_samp;

	if (write_phase == 0) {
	  plugin_data->last_delay_time = delay_time;
	  plugin_data->delay_samples = delay_samples = CALC_DELAY (delay_time);
	}

	if (delay_time == last_delay_time) {
	  long idelay_samples = (long)delay_samples;
	  delay2 = idelay_samples * 2;

	  if (xfadesamp > idelay_samples) {
	      /* force it to half */
	      xfadesamp = idelay_samples / 2;
	  }

	  for (i=0; i<sample_count; i++) {
	    long read_phase = delay2 - write_phase;
	    LADSPA_Data read;
	    LADSPA_Data insamp;

	    insamp = in[i];
	    read =  (wet * buffer[read_phase]) + (dry * insamp);

	    if ( (write_phase % idelay_samples) < xfadesamp) {
	      fadescale = (write_phase % idelay_samples) / (1.0 * xfadesamp);
	    }
	    else if ((write_phase % idelay_samples) > (idelay_samples - xfadesamp)) {
	      fadescale = (idelay_samples - (write_phase % idelay_samples)) / (1.0 * xfadesamp);
	    }
	    else {
	      fadescale = 1.0;
	    }

	    buffer[write_phase] = fadescale * (insamp + (feedback * read)); 
	    buffer[write_phase] = flush_to_zero(buffer[write_phase]);
	            
	    buffer_write(out[i], read);
	    write_phase = (write_phase + 1) % delay2;
	  }
	} else {
	  float next_delay_samples = CALC_DELAY (delay_time);
	  float delay_samples_slope = (next_delay_samples - delay_samples) / sample_count;

	  for (i=0; i<sample_count; i++) {
	    long read_phase, idelay_samples;
	    LADSPA_Data read;
	    LADSPA_Data insamp;
	    insamp = in[i];

	    delay_samples += delay_samples_slope;
	    delay2 = (long) (delay_samples * 2);
	    write_phase = (write_phase + 1) % delay2;

	    read_phase = delay2 - write_phase;
	    idelay_samples = (long)delay_samples;
	    read = wet * buffer[read_phase]   + (dry * insamp);

	    if ((write_phase % idelay_samples) < xfade_samp) {
	      fadescale = (write_phase % idelay_samples) / (1.0 * xfade_samp);
	    }
	    else if ((write_phase % idelay_samples) > (idelay_samples - xfade_samp)) {
	      fadescale = (idelay_samples - (write_phase % idelay_samples)) / (1.0 * xfade_samp);
	    }
	    else {
	      fadescale = 1.0;
	    }

	    buffer[write_phase] = fadescale * (insamp + (feedback * read)); 
	    buffer[write_phase] = flush_to_zero(buffer[write_phase]);

	    buffer_write(out[i], read);
	  }

	  plugin_data->last_delay_time = delay_time;
	  plugin_data->delay_samples = delay_samples;
	}
	
	plugin_data->write_phase = write_phase;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainRevdelay(LADSPA_Handle instance, LADSPA_Data gain) {
	((Revdelay *)instance)->run_adding_gain = gain;
}

static void runAddingRevdelay(LADSPA_Handle instance, unsigned long sample_count) {
	Revdelay *plugin_data = (Revdelay *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const in = plugin_data->in;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const out = plugin_data->out;

	/* Delay Time (s) (float value) */
	const LADSPA_Data delay_time = *(plugin_data->delay_time);

	/* Dry Level (dB) (float value) */
	const LADSPA_Data dry_level = *(plugin_data->dry_level);

	/* Wet Level (dB) (float value) */
	const LADSPA_Data wet_level = *(plugin_data->wet_level);

	/* Feedback (float value) */
	const LADSPA_Data feedback = *(plugin_data->feedback);

	/* Crossfade samples (float value) */
	const LADSPA_Data xfade_samp = *(plugin_data->xfade_samp);
	LADSPA_Data * buffer = plugin_data->buffer;
	unsigned int buffer_size = plugin_data->buffer_size;
	LADSPA_Data delay_samples = plugin_data->delay_samples;
	LADSPA_Data last_delay_time = plugin_data->last_delay_time;
	unsigned int sample_rate = plugin_data->sample_rate;
	long write_phase = plugin_data->write_phase;

	int i;
	unsigned long delay2;
	float dry = DB_CO(dry_level);
	float wet = DB_CO(wet_level);
	float fadescale;
	unsigned long xfadesamp = xfade_samp;

	if (write_phase == 0) {
	  plugin_data->last_delay_time = delay_time;
	  plugin_data->delay_samples = delay_samples = CALC_DELAY (delay_time);
	}

	if (delay_time == last_delay_time) {
	  long idelay_samples = (long)delay_samples;
	  delay2 = idelay_samples * 2;

	  if (xfadesamp > idelay_samples) {
	      /* force it to half */
	      xfadesamp = idelay_samples / 2;
	  }

	  for (i=0; i<sample_count; i++) {
	    long read_phase = delay2 - write_phase;
	    LADSPA_Data read;
	    LADSPA_Data insamp;

	    insamp = in[i];
	    read =  (wet * buffer[read_phase]) + (dry * insamp);

	    if ( (write_phase % idelay_samples) < xfadesamp) {
	      fadescale = (write_phase % idelay_samples) / (1.0 * xfadesamp);
	    }
	    else if ((write_phase % idelay_samples) > (idelay_samples - xfadesamp)) {
	      fadescale = (idelay_samples - (write_phase % idelay_samples)) / (1.0 * xfadesamp);
	    }
	    else {
	      fadescale = 1.0;
	    }

	    buffer[write_phase] = fadescale * (insamp + (feedback * read)); 
	    buffer[write_phase] = flush_to_zero(buffer[write_phase]);
	            
	    buffer_write(out[i], read);
	    write_phase = (write_phase + 1) % delay2;
	  }
	} else {
	  float next_delay_samples = CALC_DELAY (delay_time);
	  float delay_samples_slope = (next_delay_samples - delay_samples) / sample_count;

	  for (i=0; i<sample_count; i++) {
	    long read_phase, idelay_samples;
	    LADSPA_Data read;
	    LADSPA_Data insamp;
	    insamp = in[i];

	    delay_samples += delay_samples_slope;
	    delay2 = (long) (delay_samples * 2);
	    write_phase = (write_phase + 1) % delay2;

	    read_phase = delay2 - write_phase;
	    idelay_samples = (long)delay_samples;
	    read = wet * buffer[read_phase]   + (dry * insamp);

	    if ((write_phase % idelay_samples) < xfade_samp) {
	      fadescale = (write_phase % idelay_samples) / (1.0 * xfade_samp);
	    }
	    else if ((write_phase % idelay_samples) > (idelay_samples - xfade_samp)) {
	      fadescale = (idelay_samples - (write_phase % idelay_samples)) / (1.0 * xfade_samp);
	    }
	    else {
	      fadescale = 1.0;
	    }

	    buffer[write_phase] = fadescale * (insamp + (feedback * read)); 
	    buffer[write_phase] = flush_to_zero(buffer[write_phase]);

	    buffer_write(out[i], read);
	  }

	  plugin_data->last_delay_time = delay_time;
	  plugin_data->delay_samples = delay_samples;
	}
	
	plugin_data->write_phase = write_phase;
}

void __attribute__((constructor)) swh_init() {
	char **port_names;
	LADSPA_PortDescriptor *port_descriptors;
	LADSPA_PortRangeHint *port_range_hints;

#ifdef ENABLE_NLS
#define D_(s) dgettext(PACKAGE, s)
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, PACKAGE_LOCALE_DIR);
#else
#define D_(s) (s)
#endif


	revdelayDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (revdelayDescriptor) {
		revdelayDescriptor->UniqueID = 1605;
		revdelayDescriptor->Label = "revdelay";
		revdelayDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		revdelayDescriptor->Name =
		 D_("Reverse Delay (5s max)");
		revdelayDescriptor->Maker =
		 "Jesse Chappell <jesse at essej dot net>";
		revdelayDescriptor->Copyright =
		 "GPL";
		revdelayDescriptor->PortCount = 7;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(7,
		 sizeof(LADSPA_PortDescriptor));
		revdelayDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(7,
		 sizeof(LADSPA_PortRangeHint));
		revdelayDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(7, sizeof(char*));
		revdelayDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Input */
		port_descriptors[REVDELAY_IN] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[REVDELAY_IN] =
		 D_("Input");
		port_range_hints[REVDELAY_IN].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[REVDELAY_OUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[REVDELAY_OUT] =
		 D_("Output");
		port_range_hints[REVDELAY_OUT].HintDescriptor = 0;

		/* Parameters for Delay Time (s) */
		port_descriptors[REVDELAY_DELAY_TIME] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[REVDELAY_DELAY_TIME] =
		 D_("Delay Time (s)");
		port_range_hints[REVDELAY_DELAY_TIME].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[REVDELAY_DELAY_TIME].LowerBound = 0;
		port_range_hints[REVDELAY_DELAY_TIME].UpperBound = 5.0;

		/* Parameters for Dry Level (dB) */
		port_descriptors[REVDELAY_DRY_LEVEL] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[REVDELAY_DRY_LEVEL] =
		 D_("Dry Level (dB)");
		port_range_hints[REVDELAY_DRY_LEVEL].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[REVDELAY_DRY_LEVEL].LowerBound = -70;
		port_range_hints[REVDELAY_DRY_LEVEL].UpperBound = 0;

		/* Parameters for Wet Level (dB) */
		port_descriptors[REVDELAY_WET_LEVEL] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[REVDELAY_WET_LEVEL] =
		 D_("Wet Level (dB)");
		port_range_hints[REVDELAY_WET_LEVEL].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[REVDELAY_WET_LEVEL].LowerBound = -70;
		port_range_hints[REVDELAY_WET_LEVEL].UpperBound = 0;

		/* Parameters for Feedback */
		port_descriptors[REVDELAY_FEEDBACK] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[REVDELAY_FEEDBACK] =
		 D_("Feedback");
		port_range_hints[REVDELAY_FEEDBACK].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[REVDELAY_FEEDBACK].LowerBound = 0;
		port_range_hints[REVDELAY_FEEDBACK].UpperBound = 1;

		/* Parameters for Crossfade samples */
		port_descriptors[REVDELAY_XFADE_SAMP] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[REVDELAY_XFADE_SAMP] =
		 D_("Crossfade samples");
		port_range_hints[REVDELAY_XFADE_SAMP].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW | LADSPA_HINT_INTEGER;
		port_range_hints[REVDELAY_XFADE_SAMP].LowerBound = 0;
		port_range_hints[REVDELAY_XFADE_SAMP].UpperBound = 5000;

		revdelayDescriptor->activate = activateRevdelay;
		revdelayDescriptor->cleanup = cleanupRevdelay;
		revdelayDescriptor->connect_port = connectPortRevdelay;
		revdelayDescriptor->deactivate = NULL;
		revdelayDescriptor->instantiate = instantiateRevdelay;
		revdelayDescriptor->run = runRevdelay;
		revdelayDescriptor->run_adding = runAddingRevdelay;
		revdelayDescriptor->set_run_adding_gain = setRunAddingGainRevdelay;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (revdelayDescriptor) {
		free((LADSPA_PortDescriptor *)revdelayDescriptor->PortDescriptors);
		free((char **)revdelayDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)revdelayDescriptor->PortRangeHints);
		free(revdelayDescriptor);
	}

}
