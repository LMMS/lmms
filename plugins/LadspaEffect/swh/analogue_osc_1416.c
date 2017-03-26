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
static void __attribute__((constructor)) swh_init(); // forward declaration
#else
#define _WINDOWS_DLL_EXPORT_ 
#endif

#line 10 "analogue_osc_1416.xml"

#include <math.h>

#include "ladspa-util.h"
#include "util/blo.h"

#define ANALOGUEOSC_WAVE               0
#define ANALOGUEOSC_FREQ               1
#define ANALOGUEOSC_WARM               2
#define ANALOGUEOSC_INSTAB             3
#define ANALOGUEOSC_OUTPUT             4

static LADSPA_Descriptor *analogueOscDescriptor = NULL;

typedef struct {
	LADSPA_Data *wave;
	LADSPA_Data *freq;
	LADSPA_Data *warm;
	LADSPA_Data *instab;
	LADSPA_Data *output;
	float        fs;
	float        itm1;
	blo_h_osc *  osc;
	float        otm1;
	float        otm2;
	unsigned int rnda;
	unsigned int rndb;
	blo_h_tables *tables;
	LADSPA_Data run_adding_gain;
} AnalogueOsc;

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
		return analogueOscDescriptor;
	default:
		return NULL;
	}
}

static void cleanupAnalogueOsc(LADSPA_Handle instance) {
#line 37 "analogue_osc_1416.xml"
	AnalogueOsc *plugin_data = (AnalogueOsc *)instance;
	blo_h_tables_free(plugin_data->tables);
	blo_h_free(plugin_data->osc);
	free(instance);
}

static void connectPortAnalogueOsc(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	AnalogueOsc *plugin;

	plugin = (AnalogueOsc *)instance;
	switch (port) {
	case ANALOGUEOSC_WAVE:
		plugin->wave = data;
		break;
	case ANALOGUEOSC_FREQ:
		plugin->freq = data;
		break;
	case ANALOGUEOSC_WARM:
		plugin->warm = data;
		break;
	case ANALOGUEOSC_INSTAB:
		plugin->instab = data;
		break;
	case ANALOGUEOSC_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateAnalogueOsc(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	AnalogueOsc *plugin_data = (AnalogueOsc *)calloc(1, sizeof(AnalogueOsc));
	float fs;
	float itm1;
	blo_h_osc *osc = NULL;
	float otm1;
	float otm2;
	unsigned int rnda;
	unsigned int rndb;
	blo_h_tables *tables = NULL;

#line 26 "analogue_osc_1416.xml"
	tables = blo_h_tables_new(512);
	osc = blo_h_new(tables, BLO_SINE, (float)s_rate);
	fs = (float)s_rate;
	itm1 = 0.0f;
	otm1 = 0.0f;
	otm2 = 0.0f;
	rnda = 43437;
	rndb = 111145;

	plugin_data->fs = fs;
	plugin_data->itm1 = itm1;
	plugin_data->osc = osc;
	plugin_data->otm1 = otm1;
	plugin_data->otm2 = otm2;
	plugin_data->rnda = rnda;
	plugin_data->rndb = rndb;
	plugin_data->tables = tables;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runAnalogueOsc(LADSPA_Handle instance, unsigned long sample_count) {
	AnalogueOsc *plugin_data = (AnalogueOsc *)instance;

	/* Waveform (1=sin, 2=tri, 3=squ, 4=saw) (float value) */
	const LADSPA_Data wave = *(plugin_data->wave);

	/* Frequency (Hz) (float value) */
	const LADSPA_Data freq = *(plugin_data->freq);

	/* Warmth (float value) */
	const LADSPA_Data warm = *(plugin_data->warm);

	/* Instability (float value) */
	const LADSPA_Data instab = *(plugin_data->instab);

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	float fs = plugin_data->fs;
	float itm1 = plugin_data->itm1;
	blo_h_osc * osc = plugin_data->osc;
	float otm1 = plugin_data->otm1;
	float otm2 = plugin_data->otm2;
	unsigned int rnda = plugin_data->rnda;
	unsigned int rndb = plugin_data->rndb;
	blo_h_tables * tables = plugin_data->tables;

#line 42 "analogue_osc_1416.xml"
	unsigned long pos;
	LADSPA_Data x, y;
	const float q = warm - 0.999f;
	const float leak = 1.0f - warm * 0.02f;
	const unsigned int max_jump = (unsigned int)f_round(instab * 30000.0f) + 1;

	osc->wave = LIMIT(f_round(wave) - 1, 0, BLO_N_WAVES-1);
	osc->nyquist = fs * (0.47f - f_clamp(warm, 0.0f, 1.0f) * 0.41f);
	blo_hd_set_freq(osc, freq);

	tables = tables; // So gcc doesn't think it's unused

	for (pos = 0; pos < sample_count; pos++) {
	  x = blo_hd_run_cub(osc);
	  rnda += 432577;
	  rnda *= 47;
	  rndb += 7643113;
	  rnda *= 59;
	  osc->ph.all += (((rnda + rndb)/2) % max_jump) - max_jump/2;
	  osc->ph.all &= osc->ph_mask;
	  y = (x - q) / (1.0f - f_exp(-1.2f * (x - q))) +
	        q / (1.0f - f_exp(1.2f * q));
	  /* Catch the case where x ~= q */
	  if (isnan(y) || fabs(y) > 1.0f) {
	          y = 0.83333f + q / (1.0f - f_exp(1.2f * q));
	  }
	  otm2 = otm1;
	  otm1 = leak * otm1 + y - itm1;
	  itm1 = y;

	  buffer_write(output[pos], (otm1 + otm2) * 0.5f);
	}

	plugin_data->itm1 = itm1;
	plugin_data->otm1 = otm1;
	plugin_data->otm2 = otm2;
	plugin_data->rnda = rnda;
	plugin_data->rndb = rndb;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainAnalogueOsc(LADSPA_Handle instance, LADSPA_Data gain) {
	((AnalogueOsc *)instance)->run_adding_gain = gain;
}

static void runAddingAnalogueOsc(LADSPA_Handle instance, unsigned long sample_count) {
	AnalogueOsc *plugin_data = (AnalogueOsc *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Waveform (1=sin, 2=tri, 3=squ, 4=saw) (float value) */
	const LADSPA_Data wave = *(plugin_data->wave);

	/* Frequency (Hz) (float value) */
	const LADSPA_Data freq = *(plugin_data->freq);

	/* Warmth (float value) */
	const LADSPA_Data warm = *(plugin_data->warm);

	/* Instability (float value) */
	const LADSPA_Data instab = *(plugin_data->instab);

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	float fs = plugin_data->fs;
	float itm1 = plugin_data->itm1;
	blo_h_osc * osc = plugin_data->osc;
	float otm1 = plugin_data->otm1;
	float otm2 = plugin_data->otm2;
	unsigned int rnda = plugin_data->rnda;
	unsigned int rndb = plugin_data->rndb;
	blo_h_tables * tables = plugin_data->tables;

#line 42 "analogue_osc_1416.xml"
	unsigned long pos;
	LADSPA_Data x, y;
	const float q = warm - 0.999f;
	const float leak = 1.0f - warm * 0.02f;
	const unsigned int max_jump = (unsigned int)f_round(instab * 30000.0f) + 1;

	osc->wave = LIMIT(f_round(wave) - 1, 0, BLO_N_WAVES-1);
	osc->nyquist = fs * (0.47f - f_clamp(warm, 0.0f, 1.0f) * 0.41f);
	blo_hd_set_freq(osc, freq);

	tables = tables; // So gcc doesn't think it's unused

	for (pos = 0; pos < sample_count; pos++) {
	  x = blo_hd_run_cub(osc);
	  rnda += 432577;
	  rnda *= 47;
	  rndb += 7643113;
	  rnda *= 59;
	  osc->ph.all += (((rnda + rndb)/2) % max_jump) - max_jump/2;
	  osc->ph.all &= osc->ph_mask;
	  y = (x - q) / (1.0f - f_exp(-1.2f * (x - q))) +
	        q / (1.0f - f_exp(1.2f * q));
	  /* Catch the case where x ~= q */
	  if (isnan(y) || fabs(y) > 1.0f) {
	          y = 0.83333f + q / (1.0f - f_exp(1.2f * q));
	  }
	  otm2 = otm1;
	  otm1 = leak * otm1 + y - itm1;
	  itm1 = y;

	  buffer_write(output[pos], (otm1 + otm2) * 0.5f);
	}

	plugin_data->itm1 = itm1;
	plugin_data->otm1 = otm1;
	plugin_data->otm2 = otm2;
	plugin_data->rnda = rnda;
	plugin_data->rndb = rndb;
}

static void __attribute__((constructor)) swh_init() {
	char **port_names;
	LADSPA_PortDescriptor *port_descriptors;
	LADSPA_PortRangeHint *port_range_hints;

#ifdef ENABLE_NLS
#define D_(s) dgettext(PACKAGE, s)
	bindtextdomain(PACKAGE, PACKAGE_LOCALE_DIR);
#else
#define D_(s) (s)
#endif


	analogueOscDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (analogueOscDescriptor) {
		analogueOscDescriptor->UniqueID = 1416;
		analogueOscDescriptor->Label = "analogueOsc";
		analogueOscDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		analogueOscDescriptor->Name =
		 D_("Analogue Oscillator");
		analogueOscDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		analogueOscDescriptor->Copyright =
		 "GPL";
		analogueOscDescriptor->PortCount = 5;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(5,
		 sizeof(LADSPA_PortDescriptor));
		analogueOscDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(5,
		 sizeof(LADSPA_PortRangeHint));
		analogueOscDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(5, sizeof(char*));
		analogueOscDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Waveform (1=sin, 2=tri, 3=squ, 4=saw) */
		port_descriptors[ANALOGUEOSC_WAVE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[ANALOGUEOSC_WAVE] =
		 D_("Waveform (1=sin, 2=tri, 3=squ, 4=saw)");
		port_range_hints[ANALOGUEOSC_WAVE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_INTEGER | LADSPA_HINT_DEFAULT_1;
		port_range_hints[ANALOGUEOSC_WAVE].LowerBound = 1;
		port_range_hints[ANALOGUEOSC_WAVE].UpperBound = BLO_N_WAVES;

		/* Parameters for Frequency (Hz) */
		port_descriptors[ANALOGUEOSC_FREQ] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[ANALOGUEOSC_FREQ] =
		 D_("Frequency (Hz)");
		port_range_hints[ANALOGUEOSC_FREQ].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_SAMPLE_RATE | LADSPA_HINT_DEFAULT_440 | LADSPA_HINT_LOGARITHMIC;
		port_range_hints[ANALOGUEOSC_FREQ].LowerBound = 0.000001;
		port_range_hints[ANALOGUEOSC_FREQ].UpperBound = 0.499;

		/* Parameters for Warmth */
		port_descriptors[ANALOGUEOSC_WARM] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[ANALOGUEOSC_WARM] =
		 D_("Warmth");
		port_range_hints[ANALOGUEOSC_WARM].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[ANALOGUEOSC_WARM].LowerBound = 0;
		port_range_hints[ANALOGUEOSC_WARM].UpperBound = 1;

		/* Parameters for Instability */
		port_descriptors[ANALOGUEOSC_INSTAB] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[ANALOGUEOSC_INSTAB] =
		 D_("Instability");
		port_range_hints[ANALOGUEOSC_INSTAB].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[ANALOGUEOSC_INSTAB].LowerBound = 0;
		port_range_hints[ANALOGUEOSC_INSTAB].UpperBound = 1;

		/* Parameters for Output */
		port_descriptors[ANALOGUEOSC_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[ANALOGUEOSC_OUTPUT] =
		 D_("Output");
		port_range_hints[ANALOGUEOSC_OUTPUT].HintDescriptor = 0;

		analogueOscDescriptor->activate = NULL;
		analogueOscDescriptor->cleanup = cleanupAnalogueOsc;
		analogueOscDescriptor->connect_port = connectPortAnalogueOsc;
		analogueOscDescriptor->deactivate = NULL;
		analogueOscDescriptor->instantiate = instantiateAnalogueOsc;
		analogueOscDescriptor->run = runAnalogueOsc;
		analogueOscDescriptor->run_adding = runAddingAnalogueOsc;
		analogueOscDescriptor->set_run_adding_gain = setRunAddingGainAnalogueOsc;
	}
}

static void __attribute__((destructor)) swh_fini() {
	if (analogueOscDescriptor) {
		free((LADSPA_PortDescriptor *)analogueOscDescriptor->PortDescriptors);
		free((char **)analogueOscDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)analogueOscDescriptor->PortRangeHints);
		free(analogueOscDescriptor);
	}
	analogueOscDescriptor = NULL;

}
