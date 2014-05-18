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

#line 10 "smooth_decimate_1414.xml"

#include "ladspa-util.h"

#define SMOOTHDECIMATE_RATE            0
#define SMOOTHDECIMATE_SMOOTH          1
#define SMOOTHDECIMATE_INPUT           2
#define SMOOTHDECIMATE_OUTPUT          3

static LADSPA_Descriptor *smoothDecimateDescriptor = NULL;

typedef struct {
	LADSPA_Data *rate;
	LADSPA_Data *smooth;
	LADSPA_Data *input;
	LADSPA_Data *output;
	float        accum;
	float *      buffer;
	int          buffer_pos;
	float        fs;
	LADSPA_Data run_adding_gain;
} SmoothDecimate;

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
		return smoothDecimateDescriptor;
	default:
		return NULL;
	}
}

static void activateSmoothDecimate(LADSPA_Handle instance) {
	SmoothDecimate *plugin_data = (SmoothDecimate *)instance;
	float accum = plugin_data->accum;
	float *buffer = plugin_data->buffer;
	int buffer_pos = plugin_data->buffer_pos;
	float fs = plugin_data->fs;
#line 26 "smooth_decimate_1414.xml"
	buffer_pos = 0;
	accum = 0.0f;
	plugin_data->accum = accum;
	plugin_data->buffer = buffer;
	plugin_data->buffer_pos = buffer_pos;
	plugin_data->fs = fs;

}

static void cleanupSmoothDecimate(LADSPA_Handle instance) {
#line 55 "smooth_decimate_1414.xml"
	SmoothDecimate *plugin_data = (SmoothDecimate *)instance;
	free(plugin_data->buffer);
	free(instance);
}

static void connectPortSmoothDecimate(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	SmoothDecimate *plugin;

	plugin = (SmoothDecimate *)instance;
	switch (port) {
	case SMOOTHDECIMATE_RATE:
		plugin->rate = data;
		break;
	case SMOOTHDECIMATE_SMOOTH:
		plugin->smooth = data;
		break;
	case SMOOTHDECIMATE_INPUT:
		plugin->input = data;
		break;
	case SMOOTHDECIMATE_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateSmoothDecimate(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	SmoothDecimate *plugin_data = (SmoothDecimate *)malloc(sizeof(SmoothDecimate));
	float accum;
	float *buffer = NULL;
	int buffer_pos;
	float fs;

#line 19 "smooth_decimate_1414.xml"
	buffer = calloc(8, sizeof(float));
	buffer_pos = 0;
	accum = 0.0f;
	fs = (float)s_rate;

	plugin_data->accum = accum;
	plugin_data->buffer = buffer;
	plugin_data->buffer_pos = buffer_pos;
	plugin_data->fs = fs;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runSmoothDecimate(LADSPA_Handle instance, unsigned long sample_count) {
	SmoothDecimate *plugin_data = (SmoothDecimate *)instance;

	/* Resample rate (float value) */
	const LADSPA_Data rate = *(plugin_data->rate);

	/* Smoothing (float value) */
	const LADSPA_Data smooth = *(plugin_data->smooth);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	float accum = plugin_data->accum;
	float * buffer = plugin_data->buffer;
	int buffer_pos = plugin_data->buffer_pos;
	float fs = plugin_data->fs;

#line 31 "smooth_decimate_1414.xml"
	unsigned long pos;
	float smoothed;
	float inc = (rate / fs);
	inc = f_clamp(inc, 0.0f, 1.0f);

	for (pos = 0; pos < sample_count; pos++) {
	  accum += inc;
	  if (accum >= 1.0f) {
	    accum -= 1.0f;
	    buffer_pos = (buffer_pos + 1) & 7;
	    buffer[buffer_pos] = input[pos];
	  }
	  smoothed = cube_interp(accum, buffer[(buffer_pos - 3) & 7],
	                                buffer[(buffer_pos - 2) & 7],
	                                buffer[(buffer_pos - 1) & 7],
	                                buffer[buffer_pos]);
	  buffer_write(output[pos], LIN_INTERP(smooth, buffer[(buffer_pos - 3) & 7], smoothed));
	}

	plugin_data->accum = accum;
	plugin_data->buffer_pos = buffer_pos;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainSmoothDecimate(LADSPA_Handle instance, LADSPA_Data gain) {
	((SmoothDecimate *)instance)->run_adding_gain = gain;
}

static void runAddingSmoothDecimate(LADSPA_Handle instance, unsigned long sample_count) {
	SmoothDecimate *plugin_data = (SmoothDecimate *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Resample rate (float value) */
	const LADSPA_Data rate = *(plugin_data->rate);

	/* Smoothing (float value) */
	const LADSPA_Data smooth = *(plugin_data->smooth);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	float accum = plugin_data->accum;
	float * buffer = plugin_data->buffer;
	int buffer_pos = plugin_data->buffer_pos;
	float fs = plugin_data->fs;

#line 31 "smooth_decimate_1414.xml"
	unsigned long pos;
	float smoothed;
	float inc = (rate / fs);
	inc = f_clamp(inc, 0.0f, 1.0f);

	for (pos = 0; pos < sample_count; pos++) {
	  accum += inc;
	  if (accum >= 1.0f) {
	    accum -= 1.0f;
	    buffer_pos = (buffer_pos + 1) & 7;
	    buffer[buffer_pos] = input[pos];
	  }
	  smoothed = cube_interp(accum, buffer[(buffer_pos - 3) & 7],
	                                buffer[(buffer_pos - 2) & 7],
	                                buffer[(buffer_pos - 1) & 7],
	                                buffer[buffer_pos]);
	  buffer_write(output[pos], LIN_INTERP(smooth, buffer[(buffer_pos - 3) & 7], smoothed));
	}

	plugin_data->accum = accum;
	plugin_data->buffer_pos = buffer_pos;
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


	smoothDecimateDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (smoothDecimateDescriptor) {
		smoothDecimateDescriptor->UniqueID = 1414;
		smoothDecimateDescriptor->Label = "smoothDecimate";
		smoothDecimateDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		smoothDecimateDescriptor->Name =
		 D_("Smooth Decimator");
		smoothDecimateDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		smoothDecimateDescriptor->Copyright =
		 "GPL";
		smoothDecimateDescriptor->PortCount = 4;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(4,
		 sizeof(LADSPA_PortDescriptor));
		smoothDecimateDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(4,
		 sizeof(LADSPA_PortRangeHint));
		smoothDecimateDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(4, sizeof(char*));
		smoothDecimateDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Resample rate */
		port_descriptors[SMOOTHDECIMATE_RATE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SMOOTHDECIMATE_RATE] =
		 D_("Resample rate");
		port_range_hints[SMOOTHDECIMATE_RATE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_SAMPLE_RATE | LADSPA_HINT_DEFAULT_MAXIMUM;
		port_range_hints[SMOOTHDECIMATE_RATE].LowerBound = 0;
		port_range_hints[SMOOTHDECIMATE_RATE].UpperBound = 1;

		/* Parameters for Smoothing */
		port_descriptors[SMOOTHDECIMATE_SMOOTH] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SMOOTHDECIMATE_SMOOTH] =
		 D_("Smoothing");
		port_range_hints[SMOOTHDECIMATE_SMOOTH].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MAXIMUM;
		port_range_hints[SMOOTHDECIMATE_SMOOTH].LowerBound = 0;
		port_range_hints[SMOOTHDECIMATE_SMOOTH].UpperBound = 1;

		/* Parameters for Input */
		port_descriptors[SMOOTHDECIMATE_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[SMOOTHDECIMATE_INPUT] =
		 D_("Input");
		port_range_hints[SMOOTHDECIMATE_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[SMOOTHDECIMATE_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[SMOOTHDECIMATE_OUTPUT] =
		 D_("Output");
		port_range_hints[SMOOTHDECIMATE_OUTPUT].HintDescriptor = 0;

		smoothDecimateDescriptor->activate = activateSmoothDecimate;
		smoothDecimateDescriptor->cleanup = cleanupSmoothDecimate;
		smoothDecimateDescriptor->connect_port = connectPortSmoothDecimate;
		smoothDecimateDescriptor->deactivate = NULL;
		smoothDecimateDescriptor->instantiate = instantiateSmoothDecimate;
		smoothDecimateDescriptor->run = runSmoothDecimate;
		smoothDecimateDescriptor->run_adding = runAddingSmoothDecimate;
		smoothDecimateDescriptor->set_run_adding_gain = setRunAddingGainSmoothDecimate;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (smoothDecimateDescriptor) {
		free((LADSPA_PortDescriptor *)smoothDecimateDescriptor->PortDescriptors);
		free((char **)smoothDecimateDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)smoothDecimateDescriptor->PortRangeHints);
		free(smoothDecimateDescriptor);
	}

}
