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


#define INV_INPUT                      0
#define INV_OUTPUT                     1

static LADSPA_Descriptor *invDescriptor = NULL;

typedef struct {
	LADSPA_Data *input;
	LADSPA_Data *output;
	LADSPA_Data run_adding_gain;
} Inv;

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
		return invDescriptor;
	default:
		return NULL;
	}
}

static void cleanupInv(LADSPA_Handle instance) {
	free(instance);
}

static void connectPortInv(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Inv *plugin;

	plugin = (Inv *)instance;
	switch (port) {
	case INV_INPUT:
		plugin->input = data;
		break;
	case INV_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateInv(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Inv *plugin_data = (Inv *)malloc(sizeof(Inv));
	plugin_data->run_adding_gain = 1.0f;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runInv(LADSPA_Handle instance, unsigned long sample_count) {
	Inv *plugin_data = (Inv *)instance;

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;

#line 17 "inv_1429.xml"
	unsigned long pos;

	for (pos = 0; pos < sample_count; pos++) {
	  buffer_write(output[pos], -input[pos]);
	}
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainInv(LADSPA_Handle instance, LADSPA_Data gain) {
	((Inv *)instance)->run_adding_gain = gain;
}

static void runAddingInv(LADSPA_Handle instance, unsigned long sample_count) {
	Inv *plugin_data = (Inv *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;

#line 17 "inv_1429.xml"
	unsigned long pos;

	for (pos = 0; pos < sample_count; pos++) {
	  buffer_write(output[pos], -input[pos]);
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


	invDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (invDescriptor) {
		invDescriptor->UniqueID = 1429;
		invDescriptor->Label = "inv";
		invDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		invDescriptor->Name =
		 D_("Inverter");
		invDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		invDescriptor->Copyright =
		 "GPL";
		invDescriptor->PortCount = 2;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(2,
		 sizeof(LADSPA_PortDescriptor));
		invDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(2,
		 sizeof(LADSPA_PortRangeHint));
		invDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(2, sizeof(char*));
		invDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Input */
		port_descriptors[INV_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[INV_INPUT] =
		 D_("Input");
		port_range_hints[INV_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[INV_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[INV_OUTPUT] =
		 D_("Output");
		port_range_hints[INV_OUTPUT].HintDescriptor = 0;

		invDescriptor->activate = NULL;
		invDescriptor->cleanup = cleanupInv;
		invDescriptor->connect_port = connectPortInv;
		invDescriptor->deactivate = NULL;
		invDescriptor->instantiate = instantiateInv;
		invDescriptor->run = runInv;
		invDescriptor->run_adding = runAddingInv;
		invDescriptor->set_run_adding_gain = setRunAddingGainInv;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (invDescriptor) {
		free((LADSPA_PortDescriptor *)invDescriptor->PortDescriptors);
		free((char **)invDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)invDescriptor->PortRangeHints);
		free(invDescriptor);
	}

}
