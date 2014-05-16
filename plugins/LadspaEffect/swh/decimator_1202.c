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

#line 9 "decimator_1202.xml"

#include <math.h>
#include "ladspa-util.h"

#define DECIMATOR_BITS                 0
#define DECIMATOR_FS                   1
#define DECIMATOR_INPUT                2
#define DECIMATOR_OUTPUT               3

static LADSPA_Descriptor *decimatorDescriptor = NULL;

typedef struct {
	LADSPA_Data *bits;
	LADSPA_Data *fs;
	LADSPA_Data *input;
	LADSPA_Data *output;
	float        count;
	LADSPA_Data  last_out;
	long         sample_rate;
	LADSPA_Data run_adding_gain;
} Decimator;

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
		return decimatorDescriptor;
	default:
		return NULL;
	}
}

static void cleanupDecimator(LADSPA_Handle instance) {
	free(instance);
}

static void connectPortDecimator(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Decimator *plugin;

	plugin = (Decimator *)instance;
	switch (port) {
	case DECIMATOR_BITS:
		plugin->bits = data;
		break;
	case DECIMATOR_FS:
		plugin->fs = data;
		break;
	case DECIMATOR_INPUT:
		plugin->input = data;
		break;
	case DECIMATOR_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateDecimator(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Decimator *plugin_data = (Decimator *)malloc(sizeof(Decimator));
	float count;
	LADSPA_Data last_out;
	long sample_rate;

#line 20 "decimator_1202.xml"
	sample_rate = s_rate;
	count = 0.0f;
	last_out = 0.0f;

	plugin_data->count = count;
	plugin_data->last_out = last_out;
	plugin_data->sample_rate = sample_rate;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runDecimator(LADSPA_Handle instance, unsigned long sample_count) {
	Decimator *plugin_data = (Decimator *)instance;

	/* Bit depth (float value) */
	const LADSPA_Data bits = *(plugin_data->bits);

	/* Sample rate (Hz) (float value) */
	const LADSPA_Data fs = *(plugin_data->fs);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	float count = plugin_data->count;
	LADSPA_Data last_out = plugin_data->last_out;
	long sample_rate = plugin_data->sample_rate;

#line 26 "decimator_1202.xml"
	unsigned long pos;
	float step, stepr, delta, ratio;
	double dummy;
	
	if (bits >= 31.0f || bits < 1.0f) {
	        step = 0.0f;
	        stepr = 1.0f;
	} else {
	        step = pow(0.5f, bits - 0.999f);
	        stepr = 1/step;
	}
	
	if (fs >= sample_rate) {
	        ratio = 1.0f;
	} else {
	        ratio = fs/sample_rate;
	}
	
	for (pos = 0; pos < sample_count; pos++) {
	        count += ratio;
	
	        if (count >= 1.0f) {
	                count -= 1.0f;
	                delta = modf((input[pos] + (input[pos]<0?-1.0:1.0)*step*0.5) * stepr, &dummy) * step;
	                last_out = input[pos] - delta;
	                buffer_write(output[pos], last_out);
	        } else {
	                buffer_write(output[pos], last_out);
	        }
	}
	
	plugin_data->last_out = last_out;
	plugin_data->count = count;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainDecimator(LADSPA_Handle instance, LADSPA_Data gain) {
	((Decimator *)instance)->run_adding_gain = gain;
}

static void runAddingDecimator(LADSPA_Handle instance, unsigned long sample_count) {
	Decimator *plugin_data = (Decimator *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Bit depth (float value) */
	const LADSPA_Data bits = *(plugin_data->bits);

	/* Sample rate (Hz) (float value) */
	const LADSPA_Data fs = *(plugin_data->fs);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	float count = plugin_data->count;
	LADSPA_Data last_out = plugin_data->last_out;
	long sample_rate = plugin_data->sample_rate;

#line 26 "decimator_1202.xml"
	unsigned long pos;
	float step, stepr, delta, ratio;
	double dummy;
	
	if (bits >= 31.0f || bits < 1.0f) {
	        step = 0.0f;
	        stepr = 1.0f;
	} else {
	        step = pow(0.5f, bits - 0.999f);
	        stepr = 1/step;
	}
	
	if (fs >= sample_rate) {
	        ratio = 1.0f;
	} else {
	        ratio = fs/sample_rate;
	}
	
	for (pos = 0; pos < sample_count; pos++) {
	        count += ratio;
	
	        if (count >= 1.0f) {
	                count -= 1.0f;
	                delta = modf((input[pos] + (input[pos]<0?-1.0:1.0)*step*0.5) * stepr, &dummy) * step;
	                last_out = input[pos] - delta;
	                buffer_write(output[pos], last_out);
	        } else {
	                buffer_write(output[pos], last_out);
	        }
	}
	
	plugin_data->last_out = last_out;
	plugin_data->count = count;
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


	decimatorDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (decimatorDescriptor) {
		decimatorDescriptor->UniqueID = 1202;
		decimatorDescriptor->Label = "decimator";
		decimatorDescriptor->Properties =
		 0;
		decimatorDescriptor->Name =
		 D_("Decimator");
		decimatorDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		decimatorDescriptor->Copyright =
		 "GPL";
		decimatorDescriptor->PortCount = 4;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(4,
		 sizeof(LADSPA_PortDescriptor));
		decimatorDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(4,
		 sizeof(LADSPA_PortRangeHint));
		decimatorDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(4, sizeof(char*));
		decimatorDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Bit depth */
		port_descriptors[DECIMATOR_BITS] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[DECIMATOR_BITS] =
		 D_("Bit depth");
		port_range_hints[DECIMATOR_BITS].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MAXIMUM;
		port_range_hints[DECIMATOR_BITS].LowerBound = 1;
		port_range_hints[DECIMATOR_BITS].UpperBound = 24;

		/* Parameters for Sample rate (Hz) */
		port_descriptors[DECIMATOR_FS] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[DECIMATOR_FS] =
		 D_("Sample rate (Hz)");
		port_range_hints[DECIMATOR_FS].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_SAMPLE_RATE | LADSPA_HINT_DEFAULT_MAXIMUM;
		port_range_hints[DECIMATOR_FS].LowerBound = 0.001;
		port_range_hints[DECIMATOR_FS].UpperBound = 1;

		/* Parameters for Input */
		port_descriptors[DECIMATOR_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[DECIMATOR_INPUT] =
		 D_("Input");
		port_range_hints[DECIMATOR_INPUT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[DECIMATOR_INPUT].LowerBound = -1.0;
		port_range_hints[DECIMATOR_INPUT].UpperBound = +1.0;

		/* Parameters for Output */
		port_descriptors[DECIMATOR_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[DECIMATOR_OUTPUT] =
		 D_("Output");
		port_range_hints[DECIMATOR_OUTPUT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[DECIMATOR_OUTPUT].LowerBound = -1.0;
		port_range_hints[DECIMATOR_OUTPUT].UpperBound = +1.0;

		decimatorDescriptor->activate = NULL;
		decimatorDescriptor->cleanup = cleanupDecimator;
		decimatorDescriptor->connect_port = connectPortDecimator;
		decimatorDescriptor->deactivate = NULL;
		decimatorDescriptor->instantiate = instantiateDecimator;
		decimatorDescriptor->run = runDecimator;
		decimatorDescriptor->run_adding = runAddingDecimator;
		decimatorDescriptor->set_run_adding_gain = setRunAddingGainDecimator;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (decimatorDescriptor) {
		free((LADSPA_PortDescriptor *)decimatorDescriptor->PortDescriptors);
		free((char **)decimatorDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)decimatorDescriptor->PortRangeHints);
		free(decimatorDescriptor);
	}

}
