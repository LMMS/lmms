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

#line 10 "mod_delay_1419.xml"

#include "ladspa-util.h"

#define MODDELAY_BASE                  0
#define MODDELAY_DELAY                 1
#define MODDELAY_INPUT                 2
#define MODDELAY_OUTPUT                3

static LADSPA_Descriptor *modDelayDescriptor = NULL;

typedef struct {
	LADSPA_Data *base;
	LADSPA_Data *delay;
	LADSPA_Data *input;
	LADSPA_Data *output;
	LADSPA_Data *buffer;
	unsigned int buffer_mask;
	float        fs;
	unsigned int write_ptr;
	LADSPA_Data run_adding_gain;
} ModDelay;

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
		return modDelayDescriptor;
	default:
		return NULL;
	}
}

static void activateModDelay(LADSPA_Handle instance) {
	ModDelay *plugin_data = (ModDelay *)instance;
	LADSPA_Data *buffer = plugin_data->buffer;
	unsigned int buffer_mask = plugin_data->buffer_mask;
	float fs = plugin_data->fs;
	unsigned int write_ptr = plugin_data->write_ptr;
#line 33 "mod_delay_1419.xml"
	memset(buffer, 0, buffer_mask + 1);
	write_ptr = 0;
	plugin_data->buffer = buffer;
	plugin_data->buffer_mask = buffer_mask;
	plugin_data->fs = fs;
	plugin_data->write_ptr = write_ptr;

}

static void cleanupModDelay(LADSPA_Handle instance) {
#line 38 "mod_delay_1419.xml"
	ModDelay *plugin_data = (ModDelay *)instance;
	free(plugin_data->buffer);
	free(instance);
}

static void connectPortModDelay(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	ModDelay *plugin;

	plugin = (ModDelay *)instance;
	switch (port) {
	case MODDELAY_BASE:
		plugin->base = data;
		break;
	case MODDELAY_DELAY:
		plugin->delay = data;
		break;
	case MODDELAY_INPUT:
		plugin->input = data;
		break;
	case MODDELAY_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateModDelay(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	ModDelay *plugin_data = (ModDelay *)malloc(sizeof(ModDelay));
	LADSPA_Data *buffer = NULL;
	unsigned int buffer_mask;
	float fs;
	unsigned int write_ptr;

#line 21 "mod_delay_1419.xml"
	unsigned int size = 32768;

	fs = s_rate;
	while (size < 2.7f * fs) {
	  size *= 2;
	}
	buffer = calloc(size, sizeof(LADSPA_Data));
	buffer_mask = size - 1;
	write_ptr = 0;

	plugin_data->buffer = buffer;
	plugin_data->buffer_mask = buffer_mask;
	plugin_data->fs = fs;
	plugin_data->write_ptr = write_ptr;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runModDelay(LADSPA_Handle instance, unsigned long sample_count) {
	ModDelay *plugin_data = (ModDelay *)instance;

	/* Base delay (s) (float value) */
	const LADSPA_Data base = *(plugin_data->base);

	/* Delay (s) (array of floats of length sample_count) */
	const LADSPA_Data * const delay = plugin_data->delay;

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	LADSPA_Data * buffer = plugin_data->buffer;
	unsigned int buffer_mask = plugin_data->buffer_mask;
	float fs = plugin_data->fs;
	unsigned int write_ptr = plugin_data->write_ptr;

#line 42 "mod_delay_1419.xml"
	unsigned long pos;

	for (pos = 0; pos < sample_count; pos++) {
	  float tmp;
	  const float rpf = modff((base + delay[pos]) * fs, &tmp);
	  const int rp = write_ptr - 4 - f_round(tmp);

	  buffer[write_ptr++] = input[pos];
	  write_ptr &= buffer_mask;

	  buffer_write(output[pos], cube_interp(rpf, buffer[(rp - 1) & buffer_mask], buffer[rp & buffer_mask],  buffer[(rp + 1) & buffer_mask], buffer[(rp + 2) & buffer_mask]));
	}
	plugin_data->write_ptr = write_ptr;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainModDelay(LADSPA_Handle instance, LADSPA_Data gain) {
	((ModDelay *)instance)->run_adding_gain = gain;
}

static void runAddingModDelay(LADSPA_Handle instance, unsigned long sample_count) {
	ModDelay *plugin_data = (ModDelay *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Base delay (s) (float value) */
	const LADSPA_Data base = *(plugin_data->base);

	/* Delay (s) (array of floats of length sample_count) */
	const LADSPA_Data * const delay = plugin_data->delay;

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	LADSPA_Data * buffer = plugin_data->buffer;
	unsigned int buffer_mask = plugin_data->buffer_mask;
	float fs = plugin_data->fs;
	unsigned int write_ptr = plugin_data->write_ptr;

#line 42 "mod_delay_1419.xml"
	unsigned long pos;

	for (pos = 0; pos < sample_count; pos++) {
	  float tmp;
	  const float rpf = modff((base + delay[pos]) * fs, &tmp);
	  const int rp = write_ptr - 4 - f_round(tmp);

	  buffer[write_ptr++] = input[pos];
	  write_ptr &= buffer_mask;

	  buffer_write(output[pos], cube_interp(rpf, buffer[(rp - 1) & buffer_mask], buffer[rp & buffer_mask],  buffer[(rp + 1) & buffer_mask], buffer[(rp + 2) & buffer_mask]));
	}
	plugin_data->write_ptr = write_ptr;
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


	modDelayDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (modDelayDescriptor) {
		modDelayDescriptor->UniqueID = 1419;
		modDelayDescriptor->Label = "modDelay";
		modDelayDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		modDelayDescriptor->Name =
		 D_("Modulatable delay");
		modDelayDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		modDelayDescriptor->Copyright =
		 "GPL";
		modDelayDescriptor->PortCount = 4;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(4,
		 sizeof(LADSPA_PortDescriptor));
		modDelayDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(4,
		 sizeof(LADSPA_PortRangeHint));
		modDelayDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(4, sizeof(char*));
		modDelayDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Base delay (s) */
		port_descriptors[MODDELAY_BASE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[MODDELAY_BASE] =
		 D_("Base delay (s)");
		port_range_hints[MODDELAY_BASE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MAXIMUM;
		port_range_hints[MODDELAY_BASE].LowerBound = 0;
		port_range_hints[MODDELAY_BASE].UpperBound = 1;

		/* Parameters for Delay (s) */
		port_descriptors[MODDELAY_DELAY] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[MODDELAY_DELAY] =
		 D_("Delay (s)");
		port_range_hints[MODDELAY_DELAY].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[MODDELAY_DELAY].LowerBound = 0;
		port_range_hints[MODDELAY_DELAY].UpperBound = 1.7;

		/* Parameters for Input */
		port_descriptors[MODDELAY_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[MODDELAY_INPUT] =
		 D_("Input");
		port_range_hints[MODDELAY_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[MODDELAY_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[MODDELAY_OUTPUT] =
		 D_("Output");
		port_range_hints[MODDELAY_OUTPUT].HintDescriptor = 0;

		modDelayDescriptor->activate = activateModDelay;
		modDelayDescriptor->cleanup = cleanupModDelay;
		modDelayDescriptor->connect_port = connectPortModDelay;
		modDelayDescriptor->deactivate = NULL;
		modDelayDescriptor->instantiate = instantiateModDelay;
		modDelayDescriptor->run = runModDelay;
		modDelayDescriptor->run_adding = runAddingModDelay;
		modDelayDescriptor->set_run_adding_gain = setRunAddingGainModDelay;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (modDelayDescriptor) {
		free((LADSPA_PortDescriptor *)modDelayDescriptor->PortDescriptors);
		free((char **)modDelayDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)modDelayDescriptor->PortRangeHints);
		free(modDelayDescriptor);
	}

}
