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


#define ZM1_INPUT                      0
#define ZM1_OUTPUT                     1

static LADSPA_Descriptor *zm1Descriptor = NULL;

typedef struct {
	LADSPA_Data *input;
	LADSPA_Data *output;
	LADSPA_Data  xm1;
	LADSPA_Data run_adding_gain;
} Zm1;

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
		return zm1Descriptor;
	default:
		return NULL;
	}
}

static void activateZm1(LADSPA_Handle instance) {
	Zm1 *plugin_data = (Zm1 *)instance;
	LADSPA_Data xm1 = plugin_data->xm1;
#line 21 "zm1_1428.xml"
	xm1 = 0.0f;
	plugin_data->xm1 = xm1;

}

static void cleanupZm1(LADSPA_Handle instance) {
	free(instance);
}

static void connectPortZm1(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Zm1 *plugin;

	plugin = (Zm1 *)instance;
	switch (port) {
	case ZM1_INPUT:
		plugin->input = data;
		break;
	case ZM1_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateZm1(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Zm1 *plugin_data = (Zm1 *)malloc(sizeof(Zm1));
	LADSPA_Data xm1;

#line 17 "zm1_1428.xml"
	xm1 = 0.0f;

	plugin_data->xm1 = xm1;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runZm1(LADSPA_Handle instance, unsigned long sample_count) {
	Zm1 *plugin_data = (Zm1 *)instance;

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	LADSPA_Data xm1 = plugin_data->xm1;

#line 25 "zm1_1428.xml"
	unsigned long pos;
	LADSPA_Data tmp;

	for (pos = 0; pos < sample_count; pos++) {
	  tmp = input[pos];
	  buffer_write(output[pos], xm1);
	  xm1 = tmp;
	}
	plugin_data->xm1 = xm1;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainZm1(LADSPA_Handle instance, LADSPA_Data gain) {
	((Zm1 *)instance)->run_adding_gain = gain;
}

static void runAddingZm1(LADSPA_Handle instance, unsigned long sample_count) {
	Zm1 *plugin_data = (Zm1 *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	LADSPA_Data xm1 = plugin_data->xm1;

#line 25 "zm1_1428.xml"
	unsigned long pos;
	LADSPA_Data tmp;

	for (pos = 0; pos < sample_count; pos++) {
	  tmp = input[pos];
	  buffer_write(output[pos], xm1);
	  xm1 = tmp;
	}
	plugin_data->xm1 = xm1;
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


	zm1Descriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (zm1Descriptor) {
		zm1Descriptor->UniqueID = 1428;
		zm1Descriptor->Label = "zm1";
		zm1Descriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		zm1Descriptor->Name =
		 D_("z-1");
		zm1Descriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		zm1Descriptor->Copyright =
		 "GPL";
		zm1Descriptor->PortCount = 2;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(2,
		 sizeof(LADSPA_PortDescriptor));
		zm1Descriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(2,
		 sizeof(LADSPA_PortRangeHint));
		zm1Descriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(2, sizeof(char*));
		zm1Descriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Input */
		port_descriptors[ZM1_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[ZM1_INPUT] =
		 D_("Input");
		port_range_hints[ZM1_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[ZM1_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[ZM1_OUTPUT] =
		 D_("Output");
		port_range_hints[ZM1_OUTPUT].HintDescriptor = 0;

		zm1Descriptor->activate = activateZm1;
		zm1Descriptor->cleanup = cleanupZm1;
		zm1Descriptor->connect_port = connectPortZm1;
		zm1Descriptor->deactivate = NULL;
		zm1Descriptor->instantiate = instantiateZm1;
		zm1Descriptor->run = runZm1;
		zm1Descriptor->run_adding = runAddingZm1;
		zm1Descriptor->set_run_adding_gain = setRunAddingGainZm1;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (zm1Descriptor) {
		free((LADSPA_PortDescriptor *)zm1Descriptor->PortDescriptors);
		free((char **)zm1Descriptor->PortNames);
		free((LADSPA_PortRangeHint *)zm1Descriptor->PortRangeHints);
		free(zm1Descriptor);
	}

}
