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


#define DCREMOVE_INPUT                 0
#define DCREMOVE_OUTPUT                1

static LADSPA_Descriptor *dcRemoveDescriptor = NULL;

typedef struct {
	LADSPA_Data *input;
	LADSPA_Data *output;
	LADSPA_Data  itm1;
	LADSPA_Data  otm1;
	LADSPA_Data run_adding_gain;
} DcRemove;

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
		return dcRemoveDescriptor;
	default:
		return NULL;
	}
}

static void activateDcRemove(LADSPA_Handle instance) {
	DcRemove *plugin_data = (DcRemove *)instance;
	LADSPA_Data itm1 = plugin_data->itm1;
	LADSPA_Data otm1 = plugin_data->otm1;
#line 17 "dc_remove_1207.xml"
	itm1 = 0.0f;
	otm1 = 0.0f;
	plugin_data->itm1 = itm1;
	plugin_data->otm1 = otm1;

}

static void cleanupDcRemove(LADSPA_Handle instance) {
	free(instance);
}

static void connectPortDcRemove(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	DcRemove *plugin;

	plugin = (DcRemove *)instance;
	switch (port) {
	case DCREMOVE_INPUT:
		plugin->input = data;
		break;
	case DCREMOVE_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateDcRemove(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	DcRemove *plugin_data = (DcRemove *)malloc(sizeof(DcRemove));
	plugin_data->run_adding_gain = 1.0f;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runDcRemove(LADSPA_Handle instance, unsigned long sample_count) {
	DcRemove *plugin_data = (DcRemove *)instance;

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	LADSPA_Data itm1 = plugin_data->itm1;
	LADSPA_Data otm1 = plugin_data->otm1;

#line 22 "dc_remove_1207.xml"
	unsigned long pos;

	for (pos = 0; pos < sample_count; pos++) {
	  otm1 = 0.999f * otm1 + input[pos] - itm1;
	  itm1 = input[pos];
	  buffer_write(output[pos], otm1);
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

static void setRunAddingGainDcRemove(LADSPA_Handle instance, LADSPA_Data gain) {
	((DcRemove *)instance)->run_adding_gain = gain;
}

static void runAddingDcRemove(LADSPA_Handle instance, unsigned long sample_count) {
	DcRemove *plugin_data = (DcRemove *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	LADSPA_Data itm1 = plugin_data->itm1;
	LADSPA_Data otm1 = plugin_data->otm1;

#line 22 "dc_remove_1207.xml"
	unsigned long pos;

	for (pos = 0; pos < sample_count; pos++) {
	  otm1 = 0.999f * otm1 + input[pos] - itm1;
	  itm1 = input[pos];
	  buffer_write(output[pos], otm1);
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


	dcRemoveDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (dcRemoveDescriptor) {
		dcRemoveDescriptor->UniqueID = 1207;
		dcRemoveDescriptor->Label = "dcRemove";
		dcRemoveDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		dcRemoveDescriptor->Name =
		 D_("DC Offset Remover");
		dcRemoveDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		dcRemoveDescriptor->Copyright =
		 "GPL";
		dcRemoveDescriptor->PortCount = 2;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(2,
		 sizeof(LADSPA_PortDescriptor));
		dcRemoveDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(2,
		 sizeof(LADSPA_PortRangeHint));
		dcRemoveDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(2, sizeof(char*));
		dcRemoveDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Input */
		port_descriptors[DCREMOVE_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[DCREMOVE_INPUT] =
		 D_("Input");
		port_range_hints[DCREMOVE_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[DCREMOVE_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[DCREMOVE_OUTPUT] =
		 D_("Output");
		port_range_hints[DCREMOVE_OUTPUT].HintDescriptor = 0;

		dcRemoveDescriptor->activate = activateDcRemove;
		dcRemoveDescriptor->cleanup = cleanupDcRemove;
		dcRemoveDescriptor->connect_port = connectPortDcRemove;
		dcRemoveDescriptor->deactivate = NULL;
		dcRemoveDescriptor->instantiate = instantiateDcRemove;
		dcRemoveDescriptor->run = runDcRemove;
		dcRemoveDescriptor->run_adding = runAddingDcRemove;
		dcRemoveDescriptor->set_run_adding_gain = setRunAddingGainDcRemove;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (dcRemoveDescriptor) {
		free((LADSPA_PortDescriptor *)dcRemoveDescriptor->PortDescriptors);
		free((char **)dcRemoveDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)dcRemoveDescriptor->PortRangeHints);
		free(dcRemoveDescriptor);
	}

}
