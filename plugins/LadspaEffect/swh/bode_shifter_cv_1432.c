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

#line 10 "bode_shifter_cv_1432.xml"

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

#define BODESHIFTERCV_SHIFT_B          0
#define BODESHIFTERCV_MIX              1
#define BODESHIFTERCV_INPUT            2
#define BODESHIFTERCV_ATTEN            3
#define BODESHIFTERCV_SHIFT            4
#define BODESHIFTERCV_DOUT             5
#define BODESHIFTERCV_UOUT             6
#define BODESHIFTERCV_MIXOUT           7
#define BODESHIFTERCV_LATENCY          8

static LADSPA_Descriptor *bodeShifterCVDescriptor = NULL;

typedef struct {
	LADSPA_Data *shift_b;
	LADSPA_Data *mix;
	LADSPA_Data *input;
	LADSPA_Data *atten;
	LADSPA_Data *shift;
	LADSPA_Data *dout;
	LADSPA_Data *uout;
	LADSPA_Data *mixout;
	LADSPA_Data *latency;
	LADSPA_Data *delay;
	unsigned int dptr;
	float        fs;
	float        phi;
	float *      sint;
	LADSPA_Data run_adding_gain;
} BodeShifterCV;

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
		return bodeShifterCVDescriptor;
	default:
		return NULL;
	}
}

static void cleanupBodeShifterCV(LADSPA_Handle instance) {
#line 132 "bode_shifter_cv_1432.xml"
	BodeShifterCV *plugin_data = (BodeShifterCV *)instance;
	free(plugin_data->delay);
	free(plugin_data->sint);
	free(instance);
}

static void connectPortBodeShifterCV(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	BodeShifterCV *plugin;

	plugin = (BodeShifterCV *)instance;
	switch (port) {
	case BODESHIFTERCV_SHIFT_B:
		plugin->shift_b = data;
		break;
	case BODESHIFTERCV_MIX:
		plugin->mix = data;
		break;
	case BODESHIFTERCV_INPUT:
		plugin->input = data;
		break;
	case BODESHIFTERCV_ATTEN:
		plugin->atten = data;
		break;
	case BODESHIFTERCV_SHIFT:
		plugin->shift = data;
		break;
	case BODESHIFTERCV_DOUT:
		plugin->dout = data;
		break;
	case BODESHIFTERCV_UOUT:
		plugin->uout = data;
		break;
	case BODESHIFTERCV_MIXOUT:
		plugin->mixout = data;
		break;
	case BODESHIFTERCV_LATENCY:
		plugin->latency = data;
		break;
	}
}

static LADSPA_Handle instantiateBodeShifterCV(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	BodeShifterCV *plugin_data = (BodeShifterCV *)malloc(sizeof(BodeShifterCV));
	LADSPA_Data *delay = NULL;
	unsigned int dptr;
	float fs;
	float phi;
	float *sint = NULL;

#line 57 "bode_shifter_cv_1432.xml"
	unsigned int i;

	fs = (float)s_rate;

	delay = calloc(D_SIZE, sizeof(LADSPA_Data));
	sint  = calloc(SIN_T_SIZE + 4, sizeof(float));

	dptr = 0;
	phi = 0.0f;

	for (i = 0; i < SIN_T_SIZE + 4; i++) {
	  sint[i] = sinf(2.0f * M_PI * (float)i / (float)SIN_T_SIZE);
	}

	plugin_data->delay = delay;
	plugin_data->dptr = dptr;
	plugin_data->fs = fs;
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

static void runBodeShifterCV(LADSPA_Handle instance, unsigned long sample_count) {
	BodeShifterCV *plugin_data = (BodeShifterCV *)instance;

	/* Base shift (float value) */
	const LADSPA_Data shift_b = *(plugin_data->shift_b);

	/* Mix (-1=down, +1=up) (float value) */
	const LADSPA_Data mix = *(plugin_data->mix);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* CV Attenuation (float value) */
	const LADSPA_Data atten = *(plugin_data->atten);

	/* Shift CV (array of floats of length sample_count) */
	const LADSPA_Data * const shift = plugin_data->shift;

	/* Down out (array of floats of length sample_count) */
	LADSPA_Data * const dout = plugin_data->dout;

	/* Up out (array of floats of length sample_count) */
	LADSPA_Data * const uout = plugin_data->uout;

	/* Mix out (array of floats of length sample_count) */
	LADSPA_Data * const mixout = plugin_data->mixout;
	LADSPA_Data * delay = plugin_data->delay;
	unsigned int dptr = plugin_data->dptr;
	float fs = plugin_data->fs;
	float phi = plugin_data->phi;
	float * sint = plugin_data->sint;

#line 73 "bode_shifter_cv_1432.xml"
	unsigned long pos;
	unsigned int i;
	float hilb, rm1, rm2;
	int int_p;
	float frac_p;
	const float freq_fix = (float)SIN_T_SIZE * 1000.0f * f_clamp(atten, 0.0f, 10.0f) / fs;
	const float base_ofs = (float)SIN_T_SIZE * f_clamp(shift_b, 0.0f, 10000.0f) / fs;
	const float mixc = mix * 0.5f + 0.5f;

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
	  buffer_write(mixout[pos], (dout[pos] - uout[pos]) * mixc + uout[pos]);

	  dptr = (dptr + 1) & (D_SIZE - 1);
	  phi += f_clamp(shift[pos], 0.0f, 10.0f) * freq_fix + base_ofs;
	  while (phi > SIN_T_SIZE) {
	    phi -= SIN_T_SIZE;
	  }
	}

	plugin_data->dptr = dptr;
	plugin_data->phi = phi;

	*(plugin_data->latency) = 99;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainBodeShifterCV(LADSPA_Handle instance, LADSPA_Data gain) {
	((BodeShifterCV *)instance)->run_adding_gain = gain;
}

static void runAddingBodeShifterCV(LADSPA_Handle instance, unsigned long sample_count) {
	BodeShifterCV *plugin_data = (BodeShifterCV *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Base shift (float value) */
	const LADSPA_Data shift_b = *(plugin_data->shift_b);

	/* Mix (-1=down, +1=up) (float value) */
	const LADSPA_Data mix = *(plugin_data->mix);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* CV Attenuation (float value) */
	const LADSPA_Data atten = *(plugin_data->atten);

	/* Shift CV (array of floats of length sample_count) */
	const LADSPA_Data * const shift = plugin_data->shift;

	/* Down out (array of floats of length sample_count) */
	LADSPA_Data * const dout = plugin_data->dout;

	/* Up out (array of floats of length sample_count) */
	LADSPA_Data * const uout = plugin_data->uout;

	/* Mix out (array of floats of length sample_count) */
	LADSPA_Data * const mixout = plugin_data->mixout;
	LADSPA_Data * delay = plugin_data->delay;
	unsigned int dptr = plugin_data->dptr;
	float fs = plugin_data->fs;
	float phi = plugin_data->phi;
	float * sint = plugin_data->sint;

#line 73 "bode_shifter_cv_1432.xml"
	unsigned long pos;
	unsigned int i;
	float hilb, rm1, rm2;
	int int_p;
	float frac_p;
	const float freq_fix = (float)SIN_T_SIZE * 1000.0f * f_clamp(atten, 0.0f, 10.0f) / fs;
	const float base_ofs = (float)SIN_T_SIZE * f_clamp(shift_b, 0.0f, 10000.0f) / fs;
	const float mixc = mix * 0.5f + 0.5f;

	for (pos = 0; pos < sample_count; pos++) {
	  delay[dptr] = input[pos];

	  /* Perform the Hilbert FIR convolution
	   * (probably FFT would be faster) */
	  hilb = 0.0f;
	  for (i = 0; i <= NZEROS/2; i++) {
	    hilb += (xcoeffs[i] * delay[(dptr - i*2) & (D_SIZE - 1)]);
	  }

	  /* Calcuate the table positions for the sine modulator */
	  int_p = f_round(floor(phi));

	  /* Calculate ringmod1, the transformed input modulated with a shift Hz
	   * sinewave. This creates a +180 degree sideband at source-shift Hz and
	   * a 0 degree sindeband at source+shift Hz */
	  frac_p = phi - int_p;
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
	  buffer_write(mixout[pos], (dout[pos] - uout[pos]) * mixc + uout[pos]);

	  dptr = (dptr + 1) & (D_SIZE - 1);
	  phi += f_clamp(shift[pos], 0.0f, 10.0f) * freq_fix + base_ofs;
	  while (phi > SIN_T_SIZE) {
	    phi -= SIN_T_SIZE;
	  }
	}

	plugin_data->dptr = dptr;
	plugin_data->phi = phi;

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


	bodeShifterCVDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (bodeShifterCVDescriptor) {
		bodeShifterCVDescriptor->UniqueID = 1432;
		bodeShifterCVDescriptor->Label = "bodeShifterCV";
		bodeShifterCVDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		bodeShifterCVDescriptor->Name =
		 D_("Bode frequency shifter (CV)");
		bodeShifterCVDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		bodeShifterCVDescriptor->Copyright =
		 "GPL";
		bodeShifterCVDescriptor->PortCount = 9;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(9,
		 sizeof(LADSPA_PortDescriptor));
		bodeShifterCVDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(9,
		 sizeof(LADSPA_PortRangeHint));
		bodeShifterCVDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(9, sizeof(char*));
		bodeShifterCVDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Base shift */
		port_descriptors[BODESHIFTERCV_SHIFT_B] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[BODESHIFTERCV_SHIFT_B] =
		 D_("Base shift");
		port_range_hints[BODESHIFTERCV_SHIFT_B].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[BODESHIFTERCV_SHIFT_B].LowerBound = 0;
		port_range_hints[BODESHIFTERCV_SHIFT_B].UpperBound = 5000;

		/* Parameters for Mix (-1=down, +1=up) */
		port_descriptors[BODESHIFTERCV_MIX] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[BODESHIFTERCV_MIX] =
		 D_("Mix (-1=down, +1=up)");
		port_range_hints[BODESHIFTERCV_MIX].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[BODESHIFTERCV_MIX].LowerBound = -1;
		port_range_hints[BODESHIFTERCV_MIX].UpperBound = 1;

		/* Parameters for Input */
		port_descriptors[BODESHIFTERCV_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[BODESHIFTERCV_INPUT] =
		 D_("Input");
		port_range_hints[BODESHIFTERCV_INPUT].HintDescriptor = 0;

		/* Parameters for CV Attenuation */
		port_descriptors[BODESHIFTERCV_ATTEN] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[BODESHIFTERCV_ATTEN] =
		 D_("CV Attenuation");
		port_range_hints[BODESHIFTERCV_ATTEN].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MAXIMUM;
		port_range_hints[BODESHIFTERCV_ATTEN].LowerBound = 0;
		port_range_hints[BODESHIFTERCV_ATTEN].UpperBound = 1;

		/* Parameters for Shift CV */
		port_descriptors[BODESHIFTERCV_SHIFT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[BODESHIFTERCV_SHIFT] =
		 D_("Shift CV");
		port_range_hints[BODESHIFTERCV_SHIFT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[BODESHIFTERCV_SHIFT].LowerBound = 0;
		port_range_hints[BODESHIFTERCV_SHIFT].UpperBound = 5;

		/* Parameters for Down out */
		port_descriptors[BODESHIFTERCV_DOUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[BODESHIFTERCV_DOUT] =
		 D_("Down out");
		port_range_hints[BODESHIFTERCV_DOUT].HintDescriptor = 0;

		/* Parameters for Up out */
		port_descriptors[BODESHIFTERCV_UOUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[BODESHIFTERCV_UOUT] =
		 D_("Up out");
		port_range_hints[BODESHIFTERCV_UOUT].HintDescriptor = 0;

		/* Parameters for Mix out */
		port_descriptors[BODESHIFTERCV_MIXOUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[BODESHIFTERCV_MIXOUT] =
		 D_("Mix out");
		port_range_hints[BODESHIFTERCV_MIXOUT].HintDescriptor = 0;

		/* Parameters for latency */
		port_descriptors[BODESHIFTERCV_LATENCY] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL;
		port_names[BODESHIFTERCV_LATENCY] =
		 D_("latency");
		port_range_hints[BODESHIFTERCV_LATENCY].HintDescriptor = 0;

		bodeShifterCVDescriptor->activate = NULL;
		bodeShifterCVDescriptor->cleanup = cleanupBodeShifterCV;
		bodeShifterCVDescriptor->connect_port = connectPortBodeShifterCV;
		bodeShifterCVDescriptor->deactivate = NULL;
		bodeShifterCVDescriptor->instantiate = instantiateBodeShifterCV;
		bodeShifterCVDescriptor->run = runBodeShifterCV;
		bodeShifterCVDescriptor->run_adding = runAddingBodeShifterCV;
		bodeShifterCVDescriptor->set_run_adding_gain = setRunAddingGainBodeShifterCV;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (bodeShifterCVDescriptor) {
		free((LADSPA_PortDescriptor *)bodeShifterCVDescriptor->PortDescriptors);
		free((char **)bodeShifterCVDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)bodeShifterCVDescriptor->PortRangeHints);
		free(bodeShifterCVDescriptor);
	}

}
