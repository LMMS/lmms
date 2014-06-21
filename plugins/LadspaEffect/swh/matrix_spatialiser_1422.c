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

#line 10 "matrix_spatialiser_1422.xml"

/*
   thanks to Steve Harris for walking me through my first plugin !
*/

#include "ladspa-util.h"

/* we use sin/cos panning and start at pi/4. this is the correction factor
   to bring the signal back to unity gain in neutral position.
   it should be 1/x : sin(x) = cos(x) (~1.41421...). but since we are using an
   approximation of sin/cos, we take its equal gain point, which leads to 1.3333...
*/
#define EQUALGAINPOINT_OFFSET 128.0f
#define EQUALGAINPOINT_TO_UNITY 4.0f / 3.0f

#define BITSPERCYCLE 10                 /* resolution of the width parameter for */
#define BITSPERQUARTER (BITSPERCYCLE-2) /* one cycle (0-2pi) */

/* borrowed code: http://www.dspguru.com/comp.dsp/tricks/alg/sincos.htm
   i'm using a constant of 0.75, which makes the calculations simpler and does
   not yield discontinuities.
   author: Olli Niemitalo (oniemita@mail.student.oulu.fi)
*/
static inline void sin_cos_approx(int phasein, float *vsin, float *vcos) {
        // Modulo phase into quarter, convert to float 0..1
        float modphase = (phasein & ((1<<BITSPERQUARTER) - 1))
        * 1.0f / (1<<BITSPERQUARTER);
        // Extract quarter bits
        int quarter = phasein & (3<<BITSPERQUARTER);
        // Recognize quarter
        if (!quarter) {
                // First quarter, angle = 0 .. pi/2
                float x = modphase - 0.5f;
                float temp = 0.75f - x * x;
                *vsin = temp + x;
                *vcos = temp - x;
        } else if (quarter == 1<<BITSPERQUARTER) {
                // Second quarter, angle = pi/2 .. pi
                float x = 0.5f - modphase;
                float temp = 0.75f - x*x;
                *vsin = x + temp;
                *vcos = x - temp;
        } else if (quarter == 2<<BITSPERQUARTER) {
                // Third quarter, angle = pi .. 1.5pi
                float x = modphase - 0.5f;
                float temp = x*x - 0.75f;
                *vsin = temp - x;
                *vcos = temp + x;
        } else {
                // Fourth quarter, angle = 1.5pi..2pi
                float x = modphase - 0.5f;
                float temp = 0.75f - x*x;
                *vsin = x - temp;
                *vcos = x + temp;
        }
}

#define MATRIXSPATIALISER_I_LEFT       0
#define MATRIXSPATIALISER_I_RIGHT      1
#define MATRIXSPATIALISER_WIDTH        2
#define MATRIXSPATIALISER_O_LEFT       3
#define MATRIXSPATIALISER_O_RIGHT      4

static LADSPA_Descriptor *matrixSpatialiserDescriptor = NULL;

typedef struct {
	LADSPA_Data *i_left;
	LADSPA_Data *i_right;
	LADSPA_Data *width;
	LADSPA_Data *o_left;
	LADSPA_Data *o_right;
	LADSPA_Data  current_m_gain;
	LADSPA_Data  current_s_gain;
	LADSPA_Data run_adding_gain;
} MatrixSpatialiser;

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
		return matrixSpatialiserDescriptor;
	default:
		return NULL;
	}
}

static void activateMatrixSpatialiser(LADSPA_Handle instance) {
	MatrixSpatialiser *plugin_data = (MatrixSpatialiser *)instance;
	LADSPA_Data current_m_gain = plugin_data->current_m_gain;
	LADSPA_Data current_s_gain = plugin_data->current_s_gain;
#line 94 "matrix_spatialiser_1422.xml"
	sin_cos_approx(EQUALGAINPOINT_OFFSET, &current_s_gain, &current_m_gain);
	current_m_gain *= EQUALGAINPOINT_TO_UNITY; /* normalize the neutral  */
	current_s_gain *= EQUALGAINPOINT_TO_UNITY; /* setting to unity gain. */
	plugin_data->current_m_gain = current_m_gain;
	plugin_data->current_s_gain = current_s_gain;

}

static void cleanupMatrixSpatialiser(LADSPA_Handle instance) {
	free(instance);
}

static void connectPortMatrixSpatialiser(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	MatrixSpatialiser *plugin;

	plugin = (MatrixSpatialiser *)instance;
	switch (port) {
	case MATRIXSPATIALISER_I_LEFT:
		plugin->i_left = data;
		break;
	case MATRIXSPATIALISER_I_RIGHT:
		plugin->i_right = data;
		break;
	case MATRIXSPATIALISER_WIDTH:
		plugin->width = data;
		break;
	case MATRIXSPATIALISER_O_LEFT:
		plugin->o_left = data;
		break;
	case MATRIXSPATIALISER_O_RIGHT:
		plugin->o_right = data;
		break;
	}
}

static LADSPA_Handle instantiateMatrixSpatialiser(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	MatrixSpatialiser *plugin_data = (MatrixSpatialiser *)malloc(sizeof(MatrixSpatialiser));
	LADSPA_Data current_m_gain;
	LADSPA_Data current_s_gain;

#line 89 "matrix_spatialiser_1422.xml"
	current_m_gain = 0.0f;
	current_s_gain = 0.0f;

	plugin_data->current_m_gain = current_m_gain;
	plugin_data->current_s_gain = current_s_gain;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runMatrixSpatialiser(LADSPA_Handle instance, unsigned long sample_count) {
	MatrixSpatialiser *plugin_data = (MatrixSpatialiser *)instance;

	/* Input L (array of floats of length sample_count) */
	const LADSPA_Data * const i_left = plugin_data->i_left;

	/* Input R (array of floats of length sample_count) */
	const LADSPA_Data * const i_right = plugin_data->i_right;

	/* Width (float value) */
	const LADSPA_Data width = *(plugin_data->width);

	/* Output L (array of floats of length sample_count) */
	LADSPA_Data * const o_left = plugin_data->o_left;

	/* Output R (array of floats of length sample_count) */
	LADSPA_Data * const o_right = plugin_data->o_right;
	LADSPA_Data current_m_gain = plugin_data->current_m_gain;
	LADSPA_Data current_s_gain = plugin_data->current_s_gain;

#line 100 "matrix_spatialiser_1422.xml"
	unsigned long pos;
	LADSPA_Data mid, side;
	LADSPA_Data m_gain, s_gain;
	int width_ = f_round(width + EQUALGAINPOINT_OFFSET);

	/* smoothen the gain changes. to spread the curve over the entire
	   buffer length (i.e.#sample_count samples), make lp dependent on
	   sample_count.
	*/
	const float lp = 7.0f / (float) sample_count; /* value found by experiment */
	const float lp_i = 1.0f - lp;

	/* do approximately the same as
	   s_gain = sin(width); m_gain = cos(width);
	   but a lot faster:
	*/
	sin_cos_approx(width_, &s_gain, &m_gain);

	m_gain *= EQUALGAINPOINT_TO_UNITY; /* normalize the neutral  */
	s_gain *= EQUALGAINPOINT_TO_UNITY; /* setting to unity gain. */

	#ifdef DEBUG
	/* do a "hardware bypass" if width == 0 */
	/* no smoothing here                    */
	if (width_ == 128) {
	        for (pos = 0; pos < sample_count; pos++) {
	        buffer_write(o_left[pos], i_left[pos]);
	        buffer_write(o_right[pos], i_right[pos]);
	        }
	} else
	#endif

	for (pos = 0; pos < sample_count; pos++) {
	        current_m_gain = current_m_gain * lp_i + m_gain * lp;
	        current_s_gain = current_s_gain * lp_i + s_gain * lp;
	        mid = (i_left[pos] + i_right[pos]) * 0.5f * current_m_gain;
	        side = (i_left[pos] - i_right[pos]) * 0.5f * current_s_gain;
	        buffer_write(o_left[pos], mid + side);
	        buffer_write(o_right[pos], mid - side);
	}

	plugin_data->current_m_gain = current_m_gain;
	plugin_data->current_s_gain = current_s_gain;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainMatrixSpatialiser(LADSPA_Handle instance, LADSPA_Data gain) {
	((MatrixSpatialiser *)instance)->run_adding_gain = gain;
}

static void runAddingMatrixSpatialiser(LADSPA_Handle instance, unsigned long sample_count) {
	MatrixSpatialiser *plugin_data = (MatrixSpatialiser *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Input L (array of floats of length sample_count) */
	const LADSPA_Data * const i_left = plugin_data->i_left;

	/* Input R (array of floats of length sample_count) */
	const LADSPA_Data * const i_right = plugin_data->i_right;

	/* Width (float value) */
	const LADSPA_Data width = *(plugin_data->width);

	/* Output L (array of floats of length sample_count) */
	LADSPA_Data * const o_left = plugin_data->o_left;

	/* Output R (array of floats of length sample_count) */
	LADSPA_Data * const o_right = plugin_data->o_right;
	LADSPA_Data current_m_gain = plugin_data->current_m_gain;
	LADSPA_Data current_s_gain = plugin_data->current_s_gain;

#line 100 "matrix_spatialiser_1422.xml"
	unsigned long pos;
	LADSPA_Data mid, side;
	LADSPA_Data m_gain, s_gain;
	int width_ = f_round(width + EQUALGAINPOINT_OFFSET);

	/* smoothen the gain changes. to spread the curve over the entire
	   buffer length (i.e.#sample_count samples), make lp dependent on
	   sample_count.
	*/
	const float lp = 7.0f / (float) sample_count; /* value found by experiment */
	const float lp_i = 1.0f - lp;

	/* do approximately the same as
	   s_gain = sin(width); m_gain = cos(width);
	   but a lot faster:
	*/
	sin_cos_approx(width_, &s_gain, &m_gain);

	m_gain *= EQUALGAINPOINT_TO_UNITY; /* normalize the neutral  */
	s_gain *= EQUALGAINPOINT_TO_UNITY; /* setting to unity gain. */

	#ifdef DEBUG
	/* do a "hardware bypass" if width == 0 */
	/* no smoothing here                    */
	if (width_ == 128) {
	        for (pos = 0; pos < sample_count; pos++) {
	        buffer_write(o_left[pos], i_left[pos]);
	        buffer_write(o_right[pos], i_right[pos]);
	        }
	} else
	#endif

	for (pos = 0; pos < sample_count; pos++) {
	        current_m_gain = current_m_gain * lp_i + m_gain * lp;
	        current_s_gain = current_s_gain * lp_i + s_gain * lp;
	        mid = (i_left[pos] + i_right[pos]) * 0.5f * current_m_gain;
	        side = (i_left[pos] - i_right[pos]) * 0.5f * current_s_gain;
	        buffer_write(o_left[pos], mid + side);
	        buffer_write(o_right[pos], mid - side);
	}

	plugin_data->current_m_gain = current_m_gain;
	plugin_data->current_s_gain = current_s_gain;
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


	matrixSpatialiserDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (matrixSpatialiserDescriptor) {
		matrixSpatialiserDescriptor->UniqueID = 1422;
		matrixSpatialiserDescriptor->Label = "matrixSpatialiser";
		matrixSpatialiserDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		matrixSpatialiserDescriptor->Name =
		 D_("Matrix Spatialiser");
		matrixSpatialiserDescriptor->Maker =
		 "Joern Nettingsmeier <nettings@folkwang-hochschule.de>";
		matrixSpatialiserDescriptor->Copyright =
		 "GPL";
		matrixSpatialiserDescriptor->PortCount = 5;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(5,
		 sizeof(LADSPA_PortDescriptor));
		matrixSpatialiserDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(5,
		 sizeof(LADSPA_PortRangeHint));
		matrixSpatialiserDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(5, sizeof(char*));
		matrixSpatialiserDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Input L */
		port_descriptors[MATRIXSPATIALISER_I_LEFT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[MATRIXSPATIALISER_I_LEFT] =
		 D_("Input L");
		port_range_hints[MATRIXSPATIALISER_I_LEFT].HintDescriptor = 0;

		/* Parameters for Input R */
		port_descriptors[MATRIXSPATIALISER_I_RIGHT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[MATRIXSPATIALISER_I_RIGHT] =
		 D_("Input R");
		port_range_hints[MATRIXSPATIALISER_I_RIGHT].HintDescriptor = 0;

		/* Parameters for Width */
		port_descriptors[MATRIXSPATIALISER_WIDTH] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[MATRIXSPATIALISER_WIDTH] =
		 D_("Width");
		port_range_hints[MATRIXSPATIALISER_WIDTH].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_INTEGER | LADSPA_HINT_DEFAULT_0;
		port_range_hints[MATRIXSPATIALISER_WIDTH].LowerBound = -512;
		port_range_hints[MATRIXSPATIALISER_WIDTH].UpperBound = 512;

		/* Parameters for Output L */
		port_descriptors[MATRIXSPATIALISER_O_LEFT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[MATRIXSPATIALISER_O_LEFT] =
		 D_("Output L");
		port_range_hints[MATRIXSPATIALISER_O_LEFT].HintDescriptor = 0;

		/* Parameters for Output R */
		port_descriptors[MATRIXSPATIALISER_O_RIGHT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[MATRIXSPATIALISER_O_RIGHT] =
		 D_("Output R");
		port_range_hints[MATRIXSPATIALISER_O_RIGHT].HintDescriptor = 0;

		matrixSpatialiserDescriptor->activate = activateMatrixSpatialiser;
		matrixSpatialiserDescriptor->cleanup = cleanupMatrixSpatialiser;
		matrixSpatialiserDescriptor->connect_port = connectPortMatrixSpatialiser;
		matrixSpatialiserDescriptor->deactivate = NULL;
		matrixSpatialiserDescriptor->instantiate = instantiateMatrixSpatialiser;
		matrixSpatialiserDescriptor->run = runMatrixSpatialiser;
		matrixSpatialiserDescriptor->run_adding = runAddingMatrixSpatialiser;
		matrixSpatialiserDescriptor->set_run_adding_gain = setRunAddingGainMatrixSpatialiser;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (matrixSpatialiserDescriptor) {
		free((LADSPA_PortDescriptor *)matrixSpatialiserDescriptor->PortDescriptors);
		free((char **)matrixSpatialiserDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)matrixSpatialiserDescriptor->PortRangeHints);
		free(matrixSpatialiserDescriptor);
	}

}
