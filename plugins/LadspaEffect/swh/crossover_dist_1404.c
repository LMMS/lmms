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

#line 10 "crossover_dist_1404.xml"

#include "ladspa-util.h"

#define CROSSOVERDIST_AMP              0
#define CROSSOVERDIST_SMOOTH           1
#define CROSSOVERDIST_INPUT            2
#define CROSSOVERDIST_OUTPUT           3

static LADSPA_Descriptor *crossoverDistDescriptor = NULL;

typedef struct {
	LADSPA_Data *amp;
	LADSPA_Data *smooth;
	LADSPA_Data *input;
	LADSPA_Data *output;
	LADSPA_Data run_adding_gain;
} CrossoverDist;

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
		return crossoverDistDescriptor;
	default:
		return NULL;
	}
}

static void cleanupCrossoverDist(LADSPA_Handle instance) {
	free(instance);
}

static void connectPortCrossoverDist(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	CrossoverDist *plugin;

	plugin = (CrossoverDist *)instance;
	switch (port) {
	case CROSSOVERDIST_AMP:
		plugin->amp = data;
		break;
	case CROSSOVERDIST_SMOOTH:
		plugin->smooth = data;
		break;
	case CROSSOVERDIST_INPUT:
		plugin->input = data;
		break;
	case CROSSOVERDIST_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateCrossoverDist(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	CrossoverDist *plugin_data = (CrossoverDist *)malloc(sizeof(CrossoverDist));
	plugin_data->run_adding_gain = 1.0f;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runCrossoverDist(LADSPA_Handle instance, unsigned long sample_count) {
	CrossoverDist *plugin_data = (CrossoverDist *)instance;

	/* Crossover amplitude (float value) */
	const LADSPA_Data amp = *(plugin_data->amp);

	/* Smoothing (float value) */
	const LADSPA_Data smooth = *(plugin_data->smooth);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;

#line 21 "crossover_dist_1404.xml"
	unsigned long pos;
	float sig;
	const float fade = fabs(amp * smooth) + 0.0001;

	for (pos = 0; pos < sample_count; pos++) {
	  sig = fabs(input[pos]) - amp;

	  if (sig < 0.0f) {
	    sig *= (1.0f + sig/fade) * smooth;
	  }

	  if (input[pos] < 0.0f) {
	    buffer_write(output[pos], -sig);
	  } else {
	    buffer_write(output[pos], sig);
	  }
	}
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainCrossoverDist(LADSPA_Handle instance, LADSPA_Data gain) {
	((CrossoverDist *)instance)->run_adding_gain = gain;
}

static void runAddingCrossoverDist(LADSPA_Handle instance, unsigned long sample_count) {
	CrossoverDist *plugin_data = (CrossoverDist *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Crossover amplitude (float value) */
	const LADSPA_Data amp = *(plugin_data->amp);

	/* Smoothing (float value) */
	const LADSPA_Data smooth = *(plugin_data->smooth);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;

#line 21 "crossover_dist_1404.xml"
	unsigned long pos;
	float sig;
	const float fade = fabs(amp * smooth) + 0.0001;

	for (pos = 0; pos < sample_count; pos++) {
	  sig = fabs(input[pos]) - amp;

	  if (sig < 0.0f) {
	    sig *= (1.0f + sig/fade) * smooth;
	  }

	  if (input[pos] < 0.0f) {
	    buffer_write(output[pos], -sig);
	  } else {
	    buffer_write(output[pos], sig);
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


	crossoverDistDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (crossoverDistDescriptor) {
		crossoverDistDescriptor->UniqueID = 1404;
		crossoverDistDescriptor->Label = "crossoverDist";
		crossoverDistDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		crossoverDistDescriptor->Name =
		 D_("Crossover distortion");
		crossoverDistDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		crossoverDistDescriptor->Copyright =
		 "GPL";
		crossoverDistDescriptor->PortCount = 4;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(4,
		 sizeof(LADSPA_PortDescriptor));
		crossoverDistDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(4,
		 sizeof(LADSPA_PortRangeHint));
		crossoverDistDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(4, sizeof(char*));
		crossoverDistDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Crossover amplitude */
		port_descriptors[CROSSOVERDIST_AMP] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[CROSSOVERDIST_AMP] =
		 D_("Crossover amplitude");
		port_range_hints[CROSSOVERDIST_AMP].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MINIMUM;
		port_range_hints[CROSSOVERDIST_AMP].LowerBound = 0;
		port_range_hints[CROSSOVERDIST_AMP].UpperBound = 0.1;

		/* Parameters for Smoothing */
		port_descriptors[CROSSOVERDIST_SMOOTH] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[CROSSOVERDIST_SMOOTH] =
		 D_("Smoothing");
		port_range_hints[CROSSOVERDIST_SMOOTH].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MAXIMUM;
		port_range_hints[CROSSOVERDIST_SMOOTH].LowerBound = 0;
		port_range_hints[CROSSOVERDIST_SMOOTH].UpperBound = 1;

		/* Parameters for Input */
		port_descriptors[CROSSOVERDIST_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[CROSSOVERDIST_INPUT] =
		 D_("Input");
		port_range_hints[CROSSOVERDIST_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[CROSSOVERDIST_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[CROSSOVERDIST_OUTPUT] =
		 D_("Output");
		port_range_hints[CROSSOVERDIST_OUTPUT].HintDescriptor = 0;

		crossoverDistDescriptor->activate = NULL;
		crossoverDistDescriptor->cleanup = cleanupCrossoverDist;
		crossoverDistDescriptor->connect_port = connectPortCrossoverDist;
		crossoverDistDescriptor->deactivate = NULL;
		crossoverDistDescriptor->instantiate = instantiateCrossoverDist;
		crossoverDistDescriptor->run = runCrossoverDist;
		crossoverDistDescriptor->run_adding = runAddingCrossoverDist;
		crossoverDistDescriptor->set_run_adding_gain = setRunAddingGainCrossoverDist;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (crossoverDistDescriptor) {
		free((LADSPA_PortDescriptor *)crossoverDistDescriptor->PortDescriptors);
		free((char **)crossoverDistDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)crossoverDistDescriptor->PortRangeHints);
		free(crossoverDistDescriptor);
	}

}
