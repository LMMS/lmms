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

#line 10 "rate_shifter_1417.xml"

#include "ladspa-util.h"

#define RATESHIFTER_RATE               0
#define RATESHIFTER_INPUT              1
#define RATESHIFTER_OUTPUT             2

static LADSPA_Descriptor *rateShifterDescriptor = NULL;

typedef struct {
	LADSPA_Data *rate;
	LADSPA_Data *input;
	LADSPA_Data *output;
	LADSPA_Data *buffer;
	unsigned int buffer_mask;
	fixp32       read_ptr;
	unsigned int write_ptr;
	LADSPA_Data run_adding_gain;
} RateShifter;

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
		return rateShifterDescriptor;
	default:
		return NULL;
	}
}

static void activateRateShifter(LADSPA_Handle instance) {
	RateShifter *plugin_data = (RateShifter *)instance;
	LADSPA_Data *buffer = plugin_data->buffer;
	unsigned int buffer_mask = plugin_data->buffer_mask;
	fixp32 read_ptr = plugin_data->read_ptr;
	unsigned int write_ptr = plugin_data->write_ptr;
#line 36 "rate_shifter_1417.xml"
	memset(buffer, 0, buffer_mask + 1);
	read_ptr.all = 0;
	write_ptr = (buffer_mask + 1) / 2;
	write_ptr = 0;
	plugin_data->buffer = buffer;
	plugin_data->buffer_mask = buffer_mask;
	plugin_data->read_ptr = read_ptr;
	plugin_data->write_ptr = write_ptr;

}

static void cleanupRateShifter(LADSPA_Handle instance) {
#line 43 "rate_shifter_1417.xml"
	RateShifter *plugin_data = (RateShifter *)instance;
	free(plugin_data->buffer);
	free(instance);
}

static void connectPortRateShifter(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	RateShifter *plugin;

	plugin = (RateShifter *)instance;
	switch (port) {
	case RATESHIFTER_RATE:
		plugin->rate = data;
		break;
	case RATESHIFTER_INPUT:
		plugin->input = data;
		break;
	case RATESHIFTER_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateRateShifter(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	RateShifter *plugin_data = (RateShifter *)malloc(sizeof(RateShifter));
	LADSPA_Data *buffer = NULL;
	unsigned int buffer_mask;
	fixp32 read_ptr;
	unsigned int write_ptr;

#line 23 "rate_shifter_1417.xml"
	unsigned int size = 32768;
	const float fs = s_rate;

	while (size < 2.7f * fs) {
	  size *= 2;
	}
	buffer = calloc(size, sizeof(LADSPA_Data));
	buffer_mask = size - 1;
	read_ptr.all = 0;
	write_ptr = size / 2;

	plugin_data->buffer = buffer;
	plugin_data->buffer_mask = buffer_mask;
	plugin_data->read_ptr = read_ptr;
	plugin_data->write_ptr = write_ptr;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runRateShifter(LADSPA_Handle instance, unsigned long sample_count) {
	RateShifter *plugin_data = (RateShifter *)instance;

	/* Rate (float value) */
	const LADSPA_Data rate = *(plugin_data->rate);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	LADSPA_Data * buffer = plugin_data->buffer;
	unsigned int buffer_mask = plugin_data->buffer_mask;
	fixp32 read_ptr = plugin_data->read_ptr;
	unsigned int write_ptr = plugin_data->write_ptr;

#line 47 "rate_shifter_1417.xml"
	unsigned long pos;
	fixp32 read_inc;

	read_inc.all = (long long)(rate * 4294967296.0f);

	for (pos = 0; pos < sample_count; pos++) {
	  const unsigned int rp = read_ptr.part.in;

	  /* Do write pointer stuff */
	  buffer[write_ptr] = input[pos];
	  write_ptr = (write_ptr + 1) & buffer_mask;

	  /* And now read pointer */
	  buffer_write(output[pos], cube_interp((float)read_ptr.part.fr / 4294967296.0f, buffer[(rp - 1) & buffer_mask], buffer[rp],  buffer[(rp + 1) & buffer_mask], buffer[(rp + 2) & buffer_mask]));
	  read_ptr.all += read_inc.all;
	  read_ptr.part.in &= buffer_mask;
	}

	plugin_data->read_ptr.all = read_ptr.all;
	plugin_data->write_ptr = write_ptr;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainRateShifter(LADSPA_Handle instance, LADSPA_Data gain) {
	((RateShifter *)instance)->run_adding_gain = gain;
}

static void runAddingRateShifter(LADSPA_Handle instance, unsigned long sample_count) {
	RateShifter *plugin_data = (RateShifter *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Rate (float value) */
	const LADSPA_Data rate = *(plugin_data->rate);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	LADSPA_Data * buffer = plugin_data->buffer;
	unsigned int buffer_mask = plugin_data->buffer_mask;
	fixp32 read_ptr = plugin_data->read_ptr;
	unsigned int write_ptr = plugin_data->write_ptr;

#line 47 "rate_shifter_1417.xml"
	unsigned long pos;
	fixp32 read_inc;

	read_inc.all = (long long)(rate * 4294967296.0f);

	for (pos = 0; pos < sample_count; pos++) {
	  const unsigned int rp = read_ptr.part.in;

	  /* Do write pointer stuff */
	  buffer[write_ptr] = input[pos];
	  write_ptr = (write_ptr + 1) & buffer_mask;

	  /* And now read pointer */
	  buffer_write(output[pos], cube_interp((float)read_ptr.part.fr / 4294967296.0f, buffer[(rp - 1) & buffer_mask], buffer[rp],  buffer[(rp + 1) & buffer_mask], buffer[(rp + 2) & buffer_mask]));
	  read_ptr.all += read_inc.all;
	  read_ptr.part.in &= buffer_mask;
	}

	plugin_data->read_ptr.all = read_ptr.all;
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


	rateShifterDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (rateShifterDescriptor) {
		rateShifterDescriptor->UniqueID = 1417;
		rateShifterDescriptor->Label = "rateShifter";
		rateShifterDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		rateShifterDescriptor->Name =
		 D_("Rate shifter");
		rateShifterDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		rateShifterDescriptor->Copyright =
		 "GPL";
		rateShifterDescriptor->PortCount = 3;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(3,
		 sizeof(LADSPA_PortDescriptor));
		rateShifterDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(3,
		 sizeof(LADSPA_PortRangeHint));
		rateShifterDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(3, sizeof(char*));
		rateShifterDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Rate */
		port_descriptors[RATESHIFTER_RATE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[RATESHIFTER_RATE] =
		 D_("Rate");
		port_range_hints[RATESHIFTER_RATE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_1;
		port_range_hints[RATESHIFTER_RATE].LowerBound = -4;
		port_range_hints[RATESHIFTER_RATE].UpperBound = 4;

		/* Parameters for Input */
		port_descriptors[RATESHIFTER_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[RATESHIFTER_INPUT] =
		 D_("Input");
		port_range_hints[RATESHIFTER_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[RATESHIFTER_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[RATESHIFTER_OUTPUT] =
		 D_("Output");
		port_range_hints[RATESHIFTER_OUTPUT].HintDescriptor = 0;

		rateShifterDescriptor->activate = activateRateShifter;
		rateShifterDescriptor->cleanup = cleanupRateShifter;
		rateShifterDescriptor->connect_port = connectPortRateShifter;
		rateShifterDescriptor->deactivate = NULL;
		rateShifterDescriptor->instantiate = instantiateRateShifter;
		rateShifterDescriptor->run = runRateShifter;
		rateShifterDescriptor->run_adding = runAddingRateShifter;
		rateShifterDescriptor->set_run_adding_gain = setRunAddingGainRateShifter;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (rateShifterDescriptor) {
		free((LADSPA_PortDescriptor *)rateShifterDescriptor->PortDescriptors);
		free((char **)rateShifterDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)rateShifterDescriptor->PortRangeHints);
		free(rateShifterDescriptor);
	}

}
