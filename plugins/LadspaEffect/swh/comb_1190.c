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

#line 9 "comb_1190.xml"

#include "ladspa-util.h"
#define COMB_SIZE 0x4000
#define COMB_MASK 0x3FFF

#define COMB_FREQ                      0
#define COMB_FB                        1
#define COMB_INPUT                     2
#define COMB_OUTPUT                    3

static LADSPA_Descriptor *combDescriptor = NULL;

typedef struct {
	LADSPA_Data *freq;
	LADSPA_Data *fb;
	LADSPA_Data *input;
	LADSPA_Data *output;
	long         comb_pos;
	LADSPA_Data *comb_tbl;
	float        last_offset;
	long         sample_rate;
	LADSPA_Data run_adding_gain;
} Comb;

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
		return combDescriptor;
	default:
		return NULL;
	}
}

static void activateComb(LADSPA_Handle instance) {
	Comb *plugin_data = (Comb *)instance;
	long comb_pos = plugin_data->comb_pos;
	LADSPA_Data *comb_tbl = plugin_data->comb_tbl;
	float last_offset = plugin_data->last_offset;
	long sample_rate = plugin_data->sample_rate;
#line 27 "comb_1190.xml"
	int i;

	for (i = 0; i < COMB_SIZE; i++) {
	        comb_tbl[i] = 0;
	}
	comb_pos = 0;
	last_offset = 1000;
	plugin_data->comb_pos = comb_pos;
	plugin_data->comb_tbl = comb_tbl;
	plugin_data->last_offset = last_offset;
	plugin_data->sample_rate = sample_rate;

}

static void cleanupComb(LADSPA_Handle instance) {
#line 37 "comb_1190.xml"
	Comb *plugin_data = (Comb *)instance;
	free(plugin_data->comb_tbl);
	free(instance);
}

static void connectPortComb(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Comb *plugin;

	plugin = (Comb *)instance;
	switch (port) {
	case COMB_FREQ:
		plugin->freq = data;
		break;
	case COMB_FB:
		plugin->fb = data;
		break;
	case COMB_INPUT:
		plugin->input = data;
		break;
	case COMB_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateComb(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Comb *plugin_data = (Comb *)malloc(sizeof(Comb));
	long comb_pos;
	LADSPA_Data *comb_tbl = NULL;
	float last_offset;
	long sample_rate;

#line 20 "comb_1190.xml"
	sample_rate = s_rate;
	comb_tbl = malloc(sizeof(LADSPA_Data) * COMB_SIZE);
	comb_pos = 0;
	last_offset = 1000;

	plugin_data->comb_pos = comb_pos;
	plugin_data->comb_tbl = comb_tbl;
	plugin_data->last_offset = last_offset;
	plugin_data->sample_rate = sample_rate;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runComb(LADSPA_Handle instance, unsigned long sample_count) {
	Comb *plugin_data = (Comb *)instance;

	/* Band separation (Hz) (float value) */
	const LADSPA_Data freq = *(plugin_data->freq);

	/* Feedback (float value) */
	const LADSPA_Data fb = *(plugin_data->fb);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	long comb_pos = plugin_data->comb_pos;
	LADSPA_Data * comb_tbl = plugin_data->comb_tbl;
	float last_offset = plugin_data->last_offset;
	long sample_rate = plugin_data->sample_rate;

#line 41 "comb_1190.xml"
	float offset;
	int data_pos;
	unsigned long pos;
	float xf, xf_step, d_pos, fr, interp;

	offset = sample_rate / freq;
	offset = f_clamp(offset, 0, COMB_SIZE - 1);
	xf_step = 1.0f / (float)sample_count;
	xf = 0.0f;

	for (pos = 0; pos < sample_count; pos++) {
	        xf += xf_step;
	        d_pos = comb_pos - LIN_INTERP(xf, last_offset, offset);
	        data_pos = f_trunc(d_pos);
	        fr = d_pos - data_pos;
	        interp =  cube_interp(fr, comb_tbl[(data_pos - 1) & COMB_MASK], comb_tbl[data_pos & COMB_MASK], comb_tbl[(data_pos + 1) & COMB_MASK], comb_tbl[(data_pos + 2) & COMB_MASK]);
	        comb_tbl[comb_pos] = input[pos] + fb * interp;
	        buffer_write(output[pos], (input[pos] + interp) * 0.5f);
	        comb_pos = (comb_pos + 1) & COMB_MASK;
	}

	plugin_data->comb_pos = comb_pos;
	plugin_data->last_offset = offset;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainComb(LADSPA_Handle instance, LADSPA_Data gain) {
	((Comb *)instance)->run_adding_gain = gain;
}

static void runAddingComb(LADSPA_Handle instance, unsigned long sample_count) {
	Comb *plugin_data = (Comb *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Band separation (Hz) (float value) */
	const LADSPA_Data freq = *(plugin_data->freq);

	/* Feedback (float value) */
	const LADSPA_Data fb = *(plugin_data->fb);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	long comb_pos = plugin_data->comb_pos;
	LADSPA_Data * comb_tbl = plugin_data->comb_tbl;
	float last_offset = plugin_data->last_offset;
	long sample_rate = plugin_data->sample_rate;

#line 41 "comb_1190.xml"
	float offset;
	int data_pos;
	unsigned long pos;
	float xf, xf_step, d_pos, fr, interp;

	offset = sample_rate / freq;
	offset = f_clamp(offset, 0, COMB_SIZE - 1);
	xf_step = 1.0f / (float)sample_count;
	xf = 0.0f;

	for (pos = 0; pos < sample_count; pos++) {
	        xf += xf_step;
	        d_pos = comb_pos - LIN_INTERP(xf, last_offset, offset);
	        data_pos = f_trunc(d_pos);
	        fr = d_pos - data_pos;
	        interp =  cube_interp(fr, comb_tbl[(data_pos - 1) & COMB_MASK], comb_tbl[data_pos & COMB_MASK], comb_tbl[(data_pos + 1) & COMB_MASK], comb_tbl[(data_pos + 2) & COMB_MASK]);
	        comb_tbl[comb_pos] = input[pos] + fb * interp;
	        buffer_write(output[pos], (input[pos] + interp) * 0.5f);
	        comb_pos = (comb_pos + 1) & COMB_MASK;
	}

	plugin_data->comb_pos = comb_pos;
	plugin_data->last_offset = offset;
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


	combDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (combDescriptor) {
		combDescriptor->UniqueID = 1190;
		combDescriptor->Label = "comb";
		combDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		combDescriptor->Name =
		 D_("Comb Filter");
		combDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		combDescriptor->Copyright =
		 "GPL";
		combDescriptor->PortCount = 4;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(4,
		 sizeof(LADSPA_PortDescriptor));
		combDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(4,
		 sizeof(LADSPA_PortRangeHint));
		combDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(4, sizeof(char*));
		combDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Band separation (Hz) */
		port_descriptors[COMB_FREQ] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[COMB_FREQ] =
		 D_("Band separation (Hz)");
		port_range_hints[COMB_FREQ].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[COMB_FREQ].LowerBound = 16;
		port_range_hints[COMB_FREQ].UpperBound = 640;

		/* Parameters for Feedback */
		port_descriptors[COMB_FB] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[COMB_FB] =
		 D_("Feedback");
		port_range_hints[COMB_FB].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[COMB_FB].LowerBound = -0.99;
		port_range_hints[COMB_FB].UpperBound = 0.99;

		/* Parameters for Input */
		port_descriptors[COMB_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[COMB_INPUT] =
		 D_("Input");
		port_range_hints[COMB_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[COMB_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[COMB_OUTPUT] =
		 D_("Output");
		port_range_hints[COMB_OUTPUT].HintDescriptor = 0;

		combDescriptor->activate = activateComb;
		combDescriptor->cleanup = cleanupComb;
		combDescriptor->connect_port = connectPortComb;
		combDescriptor->deactivate = NULL;
		combDescriptor->instantiate = instantiateComb;
		combDescriptor->run = runComb;
		combDescriptor->run_adding = runAddingComb;
		combDescriptor->set_run_adding_gain = setRunAddingGainComb;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (combDescriptor) {
		free((LADSPA_PortDescriptor *)combDescriptor->PortDescriptors);
		free((char **)combDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)combDescriptor->PortRangeHints);
		free(combDescriptor);
	}

}
