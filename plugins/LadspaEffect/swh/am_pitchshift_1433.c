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

#line 10 "am_pitchshift_1433.xml"

#include <stdlib.h>
#include <math.h>

#include "ladspa-util.h"

/* Beware of dependcies if you change this */
#define DELAY_SIZE 8192

#define AMPITCHSHIFT_PITCH             0
#define AMPITCHSHIFT_SIZE              1
#define AMPITCHSHIFT_INPUT             2
#define AMPITCHSHIFT_OUTPUT            3
#define AMPITCHSHIFT_LATENCY           4

static LADSPA_Descriptor *amPitchshiftDescriptor = NULL;

typedef struct {
	LADSPA_Data *pitch;
	LADSPA_Data *size;
	LADSPA_Data *input;
	LADSPA_Data *output;
	LADSPA_Data *latency;
	unsigned int count;
	LADSPA_Data *delay;
	unsigned int delay_mask;
	unsigned int delay_ofs;
	float        last_gain;
	float        last_inc;
	int          last_size;
	fixp16       rptr;
	unsigned int wptr;
	LADSPA_Data run_adding_gain;
} AmPitchshift;

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
		return amPitchshiftDescriptor;
	default:
		return NULL;
	}
}

static void cleanupAmPitchshift(LADSPA_Handle instance) {
#line 39 "am_pitchshift_1433.xml"
	AmPitchshift *plugin_data = (AmPitchshift *)instance;
	free(plugin_data->delay);
	free(instance);
}

static void connectPortAmPitchshift(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	AmPitchshift *plugin;

	plugin = (AmPitchshift *)instance;
	switch (port) {
	case AMPITCHSHIFT_PITCH:
		plugin->pitch = data;
		break;
	case AMPITCHSHIFT_SIZE:
		plugin->size = data;
		break;
	case AMPITCHSHIFT_INPUT:
		plugin->input = data;
		break;
	case AMPITCHSHIFT_OUTPUT:
		plugin->output = data;
		break;
	case AMPITCHSHIFT_LATENCY:
		plugin->latency = data;
		break;
	}
}

static LADSPA_Handle instantiateAmPitchshift(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	AmPitchshift *plugin_data = (AmPitchshift *)malloc(sizeof(AmPitchshift));
	unsigned int count;
	LADSPA_Data *delay = NULL;
	unsigned int delay_mask;
	unsigned int delay_ofs;
	float last_gain;
	float last_inc;
	int last_size;
	fixp16 rptr;
	unsigned int wptr;

#line 27 "am_pitchshift_1433.xml"
	delay = calloc(DELAY_SIZE, sizeof(LADSPA_Data));
	rptr.all = 0;
	wptr = 0;
	last_size = -1;
	delay_mask = 0xFF;
	delay_ofs = 0x80;
	last_gain = 0.5f;
	count = 0;
	last_inc = 0.0f;

	plugin_data->count = count;
	plugin_data->delay = delay;
	plugin_data->delay_mask = delay_mask;
	plugin_data->delay_ofs = delay_ofs;
	plugin_data->last_gain = last_gain;
	plugin_data->last_inc = last_inc;
	plugin_data->last_size = last_size;
	plugin_data->rptr = rptr;
	plugin_data->wptr = wptr;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runAmPitchshift(LADSPA_Handle instance, unsigned long sample_count) {
	AmPitchshift *plugin_data = (AmPitchshift *)instance;

	/* Pitch shift (float value) */
	const LADSPA_Data pitch = *(plugin_data->pitch);

	/* Buffer size (float value) */
	const LADSPA_Data size = *(plugin_data->size);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	unsigned int count = plugin_data->count;
	LADSPA_Data * delay = plugin_data->delay;
	unsigned int delay_mask = plugin_data->delay_mask;
	unsigned int delay_ofs = plugin_data->delay_ofs;
	float last_gain = plugin_data->last_gain;
	float last_inc = plugin_data->last_inc;
	int last_size = plugin_data->last_size;
	fixp16 rptr = plugin_data->rptr;
	unsigned int wptr = plugin_data->wptr;

#line 43 "am_pitchshift_1433.xml"
	unsigned long pos;
	fixp16 om;
	float gain = last_gain, gain_inc = last_inc;
	unsigned int i;

	om.all = f_round(pitch * 65536.0f);

	if (size != last_size) {
	  int size_tmp = f_round(size);

	  if (size_tmp > 7) {
	    size_tmp = 5;
	  } else if (size_tmp < 1) {
	    size_tmp = 1;
	  }
	  plugin_data->last_size = size;

	  /* Calculate the ringbuf parameters, the magick constants will need
	   * to be changed if you change DELAY_SIZE */
	  delay_mask = (1 << (size_tmp + 6)) - 1;
	  delay_ofs = 1 << (size_tmp + 5);
	}

	for (pos = 0; pos < sample_count; pos++) {
	  float out = 0.0f;

	  if (count++ > 14) {
	    float tmp;
	    count = 0;
	    tmp = 0.5f * (float)((rptr.part.in - wptr + delay_ofs/2) &
	          delay_mask) / (float)delay_ofs;
	    tmp = sinf(M_PI * 2.0f * tmp) * 0.5f + 0.5f;
	    gain_inc = (tmp - gain) / 15.0f;
	  }
	  gain += gain_inc;

	  delay[wptr] = input[pos];

	  /* Add contributions from the two readpointers, scaled by thier
	   * distance from the write pointer */
	  i = rptr.part.in;
	  out += cube_interp((float)rptr.part.fr * 0.0000152587f,
	                     delay[(i - 1) & delay_mask], delay[i],
	                     delay[(i + 1) & delay_mask],
	                     delay[(i + 2) & delay_mask]) * (1.0f - gain);
	  i += delay_ofs;
	  out += cube_interp((float)rptr.part.fr * 0.0000152587f,
	                     delay[(i - 1) & delay_mask], delay[i & delay_mask],
	                     delay[(i + 1) & delay_mask],
	                     delay[(i + 2) & delay_mask]) * gain;
	  
	  buffer_write(output[pos], out);

	  /* Increment ringbuffer pointers */
	  wptr = (wptr + 1) & delay_mask;
	  rptr.all += om.all;
	  rptr.part.in &= delay_mask;
	}

    plugin_data->rptr.all = rptr.all;
    plugin_data->wptr = wptr;
    plugin_data->delay_mask = delay_mask;
    plugin_data->delay_ofs = delay_ofs;
    plugin_data->last_gain = gain;
    plugin_data->count = count;
    plugin_data->last_inc = gain_inc;

    *(plugin_data->latency) = delay_ofs/2;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainAmPitchshift(LADSPA_Handle instance, LADSPA_Data gain) {
	((AmPitchshift *)instance)->run_adding_gain = gain;
}

static void runAddingAmPitchshift(LADSPA_Handle instance, unsigned long sample_count) {
	AmPitchshift *plugin_data = (AmPitchshift *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Pitch shift (float value) */
	const LADSPA_Data pitch = *(plugin_data->pitch);

	/* Buffer size (float value) */
	const LADSPA_Data size = *(plugin_data->size);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	unsigned int count = plugin_data->count;
	LADSPA_Data * delay = plugin_data->delay;
	unsigned int delay_mask = plugin_data->delay_mask;
	unsigned int delay_ofs = plugin_data->delay_ofs;
	float last_gain = plugin_data->last_gain;
	float last_inc = plugin_data->last_inc;
	int last_size = plugin_data->last_size;
	fixp16 rptr = plugin_data->rptr;
	unsigned int wptr = plugin_data->wptr;

#line 43 "am_pitchshift_1433.xml"
	unsigned long pos;
	fixp16 om;
	float gain = last_gain, gain_inc = last_inc;
	unsigned int i;

	om.all = f_round(pitch * 65536.0f);

	if (size != last_size) {
	  int size_tmp = f_round(size);

	  if (size_tmp > 7) {
	    size_tmp = 5;
	  } else if (size_tmp < 1) {
	    size_tmp = 1;
	  }
	  plugin_data->last_size = size;

	  /* Calculate the ringbuf parameters, the magick constants will need
	   * to be changed if you change DELAY_SIZE */
	  delay_mask = (1 << (size_tmp + 6)) - 1;
	  delay_ofs = 1 << (size_tmp + 5);
	}

	for (pos = 0; pos < sample_count; pos++) {
	  float out = 0.0f;

	  if (count++ > 14) {
	    float tmp;
	    count = 0;
	    tmp = 0.5f * (float)((rptr.part.in - wptr + delay_ofs/2) &
	          delay_mask) / (float)delay_ofs;
	    tmp = sinf(M_PI * 2.0f * tmp) * 0.5f + 0.5f;
	    gain_inc = (tmp - gain) / 15.0f;
	  }
	  gain += gain_inc;

	  delay[wptr] = input[pos];

	  /* Add contributions from the two readpointers, scaled by thier
	   * distance from the write pointer */
	  i = rptr.part.in;
	  out += cube_interp((float)rptr.part.fr * 0.0000152587f,
	                     delay[(i - 1) & delay_mask], delay[i],
	                     delay[(i + 1) & delay_mask],
	                     delay[(i + 2) & delay_mask]) * (1.0f - gain);
	  i += delay_ofs;
	  out += cube_interp((float)rptr.part.fr * 0.0000152587f,
	                     delay[(i - 1) & delay_mask], delay[i & delay_mask],
	                     delay[(i + 1) & delay_mask],
	                     delay[(i + 2) & delay_mask]) * gain;
	  
	  buffer_write(output[pos], out);

	  /* Increment ringbuffer pointers */
	  wptr = (wptr + 1) & delay_mask;
	  rptr.all += om.all;
	  rptr.part.in &= delay_mask;
	}

    plugin_data->rptr.all = rptr.all;
    plugin_data->wptr = wptr;
    plugin_data->delay_mask = delay_mask;
    plugin_data->delay_ofs = delay_ofs;
    plugin_data->last_gain = gain;
    plugin_data->count = count;
    plugin_data->last_inc = gain_inc;

    *(plugin_data->latency) = delay_ofs/2;
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


	amPitchshiftDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (amPitchshiftDescriptor) {
		amPitchshiftDescriptor->UniqueID = 1433;
		amPitchshiftDescriptor->Label = "amPitchshift";
		amPitchshiftDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		amPitchshiftDescriptor->Name =
		 D_("AM pitchshifter");
		amPitchshiftDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		amPitchshiftDescriptor->Copyright =
		 "GPL";
		amPitchshiftDescriptor->PortCount = 5;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(5,
		 sizeof(LADSPA_PortDescriptor));
		amPitchshiftDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(5,
		 sizeof(LADSPA_PortRangeHint));
		amPitchshiftDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(5, sizeof(char*));
		amPitchshiftDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Pitch shift */
		port_descriptors[AMPITCHSHIFT_PITCH] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[AMPITCHSHIFT_PITCH] =
		 D_("Pitch shift");
		port_range_hints[AMPITCHSHIFT_PITCH].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_LOGARITHMIC | LADSPA_HINT_DEFAULT_1;
		port_range_hints[AMPITCHSHIFT_PITCH].LowerBound = 0.25;
		port_range_hints[AMPITCHSHIFT_PITCH].UpperBound = 4.0;

		/* Parameters for Buffer size */
		port_descriptors[AMPITCHSHIFT_SIZE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[AMPITCHSHIFT_SIZE] =
		 D_("Buffer size");
		port_range_hints[AMPITCHSHIFT_SIZE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_INTEGER | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[AMPITCHSHIFT_SIZE].LowerBound = 1;
		port_range_hints[AMPITCHSHIFT_SIZE].UpperBound = 7;

		/* Parameters for Input */
		port_descriptors[AMPITCHSHIFT_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[AMPITCHSHIFT_INPUT] =
		 D_("Input");
		port_range_hints[AMPITCHSHIFT_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[AMPITCHSHIFT_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[AMPITCHSHIFT_OUTPUT] =
		 D_("Output");
		port_range_hints[AMPITCHSHIFT_OUTPUT].HintDescriptor = 0;

		/* Parameters for latency */
		port_descriptors[AMPITCHSHIFT_LATENCY] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL;
		port_names[AMPITCHSHIFT_LATENCY] =
		 D_("latency");
		port_range_hints[AMPITCHSHIFT_LATENCY].HintDescriptor = 0;

		amPitchshiftDescriptor->activate = NULL;
		amPitchshiftDescriptor->cleanup = cleanupAmPitchshift;
		amPitchshiftDescriptor->connect_port = connectPortAmPitchshift;
		amPitchshiftDescriptor->deactivate = NULL;
		amPitchshiftDescriptor->instantiate = instantiateAmPitchshift;
		amPitchshiftDescriptor->run = runAmPitchshift;
		amPitchshiftDescriptor->run_adding = runAddingAmPitchshift;
		amPitchshiftDescriptor->set_run_adding_gain = setRunAddingGainAmPitchshift;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (amPitchshiftDescriptor) {
		free((LADSPA_PortDescriptor *)amPitchshiftDescriptor->PortDescriptors);
		free((char **)amPitchshiftDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)amPitchshiftDescriptor->PortRangeHints);
		free(amPitchshiftDescriptor);
	}

}
