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


#define ALIAS_LEVEL                    0
#define ALIAS_INPUT                    1
#define ALIAS_OUTPUT                   2

static LADSPA_Descriptor *aliasDescriptor = NULL;

typedef struct {
	LADSPA_Data *level;
	LADSPA_Data *input;
	LADSPA_Data *output;
	LADSPA_Data run_adding_gain;
} Alias;

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
		return aliasDescriptor;
	default:
		return NULL;
	}
}

static void cleanupAlias(LADSPA_Handle instance) {
	free(instance);
}

static void connectPortAlias(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Alias *plugin;

	plugin = (Alias *)instance;
	switch (port) {
	case ALIAS_LEVEL:
		plugin->level = data;
		break;
	case ALIAS_INPUT:
		plugin->input = data;
		break;
	case ALIAS_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateAlias(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Alias *plugin_data = (Alias *)malloc(sizeof(Alias));
	plugin_data->run_adding_gain = 1.0f;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runAlias(LADSPA_Handle instance, unsigned long sample_count) {
	Alias *plugin_data = (Alias *)instance;

	/* Aliasing level (float value) */
	const LADSPA_Data level = *(plugin_data->level);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;

#line 17 "alias_1407.xml"
	unsigned long pos;
	float coef = 1.0f - 2.0f * level;

	if (output != input) {
	  for (pos = 0; pos < sample_count; pos+=2) {
	    buffer_write(output[pos], input[pos]);
	  }
	}
	for (pos = 1; pos < sample_count; pos+=2) {
	  buffer_write(output[pos], input[pos] * coef);
	}
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainAlias(LADSPA_Handle instance, LADSPA_Data gain) {
	((Alias *)instance)->run_adding_gain = gain;
}

static void runAddingAlias(LADSPA_Handle instance, unsigned long sample_count) {
	Alias *plugin_data = (Alias *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Aliasing level (float value) */
	const LADSPA_Data level = *(plugin_data->level);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;

#line 17 "alias_1407.xml"
	unsigned long pos;
	float coef = 1.0f - 2.0f * level;

	if (output != input) {
	  for (pos = 0; pos < sample_count; pos+=2) {
	    buffer_write(output[pos], input[pos]);
	  }
	}
	for (pos = 1; pos < sample_count; pos+=2) {
	  buffer_write(output[pos], input[pos] * coef);
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


	aliasDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (aliasDescriptor) {
		aliasDescriptor->UniqueID = 1407;
		aliasDescriptor->Label = "alias";
		aliasDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		aliasDescriptor->Name =
		 D_("Aliasing");
		aliasDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		aliasDescriptor->Copyright =
		 "GPL";
		aliasDescriptor->PortCount = 3;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(3,
		 sizeof(LADSPA_PortDescriptor));
		aliasDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(3,
		 sizeof(LADSPA_PortRangeHint));
		aliasDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(3, sizeof(char*));
		aliasDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Aliasing level */
		port_descriptors[ALIAS_LEVEL] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[ALIAS_LEVEL] =
		 D_("Aliasing level");
		port_range_hints[ALIAS_LEVEL].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[ALIAS_LEVEL].LowerBound = 0;
		port_range_hints[ALIAS_LEVEL].UpperBound = 1;

		/* Parameters for Input */
		port_descriptors[ALIAS_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[ALIAS_INPUT] =
		 D_("Input");
		port_range_hints[ALIAS_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[ALIAS_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[ALIAS_OUTPUT] =
		 D_("Output");
		port_range_hints[ALIAS_OUTPUT].HintDescriptor = 0;

		aliasDescriptor->activate = NULL;
		aliasDescriptor->cleanup = cleanupAlias;
		aliasDescriptor->connect_port = connectPortAlias;
		aliasDescriptor->deactivate = NULL;
		aliasDescriptor->instantiate = instantiateAlias;
		aliasDescriptor->run = runAlias;
		aliasDescriptor->run_adding = runAddingAlias;
		aliasDescriptor->set_run_adding_gain = setRunAddingGainAlias;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (aliasDescriptor) {
		free((LADSPA_PortDescriptor *)aliasDescriptor->PortDescriptors);
		free((char **)aliasDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)aliasDescriptor->PortRangeHints);
		free(aliasDescriptor);
	}

}
