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

#line 9 "mbeq_1197.xml"

#include "config.h"

#ifdef FFTW3

#include <fftw3.h>

typedef fftwf_plan fft_plan;
typedef float fftw_real;

#else

#ifdef EXPLICIT_S
#include <srfftw.h>
#else
#include <rfftw.h>
#endif //EXPLICIT_S

typedef rfftw_plan fft_plan;

#endif //FFTW3

#include "ladspa-util.h"

#define FFT_LENGTH 1024
#define OVER_SAMP  4
#define BANDS      15


float bands[BANDS] =
  { 50.00f, 100.00f, 155.56f, 220.00f, 311.13f,
    440.00f, 622.25f, 880.00f, 1244.51f, 1760.00f, 2489.02f,
    3519.95, 4978.04f, 9956.08f, 19912.16f };

#define MBEQ_BAND_1                    0
#define MBEQ_BAND_2                    1
#define MBEQ_BAND_3                    2
#define MBEQ_BAND_4                    3
#define MBEQ_BAND_5                    4
#define MBEQ_BAND_6                    5
#define MBEQ_BAND_7                    6
#define MBEQ_BAND_8                    7
#define MBEQ_BAND_9                    8
#define MBEQ_BAND_10                   9
#define MBEQ_BAND_11                   10
#define MBEQ_BAND_12                   11
#define MBEQ_BAND_13                   12
#define MBEQ_BAND_14                   13
#define MBEQ_BAND_15                   14
#define MBEQ_INPUT                     15
#define MBEQ_OUTPUT                    16
#define MBEQ_LATENCY                   17

static LADSPA_Descriptor *mbeqDescriptor = NULL;

typedef struct {
	LADSPA_Data *band_1;
	LADSPA_Data *band_2;
	LADSPA_Data *band_3;
	LADSPA_Data *band_4;
	LADSPA_Data *band_5;
	LADSPA_Data *band_6;
	LADSPA_Data *band_7;
	LADSPA_Data *band_8;
	LADSPA_Data *band_9;
	LADSPA_Data *band_10;
	LADSPA_Data *band_11;
	LADSPA_Data *band_12;
	LADSPA_Data *band_13;
	LADSPA_Data *band_14;
	LADSPA_Data *band_15;
	LADSPA_Data *input;
	LADSPA_Data *output;
	LADSPA_Data *latency;
	int *        bin_base;
	float *      bin_delta;
	fftw_real *  comp;
	float *      db_table;
	long         fifo_pos;
	LADSPA_Data *in_fifo;
	LADSPA_Data *out_accum;
	LADSPA_Data *out_fifo;
	fft_plan     plan_cr;
	fft_plan     plan_rc;
	fftw_real *  real;
	float *      window;
	LADSPA_Data run_adding_gain;
} Mbeq;

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
		return mbeqDescriptor;
	default:
		return NULL;
	}
}

static void activateMbeq(LADSPA_Handle instance) {
	Mbeq *plugin_data = (Mbeq *)instance;
	int *bin_base = plugin_data->bin_base;
	float *bin_delta = plugin_data->bin_delta;
	fftw_real *comp = plugin_data->comp;
	float *db_table = plugin_data->db_table;
	long fifo_pos = plugin_data->fifo_pos;
	LADSPA_Data *in_fifo = plugin_data->in_fifo;
	LADSPA_Data *out_accum = plugin_data->out_accum;
	LADSPA_Data *out_fifo = plugin_data->out_fifo;
	fft_plan plan_cr = plugin_data->plan_cr;
	fft_plan plan_rc = plugin_data->plan_rc;
	fftw_real *real = plugin_data->real;
	float *window = plugin_data->window;
#line 109 "mbeq_1197.xml"
	fifo_pos = 0;
	plugin_data->bin_base = bin_base;
	plugin_data->bin_delta = bin_delta;
	plugin_data->comp = comp;
	plugin_data->db_table = db_table;
	plugin_data->fifo_pos = fifo_pos;
	plugin_data->in_fifo = in_fifo;
	plugin_data->out_accum = out_accum;
	plugin_data->out_fifo = out_fifo;
	plugin_data->plan_cr = plan_cr;
	plugin_data->plan_rc = plan_rc;
	plugin_data->real = real;
	plugin_data->window = window;

}

static void cleanupMbeq(LADSPA_Handle instance) {
#line 113 "mbeq_1197.xml"
	Mbeq *plugin_data = (Mbeq *)instance;
	free(plugin_data->in_fifo);
	free(plugin_data->out_fifo);
	free(plugin_data->out_accum);
	free(plugin_data->real);
	free(plugin_data->comp);
	free(plugin_data->window);
	free(plugin_data->bin_base);
	free(plugin_data->bin_delta);
	free(plugin_data->db_table);
	free(instance);
}

static void connectPortMbeq(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Mbeq *plugin;

	plugin = (Mbeq *)instance;
	switch (port) {
	case MBEQ_BAND_1:
		plugin->band_1 = data;
		break;
	case MBEQ_BAND_2:
		plugin->band_2 = data;
		break;
	case MBEQ_BAND_3:
		plugin->band_3 = data;
		break;
	case MBEQ_BAND_4:
		plugin->band_4 = data;
		break;
	case MBEQ_BAND_5:
		plugin->band_5 = data;
		break;
	case MBEQ_BAND_6:
		plugin->band_6 = data;
		break;
	case MBEQ_BAND_7:
		plugin->band_7 = data;
		break;
	case MBEQ_BAND_8:
		plugin->band_8 = data;
		break;
	case MBEQ_BAND_9:
		plugin->band_9 = data;
		break;
	case MBEQ_BAND_10:
		plugin->band_10 = data;
		break;
	case MBEQ_BAND_11:
		plugin->band_11 = data;
		break;
	case MBEQ_BAND_12:
		plugin->band_12 = data;
		break;
	case MBEQ_BAND_13:
		plugin->band_13 = data;
		break;
	case MBEQ_BAND_14:
		plugin->band_14 = data;
		break;
	case MBEQ_BAND_15:
		plugin->band_15 = data;
		break;
	case MBEQ_INPUT:
		plugin->input = data;
		break;
	case MBEQ_OUTPUT:
		plugin->output = data;
		break;
	case MBEQ_LATENCY:
		plugin->latency = data;
		break;
	}
}

static LADSPA_Handle instantiateMbeq(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Mbeq *plugin_data = (Mbeq *)malloc(sizeof(Mbeq));
	int *bin_base = NULL;
	float *bin_delta = NULL;
	fftw_real *comp = NULL;
	float *db_table = NULL;
	long fifo_pos;
	LADSPA_Data *in_fifo = NULL;
	LADSPA_Data *out_accum = NULL;
	LADSPA_Data *out_fifo = NULL;
	fft_plan plan_cr;
	fft_plan plan_rc;
	fftw_real *real = NULL;
	float *window = NULL;

#line 51 "mbeq_1197.xml"
	int i, bin;
	float last_bin, next_bin;
	float db;
	float hz_per_bin = (float)s_rate / (float)FFT_LENGTH;
	
	in_fifo = calloc(FFT_LENGTH, sizeof(LADSPA_Data));
	out_fifo = calloc(FFT_LENGTH, sizeof(LADSPA_Data));
	out_accum = calloc(FFT_LENGTH * 2, sizeof(LADSPA_Data));
	real = calloc(FFT_LENGTH, sizeof(fftw_real));
	comp = calloc(FFT_LENGTH, sizeof(fftw_real));
	window = calloc(FFT_LENGTH, sizeof(float));
	bin_base = calloc(FFT_LENGTH/2, sizeof(int));
	bin_delta = calloc(FFT_LENGTH/2, sizeof(float));
	fifo_pos = 0;
	
	#ifdef FFTW3
	plan_rc = fftwf_plan_r2r_1d(FFT_LENGTH, real, comp, FFTW_R2HC, FFTW_MEASURE);
	plan_cr = fftwf_plan_r2r_1d(FFT_LENGTH, comp, real, FFTW_HC2R, FFTW_MEASURE);
	#else
	plan_rc = rfftw_create_plan(FFT_LENGTH, FFTW_REAL_TO_COMPLEX, FFTW_ESTIMATE);
	plan_cr = rfftw_create_plan(FFT_LENGTH, FFTW_COMPLEX_TO_REAL, FFTW_ESTIMATE);
	#endif
	
	// Create raised cosine window table
	for (i=0; i < FFT_LENGTH; i++) {
	        window[i] = -0.5f*cos(2.0f*M_PI*(double)i/(double)FFT_LENGTH)+0.5f;
	        window[i] *= 2.0f;
	}
	
	// Create db->coeffiecnt lookup table
	db_table = malloc(1000 * sizeof(float));
	for (i=0; i < 1000; i++) {
	        db = ((float)i/10) - 70;
	        db_table[i] = pow(10.0f, db/20.0f);
	}
	
	// Create FFT bin -> band + delta tables
	bin = 0;
	while (bin <= bands[0]/hz_per_bin) {
	        bin_base[bin] = 0;
	        bin_delta[bin++] = 0.0f;
	}
	for (i = 1; i < BANDS-1 && bin < (FFT_LENGTH/2)-1 && bands[i+1] < s_rate/2; i++) {
	        last_bin = bin;
	        next_bin = (bands[i+1])/hz_per_bin;
	        while (bin <= next_bin) {
	                bin_base[bin] = i;
	                bin_delta[bin] = (float)(bin - last_bin) / (float)(next_bin - last_bin);
	                bin++;
	        }
	}
	for (; bin < (FFT_LENGTH/2); bin++) {
	        bin_base[bin] = BANDS-1;
	        bin_delta[bin] = 0.0f;
	}

	plugin_data->bin_base = bin_base;
	plugin_data->bin_delta = bin_delta;
	plugin_data->comp = comp;
	plugin_data->db_table = db_table;
	plugin_data->fifo_pos = fifo_pos;
	plugin_data->in_fifo = in_fifo;
	plugin_data->out_accum = out_accum;
	plugin_data->out_fifo = out_fifo;
	plugin_data->plan_cr = plan_cr;
	plugin_data->plan_rc = plan_rc;
	plugin_data->real = real;
	plugin_data->window = window;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runMbeq(LADSPA_Handle instance, unsigned long sample_count) {
	Mbeq *plugin_data = (Mbeq *)instance;

	/* 50Hz gain (low shelving) (float value) */
	const LADSPA_Data band_1 = *(plugin_data->band_1);

	/* 100Hz gain (float value) */
	const LADSPA_Data band_2 = *(plugin_data->band_2);

	/* 156Hz gain (float value) */
	const LADSPA_Data band_3 = *(plugin_data->band_3);

	/* 220Hz gain (float value) */
	const LADSPA_Data band_4 = *(plugin_data->band_4);

	/* 311Hz gain (float value) */
	const LADSPA_Data band_5 = *(plugin_data->band_5);

	/* 440Hz gain (float value) */
	const LADSPA_Data band_6 = *(plugin_data->band_6);

	/* 622Hz gain (float value) */
	const LADSPA_Data band_7 = *(plugin_data->band_7);

	/* 880Hz gain (float value) */
	const LADSPA_Data band_8 = *(plugin_data->band_8);

	/* 1250Hz gain (float value) */
	const LADSPA_Data band_9 = *(plugin_data->band_9);

	/* 1750Hz gain (float value) */
	const LADSPA_Data band_10 = *(plugin_data->band_10);

	/* 2500Hz gain (float value) */
	const LADSPA_Data band_11 = *(plugin_data->band_11);

	/* 3500Hz gain (float value) */
	const LADSPA_Data band_12 = *(plugin_data->band_12);

	/* 5000Hz gain (float value) */
	const LADSPA_Data band_13 = *(plugin_data->band_13);

	/* 10000Hz gain (float value) */
	const LADSPA_Data band_14 = *(plugin_data->band_14);

	/* 20000Hz gain (float value) */
	const LADSPA_Data band_15 = *(plugin_data->band_15);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	int * bin_base = plugin_data->bin_base;
	float * bin_delta = plugin_data->bin_delta;
	fftw_real * comp = plugin_data->comp;
	float * db_table = plugin_data->db_table;
	long fifo_pos = plugin_data->fifo_pos;
	LADSPA_Data * in_fifo = plugin_data->in_fifo;
	LADSPA_Data * out_accum = plugin_data->out_accum;
	LADSPA_Data * out_fifo = plugin_data->out_fifo;
	fft_plan plan_cr = plugin_data->plan_cr;
	fft_plan plan_rc = plugin_data->plan_rc;
	fftw_real * real = plugin_data->real;
	float * window = plugin_data->window;

#line 125 "mbeq_1197.xml"
	int i, bin, gain_idx;
	float gains[BANDS + 1] =
	  { band_1, band_2, band_3, band_4, band_5, band_6, band_7, band_8, band_9,
	    band_10, band_11, band_12, band_13, band_14, band_15, 0.0f };
	float coefs[FFT_LENGTH / 2];
	unsigned long pos;
	
	int step_size = FFT_LENGTH / OVER_SAMP;
	int fft_latency = FFT_LENGTH - step_size;
	
	// Convert gains from dB to co-efficents
	for (i = 0; i < BANDS; i++) {
	        gain_idx = (int)((gains[i] * 10) + 700);
	        gains[i] = db_table[LIMIT(gain_idx, 0, 999)];
	}
	
	// Calculate coefficients for each bin of FFT
	coefs[0] = 0.0f;
	for (bin=1; bin < (FFT_LENGTH/2-1); bin++) {
	        coefs[bin] = ((1.0f-bin_delta[bin]) * gains[bin_base[bin]])
	                      + (bin_delta[bin] * gains[bin_base[bin]+1]);
	}
	
	if (fifo_pos == 0) {
	        fifo_pos = fft_latency;
	}
	
	for (pos = 0; pos < sample_count; pos++) {
	        in_fifo[fifo_pos] = input[pos];
	        buffer_write(output[pos], out_fifo[fifo_pos-fft_latency]);
	        fifo_pos++;
	
	        // If the FIFO is full
	        if (fifo_pos >= FFT_LENGTH) {
	                fifo_pos = fft_latency;
	
	                // Window input FIFO
	                for (i=0; i < FFT_LENGTH; i++) {
	                        real[i] = in_fifo[i] * window[i];
	                }
	
	                // Run the real->complex transform
	#ifdef FFTW3
	                fftwf_execute(plan_rc);
	#else
	                rfftw_one(plan_rc, real, comp);
	#endif
	
	                // Multiply the bins magnitudes by the coeficients
	                comp[0] *= coefs[0];
	                for (i = 1; i < FFT_LENGTH/2; i++) {
	                        comp[i] *= coefs[i];
	                        comp[FFT_LENGTH-i] *= coefs[i];
	                }
	
	                // Run the complex->real transform
	#ifdef FFTW3
	                fftwf_execute(plan_cr);
	#else
	                rfftw_one(plan_cr, comp, real);
	#endif
	
	                // Window into the output accumulator
	                for (i = 0; i < FFT_LENGTH; i++) {
	                        out_accum[i] += 0.9186162f * window[i] * real[i]/(FFT_LENGTH * OVER_SAMP);
	                }
	                for (i = 0; i < step_size; i++) {
	                        out_fifo[i] = out_accum[i];
	                }
	
	                // Shift output accumulator
	                memmove(out_accum, out_accum + step_size, FFT_LENGTH*sizeof(LADSPA_Data));
	
	                // Shift input fifo
	                for (i = 0; i < fft_latency; i++) {
	                        in_fifo[i] = in_fifo[i+step_size];
	                }
	        }
	}
	
	// Store the fifo_position
	plugin_data->fifo_pos = fifo_pos;
	
	*(plugin_data->latency) = fft_latency;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainMbeq(LADSPA_Handle instance, LADSPA_Data gain) {
	((Mbeq *)instance)->run_adding_gain = gain;
}

static void runAddingMbeq(LADSPA_Handle instance, unsigned long sample_count) {
	Mbeq *plugin_data = (Mbeq *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* 50Hz gain (low shelving) (float value) */
	const LADSPA_Data band_1 = *(plugin_data->band_1);

	/* 100Hz gain (float value) */
	const LADSPA_Data band_2 = *(plugin_data->band_2);

	/* 156Hz gain (float value) */
	const LADSPA_Data band_3 = *(plugin_data->band_3);

	/* 220Hz gain (float value) */
	const LADSPA_Data band_4 = *(plugin_data->band_4);

	/* 311Hz gain (float value) */
	const LADSPA_Data band_5 = *(plugin_data->band_5);

	/* 440Hz gain (float value) */
	const LADSPA_Data band_6 = *(plugin_data->band_6);

	/* 622Hz gain (float value) */
	const LADSPA_Data band_7 = *(plugin_data->band_7);

	/* 880Hz gain (float value) */
	const LADSPA_Data band_8 = *(plugin_data->band_8);

	/* 1250Hz gain (float value) */
	const LADSPA_Data band_9 = *(plugin_data->band_9);

	/* 1750Hz gain (float value) */
	const LADSPA_Data band_10 = *(plugin_data->band_10);

	/* 2500Hz gain (float value) */
	const LADSPA_Data band_11 = *(plugin_data->band_11);

	/* 3500Hz gain (float value) */
	const LADSPA_Data band_12 = *(plugin_data->band_12);

	/* 5000Hz gain (float value) */
	const LADSPA_Data band_13 = *(plugin_data->band_13);

	/* 10000Hz gain (float value) */
	const LADSPA_Data band_14 = *(plugin_data->band_14);

	/* 20000Hz gain (float value) */
	const LADSPA_Data band_15 = *(plugin_data->band_15);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	int * bin_base = plugin_data->bin_base;
	float * bin_delta = plugin_data->bin_delta;
	fftw_real * comp = plugin_data->comp;
	float * db_table = plugin_data->db_table;
	long fifo_pos = plugin_data->fifo_pos;
	LADSPA_Data * in_fifo = plugin_data->in_fifo;
	LADSPA_Data * out_accum = plugin_data->out_accum;
	LADSPA_Data * out_fifo = plugin_data->out_fifo;
	fft_plan plan_cr = plugin_data->plan_cr;
	fft_plan plan_rc = plugin_data->plan_rc;
	fftw_real * real = plugin_data->real;
	float * window = plugin_data->window;

#line 125 "mbeq_1197.xml"
	int i, bin, gain_idx;
	float gains[BANDS + 1] =
	  { band_1, band_2, band_3, band_4, band_5, band_6, band_7, band_8, band_9,
	    band_10, band_11, band_12, band_13, band_14, band_15, 0.0f };
	float coefs[FFT_LENGTH / 2];
	unsigned long pos;
	
	int step_size = FFT_LENGTH / OVER_SAMP;
	int fft_latency = FFT_LENGTH - step_size;
	
	// Convert gains from dB to co-efficents
	for (i = 0; i < BANDS; i++) {
	        gain_idx = (int)((gains[i] * 10) + 700);
	        gains[i] = db_table[LIMIT(gain_idx, 0, 999)];
	}
	
	// Calculate coefficients for each bin of FFT
	coefs[0] = 0.0f;
	for (bin=1; bin < (FFT_LENGTH/2-1); bin++) {
	        coefs[bin] = ((1.0f-bin_delta[bin]) * gains[bin_base[bin]])
	                      + (bin_delta[bin] * gains[bin_base[bin]+1]);
	}
	
	if (fifo_pos == 0) {
	        fifo_pos = fft_latency;
	}
	
	for (pos = 0; pos < sample_count; pos++) {
	        in_fifo[fifo_pos] = input[pos];
	        buffer_write(output[pos], out_fifo[fifo_pos-fft_latency]);
	        fifo_pos++;
	
	        // If the FIFO is full
	        if (fifo_pos >= FFT_LENGTH) {
	                fifo_pos = fft_latency;
	
	                // Window input FIFO
	                for (i=0; i < FFT_LENGTH; i++) {
	                        real[i] = in_fifo[i] * window[i];
	                }
	
	                // Run the real->complex transform
	#ifdef FFTW3
	                fftwf_execute(plan_rc);
	#else
	                rfftw_one(plan_rc, real, comp);
	#endif
	
	                // Multiply the bins magnitudes by the coeficients
	                comp[0] *= coefs[0];
	                for (i = 1; i < FFT_LENGTH/2; i++) {
	                        comp[i] *= coefs[i];
	                        comp[FFT_LENGTH-i] *= coefs[i];
	                }
	
	                // Run the complex->real transform
	#ifdef FFTW3
	                fftwf_execute(plan_cr);
	#else
	                rfftw_one(plan_cr, comp, real);
	#endif
	
	                // Window into the output accumulator
	                for (i = 0; i < FFT_LENGTH; i++) {
	                        out_accum[i] += 0.9186162f * window[i] * real[i]/(FFT_LENGTH * OVER_SAMP);
	                }
	                for (i = 0; i < step_size; i++) {
	                        out_fifo[i] = out_accum[i];
	                }
	
	                // Shift output accumulator
	                memmove(out_accum, out_accum + step_size, FFT_LENGTH*sizeof(LADSPA_Data));
	
	                // Shift input fifo
	                for (i = 0; i < fft_latency; i++) {
	                        in_fifo[i] = in_fifo[i+step_size];
	                }
	        }
	}
	
	// Store the fifo_position
	plugin_data->fifo_pos = fifo_pos;
	
	*(plugin_data->latency) = fft_latency;
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


	mbeqDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (mbeqDescriptor) {
		mbeqDescriptor->UniqueID = 1197;
		mbeqDescriptor->Label = "mbeq";
		mbeqDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		mbeqDescriptor->Name =
		 D_("Multiband EQ");
		mbeqDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		mbeqDescriptor->Copyright =
		 "GPL";
		mbeqDescriptor->PortCount = 18;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(18,
		 sizeof(LADSPA_PortDescriptor));
		mbeqDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(18,
		 sizeof(LADSPA_PortRangeHint));
		mbeqDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(18, sizeof(char*));
		mbeqDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for 50Hz gain (low shelving) */
		port_descriptors[MBEQ_BAND_1] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[MBEQ_BAND_1] =
		 D_("50Hz gain (low shelving)");
		port_range_hints[MBEQ_BAND_1].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[MBEQ_BAND_1].LowerBound = -70;
		port_range_hints[MBEQ_BAND_1].UpperBound = +30;

		/* Parameters for 100Hz gain */
		port_descriptors[MBEQ_BAND_2] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[MBEQ_BAND_2] =
		 D_("100Hz gain");
		port_range_hints[MBEQ_BAND_2].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[MBEQ_BAND_2].LowerBound = -70;
		port_range_hints[MBEQ_BAND_2].UpperBound = +30;

		/* Parameters for 156Hz gain */
		port_descriptors[MBEQ_BAND_3] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[MBEQ_BAND_3] =
		 D_("156Hz gain");
		port_range_hints[MBEQ_BAND_3].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[MBEQ_BAND_3].LowerBound = -70;
		port_range_hints[MBEQ_BAND_3].UpperBound = +30;

		/* Parameters for 220Hz gain */
		port_descriptors[MBEQ_BAND_4] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[MBEQ_BAND_4] =
		 D_("220Hz gain");
		port_range_hints[MBEQ_BAND_4].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[MBEQ_BAND_4].LowerBound = -70;
		port_range_hints[MBEQ_BAND_4].UpperBound = +30;

		/* Parameters for 311Hz gain */
		port_descriptors[MBEQ_BAND_5] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[MBEQ_BAND_5] =
		 D_("311Hz gain");
		port_range_hints[MBEQ_BAND_5].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[MBEQ_BAND_5].LowerBound = -70;
		port_range_hints[MBEQ_BAND_5].UpperBound = +30;

		/* Parameters for 440Hz gain */
		port_descriptors[MBEQ_BAND_6] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[MBEQ_BAND_6] =
		 D_("440Hz gain");
		port_range_hints[MBEQ_BAND_6].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[MBEQ_BAND_6].LowerBound = -70;
		port_range_hints[MBEQ_BAND_6].UpperBound = +30;

		/* Parameters for 622Hz gain */
		port_descriptors[MBEQ_BAND_7] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[MBEQ_BAND_7] =
		 D_("622Hz gain");
		port_range_hints[MBEQ_BAND_7].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[MBEQ_BAND_7].LowerBound = -70;
		port_range_hints[MBEQ_BAND_7].UpperBound = +30;

		/* Parameters for 880Hz gain */
		port_descriptors[MBEQ_BAND_8] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[MBEQ_BAND_8] =
		 D_("880Hz gain");
		port_range_hints[MBEQ_BAND_8].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[MBEQ_BAND_8].LowerBound = -70;
		port_range_hints[MBEQ_BAND_8].UpperBound = +30;

		/* Parameters for 1250Hz gain */
		port_descriptors[MBEQ_BAND_9] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[MBEQ_BAND_9] =
		 D_("1250Hz gain");
		port_range_hints[MBEQ_BAND_9].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[MBEQ_BAND_9].LowerBound = -70;
		port_range_hints[MBEQ_BAND_9].UpperBound = +30;

		/* Parameters for 1750Hz gain */
		port_descriptors[MBEQ_BAND_10] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[MBEQ_BAND_10] =
		 D_("1750Hz gain");
		port_range_hints[MBEQ_BAND_10].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[MBEQ_BAND_10].LowerBound = -70;
		port_range_hints[MBEQ_BAND_10].UpperBound = +30;

		/* Parameters for 2500Hz gain */
		port_descriptors[MBEQ_BAND_11] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[MBEQ_BAND_11] =
		 D_("2500Hz gain");
		port_range_hints[MBEQ_BAND_11].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[MBEQ_BAND_11].LowerBound = -70;
		port_range_hints[MBEQ_BAND_11].UpperBound = +30;

		/* Parameters for 3500Hz gain */
		port_descriptors[MBEQ_BAND_12] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[MBEQ_BAND_12] =
		 D_("3500Hz gain");
		port_range_hints[MBEQ_BAND_12].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[MBEQ_BAND_12].LowerBound = -70;
		port_range_hints[MBEQ_BAND_12].UpperBound = +30;

		/* Parameters for 5000Hz gain */
		port_descriptors[MBEQ_BAND_13] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[MBEQ_BAND_13] =
		 D_("5000Hz gain");
		port_range_hints[MBEQ_BAND_13].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[MBEQ_BAND_13].LowerBound = -70;
		port_range_hints[MBEQ_BAND_13].UpperBound = +30;

		/* Parameters for 10000Hz gain */
		port_descriptors[MBEQ_BAND_14] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[MBEQ_BAND_14] =
		 D_("10000Hz gain");
		port_range_hints[MBEQ_BAND_14].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[MBEQ_BAND_14].LowerBound = -70;
		port_range_hints[MBEQ_BAND_14].UpperBound = +30;

		/* Parameters for 20000Hz gain */
		port_descriptors[MBEQ_BAND_15] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[MBEQ_BAND_15] =
		 D_("20000Hz gain");
		port_range_hints[MBEQ_BAND_15].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[MBEQ_BAND_15].LowerBound = -70;
		port_range_hints[MBEQ_BAND_15].UpperBound = +30;

		/* Parameters for Input */
		port_descriptors[MBEQ_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[MBEQ_INPUT] =
		 D_("Input");
		port_range_hints[MBEQ_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[MBEQ_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[MBEQ_OUTPUT] =
		 D_("Output");
		port_range_hints[MBEQ_OUTPUT].HintDescriptor = 0;

		/* Parameters for latency */
		port_descriptors[MBEQ_LATENCY] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL;
		port_names[MBEQ_LATENCY] =
		 D_("latency");
		port_range_hints[MBEQ_LATENCY].HintDescriptor = 0;

		mbeqDescriptor->activate = activateMbeq;
		mbeqDescriptor->cleanup = cleanupMbeq;
		mbeqDescriptor->connect_port = connectPortMbeq;
		mbeqDescriptor->deactivate = NULL;
		mbeqDescriptor->instantiate = instantiateMbeq;
		mbeqDescriptor->run = runMbeq;
		mbeqDescriptor->run_adding = runAddingMbeq;
		mbeqDescriptor->set_run_adding_gain = setRunAddingGainMbeq;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (mbeqDescriptor) {
		free((LADSPA_PortDescriptor *)mbeqDescriptor->PortDescriptors);
		free((char **)mbeqDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)mbeqDescriptor->PortRangeHints);
		free(mbeqDescriptor);
	}

}
