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

#line 10 "plate_1423.xml"

#include "util/waveguide_nl.h"

#define LP_INNER 0.96f
#define LP_OUTER 0.983f

#define RUN_WG(n, junct_a, junct_b) waveguide_nl_process_lin(w[n], junct_a - out[n*2+1], junct_b - out[n*2], out+n*2, out+n*2+1)

#define PLATE_TIME                     0
#define PLATE_DAMPING                  1
#define PLATE_WET                      2
#define PLATE_INPUT                    3
#define PLATE_OUTPUTL                  4
#define PLATE_OUTPUTR                  5

static LADSPA_Descriptor *plateDescriptor = NULL;

typedef struct {
	LADSPA_Data *time;
	LADSPA_Data *damping;
	LADSPA_Data *wet;
	LADSPA_Data *input;
	LADSPA_Data *outputl;
	LADSPA_Data *outputr;
	float *      out;
	waveguide_nl **w;
	LADSPA_Data run_adding_gain;
} Plate;

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
		return plateDescriptor;
	default:
		return NULL;
	}
}

static void activatePlate(LADSPA_Handle instance) {
	Plate *plugin_data = (Plate *)instance;
	float *out = plugin_data->out;
	waveguide_nl **w = plugin_data->w;
#line 40 "plate_1423.xml"
	unsigned int i;

	for (i = 0; i < 8; i++) {
	  waveguide_nl_reset(w[i]);
	}
	plugin_data->out = out;
	plugin_data->w = w;

}

static void cleanupPlate(LADSPA_Handle instance) {
#line 85 "plate_1423.xml"
	Plate *plugin_data = (Plate *)instance;
	unsigned int i;

	for (i = 0; i < 8; i++) {
	  waveguide_nl_free(plugin_data->w[i]);
	}
	free(plugin_data->w);
	free(plugin_data->out);
	free(instance);
}

static void connectPortPlate(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Plate *plugin;

	plugin = (Plate *)instance;
	switch (port) {
	case PLATE_TIME:
		plugin->time = data;
		break;
	case PLATE_DAMPING:
		plugin->damping = data;
		break;
	case PLATE_WET:
		plugin->wet = data;
		break;
	case PLATE_INPUT:
		plugin->input = data;
		break;
	case PLATE_OUTPUTL:
		plugin->outputl = data;
		break;
	case PLATE_OUTPUTR:
		plugin->outputr = data;
		break;
	}
}

static LADSPA_Handle instantiatePlate(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Plate *plugin_data = (Plate *)malloc(sizeof(Plate));
	float *out = NULL;
	waveguide_nl **w = NULL;

#line 26 "plate_1423.xml"
	w = malloc(8 * sizeof(waveguide_nl *));
	w[0] = waveguide_nl_new(2389, LP_INNER, 0.04f, 0.0f);
	w[1] = waveguide_nl_new(4742, LP_INNER, 0.17f, 0.0f);
	w[2] = waveguide_nl_new(4623, LP_INNER, 0.52f, 0.0f);
	w[3] = waveguide_nl_new(2142, LP_INNER, 0.48f, 0.0f);
	w[4] = waveguide_nl_new(5597, LP_OUTER, 0.32f, 0.0f);
	w[5] = waveguide_nl_new(3692, LP_OUTER, 0.89f, 0.0f);
	w[6] = waveguide_nl_new(5611, LP_OUTER, 0.28f, 0.0f);
	w[7] = waveguide_nl_new(3703, LP_OUTER, 0.29f, 0.0f);

	out = calloc(32, sizeof(float));

	plugin_data->out = out;
	plugin_data->w = w;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runPlate(LADSPA_Handle instance, unsigned long sample_count) {
	Plate *plugin_data = (Plate *)instance;

	/* Reverb time (float value) */
	const LADSPA_Data time = *(plugin_data->time);

	/* Damping (float value) */
	const LADSPA_Data damping = *(plugin_data->damping);

	/* Dry/wet mix (float value) */
	const LADSPA_Data wet = *(plugin_data->wet);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Left output (array of floats of length sample_count) */
	LADSPA_Data * const outputl = plugin_data->outputl;

	/* Right output (array of floats of length sample_count) */
	LADSPA_Data * const outputr = plugin_data->outputr;
	float * out = plugin_data->out;
	waveguide_nl ** w = plugin_data->w;

#line 48 "plate_1423.xml"
	unsigned long pos;
	const float scale = powf(time * 0.117647f, 1.34f);
	const float lpscale = 1.0f - damping * 0.93;

	for (pos=0; pos<8; pos++) {
	  waveguide_nl_set_delay(w[pos], w[pos]->size * scale);
	}
	for (pos=0; pos<4; pos++) {
	  waveguide_nl_set_fc(w[pos], LP_INNER * lpscale);
	}
	for (; pos<8; pos++) {
	  waveguide_nl_set_fc(w[pos], LP_OUTER * lpscale);
	}

	for (pos = 0; pos < sample_count; pos++) {
	  const float alpha = (out[0] + out[2] + out[4] + out[6]) * 0.5f
	                      + input[pos];
	  const float beta = (out[1] + out[9] + out[14]) * 0.666666666f;
	  const float gamma = (out[3] + out[8] + out[11]) * 0.666666666f;
	  const float delta = (out[5] + out[10] + out[13]) * 0.666666666f;
	  const float epsilon = (out[7] + out[12] + out[15]) * 0.666666666f;

	  RUN_WG(0, beta, alpha);
	  RUN_WG(1, gamma, alpha);
	  RUN_WG(2, delta, alpha);
	  RUN_WG(3, epsilon, alpha);
	  RUN_WG(4, beta, gamma);
	  RUN_WG(5, gamma, delta);
	  RUN_WG(6, delta, epsilon);
	  RUN_WG(7, epsilon, beta);

	  buffer_write(outputl[pos], beta * wet + input[pos] * (1.0f - wet));
	  buffer_write(outputr[pos], gamma * wet + input[pos] * (1.0f - wet));
	}
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainPlate(LADSPA_Handle instance, LADSPA_Data gain) {
	((Plate *)instance)->run_adding_gain = gain;
}

static void runAddingPlate(LADSPA_Handle instance, unsigned long sample_count) {
	Plate *plugin_data = (Plate *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Reverb time (float value) */
	const LADSPA_Data time = *(plugin_data->time);

	/* Damping (float value) */
	const LADSPA_Data damping = *(plugin_data->damping);

	/* Dry/wet mix (float value) */
	const LADSPA_Data wet = *(plugin_data->wet);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Left output (array of floats of length sample_count) */
	LADSPA_Data * const outputl = plugin_data->outputl;

	/* Right output (array of floats of length sample_count) */
	LADSPA_Data * const outputr = plugin_data->outputr;
	float * out = plugin_data->out;
	waveguide_nl ** w = plugin_data->w;

#line 48 "plate_1423.xml"
	unsigned long pos;
	const float scale = powf(time * 0.117647f, 1.34f);
	const float lpscale = 1.0f - damping * 0.93;

	for (pos=0; pos<8; pos++) {
	  waveguide_nl_set_delay(w[pos], w[pos]->size * scale);
	}
	for (pos=0; pos<4; pos++) {
	  waveguide_nl_set_fc(w[pos], LP_INNER * lpscale);
	}
	for (; pos<8; pos++) {
	  waveguide_nl_set_fc(w[pos], LP_OUTER * lpscale);
	}

	for (pos = 0; pos < sample_count; pos++) {
	  const float alpha = (out[0] + out[2] + out[4] + out[6]) * 0.5f
	                      + input[pos];
	  const float beta = (out[1] + out[9] + out[14]) * 0.666666666f;
	  const float gamma = (out[3] + out[8] + out[11]) * 0.666666666f;
	  const float delta = (out[5] + out[10] + out[13]) * 0.666666666f;
	  const float epsilon = (out[7] + out[12] + out[15]) * 0.666666666f;

	  RUN_WG(0, beta, alpha);
	  RUN_WG(1, gamma, alpha);
	  RUN_WG(2, delta, alpha);
	  RUN_WG(3, epsilon, alpha);
	  RUN_WG(4, beta, gamma);
	  RUN_WG(5, gamma, delta);
	  RUN_WG(6, delta, epsilon);
	  RUN_WG(7, epsilon, beta);

	  buffer_write(outputl[pos], beta * wet + input[pos] * (1.0f - wet));
	  buffer_write(outputr[pos], gamma * wet + input[pos] * (1.0f - wet));
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


	plateDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (plateDescriptor) {
		plateDescriptor->UniqueID = 1423;
		plateDescriptor->Label = "plate";
		plateDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		plateDescriptor->Name =
		 D_("Plate reverb");
		plateDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		plateDescriptor->Copyright =
		 "GPL";
		plateDescriptor->PortCount = 6;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(6,
		 sizeof(LADSPA_PortDescriptor));
		plateDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(6,
		 sizeof(LADSPA_PortRangeHint));
		plateDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(6, sizeof(char*));
		plateDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Reverb time */
		port_descriptors[PLATE_TIME] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[PLATE_TIME] =
		 D_("Reverb time");
		port_range_hints[PLATE_TIME].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[PLATE_TIME].LowerBound = 0.01;
		port_range_hints[PLATE_TIME].UpperBound = 8.5;

		/* Parameters for Damping */
		port_descriptors[PLATE_DAMPING] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[PLATE_DAMPING] =
		 D_("Damping");
		port_range_hints[PLATE_DAMPING].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[PLATE_DAMPING].LowerBound = 0;
		port_range_hints[PLATE_DAMPING].UpperBound = 1;

		/* Parameters for Dry/wet mix */
		port_descriptors[PLATE_WET] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[PLATE_WET] =
		 D_("Dry/wet mix");
		port_range_hints[PLATE_WET].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[PLATE_WET].LowerBound = 0;
		port_range_hints[PLATE_WET].UpperBound = 1;

		/* Parameters for Input */
		port_descriptors[PLATE_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[PLATE_INPUT] =
		 D_("Input");
		port_range_hints[PLATE_INPUT].HintDescriptor = 0;

		/* Parameters for Left output */
		port_descriptors[PLATE_OUTPUTL] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[PLATE_OUTPUTL] =
		 D_("Left output");
		port_range_hints[PLATE_OUTPUTL].HintDescriptor = 0;

		/* Parameters for Right output */
		port_descriptors[PLATE_OUTPUTR] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[PLATE_OUTPUTR] =
		 D_("Right output");
		port_range_hints[PLATE_OUTPUTR].HintDescriptor = 0;

		plateDescriptor->activate = activatePlate;
		plateDescriptor->cleanup = cleanupPlate;
		plateDescriptor->connect_port = connectPortPlate;
		plateDescriptor->deactivate = NULL;
		plateDescriptor->instantiate = instantiatePlate;
		plateDescriptor->run = runPlate;
		plateDescriptor->run_adding = runAddingPlate;
		plateDescriptor->set_run_adding_gain = setRunAddingGainPlate;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (plateDescriptor) {
		free((LADSPA_PortDescriptor *)plateDescriptor->PortDescriptors);
		free((char **)plateDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)plateDescriptor->PortRangeHints);
		free(plateDescriptor);
	}

}
