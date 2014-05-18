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


#define DIODE_MODE                     0
#define DIODE_INPUT                    1
#define DIODE_OUTPUT                   2

static LADSPA_Descriptor *diodeDescriptor = NULL;

typedef struct {
	LADSPA_Data *mode;
	LADSPA_Data *input;
	LADSPA_Data *output;
	LADSPA_Data run_adding_gain;
} Diode;

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
		return diodeDescriptor;
	default:
		return NULL;
	}
}

static void cleanupDiode(LADSPA_Handle instance) {
	free(instance);
}

static void connectPortDiode(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Diode *plugin;

	plugin = (Diode *)instance;
	switch (port) {
	case DIODE_MODE:
		plugin->mode = data;
		break;
	case DIODE_INPUT:
		plugin->input = data;
		break;
	case DIODE_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateDiode(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Diode *plugin_data = (Diode *)malloc(sizeof(Diode));
	plugin_data->run_adding_gain = 1.0f;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runDiode(LADSPA_Handle instance, unsigned long sample_count) {
	Diode *plugin_data = (Diode *)instance;

	/* Mode (0 for none, 1 for half wave, 2 for full wave) (float value) */
	const LADSPA_Data mode = *(plugin_data->mode);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;

#line 17 "diode_1185.xml"
	unsigned long pos;
	
	if (mode >= 0.0f && mode < 1.0f) {
	        for (pos = 0; pos < sample_count; pos++) {
	                buffer_write(output[pos], ((1.0f-mode) * input[pos]) +
	                 (mode * (input[pos] > 0.0f ? input[pos] : 0.0f)));
	        }
	} else if (mode >= 1.0f && mode < 2.0f) {
	        float fac = mode - 1.0f;
	        for (pos = 0; pos < sample_count; pos++) {
	                buffer_write(output[pos], ((1.0f-fac) * (input[pos] > 0 ?
	                 input[pos] : 0.0)) + (fac * fabs(input[pos])));
	        }
	} else if (mode >= 2) {
	        float fac = mode < 3 ? mode - 2 : 1.0;
	        for (pos = 0; pos < sample_count; pos++) {
	                buffer_write(output[pos], (1.0-fac) * fabs(input[pos]));
	        }
	} else {
	        for (pos = 0; pos < sample_count; pos++) {
	                buffer_write(output[pos], input[pos]);
	        }
	}
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainDiode(LADSPA_Handle instance, LADSPA_Data gain) {
	((Diode *)instance)->run_adding_gain = gain;
}

static void runAddingDiode(LADSPA_Handle instance, unsigned long sample_count) {
	Diode *plugin_data = (Diode *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Mode (0 for none, 1 for half wave, 2 for full wave) (float value) */
	const LADSPA_Data mode = *(plugin_data->mode);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;

#line 17 "diode_1185.xml"
	unsigned long pos;
	
	if (mode >= 0.0f && mode < 1.0f) {
	        for (pos = 0; pos < sample_count; pos++) {
	                buffer_write(output[pos], ((1.0f-mode) * input[pos]) +
	                 (mode * (input[pos] > 0.0f ? input[pos] : 0.0f)));
	        }
	} else if (mode >= 1.0f && mode < 2.0f) {
	        float fac = mode - 1.0f;
	        for (pos = 0; pos < sample_count; pos++) {
	                buffer_write(output[pos], ((1.0f-fac) * (input[pos] > 0 ?
	                 input[pos] : 0.0)) + (fac * fabs(input[pos])));
	        }
	} else if (mode >= 2) {
	        float fac = mode < 3 ? mode - 2 : 1.0;
	        for (pos = 0; pos < sample_count; pos++) {
	                buffer_write(output[pos], (1.0-fac) * fabs(input[pos]));
	        }
	} else {
	        for (pos = 0; pos < sample_count; pos++) {
	                buffer_write(output[pos], input[pos]);
	        }
	}
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


	diodeDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (diodeDescriptor) {
		diodeDescriptor->UniqueID = 1185;
		diodeDescriptor->Label = "diode";
		diodeDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		diodeDescriptor->Name =
		 D_("Diode Processor");
		diodeDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		diodeDescriptor->Copyright =
		 "GPL";
		diodeDescriptor->PortCount = 3;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(3,
		 sizeof(LADSPA_PortDescriptor));
		diodeDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(3,
		 sizeof(LADSPA_PortRangeHint));
		diodeDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(3, sizeof(char*));
		diodeDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Mode (0 for none, 1 for half wave, 2 for full wave) */
		port_descriptors[DIODE_MODE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[DIODE_MODE] =
		 D_("Mode (0 for none, 1 for half wave, 2 for full wave)");
		port_range_hints[DIODE_MODE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[DIODE_MODE].LowerBound = 0;
		port_range_hints[DIODE_MODE].UpperBound = 3;

		/* Parameters for Input */
		port_descriptors[DIODE_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[DIODE_INPUT] =
		 D_("Input");
		port_range_hints[DIODE_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[DIODE_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[DIODE_OUTPUT] =
		 D_("Output");
		port_range_hints[DIODE_OUTPUT].HintDescriptor = 0;

		diodeDescriptor->activate = NULL;
		diodeDescriptor->cleanup = cleanupDiode;
		diodeDescriptor->connect_port = connectPortDiode;
		diodeDescriptor->deactivate = NULL;
		diodeDescriptor->instantiate = instantiateDiode;
		diodeDescriptor->run = runDiode;
		diodeDescriptor->run_adding = runAddingDiode;
		diodeDescriptor->set_run_adding_gain = setRunAddingGainDiode;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (diodeDescriptor) {
		free((LADSPA_PortDescriptor *)diodeDescriptor->PortDescriptors);
		free((char **)diodeDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)diodeDescriptor->PortRangeHints);
		free(diodeDescriptor);
	}

}
