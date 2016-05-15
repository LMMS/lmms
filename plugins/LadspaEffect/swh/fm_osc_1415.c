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

#line 10 "fm_osc_1415.xml"

#include "ladspa-util.h"
#include "util/blo.h"

#define FMOSC_WAVE                     0
#define FMOSC_FM                       1
#define FMOSC_OUTPUT                   2

static LADSPA_Descriptor *fmOscDescriptor = NULL;

typedef struct {
	LADSPA_Data *wave;
	LADSPA_Data *fm;
	LADSPA_Data *output;
	blo_h_osc *  osc;
	blo_h_tables *tables;
	LADSPA_Data run_adding_gain;
} FmOsc;

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
		return fmOscDescriptor;
	default:
		return NULL;
	}
}

static void cleanupFmOsc(LADSPA_Handle instance) {
#line 37 "fm_osc_1415.xml"
	FmOsc *plugin_data = (FmOsc *)instance;
	blo_h_tables_free(plugin_data->tables);
	blo_h_free(plugin_data->osc);
	free(instance);
}

static void connectPortFmOsc(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	FmOsc *plugin;

	plugin = (FmOsc *)instance;
	switch (port) {
	case FMOSC_WAVE:
		plugin->wave = data;
		break;
	case FMOSC_FM:
		plugin->fm = data;
		break;
	case FMOSC_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateFmOsc(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	FmOsc *plugin_data = (FmOsc *)calloc(1, sizeof(FmOsc));
	blo_h_osc *osc = NULL;
	blo_h_tables *tables = NULL;

#line 20 "fm_osc_1415.xml"
	tables = blo_h_tables_new(1024);
	osc = blo_h_new(tables, BLO_SINE, (float)s_rate);

	plugin_data->osc = osc;
	plugin_data->tables = tables;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runFmOsc(LADSPA_Handle instance, unsigned long sample_count) {
	FmOsc *plugin_data = (FmOsc *)instance;

	/* Waveform (1=sin, 2=tri, 3=squ, 4=saw) (float value) */
	const LADSPA_Data wave = *(plugin_data->wave);

	/* Frequency (Hz) (array of floats of length sample_count) */
	const LADSPA_Data * const fm = plugin_data->fm;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	blo_h_osc * osc = plugin_data->osc;
	blo_h_tables * tables = plugin_data->tables;

#line 25 "fm_osc_1415.xml"
	unsigned long pos;
	osc->wave = LIMIT(f_round(wave) - 1, 0, BLO_N_WAVES-1);

	tables = tables; // So gcc doesn't think it's unused

	for (pos = 0; pos < sample_count; pos++) {
	  blo_hd_set_freq(osc, fm[pos]);
	  buffer_write(output[pos], blo_hd_run_cub(osc));
	}
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainFmOsc(LADSPA_Handle instance, LADSPA_Data gain) {
	((FmOsc *)instance)->run_adding_gain = gain;
}

static void runAddingFmOsc(LADSPA_Handle instance, unsigned long sample_count) {
	FmOsc *plugin_data = (FmOsc *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Waveform (1=sin, 2=tri, 3=squ, 4=saw) (float value) */
	const LADSPA_Data wave = *(plugin_data->wave);

	/* Frequency (Hz) (array of floats of length sample_count) */
	const LADSPA_Data * const fm = plugin_data->fm;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	blo_h_osc * osc = plugin_data->osc;
	blo_h_tables * tables = plugin_data->tables;

#line 25 "fm_osc_1415.xml"
	unsigned long pos;
	osc->wave = LIMIT(f_round(wave) - 1, 0, BLO_N_WAVES-1);

	tables = tables; // So gcc doesn't think it's unused

	for (pos = 0; pos < sample_count; pos++) {
	  blo_hd_set_freq(osc, fm[pos]);
	  buffer_write(output[pos], blo_hd_run_cub(osc));
	}
}

void __attribute__((constructor)) swh_init() {
	char **port_names;
	LADSPA_PortDescriptor *port_descriptors;
	LADSPA_PortRangeHint *port_range_hints;

#ifdef ENABLE_NLS
#define D_(s) dgettext(PACKAGE, s)
	bindtextdomain(PACKAGE, PACKAGE_LOCALE_DIR);
#else
#define D_(s) (s)
#endif


	fmOscDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (fmOscDescriptor) {
		fmOscDescriptor->UniqueID = 1415;
		fmOscDescriptor->Label = "fmOsc";
		fmOscDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		fmOscDescriptor->Name =
		 D_("FM Oscillator");
		fmOscDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		fmOscDescriptor->Copyright =
		 "GPL";
		fmOscDescriptor->PortCount = 3;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(3,
		 sizeof(LADSPA_PortDescriptor));
		fmOscDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(3,
		 sizeof(LADSPA_PortRangeHint));
		fmOscDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(3, sizeof(char*));
		fmOscDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Waveform (1=sin, 2=tri, 3=squ, 4=saw) */
		port_descriptors[FMOSC_WAVE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[FMOSC_WAVE] =
		 D_("Waveform (1=sin, 2=tri, 3=squ, 4=saw)");
		port_range_hints[FMOSC_WAVE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_INTEGER | LADSPA_HINT_DEFAULT_1;
		port_range_hints[FMOSC_WAVE].LowerBound = 1;
		port_range_hints[FMOSC_WAVE].UpperBound = BLO_N_WAVES;

		/* Parameters for Frequency (Hz) */
		port_descriptors[FMOSC_FM] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[FMOSC_FM] =
		 D_("Frequency (Hz)");
		port_range_hints[FMOSC_FM].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_SAMPLE_RATE | LADSPA_HINT_DEFAULT_440;
		port_range_hints[FMOSC_FM].LowerBound = -0.25;
		port_range_hints[FMOSC_FM].UpperBound = 0.25;

		/* Parameters for Output */
		port_descriptors[FMOSC_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[FMOSC_OUTPUT] =
		 D_("Output");
		port_range_hints[FMOSC_OUTPUT].HintDescriptor = 0;

		fmOscDescriptor->activate = NULL;
		fmOscDescriptor->cleanup = cleanupFmOsc;
		fmOscDescriptor->connect_port = connectPortFmOsc;
		fmOscDescriptor->deactivate = NULL;
		fmOscDescriptor->instantiate = instantiateFmOsc;
		fmOscDescriptor->run = runFmOsc;
		fmOscDescriptor->run_adding = runAddingFmOsc;
		fmOscDescriptor->set_run_adding_gain = setRunAddingGainFmOsc;
	}
}

void __attribute__((destructor)) swh_fini() {
	if (fmOscDescriptor) {
		free((LADSPA_PortDescriptor *)fmOscDescriptor->PortDescriptors);
		free((char **)fmOscDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)fmOscDescriptor->PortRangeHints);
		free(fmOscDescriptor);
	}
	fmOscDescriptor = NULL;

}
