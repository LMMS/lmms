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

#line 10 "valve_1209.xml"

#include "ladspa-util.h"

#define VALVE_Q_P                      0
#define VALVE_DIST_P                   1
#define VALVE_INPUT                    2
#define VALVE_OUTPUT                   3

static LADSPA_Descriptor *valveDescriptor = NULL;

typedef struct {
	LADSPA_Data *q_p;
	LADSPA_Data *dist_p;
	LADSPA_Data *input;
	LADSPA_Data *output;
	LADSPA_Data  itm1;
	LADSPA_Data  otm1;
	LADSPA_Data run_adding_gain;
} Valve;

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
		return valveDescriptor;
	default:
		return NULL;
	}
}

static void activateValve(LADSPA_Handle instance) {
	Valve *plugin_data = (Valve *)instance;
	LADSPA_Data itm1 = plugin_data->itm1;
	LADSPA_Data otm1 = plugin_data->otm1;
#line 21 "valve_1209.xml"
	itm1 = 0.0f;
	otm1 = 0.0f;
	plugin_data->itm1 = itm1;
	plugin_data->otm1 = otm1;

}

static void cleanupValve(LADSPA_Handle instance) {
	free(instance);
}

static void connectPortValve(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Valve *plugin;

	plugin = (Valve *)instance;
	switch (port) {
	case VALVE_Q_P:
		plugin->q_p = data;
		break;
	case VALVE_DIST_P:
		plugin->dist_p = data;
		break;
	case VALVE_INPUT:
		plugin->input = data;
		break;
	case VALVE_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateValve(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Valve *plugin_data = (Valve *)malloc(sizeof(Valve));
	plugin_data->run_adding_gain = 1.0f;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runValve(LADSPA_Handle instance, unsigned long sample_count) {
	Valve *plugin_data = (Valve *)instance;

	/* Distortion level (float value) */
	const LADSPA_Data q_p = *(plugin_data->q_p);

	/* Distortion character (float value) */
	const LADSPA_Data dist_p = *(plugin_data->dist_p);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	LADSPA_Data itm1 = plugin_data->itm1;
	LADSPA_Data otm1 = plugin_data->otm1;

#line 26 "valve_1209.xml"
	unsigned long pos;
	LADSPA_Data fx;
	
	const float q = q_p - 0.999f;
	const float dist = dist_p * 40.0f + 0.1f;
	
	if (q == 0.0f) {
	        for (pos = 0; pos < sample_count; pos++) {
	                if (input[pos] == q) {
	                        fx = 1.0f / dist;
	                } else {
	                        fx = input[pos] / (1.0f - f_exp(-dist * input[pos]));
	                }
	                otm1 = 0.999f * otm1 + fx - itm1;
	                round_to_zero(&otm1);
	                itm1 = fx;
	                buffer_write(output[pos], otm1);
	        }
	} else {
	        for (pos = 0; pos < sample_count; pos++) {
	                if (input[pos] == q) {
	                        fx = 1.0f / dist + q / (1.0f - f_exp(dist * q));
	                } else {
	                        fx = (input[pos] - q) /
	                         (1.0f - f_exp(-dist * (input[pos] - q))) +
	                         q / (1.0f - f_exp(dist * q));
	                }
	                otm1 = 0.999f * otm1 + fx - itm1;
	                round_to_zero(&otm1);
	                itm1 = fx;
	                buffer_write(output[pos], otm1);
	        }
	}
	
	plugin_data->itm1 = itm1;
	plugin_data->otm1 = otm1;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainValve(LADSPA_Handle instance, LADSPA_Data gain) {
	((Valve *)instance)->run_adding_gain = gain;
}

static void runAddingValve(LADSPA_Handle instance, unsigned long sample_count) {
	Valve *plugin_data = (Valve *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Distortion level (float value) */
	const LADSPA_Data q_p = *(plugin_data->q_p);

	/* Distortion character (float value) */
	const LADSPA_Data dist_p = *(plugin_data->dist_p);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	LADSPA_Data itm1 = plugin_data->itm1;
	LADSPA_Data otm1 = plugin_data->otm1;

#line 26 "valve_1209.xml"
	unsigned long pos;
	LADSPA_Data fx;
	
	const float q = q_p - 0.999f;
	const float dist = dist_p * 40.0f + 0.1f;
	
	if (q == 0.0f) {
	        for (pos = 0; pos < sample_count; pos++) {
	                if (input[pos] == q) {
	                        fx = 1.0f / dist;
	                } else {
	                        fx = input[pos] / (1.0f - f_exp(-dist * input[pos]));
	                }
	                otm1 = 0.999f * otm1 + fx - itm1;
	                round_to_zero(&otm1);
	                itm1 = fx;
	                buffer_write(output[pos], otm1);
	        }
	} else {
	        for (pos = 0; pos < sample_count; pos++) {
	                if (input[pos] == q) {
	                        fx = 1.0f / dist + q / (1.0f - f_exp(dist * q));
	                } else {
	                        fx = (input[pos] - q) /
	                         (1.0f - f_exp(-dist * (input[pos] - q))) +
	                         q / (1.0f - f_exp(dist * q));
	                }
	                otm1 = 0.999f * otm1 + fx - itm1;
	                round_to_zero(&otm1);
	                itm1 = fx;
	                buffer_write(output[pos], otm1);
	        }
	}
	
	plugin_data->itm1 = itm1;
	plugin_data->otm1 = otm1;
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


	valveDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (valveDescriptor) {
		valveDescriptor->UniqueID = 1209;
		valveDescriptor->Label = "valve";
		valveDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		valveDescriptor->Name =
		 D_("Valve saturation");
		valveDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		valveDescriptor->Copyright =
		 "GPL";
		valveDescriptor->PortCount = 4;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(4,
		 sizeof(LADSPA_PortDescriptor));
		valveDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(4,
		 sizeof(LADSPA_PortRangeHint));
		valveDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(4, sizeof(char*));
		valveDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Distortion level */
		port_descriptors[VALVE_Q_P] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[VALVE_Q_P] =
		 D_("Distortion level");
		port_range_hints[VALVE_Q_P].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[VALVE_Q_P].LowerBound = 0;
		port_range_hints[VALVE_Q_P].UpperBound = 1;

		/* Parameters for Distortion character */
		port_descriptors[VALVE_DIST_P] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[VALVE_DIST_P] =
		 D_("Distortion character");
		port_range_hints[VALVE_DIST_P].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[VALVE_DIST_P].LowerBound = 0;
		port_range_hints[VALVE_DIST_P].UpperBound = 1;

		/* Parameters for Input */
		port_descriptors[VALVE_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[VALVE_INPUT] =
		 D_("Input");
		port_range_hints[VALVE_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[VALVE_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[VALVE_OUTPUT] =
		 D_("Output");
		port_range_hints[VALVE_OUTPUT].HintDescriptor = 0;

		valveDescriptor->activate = activateValve;
		valveDescriptor->cleanup = cleanupValve;
		valveDescriptor->connect_port = connectPortValve;
		valveDescriptor->deactivate = NULL;
		valveDescriptor->instantiate = instantiateValve;
		valveDescriptor->run = runValve;
		valveDescriptor->run_adding = runAddingValve;
		valveDescriptor->set_run_adding_gain = setRunAddingGainValve;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (valveDescriptor) {
		free((LADSPA_PortDescriptor *)valveDescriptor->PortDescriptors);
		free((char **)valveDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)valveDescriptor->PortRangeHints);
		free(valveDescriptor);
	}

}
