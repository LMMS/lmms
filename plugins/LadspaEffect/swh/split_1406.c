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


#define SPLIT_INPUT                    0
#define SPLIT_OUT2                     1
#define SPLIT_OUT1                     2

static LADSPA_Descriptor *splitDescriptor = NULL;

typedef struct {
	LADSPA_Data *input;
	LADSPA_Data *out2;
	LADSPA_Data *out1;
	LADSPA_Data run_adding_gain;
} Split;

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
		return splitDescriptor;
	default:
		return NULL;
	}
}

static void cleanupSplit(LADSPA_Handle instance) {
	free(instance);
}

static void connectPortSplit(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Split *plugin;

	plugin = (Split *)instance;
	switch (port) {
	case SPLIT_INPUT:
		plugin->input = data;
		break;
	case SPLIT_OUT2:
		plugin->out2 = data;
		break;
	case SPLIT_OUT1:
		plugin->out1 = data;
		break;
	}
}

static LADSPA_Handle instantiateSplit(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Split *plugin_data = (Split *)malloc(sizeof(Split));
	plugin_data->run_adding_gain = 1.0f;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runSplit(LADSPA_Handle instance, unsigned long sample_count) {
	Split *plugin_data = (Split *)instance;

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output 1 (array of floats of length sample_count) */
	LADSPA_Data * const out2 = plugin_data->out2;

	/* Output 2 (array of floats of length sample_count) */
	LADSPA_Data * const out1 = plugin_data->out1;

#line 16 "split_1406.xml"
	unsigned long pos;

	for (pos = 0; pos < sample_count; pos++) {
	        const LADSPA_Data in = input[pos];

	        buffer_write(out1[pos], in);
	        buffer_write(out2[pos], in);
	}
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainSplit(LADSPA_Handle instance, LADSPA_Data gain) {
	((Split *)instance)->run_adding_gain = gain;
}

static void runAddingSplit(LADSPA_Handle instance, unsigned long sample_count) {
	Split *plugin_data = (Split *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output 1 (array of floats of length sample_count) */
	LADSPA_Data * const out2 = plugin_data->out2;

	/* Output 2 (array of floats of length sample_count) */
	LADSPA_Data * const out1 = plugin_data->out1;

#line 16 "split_1406.xml"
	unsigned long pos;

	for (pos = 0; pos < sample_count; pos++) {
	        const LADSPA_Data in = input[pos];

	        buffer_write(out1[pos], in);
	        buffer_write(out2[pos], in);
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


	splitDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (splitDescriptor) {
		splitDescriptor->UniqueID = 1406;
		splitDescriptor->Label = "split";
		splitDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		splitDescriptor->Name =
		 D_("Mono to Stereo splitter");
		splitDescriptor->Maker =
		 "Frank Neumann <franky@users.sourceforge.net>";
		splitDescriptor->Copyright =
		 "GPL";
		splitDescriptor->PortCount = 3;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(3,
		 sizeof(LADSPA_PortDescriptor));
		splitDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(3,
		 sizeof(LADSPA_PortRangeHint));
		splitDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(3, sizeof(char*));
		splitDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Input */
		port_descriptors[SPLIT_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[SPLIT_INPUT] =
		 D_("Input");
		port_range_hints[SPLIT_INPUT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[SPLIT_INPUT].LowerBound = -1;
		port_range_hints[SPLIT_INPUT].UpperBound = +1;

		/* Parameters for Output 1 */
		port_descriptors[SPLIT_OUT2] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[SPLIT_OUT2] =
		 D_("Output 1");
		port_range_hints[SPLIT_OUT2].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[SPLIT_OUT2].LowerBound = -1;
		port_range_hints[SPLIT_OUT2].UpperBound = +1;

		/* Parameters for Output 2 */
		port_descriptors[SPLIT_OUT1] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[SPLIT_OUT1] =
		 D_("Output 2");
		port_range_hints[SPLIT_OUT1].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[SPLIT_OUT1].LowerBound = -1;
		port_range_hints[SPLIT_OUT1].UpperBound = +1;

		splitDescriptor->activate = NULL;
		splitDescriptor->cleanup = cleanupSplit;
		splitDescriptor->connect_port = connectPortSplit;
		splitDescriptor->deactivate = NULL;
		splitDescriptor->instantiate = instantiateSplit;
		splitDescriptor->run = runSplit;
		splitDescriptor->run_adding = runAddingSplit;
		splitDescriptor->set_run_adding_gain = setRunAddingGainSplit;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (splitDescriptor) {
		free((LADSPA_PortDescriptor *)splitDescriptor->PortDescriptors);
		free((char **)splitDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)splitDescriptor->PortRangeHints);
		free(splitDescriptor);
	}

}
