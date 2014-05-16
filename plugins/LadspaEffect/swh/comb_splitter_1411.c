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

#line 9 "comb_splitter_1411.xml"

#include "ladspa-util.h"
#define COMB_SIZE 0x4000
#define COMB_MASK 0x3FFF

#define COMBSPLITTER_FREQ              0
#define COMBSPLITTER_INPUT             1
#define COMBSPLITTER_OUT1              2
#define COMBSPLITTER_OUT2              3

static LADSPA_Descriptor *combSplitterDescriptor = NULL;

typedef struct {
	LADSPA_Data *freq;
	LADSPA_Data *input;
	LADSPA_Data *out1;
	LADSPA_Data *out2;
	long         comb_pos;
	LADSPA_Data *comb_tbl;
	float        last_offset;
	long         sample_rate;
	LADSPA_Data run_adding_gain;
} CombSplitter;

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
		return combSplitterDescriptor;
	default:
		return NULL;
	}
}

static void activateCombSplitter(LADSPA_Handle instance) {
	CombSplitter *plugin_data = (CombSplitter *)instance;
	long comb_pos = plugin_data->comb_pos;
	LADSPA_Data *comb_tbl = plugin_data->comb_tbl;
	float last_offset = plugin_data->last_offset;
	long sample_rate = plugin_data->sample_rate;
#line 29 "comb_splitter_1411.xml"
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

static void cleanupCombSplitter(LADSPA_Handle instance) {
#line 39 "comb_splitter_1411.xml"
	CombSplitter *plugin_data = (CombSplitter *)instance;
	free(plugin_data->comb_tbl);
	free(instance);
}

static void connectPortCombSplitter(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	CombSplitter *plugin;

	plugin = (CombSplitter *)instance;
	switch (port) {
	case COMBSPLITTER_FREQ:
		plugin->freq = data;
		break;
	case COMBSPLITTER_INPUT:
		plugin->input = data;
		break;
	case COMBSPLITTER_OUT1:
		plugin->out1 = data;
		break;
	case COMBSPLITTER_OUT2:
		plugin->out2 = data;
		break;
	}
}

static LADSPA_Handle instantiateCombSplitter(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	CombSplitter *plugin_data = (CombSplitter *)malloc(sizeof(CombSplitter));
	long comb_pos;
	LADSPA_Data *comb_tbl = NULL;
	float last_offset;
	long sample_rate;

#line 22 "comb_splitter_1411.xml"
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

static void runCombSplitter(LADSPA_Handle instance, unsigned long sample_count) {
	CombSplitter *plugin_data = (CombSplitter *)instance;

	/* Band separation (Hz) (float value) */
	const LADSPA_Data freq = *(plugin_data->freq);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output 1 (array of floats of length sample_count) */
	LADSPA_Data * const out1 = plugin_data->out1;

	/* Output 2 (array of floats of length sample_count) */
	LADSPA_Data * const out2 = plugin_data->out2;
	long comb_pos = plugin_data->comb_pos;
	LADSPA_Data * comb_tbl = plugin_data->comb_tbl;
	float last_offset = plugin_data->last_offset;
	long sample_rate = plugin_data->sample_rate;

#line 43 "comb_splitter_1411.xml"
	float offset;
	int data_pos;
	unsigned long pos;
	float xf, xf_step, d_pos, fr, interp, in;

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
	        in = input[pos];
	        comb_tbl[comb_pos] = in;
	        buffer_write(out1[pos], (in + interp) * 0.5f);
	        buffer_write(out2[pos], (in - interp) * 0.5f);
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

static void setRunAddingGainCombSplitter(LADSPA_Handle instance, LADSPA_Data gain) {
	((CombSplitter *)instance)->run_adding_gain = gain;
}

static void runAddingCombSplitter(LADSPA_Handle instance, unsigned long sample_count) {
	CombSplitter *plugin_data = (CombSplitter *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Band separation (Hz) (float value) */
	const LADSPA_Data freq = *(plugin_data->freq);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output 1 (array of floats of length sample_count) */
	LADSPA_Data * const out1 = plugin_data->out1;

	/* Output 2 (array of floats of length sample_count) */
	LADSPA_Data * const out2 = plugin_data->out2;
	long comb_pos = plugin_data->comb_pos;
	LADSPA_Data * comb_tbl = plugin_data->comb_tbl;
	float last_offset = plugin_data->last_offset;
	long sample_rate = plugin_data->sample_rate;

#line 43 "comb_splitter_1411.xml"
	float offset;
	int data_pos;
	unsigned long pos;
	float xf, xf_step, d_pos, fr, interp, in;

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
	        in = input[pos];
	        comb_tbl[comb_pos] = in;
	        buffer_write(out1[pos], (in + interp) * 0.5f);
	        buffer_write(out2[pos], (in - interp) * 0.5f);
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


	combSplitterDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (combSplitterDescriptor) {
		combSplitterDescriptor->UniqueID = 1411;
		combSplitterDescriptor->Label = "combSplitter";
		combSplitterDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		combSplitterDescriptor->Name =
		 D_("Comb Splitter");
		combSplitterDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		combSplitterDescriptor->Copyright =
		 "GPL";
		combSplitterDescriptor->PortCount = 4;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(4,
		 sizeof(LADSPA_PortDescriptor));
		combSplitterDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(4,
		 sizeof(LADSPA_PortRangeHint));
		combSplitterDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(4, sizeof(char*));
		combSplitterDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Band separation (Hz) */
		port_descriptors[COMBSPLITTER_FREQ] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[COMBSPLITTER_FREQ] =
		 D_("Band separation (Hz)");
		port_range_hints[COMBSPLITTER_FREQ].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[COMBSPLITTER_FREQ].LowerBound = 16;
		port_range_hints[COMBSPLITTER_FREQ].UpperBound = 640;

		/* Parameters for Input */
		port_descriptors[COMBSPLITTER_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[COMBSPLITTER_INPUT] =
		 D_("Input");
		port_range_hints[COMBSPLITTER_INPUT].HintDescriptor = 0;

		/* Parameters for Output 1 */
		port_descriptors[COMBSPLITTER_OUT1] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[COMBSPLITTER_OUT1] =
		 D_("Output 1");
		port_range_hints[COMBSPLITTER_OUT1].HintDescriptor = 0;

		/* Parameters for Output 2 */
		port_descriptors[COMBSPLITTER_OUT2] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[COMBSPLITTER_OUT2] =
		 D_("Output 2");
		port_range_hints[COMBSPLITTER_OUT2].HintDescriptor = 0;

		combSplitterDescriptor->activate = activateCombSplitter;
		combSplitterDescriptor->cleanup = cleanupCombSplitter;
		combSplitterDescriptor->connect_port = connectPortCombSplitter;
		combSplitterDescriptor->deactivate = NULL;
		combSplitterDescriptor->instantiate = instantiateCombSplitter;
		combSplitterDescriptor->run = runCombSplitter;
		combSplitterDescriptor->run_adding = runAddingCombSplitter;
		combSplitterDescriptor->set_run_adding_gain = setRunAddingGainCombSplitter;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (combSplitterDescriptor) {
		free((LADSPA_PortDescriptor *)combSplitterDescriptor->PortDescriptors);
		free((char **)combSplitterDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)combSplitterDescriptor->PortRangeHints);
		free(combSplitterDescriptor);
	}

}
