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

#line 10 "gong_1424.xml"

#include "util/waveguide_nl.h"

#define RUN_WG(n, junct_a, junct_b) waveguide_nl_process(w[n], junct_a - out[n*2+1], junct_b - out[n*2], out+n*2, out+n*2+1)

#define GONG_DAMP_I                    0
#define GONG_DAMP_O                    1
#define GONG_MICPOS                    2
#define GONG_SCALE0                    3
#define GONG_APA0                      4
#define GONG_APB0                      5
#define GONG_SCALE1                    6
#define GONG_APA1                      7
#define GONG_APB1                      8
#define GONG_SCALE2                    9
#define GONG_APA2                      10
#define GONG_APB2                      11
#define GONG_SCALE3                    12
#define GONG_APA3                      13
#define GONG_APB3                      14
#define GONG_SCALE4                    15
#define GONG_APA4                      16
#define GONG_APB4                      17
#define GONG_SCALE5                    18
#define GONG_APA5                      19
#define GONG_APB5                      20
#define GONG_SCALE6                    21
#define GONG_APA6                      22
#define GONG_APB6                      23
#define GONG_SCALE7                    24
#define GONG_APA7                      25
#define GONG_APB7                      26
#define GONG_INPUT                     27
#define GONG_OUTPUT                    28

static LADSPA_Descriptor *gongDescriptor = NULL;

typedef struct {
	LADSPA_Data *damp_i;
	LADSPA_Data *damp_o;
	LADSPA_Data *micpos;
	LADSPA_Data *scale0;
	LADSPA_Data *apa0;
	LADSPA_Data *apb0;
	LADSPA_Data *scale1;
	LADSPA_Data *apa1;
	LADSPA_Data *apb1;
	LADSPA_Data *scale2;
	LADSPA_Data *apa2;
	LADSPA_Data *apb2;
	LADSPA_Data *scale3;
	LADSPA_Data *apa3;
	LADSPA_Data *apb3;
	LADSPA_Data *scale4;
	LADSPA_Data *apa4;
	LADSPA_Data *apb4;
	LADSPA_Data *scale5;
	LADSPA_Data *apa5;
	LADSPA_Data *apb5;
	LADSPA_Data *scale6;
	LADSPA_Data *apa6;
	LADSPA_Data *apb6;
	LADSPA_Data *scale7;
	LADSPA_Data *apa7;
	LADSPA_Data *apb7;
	LADSPA_Data *input;
	LADSPA_Data *output;
	int          maxsize_i;
	int          maxsize_o;
	float *      out;
	waveguide_nl **w;
	LADSPA_Data run_adding_gain;
} Gong;

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
		return gongDescriptor;
	default:
		return NULL;
	}
}

static void activateGong(LADSPA_Handle instance) {
	Gong *plugin_data = (Gong *)instance;
	int maxsize_i = plugin_data->maxsize_i;
	int maxsize_o = plugin_data->maxsize_o;
	float *out = plugin_data->out;
	waveguide_nl **w = plugin_data->w;
#line 44 "gong_1424.xml"
	unsigned int i;

	for (i = 0; i < 8; i++) {
	  waveguide_nl_reset(w[i]);
	}
	plugin_data->maxsize_i = maxsize_i;
	plugin_data->maxsize_o = maxsize_o;
	plugin_data->out = out;
	plugin_data->w = w;

}

static void cleanupGong(LADSPA_Handle instance) {
#line 110 "gong_1424.xml"
	Gong *plugin_data = (Gong *)instance;
	unsigned int i;

	for (i = 0; i < 8; i++) {
	  waveguide_nl_free(plugin_data->w[i]);
	}
	free(plugin_data->w);
	free(plugin_data->out);
	free(instance);
}

static void connectPortGong(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Gong *plugin;

	plugin = (Gong *)instance;
	switch (port) {
	case GONG_DAMP_I:
		plugin->damp_i = data;
		break;
	case GONG_DAMP_O:
		plugin->damp_o = data;
		break;
	case GONG_MICPOS:
		plugin->micpos = data;
		break;
	case GONG_SCALE0:
		plugin->scale0 = data;
		break;
	case GONG_APA0:
		plugin->apa0 = data;
		break;
	case GONG_APB0:
		plugin->apb0 = data;
		break;
	case GONG_SCALE1:
		plugin->scale1 = data;
		break;
	case GONG_APA1:
		plugin->apa1 = data;
		break;
	case GONG_APB1:
		plugin->apb1 = data;
		break;
	case GONG_SCALE2:
		plugin->scale2 = data;
		break;
	case GONG_APA2:
		plugin->apa2 = data;
		break;
	case GONG_APB2:
		plugin->apb2 = data;
		break;
	case GONG_SCALE3:
		plugin->scale3 = data;
		break;
	case GONG_APA3:
		plugin->apa3 = data;
		break;
	case GONG_APB3:
		plugin->apb3 = data;
		break;
	case GONG_SCALE4:
		plugin->scale4 = data;
		break;
	case GONG_APA4:
		plugin->apa4 = data;
		break;
	case GONG_APB4:
		plugin->apb4 = data;
		break;
	case GONG_SCALE5:
		plugin->scale5 = data;
		break;
	case GONG_APA5:
		plugin->apa5 = data;
		break;
	case GONG_APB5:
		plugin->apb5 = data;
		break;
	case GONG_SCALE6:
		plugin->scale6 = data;
		break;
	case GONG_APA6:
		plugin->apa6 = data;
		break;
	case GONG_APB6:
		plugin->apb6 = data;
		break;
	case GONG_SCALE7:
		plugin->scale7 = data;
		break;
	case GONG_APA7:
		plugin->apa7 = data;
		break;
	case GONG_APB7:
		plugin->apb7 = data;
		break;
	case GONG_INPUT:
		plugin->input = data;
		break;
	case GONG_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateGong(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Gong *plugin_data = (Gong *)malloc(sizeof(Gong));
	int maxsize_i;
	int maxsize_o;
	float *out = NULL;
	waveguide_nl **w = NULL;

#line 23 "gong_1424.xml"
	/* Max delay length for inner waveguides */
	maxsize_i = (float)s_rate * 0.03643242f;
	/* Max delay length for outer waveguides */
	maxsize_o = (float)s_rate * 0.05722782f;

	/* The waveguide structures */
	w = malloc(8 * sizeof(waveguide_nl *));
	w[0] = waveguide_nl_new(maxsize_i, 0.5, 0.0f, 0.0f);
	w[1] = waveguide_nl_new(maxsize_i, 0.5, 0.0f, 0.0f);
	w[2] = waveguide_nl_new(maxsize_i, 0.5, 0.0f, 0.0f);
	w[3] = waveguide_nl_new(maxsize_i, 0.5, 0.0f, 0.0f);
	w[4] = waveguide_nl_new(maxsize_o, 0.5, 0.0f, 0.0f);
	w[5] = waveguide_nl_new(maxsize_o, 0.5, 0.0f, 0.0f);
	w[6] = waveguide_nl_new(maxsize_o, 0.5, 0.0f, 0.0f);
	w[7] = waveguide_nl_new(maxsize_o, 0.5, 0.0f, 0.0f);

	/* Buffers to hold the currect deflections */
	out = calloc(32, sizeof(float));

	plugin_data->maxsize_i = maxsize_i;
	plugin_data->maxsize_o = maxsize_o;
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

static void runGong(LADSPA_Handle instance, unsigned long sample_count) {
	Gong *plugin_data = (Gong *)instance;

	/* Inner damping (float value) */
	const LADSPA_Data damp_i = *(plugin_data->damp_i);

	/* Outer damping (float value) */
	const LADSPA_Data damp_o = *(plugin_data->damp_o);

	/* Mic position (float value) */
	const LADSPA_Data micpos = *(plugin_data->micpos);

	/* Inner size 1 (float value) */
	const LADSPA_Data scale0 = *(plugin_data->scale0);

	/* Inner stiffness 1 + (float value) */
	const LADSPA_Data apa0 = *(plugin_data->apa0);

	/* Inner stiffness 1 - (float value) */
	const LADSPA_Data apb0 = *(plugin_data->apb0);

	/* Inner size 2 (float value) */
	const LADSPA_Data scale1 = *(plugin_data->scale1);

	/* Inner stiffness 2 + (float value) */
	const LADSPA_Data apa1 = *(plugin_data->apa1);

	/* Inner stiffness 2 - (float value) */
	const LADSPA_Data apb1 = *(plugin_data->apb1);

	/* Inner size 3 (float value) */
	const LADSPA_Data scale2 = *(plugin_data->scale2);

	/* Inner stiffness 3 + (float value) */
	const LADSPA_Data apa2 = *(plugin_data->apa2);

	/* Inner stiffness 3 - (float value) */
	const LADSPA_Data apb2 = *(plugin_data->apb2);

	/* Inner size 4 (float value) */
	const LADSPA_Data scale3 = *(plugin_data->scale3);

	/* Inner stiffness 4 + (float value) */
	const LADSPA_Data apa3 = *(plugin_data->apa3);

	/* Inner stiffness 4 - (float value) */
	const LADSPA_Data apb3 = *(plugin_data->apb3);

	/* Outer size 1 (float value) */
	const LADSPA_Data scale4 = *(plugin_data->scale4);

	/* Outer stiffness 1 + (float value) */
	const LADSPA_Data apa4 = *(plugin_data->apa4);

	/* Outer stiffness 1 - (float value) */
	const LADSPA_Data apb4 = *(plugin_data->apb4);

	/* Outer size 2 (float value) */
	const LADSPA_Data scale5 = *(plugin_data->scale5);

	/* Outer stiffness 2 + (float value) */
	const LADSPA_Data apa5 = *(plugin_data->apa5);

	/* Outer stiffness 2 - (float value) */
	const LADSPA_Data apb5 = *(plugin_data->apb5);

	/* Outer size 3 (float value) */
	const LADSPA_Data scale6 = *(plugin_data->scale6);

	/* Outer stiffness 3 + (float value) */
	const LADSPA_Data apa6 = *(plugin_data->apa6);

	/* Outer stiffness 3 - (float value) */
	const LADSPA_Data apb6 = *(plugin_data->apb6);

	/* Outer size 4 (float value) */
	const LADSPA_Data scale7 = *(plugin_data->scale7);

	/* Outer stiffness 4 + (float value) */
	const LADSPA_Data apa7 = *(plugin_data->apa7);

	/* Outer stiffness 4 - (float value) */
	const LADSPA_Data apb7 = *(plugin_data->apb7);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	int maxsize_i = plugin_data->maxsize_i;
	int maxsize_o = plugin_data->maxsize_o;
	float * out = plugin_data->out;
	waveguide_nl ** w = plugin_data->w;

#line 52 "gong_1424.xml"
	unsigned long pos;
	/* The a coef of the inner lowpass */
	const float lpi = 1.0f - damp_i * 0.1423f;
	/* The a coef of the outer lowpass */
	const float lpo = 1.0f - damp_o * 0.19543f;

	/* Set the parameters of the waveguides */
	waveguide_nl_set_delay(w[0], maxsize_i * scale0);
	waveguide_nl_set_ap(w[0], apa0, apb0);
	waveguide_nl_set_delay(w[1], maxsize_i * scale1);
	waveguide_nl_set_ap(w[1], apa1, apb1);
	waveguide_nl_set_delay(w[2], maxsize_i * scale2);
	waveguide_nl_set_ap(w[2], apa2, apb2);
	waveguide_nl_set_delay(w[3], maxsize_i * scale3);
	waveguide_nl_set_ap(w[3], apa3, apb3);
	waveguide_nl_set_delay(w[4], maxsize_o * scale4);
	waveguide_nl_set_ap(w[4], apa4, apb4);
	waveguide_nl_set_delay(w[5], maxsize_o * scale5);
	waveguide_nl_set_ap(w[5], apa5, apb5);
	waveguide_nl_set_delay(w[6], maxsize_o * scale6);
	waveguide_nl_set_ap(w[6], apa6, apb6);
	waveguide_nl_set_delay(w[7], maxsize_o * scale7);
	waveguide_nl_set_ap(w[7], apa7, apb7);

	for (pos=0; pos<4; pos++) {
	  waveguide_nl_set_fc(w[pos], lpi);
	}
	for (; pos<8; pos++) {
	  waveguide_nl_set_fc(w[pos], lpo);
	}

	for (pos = 0; pos < sample_count; pos++) {
	  /* Calcualte the deflections at the wavejunctions
	     alpha is the centre, beta is north, gamma is east,
	     delta is south and epsilon is west */
	  const float alpha = (out[0] + out[2] + out[4] + out[6]) * 0.5f
	                      + input[pos];
	  const float beta = (out[1] + out[9] + out[14]) * 0.666666666666f;
	  const float gamma = (out[3] + out[8] + out[11]) * 0.666666666666f;
	  const float delta = (out[5] + out[10] + out[13]) * 0.666666666666f;
	  const float epsilon = (out[7] + out[12] + out[15]) * 0.666666666666f;

	  /* Inject the energy at the junctions + reflections into the
	     waveguides (the macro gives the reflection calcs) */
	  RUN_WG(0, beta, alpha);
	  RUN_WG(1, gamma, alpha);
	  RUN_WG(2, delta, alpha);
	  RUN_WG(3, epsilon, alpha);
	  RUN_WG(4, beta, gamma);
	  RUN_WG(5, gamma, delta);
	  RUN_WG(6, delta, epsilon);
	  RUN_WG(7, epsilon, beta);

	  buffer_write(output[pos], (1.0f - micpos) * alpha + micpos * delta);
	}
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainGong(LADSPA_Handle instance, LADSPA_Data gain) {
	((Gong *)instance)->run_adding_gain = gain;
}

static void runAddingGong(LADSPA_Handle instance, unsigned long sample_count) {
	Gong *plugin_data = (Gong *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Inner damping (float value) */
	const LADSPA_Data damp_i = *(plugin_data->damp_i);

	/* Outer damping (float value) */
	const LADSPA_Data damp_o = *(plugin_data->damp_o);

	/* Mic position (float value) */
	const LADSPA_Data micpos = *(plugin_data->micpos);

	/* Inner size 1 (float value) */
	const LADSPA_Data scale0 = *(plugin_data->scale0);

	/* Inner stiffness 1 + (float value) */
	const LADSPA_Data apa0 = *(plugin_data->apa0);

	/* Inner stiffness 1 - (float value) */
	const LADSPA_Data apb0 = *(plugin_data->apb0);

	/* Inner size 2 (float value) */
	const LADSPA_Data scale1 = *(plugin_data->scale1);

	/* Inner stiffness 2 + (float value) */
	const LADSPA_Data apa1 = *(plugin_data->apa1);

	/* Inner stiffness 2 - (float value) */
	const LADSPA_Data apb1 = *(plugin_data->apb1);

	/* Inner size 3 (float value) */
	const LADSPA_Data scale2 = *(plugin_data->scale2);

	/* Inner stiffness 3 + (float value) */
	const LADSPA_Data apa2 = *(plugin_data->apa2);

	/* Inner stiffness 3 - (float value) */
	const LADSPA_Data apb2 = *(plugin_data->apb2);

	/* Inner size 4 (float value) */
	const LADSPA_Data scale3 = *(plugin_data->scale3);

	/* Inner stiffness 4 + (float value) */
	const LADSPA_Data apa3 = *(plugin_data->apa3);

	/* Inner stiffness 4 - (float value) */
	const LADSPA_Data apb3 = *(plugin_data->apb3);

	/* Outer size 1 (float value) */
	const LADSPA_Data scale4 = *(plugin_data->scale4);

	/* Outer stiffness 1 + (float value) */
	const LADSPA_Data apa4 = *(plugin_data->apa4);

	/* Outer stiffness 1 - (float value) */
	const LADSPA_Data apb4 = *(plugin_data->apb4);

	/* Outer size 2 (float value) */
	const LADSPA_Data scale5 = *(plugin_data->scale5);

	/* Outer stiffness 2 + (float value) */
	const LADSPA_Data apa5 = *(plugin_data->apa5);

	/* Outer stiffness 2 - (float value) */
	const LADSPA_Data apb5 = *(plugin_data->apb5);

	/* Outer size 3 (float value) */
	const LADSPA_Data scale6 = *(plugin_data->scale6);

	/* Outer stiffness 3 + (float value) */
	const LADSPA_Data apa6 = *(plugin_data->apa6);

	/* Outer stiffness 3 - (float value) */
	const LADSPA_Data apb6 = *(plugin_data->apb6);

	/* Outer size 4 (float value) */
	const LADSPA_Data scale7 = *(plugin_data->scale7);

	/* Outer stiffness 4 + (float value) */
	const LADSPA_Data apa7 = *(plugin_data->apa7);

	/* Outer stiffness 4 - (float value) */
	const LADSPA_Data apb7 = *(plugin_data->apb7);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	int maxsize_i = plugin_data->maxsize_i;
	int maxsize_o = plugin_data->maxsize_o;
	float * out = plugin_data->out;
	waveguide_nl ** w = plugin_data->w;

#line 52 "gong_1424.xml"
	unsigned long pos;
	/* The a coef of the inner lowpass */
	const float lpi = 1.0f - damp_i * 0.1423f;
	/* The a coef of the outer lowpass */
	const float lpo = 1.0f - damp_o * 0.19543f;

	/* Set the parameters of the waveguides */
	waveguide_nl_set_delay(w[0], maxsize_i * scale0);
	waveguide_nl_set_ap(w[0], apa0, apb0);
	waveguide_nl_set_delay(w[1], maxsize_i * scale1);
	waveguide_nl_set_ap(w[1], apa1, apb1);
	waveguide_nl_set_delay(w[2], maxsize_i * scale2);
	waveguide_nl_set_ap(w[2], apa2, apb2);
	waveguide_nl_set_delay(w[3], maxsize_i * scale3);
	waveguide_nl_set_ap(w[3], apa3, apb3);
	waveguide_nl_set_delay(w[4], maxsize_o * scale4);
	waveguide_nl_set_ap(w[4], apa4, apb4);
	waveguide_nl_set_delay(w[5], maxsize_o * scale5);
	waveguide_nl_set_ap(w[5], apa5, apb5);
	waveguide_nl_set_delay(w[6], maxsize_o * scale6);
	waveguide_nl_set_ap(w[6], apa6, apb6);
	waveguide_nl_set_delay(w[7], maxsize_o * scale7);
	waveguide_nl_set_ap(w[7], apa7, apb7);

	for (pos=0; pos<4; pos++) {
	  waveguide_nl_set_fc(w[pos], lpi);
	}
	for (; pos<8; pos++) {
	  waveguide_nl_set_fc(w[pos], lpo);
	}

	for (pos = 0; pos < sample_count; pos++) {
	  /* Calcualte the deflections at the wavejunctions
	     alpha is the centre, beta is north, gamma is east,
	     delta is south and epsilon is west */
	  const float alpha = (out[0] + out[2] + out[4] + out[6]) * 0.5f
	                      + input[pos];
	  const float beta = (out[1] + out[9] + out[14]) * 0.666666666666f;
	  const float gamma = (out[3] + out[8] + out[11]) * 0.666666666666f;
	  const float delta = (out[5] + out[10] + out[13]) * 0.666666666666f;
	  const float epsilon = (out[7] + out[12] + out[15]) * 0.666666666666f;

	  /* Inject the energy at the junctions + reflections into the
	     waveguides (the macro gives the reflection calcs) */
	  RUN_WG(0, beta, alpha);
	  RUN_WG(1, gamma, alpha);
	  RUN_WG(2, delta, alpha);
	  RUN_WG(3, epsilon, alpha);
	  RUN_WG(4, beta, gamma);
	  RUN_WG(5, gamma, delta);
	  RUN_WG(6, delta, epsilon);
	  RUN_WG(7, epsilon, beta);

	  buffer_write(output[pos], (1.0f - micpos) * alpha + micpos * delta);
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


	gongDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (gongDescriptor) {
		gongDescriptor->UniqueID = 1424;
		gongDescriptor->Label = "gong";
		gongDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		gongDescriptor->Name =
		 D_("Gong model");
		gongDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		gongDescriptor->Copyright =
		 "GPL";
		gongDescriptor->PortCount = 29;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(29,
		 sizeof(LADSPA_PortDescriptor));
		gongDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(29,
		 sizeof(LADSPA_PortRangeHint));
		gongDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(29, sizeof(char*));
		gongDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Inner damping */
		port_descriptors[GONG_DAMP_I] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GONG_DAMP_I] =
		 D_("Inner damping");
		port_range_hints[GONG_DAMP_I].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[GONG_DAMP_I].LowerBound = 0;
		port_range_hints[GONG_DAMP_I].UpperBound = 1;

		/* Parameters for Outer damping */
		port_descriptors[GONG_DAMP_O] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GONG_DAMP_O] =
		 D_("Outer damping");
		port_range_hints[GONG_DAMP_O].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[GONG_DAMP_O].LowerBound = 0;
		port_range_hints[GONG_DAMP_O].UpperBound = 1;

		/* Parameters for Mic position */
		port_descriptors[GONG_MICPOS] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GONG_MICPOS] =
		 D_("Mic position");
		port_range_hints[GONG_MICPOS].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[GONG_MICPOS].LowerBound = 0;
		port_range_hints[GONG_MICPOS].UpperBound = 1;

		/* Parameters for Inner size 1 */
		port_descriptors[GONG_SCALE0] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GONG_SCALE0] =
		 D_("Inner size 1");
		port_range_hints[GONG_SCALE0].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[GONG_SCALE0].LowerBound = 0;
		port_range_hints[GONG_SCALE0].UpperBound = 1;

		/* Parameters for Inner stiffness 1 + */
		port_descriptors[GONG_APA0] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GONG_APA0] =
		 D_("Inner stiffness 1 +");
		port_range_hints[GONG_APA0].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[GONG_APA0].LowerBound = 0;
		port_range_hints[GONG_APA0].UpperBound = 1;

		/* Parameters for Inner stiffness 1 - */
		port_descriptors[GONG_APB0] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GONG_APB0] =
		 D_("Inner stiffness 1 -");
		port_range_hints[GONG_APB0].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[GONG_APB0].LowerBound = 0;
		port_range_hints[GONG_APB0].UpperBound = 1;

		/* Parameters for Inner size 2 */
		port_descriptors[GONG_SCALE1] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GONG_SCALE1] =
		 D_("Inner size 2");
		port_range_hints[GONG_SCALE1].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[GONG_SCALE1].LowerBound = 0;
		port_range_hints[GONG_SCALE1].UpperBound = 1;

		/* Parameters for Inner stiffness 2 + */
		port_descriptors[GONG_APA1] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GONG_APA1] =
		 D_("Inner stiffness 2 +");
		port_range_hints[GONG_APA1].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[GONG_APA1].LowerBound = 0;
		port_range_hints[GONG_APA1].UpperBound = 1;

		/* Parameters for Inner stiffness 2 - */
		port_descriptors[GONG_APB1] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GONG_APB1] =
		 D_("Inner stiffness 2 -");
		port_range_hints[GONG_APB1].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[GONG_APB1].LowerBound = 0;
		port_range_hints[GONG_APB1].UpperBound = 1;

		/* Parameters for Inner size 3 */
		port_descriptors[GONG_SCALE2] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GONG_SCALE2] =
		 D_("Inner size 3");
		port_range_hints[GONG_SCALE2].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[GONG_SCALE2].LowerBound = 0;
		port_range_hints[GONG_SCALE2].UpperBound = 1;

		/* Parameters for Inner stiffness 3 + */
		port_descriptors[GONG_APA2] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GONG_APA2] =
		 D_("Inner stiffness 3 +");
		port_range_hints[GONG_APA2].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[GONG_APA2].LowerBound = 0;
		port_range_hints[GONG_APA2].UpperBound = 1;

		/* Parameters for Inner stiffness 3 - */
		port_descriptors[GONG_APB2] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GONG_APB2] =
		 D_("Inner stiffness 3 -");
		port_range_hints[GONG_APB2].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[GONG_APB2].LowerBound = 0;
		port_range_hints[GONG_APB2].UpperBound = 1;

		/* Parameters for Inner size 4 */
		port_descriptors[GONG_SCALE3] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GONG_SCALE3] =
		 D_("Inner size 4");
		port_range_hints[GONG_SCALE3].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[GONG_SCALE3].LowerBound = 0;
		port_range_hints[GONG_SCALE3].UpperBound = 1;

		/* Parameters for Inner stiffness 4 + */
		port_descriptors[GONG_APA3] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GONG_APA3] =
		 D_("Inner stiffness 4 +");
		port_range_hints[GONG_APA3].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[GONG_APA3].LowerBound = 0;
		port_range_hints[GONG_APA3].UpperBound = 1;

		/* Parameters for Inner stiffness 4 - */
		port_descriptors[GONG_APB3] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GONG_APB3] =
		 D_("Inner stiffness 4 -");
		port_range_hints[GONG_APB3].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[GONG_APB3].LowerBound = 0;
		port_range_hints[GONG_APB3].UpperBound = 1;

		/* Parameters for Outer size 1 */
		port_descriptors[GONG_SCALE4] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GONG_SCALE4] =
		 D_("Outer size 1");
		port_range_hints[GONG_SCALE4].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[GONG_SCALE4].LowerBound = 0;
		port_range_hints[GONG_SCALE4].UpperBound = 1;

		/* Parameters for Outer stiffness 1 + */
		port_descriptors[GONG_APA4] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GONG_APA4] =
		 D_("Outer stiffness 1 +");
		port_range_hints[GONG_APA4].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[GONG_APA4].LowerBound = 0;
		port_range_hints[GONG_APA4].UpperBound = 1;

		/* Parameters for Outer stiffness 1 - */
		port_descriptors[GONG_APB4] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GONG_APB4] =
		 D_("Outer stiffness 1 -");
		port_range_hints[GONG_APB4].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[GONG_APB4].LowerBound = 0;
		port_range_hints[GONG_APB4].UpperBound = 1;

		/* Parameters for Outer size 2 */
		port_descriptors[GONG_SCALE5] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GONG_SCALE5] =
		 D_("Outer size 2");
		port_range_hints[GONG_SCALE5].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[GONG_SCALE5].LowerBound = 0;
		port_range_hints[GONG_SCALE5].UpperBound = 1;

		/* Parameters for Outer stiffness 2 + */
		port_descriptors[GONG_APA5] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GONG_APA5] =
		 D_("Outer stiffness 2 +");
		port_range_hints[GONG_APA5].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[GONG_APA5].LowerBound = 0;
		port_range_hints[GONG_APA5].UpperBound = 1;

		/* Parameters for Outer stiffness 2 - */
		port_descriptors[GONG_APB5] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GONG_APB5] =
		 D_("Outer stiffness 2 -");
		port_range_hints[GONG_APB5].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[GONG_APB5].LowerBound = 0;
		port_range_hints[GONG_APB5].UpperBound = 1;

		/* Parameters for Outer size 3 */
		port_descriptors[GONG_SCALE6] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GONG_SCALE6] =
		 D_("Outer size 3");
		port_range_hints[GONG_SCALE6].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[GONG_SCALE6].LowerBound = 0;
		port_range_hints[GONG_SCALE6].UpperBound = 1;

		/* Parameters for Outer stiffness 3 + */
		port_descriptors[GONG_APA6] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GONG_APA6] =
		 D_("Outer stiffness 3 +");
		port_range_hints[GONG_APA6].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[GONG_APA6].LowerBound = 0;
		port_range_hints[GONG_APA6].UpperBound = 1;

		/* Parameters for Outer stiffness 3 - */
		port_descriptors[GONG_APB6] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GONG_APB6] =
		 D_("Outer stiffness 3 -");
		port_range_hints[GONG_APB6].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[GONG_APB6].LowerBound = 0;
		port_range_hints[GONG_APB6].UpperBound = 1;

		/* Parameters for Outer size 4 */
		port_descriptors[GONG_SCALE7] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GONG_SCALE7] =
		 D_("Outer size 4");
		port_range_hints[GONG_SCALE7].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[GONG_SCALE7].LowerBound = 0;
		port_range_hints[GONG_SCALE7].UpperBound = 1;

		/* Parameters for Outer stiffness 4 + */
		port_descriptors[GONG_APA7] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GONG_APA7] =
		 D_("Outer stiffness 4 +");
		port_range_hints[GONG_APA7].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[GONG_APA7].LowerBound = 0;
		port_range_hints[GONG_APA7].UpperBound = 1;

		/* Parameters for Outer stiffness 4 - */
		port_descriptors[GONG_APB7] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[GONG_APB7] =
		 D_("Outer stiffness 4 -");
		port_range_hints[GONG_APB7].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[GONG_APB7].LowerBound = 0;
		port_range_hints[GONG_APB7].UpperBound = 1;

		/* Parameters for Input */
		port_descriptors[GONG_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[GONG_INPUT] =
		 D_("Input");
		port_range_hints[GONG_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[GONG_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[GONG_OUTPUT] =
		 D_("Output");
		port_range_hints[GONG_OUTPUT].HintDescriptor = 0;

		gongDescriptor->activate = activateGong;
		gongDescriptor->cleanup = cleanupGong;
		gongDescriptor->connect_port = connectPortGong;
		gongDescriptor->deactivate = NULL;
		gongDescriptor->instantiate = instantiateGong;
		gongDescriptor->run = runGong;
		gongDescriptor->run_adding = runAddingGong;
		gongDescriptor->set_run_adding_gain = setRunAddingGainGong;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (gongDescriptor) {
		free((LADSPA_PortDescriptor *)gongDescriptor->PortDescriptors);
		free((char **)gongDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)gongDescriptor->PortRangeHints);
		free(gongDescriptor);
	}

}
