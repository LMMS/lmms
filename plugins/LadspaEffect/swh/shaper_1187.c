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


#define SHAPER_SHAPEP                  0
#define SHAPER_INPUT                   1
#define SHAPER_OUTPUT                  2

static LADSPA_Descriptor *shaperDescriptor = NULL;

typedef struct {
	LADSPA_Data *shapep;
	LADSPA_Data *input;
	LADSPA_Data *output;
	LADSPA_Data run_adding_gain;
} Shaper;

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
		return shaperDescriptor;
	default:
		return NULL;
	}
}

static void cleanupShaper(LADSPA_Handle instance) {
	free(instance);
}

static void connectPortShaper(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Shaper *plugin;

	plugin = (Shaper *)instance;
	switch (port) {
	case SHAPER_SHAPEP:
		plugin->shapep = data;
		break;
	case SHAPER_INPUT:
		plugin->input = data;
		break;
	case SHAPER_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateShaper(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Shaper *plugin_data = (Shaper *)malloc(sizeof(Shaper));
	plugin_data->run_adding_gain = 1.0f;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runShaper(LADSPA_Handle instance, unsigned long sample_count) {
	Shaper *plugin_data = (Shaper *)instance;

	/* Waveshape (float value) */
	const LADSPA_Data shapep = *(plugin_data->shapep);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;

#line 17 "shaper_1187.xml"
	int pos;
	float shape = 0.0f;
	
	if (shapep < 1.0f && shapep > -1.0f) {
	        shape = 1.0f;
	} else if (shape < 0) {
	        shape = -1.0f / shape;
	} else {
	        shape = shapep;
	}
	
	for (pos = 0; pos < sample_count; pos++) {
	        if (input[pos] < 0.0f) {
	                buffer_write(output[pos], -pow(-input[pos], shape));
	        } else {
	                buffer_write(output[pos], pow(input[pos], shape));
	        }
	}
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainShaper(LADSPA_Handle instance, LADSPA_Data gain) {
	((Shaper *)instance)->run_adding_gain = gain;
}

static void runAddingShaper(LADSPA_Handle instance, unsigned long sample_count) {
	Shaper *plugin_data = (Shaper *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Waveshape (float value) */
	const LADSPA_Data shapep = *(plugin_data->shapep);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;

#line 17 "shaper_1187.xml"
	int pos;
	float shape = 0.0f;
	
	if (shapep < 1.0f && shapep > -1.0f) {
	        shape = 1.0f;
	} else if (shape < 0) {
	        shape = -1.0f / shape;
	} else {
	        shape = shapep;
	}
	
	for (pos = 0; pos < sample_count; pos++) {
	        if (input[pos] < 0.0f) {
	                buffer_write(output[pos], -pow(-input[pos], shape));
	        } else {
	                buffer_write(output[pos], pow(input[pos], shape));
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


	shaperDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (shaperDescriptor) {
		shaperDescriptor->UniqueID = 1187;
		shaperDescriptor->Label = "shaper";
		shaperDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		shaperDescriptor->Name =
		 D_("Wave shaper");
		shaperDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		shaperDescriptor->Copyright =
		 "GPL";
		shaperDescriptor->PortCount = 3;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(3,
		 sizeof(LADSPA_PortDescriptor));
		shaperDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(3,
		 sizeof(LADSPA_PortRangeHint));
		shaperDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(3, sizeof(char*));
		shaperDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Waveshape */
		port_descriptors[SHAPER_SHAPEP] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[SHAPER_SHAPEP] =
		 D_("Waveshape");
		port_range_hints[SHAPER_SHAPEP].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[SHAPER_SHAPEP].LowerBound = -10;
		port_range_hints[SHAPER_SHAPEP].UpperBound = +10;

		/* Parameters for Input */
		port_descriptors[SHAPER_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[SHAPER_INPUT] =
		 D_("Input");
		port_range_hints[SHAPER_INPUT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[SHAPER_INPUT].LowerBound = -1;
		port_range_hints[SHAPER_INPUT].UpperBound = +1;

		/* Parameters for Output */
		port_descriptors[SHAPER_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[SHAPER_OUTPUT] =
		 D_("Output");
		port_range_hints[SHAPER_OUTPUT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[SHAPER_OUTPUT].LowerBound = -1;
		port_range_hints[SHAPER_OUTPUT].UpperBound = +1;

		shaperDescriptor->activate = NULL;
		shaperDescriptor->cleanup = cleanupShaper;
		shaperDescriptor->connect_port = connectPortShaper;
		shaperDescriptor->deactivate = NULL;
		shaperDescriptor->instantiate = instantiateShaper;
		shaperDescriptor->run = runShaper;
		shaperDescriptor->run_adding = runAddingShaper;
		shaperDescriptor->set_run_adding_gain = setRunAddingGainShaper;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (shaperDescriptor) {
		free((LADSPA_PortDescriptor *)shaperDescriptor->PortDescriptors);
		free((char **)shaperDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)shaperDescriptor->PortRangeHints);
		free(shaperDescriptor);
	}

}
