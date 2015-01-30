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

#line 10 "valve_rect_1405.xml"

#include "ladspa-util.h"

#define VALVERECT_SAG                  0
#define VALVERECT_DIST_P               1
#define VALVERECT_INPUT                2
#define VALVERECT_OUTPUT               3

static LADSPA_Descriptor *valveRectDescriptor = NULL;

typedef struct {
	LADSPA_Data *sag;
	LADSPA_Data *dist_p;
	LADSPA_Data *input;
	LADSPA_Data *output;
	unsigned int apos;
	float *      avg;
	int          avg_size;
	float        avg_sizer;
	float        avgs;
	float        lp1tm1;
	float        lp2tm1;
	LADSPA_Data run_adding_gain;
} ValveRect;

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
		return valveRectDescriptor;
	default:
		return NULL;
	}
}

static void activateValveRect(LADSPA_Handle instance) {
	ValveRect *plugin_data = (ValveRect *)instance;
	unsigned int apos = plugin_data->apos;
	float *avg = plugin_data->avg;
	int avg_size = plugin_data->avg_size;
	float avg_sizer = plugin_data->avg_sizer;
	float avgs = plugin_data->avgs;
	float lp1tm1 = plugin_data->lp1tm1;
	float lp2tm1 = plugin_data->lp2tm1;
#line 36 "valve_rect_1405.xml"
	memset(avg, 0, avg_size * sizeof(float));
	avgs = 0.0f;
	apos = 0;
	lp1tm1 = 0.0f;
	lp2tm1 = 0.0f;
	plugin_data->apos = apos;
	plugin_data->avg = avg;
	plugin_data->avg_size = avg_size;
	plugin_data->avg_sizer = avg_sizer;
	plugin_data->avgs = avgs;
	plugin_data->lp1tm1 = lp1tm1;
	plugin_data->lp2tm1 = lp2tm1;

}

static void cleanupValveRect(LADSPA_Handle instance) {
#line 44 "valve_rect_1405.xml"
	ValveRect *plugin_data = (ValveRect *)instance;
	free(plugin_data->avg);
	free(instance);
}

static void connectPortValveRect(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	ValveRect *plugin;

	plugin = (ValveRect *)instance;
	switch (port) {
	case VALVERECT_SAG:
		plugin->sag = data;
		break;
	case VALVERECT_DIST_P:
		plugin->dist_p = data;
		break;
	case VALVERECT_INPUT:
		plugin->input = data;
		break;
	case VALVERECT_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateValveRect(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	ValveRect *plugin_data = (ValveRect *)malloc(sizeof(ValveRect));
	unsigned int apos;
	float *avg = NULL;
	int avg_size;
	float avg_sizer;
	float avgs;
	float lp1tm1;
	float lp2tm1;

#line 19 "valve_rect_1405.xml"
	// Number of samples in averaging buffer
	avg_size = s_rate / 9;
	// Reciprocal of obove
	avg_sizer = 9.0f / (float)s_rate;
	// Averaging buffer
	avg = calloc(avg_size, sizeof(float));
	// Sum of samples in averaging buffer
	avgs = 0.0f;
	// Position in averaging buffer
	apos = 0;
	// Last value in lowpass 1
	lp1tm1 = 0.0f;
	// Last value in lowpass 2
	lp2tm1 = 0.0f;

	plugin_data->apos = apos;
	plugin_data->avg = avg;
	plugin_data->avg_size = avg_size;
	plugin_data->avg_sizer = avg_sizer;
	plugin_data->avgs = avgs;
	plugin_data->lp1tm1 = lp1tm1;
	plugin_data->lp2tm1 = lp2tm1;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runValveRect(LADSPA_Handle instance, unsigned long sample_count) {
	ValveRect *plugin_data = (ValveRect *)instance;

	/* Sag level (float value) */
	const LADSPA_Data sag = *(plugin_data->sag);

	/* Distortion (float value) */
	const LADSPA_Data dist_p = *(plugin_data->dist_p);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	unsigned int apos = plugin_data->apos;
	float * avg = plugin_data->avg;
	int avg_size = plugin_data->avg_size;
	float avg_sizer = plugin_data->avg_sizer;
	float avgs = plugin_data->avgs;
	float lp1tm1 = plugin_data->lp1tm1;
	float lp2tm1 = plugin_data->lp2tm1;

#line 48 "valve_rect_1405.xml"
	unsigned long pos;
	float q, x, fx;
	const float dist = dist_p * 40.0f + 0.1f;

	for (pos = 0; pos < sample_count; pos++) {
	  x = fabs(input[pos]);
	  if (x > lp1tm1) {
	    lp1tm1 = x;
	  } else {
	    lp1tm1 = 0.9999f * lp1tm1 + 0.0001f * x;
	  }

	  avgs -= avg[apos];
	  avgs += lp1tm1;
	  avg[apos++] = lp1tm1;
	  apos %= avg_size;

	  lp2tm1 = 0.999f * lp2tm1 + avgs*avg_sizer * 0.001f;
	  q = lp1tm1 * sag - lp2tm1 * 1.02f - 1.0f;
	  if (q > -0.01f) {
	    q = -0.01f;
	  } else if (q < -1.0f) {
	    q = -1.0f;
	  }

	  if (input[pos] == q) {
	    fx = 1.0f / dist + q / (1.0f - f_exp(dist * q));
	  } else {
	    fx = (input[pos] - q) /
	     (1.0f - f_exp(-dist * (input[pos] - q))) +
	     q / (1.0f - f_exp(dist * q));
	  }

	  buffer_write(output[pos], fx);
	}

	plugin_data->lp1tm1 = lp1tm1;
	plugin_data->lp2tm1 = lp2tm1;
	plugin_data->avgs = avgs;
	plugin_data->apos = apos;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainValveRect(LADSPA_Handle instance, LADSPA_Data gain) {
	((ValveRect *)instance)->run_adding_gain = gain;
}

static void runAddingValveRect(LADSPA_Handle instance, unsigned long sample_count) {
	ValveRect *plugin_data = (ValveRect *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Sag level (float value) */
	const LADSPA_Data sag = *(plugin_data->sag);

	/* Distortion (float value) */
	const LADSPA_Data dist_p = *(plugin_data->dist_p);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	unsigned int apos = plugin_data->apos;
	float * avg = plugin_data->avg;
	int avg_size = plugin_data->avg_size;
	float avg_sizer = plugin_data->avg_sizer;
	float avgs = plugin_data->avgs;
	float lp1tm1 = plugin_data->lp1tm1;
	float lp2tm1 = plugin_data->lp2tm1;

#line 48 "valve_rect_1405.xml"
	unsigned long pos;
	float q, x, fx;
	const float dist = dist_p * 40.0f + 0.1f;

	for (pos = 0; pos < sample_count; pos++) {
	  x = fabs(input[pos]);
	  if (x > lp1tm1) {
	    lp1tm1 = x;
	  } else {
	    lp1tm1 = 0.9999f * lp1tm1 + 0.0001f * x;
	  }

	  avgs -= avg[apos];
	  avgs += lp1tm1;
	  avg[apos++] = lp1tm1;
	  apos %= avg_size;

	  lp2tm1 = 0.999f * lp2tm1 + avgs*avg_sizer * 0.001f;
	  q = lp1tm1 * sag - lp2tm1 * 1.02f - 1.0f;
	  if (q > -0.01f) {
	    q = -0.01f;
	  } else if (q < -1.0f) {
	    q = -1.0f;
	  }

	  if (input[pos] == q) {
	    fx = 1.0f / dist + q / (1.0f - f_exp(dist * q));
	  } else {
	    fx = (input[pos] - q) /
	     (1.0f - f_exp(-dist * (input[pos] - q))) +
	     q / (1.0f - f_exp(dist * q));
	  }

	  buffer_write(output[pos], fx);
	}

	plugin_data->lp1tm1 = lp1tm1;
	plugin_data->lp2tm1 = lp2tm1;
	plugin_data->avgs = avgs;
	plugin_data->apos = apos;
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


	valveRectDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (valveRectDescriptor) {
		valveRectDescriptor->UniqueID = 1405;
		valveRectDescriptor->Label = "valveRect";
		valveRectDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		valveRectDescriptor->Name =
		 D_("Valve rectifier");
		valveRectDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		valveRectDescriptor->Copyright =
		 "GPL";
		valveRectDescriptor->PortCount = 4;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(4,
		 sizeof(LADSPA_PortDescriptor));
		valveRectDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(4,
		 sizeof(LADSPA_PortRangeHint));
		valveRectDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(4, sizeof(char*));
		valveRectDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Sag level */
		port_descriptors[VALVERECT_SAG] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[VALVERECT_SAG] =
		 D_("Sag level");
		port_range_hints[VALVERECT_SAG].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[VALVERECT_SAG].LowerBound = 0;
		port_range_hints[VALVERECT_SAG].UpperBound = 1;

		/* Parameters for Distortion */
		port_descriptors[VALVERECT_DIST_P] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[VALVERECT_DIST_P] =
		 D_("Distortion");
		port_range_hints[VALVERECT_DIST_P].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[VALVERECT_DIST_P].LowerBound = 0;
		port_range_hints[VALVERECT_DIST_P].UpperBound = 1;

		/* Parameters for Input */
		port_descriptors[VALVERECT_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[VALVERECT_INPUT] =
		 D_("Input");
		port_range_hints[VALVERECT_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[VALVERECT_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[VALVERECT_OUTPUT] =
		 D_("Output");
		port_range_hints[VALVERECT_OUTPUT].HintDescriptor = 0;

		valveRectDescriptor->activate = activateValveRect;
		valveRectDescriptor->cleanup = cleanupValveRect;
		valveRectDescriptor->connect_port = connectPortValveRect;
		valveRectDescriptor->deactivate = NULL;
		valveRectDescriptor->instantiate = instantiateValveRect;
		valveRectDescriptor->run = runValveRect;
		valveRectDescriptor->run_adding = runAddingValveRect;
		valveRectDescriptor->set_run_adding_gain = setRunAddingGainValveRect;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (valveRectDescriptor) {
		free((LADSPA_PortDescriptor *)valveRectDescriptor->PortDescriptors);
		free((char **)valveRectDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)valveRectDescriptor->PortRangeHints);
		free(valveRectDescriptor);
	}

}
