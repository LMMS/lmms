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


#define FOLDOVER_DRIVE_P               0
#define FOLDOVER_PUSH                  1
#define FOLDOVER_INPUT                 2
#define FOLDOVER_OUTPUT                3

static LADSPA_Descriptor *foldoverDescriptor = NULL;

typedef struct {
	LADSPA_Data *drive_p;
	LADSPA_Data *push;
	LADSPA_Data *input;
	LADSPA_Data *output;
	LADSPA_Data run_adding_gain;
} Foldover;

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
		return foldoverDescriptor;
	default:
		return NULL;
	}
}

static void cleanupFoldover(LADSPA_Handle instance) {
	free(instance);
}

static void connectPortFoldover(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Foldover *plugin;

	plugin = (Foldover *)instance;
	switch (port) {
	case FOLDOVER_DRIVE_P:
		plugin->drive_p = data;
		break;
	case FOLDOVER_PUSH:
		plugin->push = data;
		break;
	case FOLDOVER_INPUT:
		plugin->input = data;
		break;
	case FOLDOVER_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateFoldover(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Foldover *plugin_data = (Foldover *)malloc(sizeof(Foldover));
	plugin_data->run_adding_gain = 1.0f;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runFoldover(LADSPA_Handle instance, unsigned long sample_count) {
	Foldover *plugin_data = (Foldover *)instance;

	/* Drive (float value) */
	const LADSPA_Data drive_p = *(plugin_data->drive_p);

	/* Skew (float value) */
	const LADSPA_Data push = *(plugin_data->push);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;

#line 18 "foldover_1213.xml"
	unsigned long pos;
	float x;
	const float drive = drive_p + 1.0f;

	for (pos = 0; pos < sample_count; pos++) {
	  x = input[pos] * drive + push;
	  buffer_write(output[pos], 1.5f * x - 0.5f * x * x * x);
	}
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainFoldover(LADSPA_Handle instance, LADSPA_Data gain) {
	((Foldover *)instance)->run_adding_gain = gain;
}

static void runAddingFoldover(LADSPA_Handle instance, unsigned long sample_count) {
	Foldover *plugin_data = (Foldover *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Drive (float value) */
	const LADSPA_Data drive_p = *(plugin_data->drive_p);

	/* Skew (float value) */
	const LADSPA_Data push = *(plugin_data->push);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;

#line 18 "foldover_1213.xml"
	unsigned long pos;
	float x;
	const float drive = drive_p + 1.0f;

	for (pos = 0; pos < sample_count; pos++) {
	  x = input[pos] * drive + push;
	  buffer_write(output[pos], 1.5f * x - 0.5f * x * x * x);
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


	foldoverDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (foldoverDescriptor) {
		foldoverDescriptor->UniqueID = 1213;
		foldoverDescriptor->Label = "foldover";
		foldoverDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		foldoverDescriptor->Name =
		 D_("Foldover distortion");
		foldoverDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		foldoverDescriptor->Copyright =
		 "GPL";
		foldoverDescriptor->PortCount = 4;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(4,
		 sizeof(LADSPA_PortDescriptor));
		foldoverDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(4,
		 sizeof(LADSPA_PortRangeHint));
		foldoverDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(4, sizeof(char*));
		foldoverDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Drive */
		port_descriptors[FOLDOVER_DRIVE_P] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[FOLDOVER_DRIVE_P] =
		 D_("Drive");
		port_range_hints[FOLDOVER_DRIVE_P].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[FOLDOVER_DRIVE_P].LowerBound = 0;
		port_range_hints[FOLDOVER_DRIVE_P].UpperBound = 1;

		/* Parameters for Skew */
		port_descriptors[FOLDOVER_PUSH] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[FOLDOVER_PUSH] =
		 D_("Skew");
		port_range_hints[FOLDOVER_PUSH].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[FOLDOVER_PUSH].LowerBound = 0;
		port_range_hints[FOLDOVER_PUSH].UpperBound = 1;

		/* Parameters for Input */
		port_descriptors[FOLDOVER_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[FOLDOVER_INPUT] =
		 D_("Input");
		port_range_hints[FOLDOVER_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[FOLDOVER_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[FOLDOVER_OUTPUT] =
		 D_("Output");
		port_range_hints[FOLDOVER_OUTPUT].HintDescriptor = 0;

		foldoverDescriptor->activate = NULL;
		foldoverDescriptor->cleanup = cleanupFoldover;
		foldoverDescriptor->connect_port = connectPortFoldover;
		foldoverDescriptor->deactivate = NULL;
		foldoverDescriptor->instantiate = instantiateFoldover;
		foldoverDescriptor->run = runFoldover;
		foldoverDescriptor->run_adding = runAddingFoldover;
		foldoverDescriptor->set_run_adding_gain = setRunAddingGainFoldover;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (foldoverDescriptor) {
		free((LADSPA_PortDescriptor *)foldoverDescriptor->PortDescriptors);
		free((char **)foldoverDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)foldoverDescriptor->PortRangeHints);
		free(foldoverDescriptor);
	}

}
