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

#line 10 "bode_shifter_1431.xml"

#include <math.h>

#include "ladspa-util.h"

#define SIN_T_SIZE 1024
#define D_SIZE 256
#define NZEROS 200

/* The non-zero taps of the Hilbert transformer */
static float xcoeffs[] = {
     +0.0008103736f, +0.0008457886f, +0.0009017196f, +0.0009793364f,
     +0.0010798341f, +0.0012044365f, +0.0013544008f, +0.0015310235f,
     +0.0017356466f, +0.0019696659f, +0.0022345404f, +0.0025318040f,
     +0.0028630784f, +0.0032300896f, +0.0036346867f, +0.0040788644f,
     +0.0045647903f, +0.0050948365f, +0.0056716186f, +0.0062980419f,
     +0.0069773575f, +0.0077132300f, +0.0085098208f, +0.0093718901f,
     +0.0103049226f, +0.0113152847f, +0.0124104218f, +0.0135991079f,
     +0.0148917649f, +0.0163008758f, +0.0178415242f, +0.0195321089f,
     +0.0213953037f, +0.0234593652f, +0.0257599469f, +0.0283426636f,
     +0.0312667947f, +0.0346107648f, +0.0384804823f, +0.0430224431f,
     +0.0484451086f, +0.0550553725f, +0.0633242001f, +0.0740128560f,
     +0.0884368322f, +0.1090816773f, +0.1412745301f, +0.1988673273f,
     +0.3326528346f, +0.9997730178f, -0.9997730178f, -0.3326528346f,
     -0.1988673273f, -0.1412745301f, -0.1090816773f, -0.0884368322f,
     -0.0740128560f, -0.0633242001f, -0.0550553725f, -0.0484451086f,
     -0.0430224431f, -0.0384804823f, -0.0346107648f, -0.0312667947f,
     -0.0283426636f, -0.0257599469f, -0.0234593652f, -0.0213953037f,
     -0.0195321089f, -0.0178415242f, -0.0163008758f, -0.0148917649f,
     -0.0135991079f, -0.0124104218f, -0.0113152847f, -0.0103049226f,
     -0.0093718901f, -0.0085098208f, -0.0077132300f, -0.0069773575f,
     -0.0062980419f, -0.0056716186f, -0.0050948365f, -0.0045647903f,
     -0.0040788644f, -0.0036346867f, -0.0032300896f, -0.0028630784f,
     -0.0025318040f, -0.0022345404f, -0.0019696659f, -0.0017356466f,
     -0.0015310235f, -0.0013544008f, -0.0012044365f, -0.0010798341f,
     -0.0009793364f, -0.0009017196f, -0.0008457886f, -0.0008103736f,
};

#define BODESHIFTER_SHIFT              0
#define BODESHIFTER_INPUT              1
#define BODESHIFTER_DOUT               2
#define BODESHIFTER_UOUT               3
#define BODESHIFTER_LATENCY            4

static LADSPA_Descriptor *bodeShifterDescriptor = NULL;

typedef struct {
	LADSPA_Data *shift;
	LADSPA_Data *input;
	LADSPA_Data *dout;
	LADSPA_Data *uout;
	LADSPA_Data *latency;
	LADSPA_Data *delay;
	unsigned int dptr;
	float        fs;
	float        last_shift;
	float        phi;
	float *      sint;
	LADSPA_Data run_adding_gain;
} BodeShifter;

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
		return bodeShifterDescriptor;
	default:
		return NULL;
	}
}

static void cleanupBodeShifter(LADSPA_Handle instance) {
#line 75 "bode_shifter_1431.xml"
	BodeShifter *plugin_data = (BodeShifter *)instance;
	free(plugin_data->delay);
	free(plugin_data->sint);
	free(instance);
}

static void connectPortBodeShifter(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	BodeShifter *plugin;

	plugin = (BodeShifter *)instance;
	switch (port) {
	case BODESHIFTER_SHIFT:
		plugin->shift = data;
		break;
	case BODESHIFTER_INPUT:
		plugin->input = data;
		break;
	case BODESHIFTER_DOUT:
		plugin->dout = data;
		break;
	case BODESHIFTER_UOUT:
		plugin->uout = data;
		break;
	case BODESHIFTER_LATENCY:
		plugin->latency = data;
		break;
	}
}

static LADSPA_Handle instantiateBodeShifter(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	BodeShifter *plugin_data = (BodeShifter *)malloc(sizeof(BodeShifter));
	LADSPA_Data *delay = NULL;
	unsigned int dptr;
	float fs;
	float last_shift;
	float phi;
	float *sint = NULL;

#line 58 "bode_shifter_1431.xml"
	unsigned int i;

	fs = (float)s_rate;

	delay = calloc(D_SIZE, sizeof(LADSPA_Data));
	sint  = calloc(SIN_T_SIZE + 4, sizeof(float));

	dptr = 0;
	phi = 0.0f;
	last_shift = 0.0f;

	for (i = 0; i < SIN_T_SIZE + 4; i++) {
	  sint[i] = sinf(2.0f * M_PI * (float)i / (float)SIN_T_SIZE);
	}

	plugin_data->delay = delay;
	plugin_data->dptr = dptr;
	plugin_data->fs = fs;
	plugin_data->last_shift = last_shift;
	plugin_data->phi = phi;
	plugin_data->sint = sint;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runBodeShifter(LADSPA_Handle instance, unsigned long sample_count) {
	BodeShifter *plugin_data = (BodeShifter *)instance;

	/* Frequency shift (float value) */
	const LADSPA_Data shift = *(plugin_data->shift);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Down out (array of floats of length sample_count) */
	LADSPA_Data * const dout = plugin_data->dout;

	/* Up out (array of floats of length sample_count) */
	LADSPA_Data * const uout = plugin_data->uout;
	LADSPA_Data * delay = plugin_data->delay;
	unsigned int dptr = plugin_data->dptr;
	float fs = plugin_data->fs;
	float last_shift = plugin_data->last_shift;
	float phi = plugin_data->phi;
	float * sint = plugin_data->sint;

#line 80 "bode_shifter_1431.xml"
	unsigned long pos;
	unsigned int i;
	float hilb, rm1, rm2;
	float shift_i = last_shift;
	int int_p;
	float frac_p;
	const float shift_c = f_clamp(shift, 0.0f, 10000.0f);
	const float shift_inc = (shift_c - last_shift) / (float)sample_count;
	const float freq_fix = (float)SIN_T_SIZE / fs;

	for (pos = 0; pos < sample_count; pos++) {
	  delay[dptr] = input[pos];

	  /* Perform the Hilbert FIR convolution
	   * (probably FFT would be faster) */
	  hilb = 0.0f;
	  for (i = 0; i < NZEROS/2; i++) {
	      hilb += (xcoeffs[i] * delay[(dptr - i*2) & (D_SIZE - 1)]);
	  }

	  /* Calcuate the table positions for the sine modulator */
	  int_p = f_round(floor(phi));

	  /* Calculate ringmod1, the transformed input modulated with a shift Hz
	   * sinewave. This creates a +180 degree sideband at source-shift Hz and
	   * a 0 degree sindeband at source+shift Hz */
	  frac_p = phi - int_p;

	  /* the Hilbert has a gain of pi/2, which we have to correct for, thanks
	   * Fons! */
	  rm1 = hilb * 0.63661978f * cube_interp(frac_p, sint[int_p],
	                  sint[int_p+1], sint[int_p+2], sint[int_p+3]);

	  /* Calcuate the table positions for the cosine modulator */
	  int_p = (int_p + SIN_T_SIZE / 4) & (SIN_T_SIZE - 1);

	  /* Calculate ringmod2, the delayed input modulated with a shift Hz
	   * cosinewave. This creates a 0 degree sideband at source+shift Hz
	   * and a -180 degree sindeband at source-shift Hz */
	  rm2 = delay[(dptr - 99) & (D_SIZE - 1)] * cube_interp(frac_p,
	        sint[int_p], sint[int_p+1], sint[int_p+2], sint[int_p+3]);

	  /* Output the sum and differences of the ringmods. The +/-180 degree
	   * sidebands cancel (more of less) and just leave the shifted
	   * components */
	  buffer_write(dout[pos], (rm2 - rm1) * 0.5f);
	  buffer_write(uout[pos], (rm2 + rm1) * 0.5f);

	  dptr = (dptr + 1) & (D_SIZE - 1);
	  phi += shift_i * freq_fix;
	  while (phi > SIN_T_SIZE) {
	          phi -= SIN_T_SIZE;
	  }
	  shift_i += shift_inc;
	}

	plugin_data->dptr = dptr;
	plugin_data->phi = phi;
	plugin_data->last_shift = shift_c;

	*(plugin_data->latency) = 99;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainBodeShifter(LADSPA_Handle instance, LADSPA_Data gain) {
	((BodeShifter *)instance)->run_adding_gain = gain;
}

static void runAddingBodeShifter(LADSPA_Handle instance, unsigned long sample_count) {
	BodeShifter *plugin_data = (BodeShifter *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Frequency shift (float value) */
	const LADSPA_Data shift = *(plugin_data->shift);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Down out (array of floats of length sample_count) */
	LADSPA_Data * const dout = plugin_data->dout;

	/* Up out (array of floats of length sample_count) */
	LADSPA_Data * const uout = plugin_data->uout;
	LADSPA_Data * delay = plugin_data->delay;
	unsigned int dptr = plugin_data->dptr;
	float fs = plugin_data->fs;
	float last_shift = plugin_data->last_shift;
	float phi = plugin_data->phi;
	float * sint = plugin_data->sint;

#line 80 "bode_shifter_1431.xml"
	unsigned long pos;
	unsigned int i;
	float hilb, rm1, rm2;
	float shift_i = last_shift;
	int int_p;
	float frac_p;
	const float shift_c = f_clamp(shift, 0.0f, 10000.0f);
	const float shift_inc = (shift_c - last_shift) / (float)sample_count;
	const float freq_fix = (float)SIN_T_SIZE / fs;

	for (pos = 0; pos < sample_count; pos++) {
	  delay[dptr] = input[pos];

	  /* Perform the Hilbert FIR convolution
	   * (probably FFT would be faster) */
	  hilb = 0.0f;
	  for (i = 0; i < NZEROS/2; i++) {
	      hilb += (xcoeffs[i] * delay[(dptr - i*2) & (D_SIZE - 1)]);
	  }

	  /* Calcuate the table positions for the sine modulator */
	  int_p = f_round(floor(phi));

	  /* Calculate ringmod1, the transformed input modulated with a shift Hz
	   * sinewave. This creates a +180 degree sideband at source-shift Hz and
	   * a 0 degree sindeband at source+shift Hz */
	  frac_p = phi - int_p;

	  /* the Hilbert has a gain of pi/2, which we have to correct for, thanks
	   * Fons! */
	  rm1 = hilb * 0.63661978f * cube_interp(frac_p, sint[int_p],
	                  sint[int_p+1], sint[int_p+2], sint[int_p+3]);

	  /* Calcuate the table positions for the cosine modulator */
	  int_p = (int_p + SIN_T_SIZE / 4) & (SIN_T_SIZE - 1);

	  /* Calculate ringmod2, the delayed input modulated with a shift Hz
	   * cosinewave. This creates a 0 degree sideband at source+shift Hz
	   * and a -180 degree sindeband at source-shift Hz */
	  rm2 = delay[(dptr - 99) & (D_SIZE - 1)] * cube_interp(frac_p,
	        sint[int_p], sint[int_p+1], sint[int_p+2], sint[int_p+3]);

	  /* Output the sum and differences of the ringmods. The +/-180 degree
	   * sidebands cancel (more of less) and just leave the shifted
	   * components */
	  buffer_write(dout[pos], (rm2 - rm1) * 0.5f);
	  buffer_write(uout[pos], (rm2 + rm1) * 0.5f);

	  dptr = (dptr + 1) & (D_SIZE - 1);
	  phi += shift_i * freq_fix;
	  while (phi > SIN_T_SIZE) {
	          phi -= SIN_T_SIZE;
	  }
	  shift_i += shift_inc;
	}

	plugin_data->dptr = dptr;
	plugin_data->phi = phi;
	plugin_data->last_shift = shift_c;

	*(plugin_data->latency) = 99;
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


	bodeShifterDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (bodeShifterDescriptor) {
		bodeShifterDescriptor->UniqueID = 1431;
		bodeShifterDescriptor->Label = "bodeShifter";
		bodeShifterDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		bodeShifterDescriptor->Name =
		 D_("Bode frequency shifter");
		bodeShifterDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		bodeShifterDescriptor->Copyright =
		 "GPL";
		bodeShifterDescriptor->PortCount = 5;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(5,
		 sizeof(LADSPA_PortDescriptor));
		bodeShifterDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(5,
		 sizeof(LADSPA_PortRangeHint));
		bodeShifterDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(5, sizeof(char*));
		bodeShifterDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Frequency shift */
		port_descriptors[BODESHIFTER_SHIFT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[BODESHIFTER_SHIFT] =
		 D_("Frequency shift");
		port_range_hints[BODESHIFTER_SHIFT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[BODESHIFTER_SHIFT].LowerBound = 0;
		port_range_hints[BODESHIFTER_SHIFT].UpperBound = 5000;

		/* Parameters for Input */
		port_descriptors[BODESHIFTER_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[BODESHIFTER_INPUT] =
		 D_("Input");
		port_range_hints[BODESHIFTER_INPUT].HintDescriptor = 0;

		/* Parameters for Down out */
		port_descriptors[BODESHIFTER_DOUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[BODESHIFTER_DOUT] =
		 D_("Down out");
		port_range_hints[BODESHIFTER_DOUT].HintDescriptor = 0;

		/* Parameters for Up out */
		port_descriptors[BODESHIFTER_UOUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[BODESHIFTER_UOUT] =
		 D_("Up out");
		port_range_hints[BODESHIFTER_UOUT].HintDescriptor = 0;

		/* Parameters for latency */
		port_descriptors[BODESHIFTER_LATENCY] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL;
		port_names[BODESHIFTER_LATENCY] =
		 D_("latency");
		port_range_hints[BODESHIFTER_LATENCY].HintDescriptor = 0;

		bodeShifterDescriptor->activate = NULL;
		bodeShifterDescriptor->cleanup = cleanupBodeShifter;
		bodeShifterDescriptor->connect_port = connectPortBodeShifter;
		bodeShifterDescriptor->deactivate = NULL;
		bodeShifterDescriptor->instantiate = instantiateBodeShifter;
		bodeShifterDescriptor->run = runBodeShifter;
		bodeShifterDescriptor->run_adding = runAddingBodeShifter;
		bodeShifterDescriptor->set_run_adding_gain = setRunAddingGainBodeShifter;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (bodeShifterDescriptor) {
		free((LADSPA_PortDescriptor *)bodeShifterDescriptor->PortDescriptors);
		free((char **)bodeShifterDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)bodeShifterDescriptor->PortRangeHints);
		free(bodeShifterDescriptor);
	}

}
