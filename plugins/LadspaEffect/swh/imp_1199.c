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

#line 9 "imp_1199.xml"

#include <string.h>

#include "config.h"

#ifdef FFTW3

#include <fftw3.h>

typedef fftwf_plan fft_plan;
typedef float fftw_real;
#define local_malloc(s) fftwf_malloc(s)
#define local_free(s) fftwf_free(s)

#else

#ifdef EXPLICIT_S
#include <srfftw.h>
#else
#include <rfftw.h>
#endif //EXPLICIT_S

typedef rfftw_plan fft_plan;
#define local_malloc(s) malloc(s)
#define local_free(s) free(s)

#endif //FFTW3

#include "ladspa-util.h"

#define MAX_FFT_LENGTH 16384
#define SEG_LENGTH     128

#define IMP_LENGTH(a) (sizeof(a) / sizeof(float))

#define MK_IMP(i) impulse2freq(c, i, IMP_LENGTH(i), impulse_freq[c]); c++

inline void impulse2freq(int id, float *imp, unsigned int length, fftw_real *out);

#include "impulses/all.h"

fft_plan plan_rc[IMPULSES],
         plan_cr[IMPULSES];

static fftw_real *real_in, *real_out, *comp_in, *comp_out;

unsigned int fft_length[IMPULSES];

#ifdef __clang__
void impulse2freq(int id, float *imp, unsigned int length, fftw_real *out)
#else
inline void impulse2freq(int id, float *imp, unsigned int length, fftw_real *out)
#endif
{
  fftw_real impulse_time[MAX_FFT_LENGTH];
#ifdef FFTW3
  fft_plan tmp_plan;
#endif
  unsigned int i, fftl = 128;

  while (fftl < length+SEG_LENGTH) {
          fftl *= 2;
  }

  fft_length[id] = fftl;
#ifdef FFTW3
  plan_rc[id] = fftwf_plan_r2r_1d(fftl, real_in, comp_out, FFTW_R2HC, FFTW_MEASURE);
  plan_cr[id] = fftwf_plan_r2r_1d(fftl, comp_in, real_out, FFTW_HC2R, FFTW_MEASURE);
  tmp_plan = fftwf_plan_r2r_1d(fftl, impulse_time, out, FFTW_R2HC, FFTW_MEASURE);
#else
  plan_rc[id] = rfftw_create_plan(fftl, FFTW_REAL_TO_COMPLEX, FFTW_ESTIMATE);
  plan_cr[id] = rfftw_create_plan(fftl, FFTW_COMPLEX_TO_REAL, FFTW_ESTIMATE);
#endif

  for (i=0; i<length; i++) {
    impulse_time[i] = imp[i];
  }
  
  int last = i;
  for (i = 0; i<fftl; i++) {
    if (i >=last) impulse_time[i] = 0.0f;
  }
#ifdef FFTW3
  fftwf_execute(tmp_plan);
  fftwf_destroy_plan(tmp_plan);
#else
  rfftw_one(plan_rc[id], impulse_time, out);
#endif
}

#define IMP_IMPULSE                    0
#define IMP_HIGH_LAT                   1
#define IMP_GAIN                       2
#define IMP_INPUT                      3
#define IMP_OUTPUT                     4
#define IMP_LATENCY                    5

static LADSPA_Descriptor *impDescriptor = NULL;

typedef struct {
	LADSPA_Data *impulse;
	LADSPA_Data *high_lat;
	LADSPA_Data *gain;
	LADSPA_Data *input;
	LADSPA_Data *output;
	LADSPA_Data *latency;
	fftw_real *  block_freq;
	fftw_real *  block_time;
	unsigned int count;
	fftw_real ** impulse_freq;
	unsigned long in_ptr;
	fftw_real *  op;
	LADSPA_Data *opc;
	unsigned long out_ptr;
	LADSPA_Data *overlap;
	LADSPA_Data run_adding_gain;
} Imp;

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
		return impDescriptor;
	default:
		return NULL;
	}
}

static void activateImp(LADSPA_Handle instance) {
	Imp *plugin_data = (Imp *)instance;
	fftw_real *block_freq = plugin_data->block_freq;
	fftw_real *block_time = plugin_data->block_time;
	unsigned int count = plugin_data->count;
	fftw_real **impulse_freq = plugin_data->impulse_freq;
	unsigned long in_ptr = plugin_data->in_ptr;
	fftw_real *op = plugin_data->op;
	LADSPA_Data *opc = plugin_data->opc;
	unsigned long out_ptr = plugin_data->out_ptr;
	LADSPA_Data *overlap = plugin_data->overlap;
#line 161 "imp_1199.xml"
	memset(block_time, 0, MAX_FFT_LENGTH * sizeof(fftw_real));
	memset(block_freq, 0, MAX_FFT_LENGTH * sizeof(fftw_real));
	memset(op, 0, MAX_FFT_LENGTH * sizeof(fftw_real));
	memset(overlap, 0, (MAX_FFT_LENGTH - SEG_LENGTH) * sizeof(float));
	memset(opc, 0, SEG_LENGTH * sizeof(LADSPA_Data));

	in_ptr = 0;
	out_ptr = 0;
	count = 0;
	plugin_data->block_freq = block_freq;
	plugin_data->block_time = block_time;
	plugin_data->count = count;
	plugin_data->impulse_freq = impulse_freq;
	plugin_data->in_ptr = in_ptr;
	plugin_data->op = op;
	plugin_data->opc = opc;
	plugin_data->out_ptr = out_ptr;
	plugin_data->overlap = overlap;

}

static void cleanupImp(LADSPA_Handle instance) {
#line 173 "imp_1199.xml"
	Imp *plugin_data = (Imp *)instance;
	local_free(plugin_data->block_time);
	local_free(plugin_data->block_freq);
	local_free(plugin_data->op);
	local_free(plugin_data->overlap);
	local_free(plugin_data->opc);
	unsigned int i;
	for (i=0; i<IMPULSES; i++) {
	  local_free(plugin_data->impulse_freq[i]);
	}
	local_free(plugin_data->impulse_freq);
	free(instance);
}

static void connectPortImp(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Imp *plugin;

	plugin = (Imp *)instance;
	switch (port) {
	case IMP_IMPULSE:
		plugin->impulse = data;
		break;
	case IMP_HIGH_LAT:
		plugin->high_lat = data;
		break;
	case IMP_GAIN:
		plugin->gain = data;
		break;
	case IMP_INPUT:
		plugin->input = data;
		break;
	case IMP_OUTPUT:
		plugin->output = data;
		break;
	case IMP_LATENCY:
		plugin->latency = data;
		break;
	}
}

static LADSPA_Handle instantiateImp(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Imp *plugin_data = (Imp *)malloc(sizeof(Imp));
	fftw_real *block_freq = NULL;
	fftw_real *block_time = NULL;
	unsigned int count;
	fftw_real **impulse_freq = NULL;
	unsigned long in_ptr;
	fftw_real *op = NULL;
	LADSPA_Data *opc = NULL;
	unsigned long out_ptr;
	LADSPA_Data *overlap = NULL;

#line 135 "imp_1199.xml"
	unsigned int i;

	impulse_freq = local_malloc(IMPULSES * sizeof(fftw_real *));
	for (i=0; i<IMPULSES; i++) {
	  impulse_freq[i] = local_malloc(MAX_FFT_LENGTH * sizeof(fftw_real));
	}

	block_time = local_malloc(MAX_FFT_LENGTH * sizeof(fftw_real));
	block_freq = local_malloc(MAX_FFT_LENGTH * sizeof(fftw_real));
	op = local_malloc(MAX_FFT_LENGTH * sizeof(fftw_real));
	overlap = local_malloc(MAX_FFT_LENGTH * sizeof(float));
	opc = local_malloc(SEG_LENGTH * sizeof(LADSPA_Data));

	/* transform the impulses */
	real_in = block_time;
	comp_out = block_freq;
	comp_in = block_freq;
	real_out = op;
	mk_imps(impulse_freq);

	in_ptr = 0;
	out_ptr = 0;
	count = 0;

	plugin_data->block_freq = block_freq;
	plugin_data->block_time = block_time;
	plugin_data->count = count;
	plugin_data->impulse_freq = impulse_freq;
	plugin_data->in_ptr = in_ptr;
	plugin_data->op = op;
	plugin_data->opc = opc;
	plugin_data->out_ptr = out_ptr;
	plugin_data->overlap = overlap;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runImp(LADSPA_Handle instance, unsigned long sample_count) {
	Imp *plugin_data = (Imp *)instance;

	/* Impulse ID (float value) */
	const LADSPA_Data impulse = *(plugin_data->impulse);

	/* High latency mode (float value) */
	const LADSPA_Data high_lat = *(plugin_data->high_lat);

	/* Gain (dB) (float value) */
	const LADSPA_Data gain = *(plugin_data->gain);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	fftw_real * block_freq = plugin_data->block_freq;
	fftw_real * block_time = plugin_data->block_time;
	unsigned int count = plugin_data->count;
	fftw_real ** impulse_freq = plugin_data->impulse_freq;
	unsigned long in_ptr = plugin_data->in_ptr;
	fftw_real * op = plugin_data->op;
	LADSPA_Data * opc = plugin_data->opc;
	unsigned long out_ptr = plugin_data->out_ptr;
	LADSPA_Data * overlap = plugin_data->overlap;

#line 181 "imp_1199.xml"
	unsigned long i, pos, ipos, limit;
	unsigned int im;
	unsigned int len;
	fftw_real tmp;
	fftw_real *imp_freq;
	float coef;

	im = f_round(impulse) - 1;
	if (im >= IMPULSES) {
	  im = 0;
	}

	coef = pow(10.0f, gain * 0.05f) / (float)fft_length[im];

	imp_freq = impulse_freq[im];

	for (pos = 0; pos < sample_count; pos += SEG_LENGTH) {
	  limit = pos + SEG_LENGTH;

	  for (ipos = pos; ipos < sample_count && ipos<limit; ipos++) {
	    block_time[in_ptr++] = input[ipos];

	    if (in_ptr == SEG_LENGTH) {
#ifdef FFTW3
	      fftwf_execute(plan_rc[im]);
#else
	      rfftw_one(plan_rc[im], block_time, block_freq);
#endif
 
	      len = fft_length[im];
	      for (i=1; i<fft_length[im]/2; i++) {
	        len--;
	        tmp = block_freq[i] * imp_freq[i] -
	         block_freq[len] * imp_freq[len];
	        block_freq[len] =
	         block_freq[i] * imp_freq[len] +
	         block_freq[len] * imp_freq[i];
	        block_freq[i] = tmp;
	      }

	      block_freq[0] = imp_freq[0] * block_freq[0];
	      block_freq[fft_length[im]/2] = imp_freq[fft_length[im]/2] * block_freq[fft_length[im]/2];

#ifdef FFTW3
	      fftwf_execute(plan_cr[im]);
#else
	      rfftw_one(plan_cr[im], block_freq, op);
#endif

	      for (i=0; i<fft_length[im]-SEG_LENGTH; i++) {
	        op[i] += overlap[i];
	      }
	      for (i=SEG_LENGTH; i<fft_length[im]; i++) {
	        overlap[i-SEG_LENGTH] = op[i];
	      }

	      in_ptr = 0;
	      if (count == 0 && high_lat < 1.0f) {
	        count = 1;
	        plugin_data->count = 1;
	        out_ptr = 0;
	      }
	    }
	  }

	  for (ipos = pos; ipos < sample_count && ipos<limit; ipos++) {
	    buffer_write(output[ipos], opc[out_ptr++] * coef);
	    if (out_ptr == SEG_LENGTH) {
	      for (i=0; i<SEG_LENGTH; i++) {
	        opc[i] = op[i];
	      }
	      out_ptr = 0;
	    }
	  }
	}

	plugin_data->in_ptr = in_ptr;
	plugin_data->out_ptr = out_ptr;

	*(plugin_data->latency) = SEG_LENGTH;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainImp(LADSPA_Handle instance, LADSPA_Data gain) {
	((Imp *)instance)->run_adding_gain = gain;
}

static void runAddingImp(LADSPA_Handle instance, unsigned long sample_count) {
	Imp *plugin_data = (Imp *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Impulse ID (float value) */
	const LADSPA_Data impulse = *(plugin_data->impulse);

	/* High latency mode (float value) */
	const LADSPA_Data high_lat = *(plugin_data->high_lat);

	/* Gain (dB) (float value) */
	const LADSPA_Data gain = *(plugin_data->gain);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	fftw_real * block_freq = plugin_data->block_freq;
	fftw_real * block_time = plugin_data->block_time;
	unsigned int count = plugin_data->count;
	fftw_real ** impulse_freq = plugin_data->impulse_freq;
	unsigned long in_ptr = plugin_data->in_ptr;
	fftw_real * op = plugin_data->op;
	LADSPA_Data * opc = plugin_data->opc;
	unsigned long out_ptr = plugin_data->out_ptr;
	LADSPA_Data * overlap = plugin_data->overlap;

#line 181 "imp_1199.xml"
	unsigned long i, pos, ipos, limit;
	unsigned int im;
	unsigned int len;
	fftw_real tmp;
	fftw_real *imp_freq;
	float coef;

	im = f_round(impulse) - 1;
	if (im >= IMPULSES) {
	  im = 0;
	}

	coef = pow(10.0f, gain * 0.05f) / (float)fft_length[im];

	imp_freq = impulse_freq[im];

	for (pos = 0; pos < sample_count; pos += SEG_LENGTH) {
	  limit = pos + SEG_LENGTH;

	  for (ipos = pos; ipos < sample_count && ipos<limit; ipos++) {
	    block_time[in_ptr++] = input[ipos];

	    if (in_ptr == SEG_LENGTH) {
#ifdef FFTW3
	      fftwf_execute(plan_rc[im]);
#else
	      rfftw_one(plan_rc[im], block_time, block_freq);
#endif
 
	      len = fft_length[im];
	      for (i=1; i<fft_length[im]/2; i++) {
	        len--;
	        tmp = block_freq[i] * imp_freq[i] -
	         block_freq[len] * imp_freq[len];
	        block_freq[len] =
	         block_freq[i] * imp_freq[len] +
	         block_freq[len] * imp_freq[i];
	        block_freq[i] = tmp;
	      }

	      block_freq[0] = imp_freq[0] * block_freq[0];
	      block_freq[fft_length[im]/2] = imp_freq[fft_length[im]/2] * block_freq[fft_length[im]/2];

#ifdef FFTW3
	      fftwf_execute(plan_cr[im]);
#else
	      rfftw_one(plan_cr[im], block_freq, op);
#endif

	      for (i=0; i<fft_length[im]-SEG_LENGTH; i++) {
	        op[i] += overlap[i];
	      }
	      for (i=SEG_LENGTH; i<fft_length[im]; i++) {
	        overlap[i-SEG_LENGTH] = op[i];
	      }

	      in_ptr = 0;
	      if (count == 0 && high_lat < 1.0f) {
	        count = 1;
	        plugin_data->count = 1;
	        out_ptr = 0;
	      }
	    }
	  }

	  for (ipos = pos; ipos < sample_count && ipos<limit; ipos++) {
	    buffer_write(output[ipos], opc[out_ptr++] * coef);
	    if (out_ptr == SEG_LENGTH) {
	      for (i=0; i<SEG_LENGTH; i++) {
	        opc[i] = op[i];
	      }
	      out_ptr = 0;
	    }
	  }
	}

	plugin_data->in_ptr = in_ptr;
	plugin_data->out_ptr = out_ptr;

	*(plugin_data->latency) = SEG_LENGTH;
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


	impDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (impDescriptor) {
		impDescriptor->UniqueID = 1199;
		impDescriptor->Label = "imp";
		impDescriptor->Properties =
		 0;
		impDescriptor->Name =
		 D_("Impulse convolver");
		impDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		impDescriptor->Copyright =
		 "GPL";
		impDescriptor->PortCount = 6;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(6,
		 sizeof(LADSPA_PortDescriptor));
		impDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(6,
		 sizeof(LADSPA_PortRangeHint));
		impDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(6, sizeof(char*));
		impDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Impulse ID */
		port_descriptors[IMP_IMPULSE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[IMP_IMPULSE] =
		 D_("Impulse ID");
		port_range_hints[IMP_IMPULSE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_INTEGER | LADSPA_HINT_DEFAULT_1;
		port_range_hints[IMP_IMPULSE].LowerBound = 1;
		port_range_hints[IMP_IMPULSE].UpperBound = IMPULSES;

		/* Parameters for High latency mode */
		port_descriptors[IMP_HIGH_LAT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[IMP_HIGH_LAT] =
		 D_("High latency mode");
		port_range_hints[IMP_HIGH_LAT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_INTEGER | LADSPA_HINT_DEFAULT_0;
		port_range_hints[IMP_HIGH_LAT].LowerBound = 0;
		port_range_hints[IMP_HIGH_LAT].UpperBound = 1;

		/* Parameters for Gain (dB) */
		port_descriptors[IMP_GAIN] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[IMP_GAIN] =
		 D_("Gain (dB)");
		port_range_hints[IMP_GAIN].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[IMP_GAIN].LowerBound = -90;
		port_range_hints[IMP_GAIN].UpperBound = +24;

		/* Parameters for Input */
		port_descriptors[IMP_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[IMP_INPUT] =
		 D_("Input");
		port_range_hints[IMP_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[IMP_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[IMP_OUTPUT] =
		 D_("Output");
		port_range_hints[IMP_OUTPUT].HintDescriptor = 0;

		/* Parameters for latency */
		port_descriptors[IMP_LATENCY] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL;
		port_names[IMP_LATENCY] =
		 D_("latency");
		port_range_hints[IMP_LATENCY].HintDescriptor = 0;

		impDescriptor->activate = activateImp;
		impDescriptor->cleanup = cleanupImp;
		impDescriptor->connect_port = connectPortImp;
		impDescriptor->deactivate = NULL;
		impDescriptor->instantiate = instantiateImp;
		impDescriptor->run = runImp;
		impDescriptor->run_adding = runAddingImp;
		impDescriptor->set_run_adding_gain = setRunAddingGainImp;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (impDescriptor) {
		free((LADSPA_PortDescriptor *)impDescriptor->PortDescriptors);
		free((char **)impDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)impDescriptor->PortRangeHints);
		free(impDescriptor);
	}

}
