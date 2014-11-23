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

#line 10 "fast_lookahead_limiter_1913.xml"

#include "ladspa-util.h"

//#define DEBUG

#define NUM_CHUNKS 16
#define BUFFER_TIME 0.0053

#ifdef DEBUG
  #include "stdio.h"
#endif

#define FASTLOOKAHEADLIMITER_INGAIN    0
#define FASTLOOKAHEADLIMITER_LIMIT     1
#define FASTLOOKAHEADLIMITER_RELEASE   2
#define FASTLOOKAHEADLIMITER_ATTENUATION 3
#define FASTLOOKAHEADLIMITER_IN_1      4
#define FASTLOOKAHEADLIMITER_IN_2      5
#define FASTLOOKAHEADLIMITER_OUT_1     6
#define FASTLOOKAHEADLIMITER_OUT_2     7
#define FASTLOOKAHEADLIMITER_LATENCY   8

static LADSPA_Descriptor *fastLookaheadLimiterDescriptor = NULL;

typedef struct {
	LADSPA_Data *ingain;
	LADSPA_Data *limit;
	LADSPA_Data *release;
	LADSPA_Data *attenuation;
	LADSPA_Data *in_1;
	LADSPA_Data *in_2;
	LADSPA_Data *out_1;
	LADSPA_Data *out_2;
	LADSPA_Data *latency;
	float        atten;
	float        atten_lp;
	LADSPA_Data *buffer;
	unsigned int buffer_len;
	unsigned int buffer_pos;
	unsigned int chunk_num;
	unsigned int chunk_pos;
	unsigned int chunk_size;
	float *      chunks;
	unsigned int delay;
	float        delta;
	unsigned int fs;
	float        peak;
	LADSPA_Data run_adding_gain;
} FastLookaheadLimiter;

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
		return fastLookaheadLimiterDescriptor;
	default:
		return NULL;
	}
}

static void activateFastLookaheadLimiter(LADSPA_Handle instance) {
	FastLookaheadLimiter *plugin_data = (FastLookaheadLimiter *)instance;
	float atten = plugin_data->atten;
	float atten_lp = plugin_data->atten_lp;
	LADSPA_Data *buffer = plugin_data->buffer;
	unsigned int buffer_len = plugin_data->buffer_len;
	unsigned int buffer_pos = plugin_data->buffer_pos;
	unsigned int chunk_num = plugin_data->chunk_num;
	unsigned int chunk_pos = plugin_data->chunk_pos;
	unsigned int chunk_size = plugin_data->chunk_size;
	float *chunks = plugin_data->chunks;
	unsigned int delay = plugin_data->delay;
	float delta = plugin_data->delta;
	unsigned int fs = plugin_data->fs;
	float peak = plugin_data->peak;
#line 56 "fast_lookahead_limiter_1913.xml"
	memset(buffer, 0, NUM_CHUNKS * sizeof(float));

	chunk_pos = 0;
	chunk_num = 0;
	peak = 0.0f;
	atten = 1.0f;
	atten_lp = 1.0f;
	delta = 0.0f;
	plugin_data->atten = atten;
	plugin_data->atten_lp = atten_lp;
	plugin_data->buffer = buffer;
	plugin_data->buffer_len = buffer_len;
	plugin_data->buffer_pos = buffer_pos;
	plugin_data->chunk_num = chunk_num;
	plugin_data->chunk_pos = chunk_pos;
	plugin_data->chunk_size = chunk_size;
	plugin_data->chunks = chunks;
	plugin_data->delay = delay;
	plugin_data->delta = delta;
	plugin_data->fs = fs;
	plugin_data->peak = peak;

}

static void cleanupFastLookaheadLimiter(LADSPA_Handle instance) {
#line 188 "fast_lookahead_limiter_1913.xml"
	FastLookaheadLimiter *plugin_data = (FastLookaheadLimiter *)instance;
	free(plugin_data->buffer);
	free(instance);
}

static void connectPortFastLookaheadLimiter(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	FastLookaheadLimiter *plugin;

	plugin = (FastLookaheadLimiter *)instance;
	switch (port) {
	case FASTLOOKAHEADLIMITER_INGAIN:
		plugin->ingain = data;
		break;
	case FASTLOOKAHEADLIMITER_LIMIT:
		plugin->limit = data;
		break;
	case FASTLOOKAHEADLIMITER_RELEASE:
		plugin->release = data;
		break;
	case FASTLOOKAHEADLIMITER_ATTENUATION:
		plugin->attenuation = data;
		break;
	case FASTLOOKAHEADLIMITER_IN_1:
		plugin->in_1 = data;
		break;
	case FASTLOOKAHEADLIMITER_IN_2:
		plugin->in_2 = data;
		break;
	case FASTLOOKAHEADLIMITER_OUT_1:
		plugin->out_1 = data;
		break;
	case FASTLOOKAHEADLIMITER_OUT_2:
		plugin->out_2 = data;
		break;
	case FASTLOOKAHEADLIMITER_LATENCY:
		plugin->latency = data;
		break;
	}
}

static LADSPA_Handle instantiateFastLookaheadLimiter(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	FastLookaheadLimiter *plugin_data = (FastLookaheadLimiter *)malloc(sizeof(FastLookaheadLimiter));
	float atten;
	float atten_lp;
	LADSPA_Data *buffer = NULL;
	unsigned int buffer_len;
	unsigned int buffer_pos;
	unsigned int chunk_num;
	unsigned int chunk_pos;
	unsigned int chunk_size;
	float *chunks = NULL;
	unsigned int delay;
	float delta;
	unsigned int fs;
	float peak;

#line 31 "fast_lookahead_limiter_1913.xml"
	fs = s_rate;
	buffer_len = 128;
	buffer_pos = 0;

	/* Find size for power-of-two interleaved delay buffer */
	while(buffer_len < fs * BUFFER_TIME * 2) {
	  buffer_len *= 2;
	}
	buffer = calloc(buffer_len, sizeof(float));
	delay = (int)(0.005 * fs);

	chunk_pos = 0;
	chunk_num = 0;

	/* find a chunk size (in smaples) thats roughly 0.5ms */
	chunk_size = s_rate / 2000; 
	chunks = calloc(NUM_CHUNKS, sizeof(float));

	peak = 0.0f;
	atten = 1.0f;
	atten_lp = 1.0f;
	delta = 0.0f;

	plugin_data->atten = atten;
	plugin_data->atten_lp = atten_lp;
	plugin_data->buffer = buffer;
	plugin_data->buffer_len = buffer_len;
	plugin_data->buffer_pos = buffer_pos;
	plugin_data->chunk_num = chunk_num;
	plugin_data->chunk_pos = chunk_pos;
	plugin_data->chunk_size = chunk_size;
	plugin_data->chunks = chunks;
	plugin_data->delay = delay;
	plugin_data->delta = delta;
	plugin_data->fs = fs;
	plugin_data->peak = peak;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runFastLookaheadLimiter(LADSPA_Handle instance, unsigned long sample_count) {
	FastLookaheadLimiter *plugin_data = (FastLookaheadLimiter *)instance;

	/* Input gain (dB) (float value) */
	const LADSPA_Data ingain = *(plugin_data->ingain);

	/* Limit (dB) (float value) */
	const LADSPA_Data limit = *(plugin_data->limit);

	/* Release time (s) (float value) */
	const LADSPA_Data release = *(plugin_data->release);

	/* Input 1 (array of floats of length sample_count) */
	const LADSPA_Data * const in_1 = plugin_data->in_1;

	/* Input 2 (array of floats of length sample_count) */
	const LADSPA_Data * const in_2 = plugin_data->in_2;

	/* Output 1 (array of floats of length sample_count) */
	LADSPA_Data * const out_1 = plugin_data->out_1;

	/* Output 2 (array of floats of length sample_count) */
	LADSPA_Data * const out_2 = plugin_data->out_2;
	float atten = plugin_data->atten;
	float atten_lp = plugin_data->atten_lp;
	LADSPA_Data * buffer = plugin_data->buffer;
	unsigned int buffer_len = plugin_data->buffer_len;
	unsigned int buffer_pos = plugin_data->buffer_pos;
	unsigned int chunk_num = plugin_data->chunk_num;
	unsigned int chunk_pos = plugin_data->chunk_pos;
	unsigned int chunk_size = plugin_data->chunk_size;
	float * chunks = plugin_data->chunks;
	unsigned int delay = plugin_data->delay;
	float delta = plugin_data->delta;
	unsigned int fs = plugin_data->fs;
	float peak = plugin_data->peak;

#line 67 "fast_lookahead_limiter_1913.xml"
	unsigned long pos;
	const float max = DB_CO(limit);
	const float trim = DB_CO(ingain);
	float sig;
	unsigned int i;

	#ifdef DEBUG
	float clip = 0.0, clipp = 0.0;
	int clipc = 0;
	#endif

	for (pos = 0; pos < sample_count; pos++) {
	  if (chunk_pos++ == chunk_size) {
	    /* we've got a full chunk */
	   
	    delta = (1.0f - atten) / (fs * release);
	    round_to_zero(&delta);
	    for (i=0; i<10; i++) {
	      const int p = (chunk_num - 9 + i) & (NUM_CHUNKS - 1);
	      const float this_delta = (max / chunks[p] - atten) /
	                                ((float)(i+1) * fs * 0.0005f + 1.0f);

	      if (this_delta < delta) {
	        delta = this_delta;
	      }
	    }

	    chunks[chunk_num++ & (NUM_CHUNKS - 1)] = peak;
	    peak = 0.0f;
	    chunk_pos = 0;
	  }

	  buffer[(buffer_pos * 2) & (buffer_len - 1)] =     in_1[pos] * trim
	                                                  + 1.0e-30;
	  buffer[(buffer_pos * 2 + 1) & (buffer_len - 1)] = in_2[pos] * trim
	                                                  + 1.0e-30;

	  sig = fabs(in_1[pos]) > fabs(in_2[pos]) ? fabs(in_1[pos]) :
	          fabs(in_2[pos]);
	  sig += 1.0e-30;
	  if (sig * trim > peak) {
	    peak = sig * trim;
	  }
	  //round_to_zero(&peak);
	  //round_to_zero(&sig);

	  atten += delta;
	  atten_lp = atten * 0.1f + atten_lp * 0.9f;
	  //round_to_zero(&atten_lp);
	  if (delta > 0.0f && atten > 1.0f) {
	    atten = 1.0f;
	    delta = 0.0f;
	  }

	  buffer_write(out_1[pos], buffer[(buffer_pos * 2 - delay * 2) &
	                                  (buffer_len - 1)] * atten_lp);
	  buffer_write(out_2[pos], buffer[(buffer_pos * 2 - delay * 2 + 1) &
	                                  (buffer_len - 1)] * atten_lp);
	  round_to_zero(&out_1[pos]);
	  round_to_zero(&out_2[pos]);

	  if (out_1[pos] < -max) {
	    #ifdef DEBUG
	    clip += 20.0*log10(out_1[pos] / -max);
	    clipc++;
	    if (fabs(out_1[pos] - max) > clipp) {
	      clipp = fabs(out_1[pos] / -max);
	    }
	    #endif
	    buffer_write(out_1[pos], -max);
	  } else if (out_1[pos] > max) {
	    #ifdef DEBUG
	    clip += 20.0*log10(out_1[pos] / max);
	    clipc++;
	    if (fabs(out_1[pos] - max) > clipp) {
	      clipp = fabs(out_1[pos] / max);
	    }
	    #endif
	    buffer_write(out_1[pos], max);
	  }
	  if (out_2[pos] < -max) {
	    #ifdef DEBUG
	    clip += 20.0*log10(out_2[pos] / -max);
	    clipc++;
	    if (fabs(out_2[pos] - max) > clipp) {
	      clipp = fabs(out_2[pos] / -max);
	    }
	    #endif
	    buffer_write(out_2[pos], -max);
	  } else if (out_2[pos] > max) {
	    #ifdef DEBUG
	    clip += 20.0*log10(out_2[pos] / max);
	    clipc++;
	    if (fabs(out_2[pos] - max) > clipp) {
	      clipp = fabs(out_2[pos] / max);
	    }
	    #endif
	    buffer_write(out_2[pos], max);
	  }

	  buffer_pos++;
	}

	#ifdef DEBUG
	if (clipc > 0) {
	  printf("%d overs: %fdB avg, %fdB peak\n", clipc, clip/(float)clipc, 20.0*log10(clipp));
	}
	#endif

	plugin_data->buffer_pos = buffer_pos;
	plugin_data->peak = peak;
	plugin_data->atten = atten;
	plugin_data->atten_lp = atten_lp;
	plugin_data->chunk_pos = chunk_pos;
	plugin_data->chunk_num = chunk_num;

	*(plugin_data->attenuation) = -CO_DB(atten);
	*(plugin_data->latency) = delay;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainFastLookaheadLimiter(LADSPA_Handle instance, LADSPA_Data gain) {
	((FastLookaheadLimiter *)instance)->run_adding_gain = gain;
}

static void runAddingFastLookaheadLimiter(LADSPA_Handle instance, unsigned long sample_count) {
	FastLookaheadLimiter *plugin_data = (FastLookaheadLimiter *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Input gain (dB) (float value) */
	const LADSPA_Data ingain = *(plugin_data->ingain);

	/* Limit (dB) (float value) */
	const LADSPA_Data limit = *(plugin_data->limit);

	/* Release time (s) (float value) */
	const LADSPA_Data release = *(plugin_data->release);

	/* Input 1 (array of floats of length sample_count) */
	const LADSPA_Data * const in_1 = plugin_data->in_1;

	/* Input 2 (array of floats of length sample_count) */
	const LADSPA_Data * const in_2 = plugin_data->in_2;

	/* Output 1 (array of floats of length sample_count) */
	LADSPA_Data * const out_1 = plugin_data->out_1;

	/* Output 2 (array of floats of length sample_count) */
	LADSPA_Data * const out_2 = plugin_data->out_2;
	float atten = plugin_data->atten;
	float atten_lp = plugin_data->atten_lp;
	LADSPA_Data * buffer = plugin_data->buffer;
	unsigned int buffer_len = plugin_data->buffer_len;
	unsigned int buffer_pos = plugin_data->buffer_pos;
	unsigned int chunk_num = plugin_data->chunk_num;
	unsigned int chunk_pos = plugin_data->chunk_pos;
	unsigned int chunk_size = plugin_data->chunk_size;
	float * chunks = plugin_data->chunks;
	unsigned int delay = plugin_data->delay;
	float delta = plugin_data->delta;
	unsigned int fs = plugin_data->fs;
	float peak = plugin_data->peak;

#line 67 "fast_lookahead_limiter_1913.xml"
	unsigned long pos;
	const float max = DB_CO(limit);
	const float trim = DB_CO(ingain);
	float sig;
	unsigned int i;

	#ifdef DEBUG
	float clip = 0.0, clipp = 0.0;
	int clipc = 0;
	#endif

	for (pos = 0; pos < sample_count; pos++) {
	  if (chunk_pos++ == chunk_size) {
	    /* we've got a full chunk */
	   
	    delta = (1.0f - atten) / (fs * release);
	    round_to_zero(&delta);
	    for (i=0; i<10; i++) {
	      const int p = (chunk_num - 9 + i) & (NUM_CHUNKS - 1);
	      const float this_delta = (max / chunks[p] - atten) /
	                                ((float)(i+1) * fs * 0.0005f + 1.0f);

	      if (this_delta < delta) {
	        delta = this_delta;
	      }
	    }

	    chunks[chunk_num++ & (NUM_CHUNKS - 1)] = peak;
	    peak = 0.0f;
	    chunk_pos = 0;
	  }

	  buffer[(buffer_pos * 2) & (buffer_len - 1)] =     in_1[pos] * trim
	                                                  + 1.0e-30;
	  buffer[(buffer_pos * 2 + 1) & (buffer_len - 1)] = in_2[pos] * trim
	                                                  + 1.0e-30;

	  sig = fabs(in_1[pos]) > fabs(in_2[pos]) ? fabs(in_1[pos]) :
	          fabs(in_2[pos]);
	  sig += 1.0e-30;
	  if (sig * trim > peak) {
	    peak = sig * trim;
	  }
	  //round_to_zero(&peak);
	  //round_to_zero(&sig);

	  atten += delta;
	  atten_lp = atten * 0.1f + atten_lp * 0.9f;
	  //round_to_zero(&atten_lp);
	  if (delta > 0.0f && atten > 1.0f) {
	    atten = 1.0f;
	    delta = 0.0f;
	  }

	  buffer_write(out_1[pos], buffer[(buffer_pos * 2 - delay * 2) &
	                                  (buffer_len - 1)] * atten_lp);
	  buffer_write(out_2[pos], buffer[(buffer_pos * 2 - delay * 2 + 1) &
	                                  (buffer_len - 1)] * atten_lp);
	  round_to_zero(&out_1[pos]);
	  round_to_zero(&out_2[pos]);

	  if (out_1[pos] < -max) {
	    #ifdef DEBUG
	    clip += 20.0*log10(out_1[pos] / -max);
	    clipc++;
	    if (fabs(out_1[pos] - max) > clipp) {
	      clipp = fabs(out_1[pos] / -max);
	    }
	    #endif
	    buffer_write(out_1[pos], -max);
	  } else if (out_1[pos] > max) {
	    #ifdef DEBUG
	    clip += 20.0*log10(out_1[pos] / max);
	    clipc++;
	    if (fabs(out_1[pos] - max) > clipp) {
	      clipp = fabs(out_1[pos] / max);
	    }
	    #endif
	    buffer_write(out_1[pos], max);
	  }
	  if (out_2[pos] < -max) {
	    #ifdef DEBUG
	    clip += 20.0*log10(out_2[pos] / -max);
	    clipc++;
	    if (fabs(out_2[pos] - max) > clipp) {
	      clipp = fabs(out_2[pos] / -max);
	    }
	    #endif
	    buffer_write(out_2[pos], -max);
	  } else if (out_2[pos] > max) {
	    #ifdef DEBUG
	    clip += 20.0*log10(out_2[pos] / max);
	    clipc++;
	    if (fabs(out_2[pos] - max) > clipp) {
	      clipp = fabs(out_2[pos] / max);
	    }
	    #endif
	    buffer_write(out_2[pos], max);
	  }

	  buffer_pos++;
	}

	#ifdef DEBUG
	if (clipc > 0) {
	  printf("%d overs: %fdB avg, %fdB peak\n", clipc, clip/(float)clipc, 20.0*log10(clipp));
	}
	#endif

	plugin_data->buffer_pos = buffer_pos;
	plugin_data->peak = peak;
	plugin_data->atten = atten;
	plugin_data->atten_lp = atten_lp;
	plugin_data->chunk_pos = chunk_pos;
	plugin_data->chunk_num = chunk_num;

	*(plugin_data->attenuation) = -CO_DB(atten);
	*(plugin_data->latency) = delay;
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


	fastLookaheadLimiterDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (fastLookaheadLimiterDescriptor) {
		fastLookaheadLimiterDescriptor->UniqueID = 1913;
		fastLookaheadLimiterDescriptor->Label = "fastLookaheadLimiter";
		fastLookaheadLimiterDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		fastLookaheadLimiterDescriptor->Name =
		 D_("Fast Lookahead limiter");
		fastLookaheadLimiterDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		fastLookaheadLimiterDescriptor->Copyright =
		 "GPL";
		fastLookaheadLimiterDescriptor->PortCount = 9;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(9,
		 sizeof(LADSPA_PortDescriptor));
		fastLookaheadLimiterDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(9,
		 sizeof(LADSPA_PortRangeHint));
		fastLookaheadLimiterDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(9, sizeof(char*));
		fastLookaheadLimiterDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Input gain (dB) */
		port_descriptors[FASTLOOKAHEADLIMITER_INGAIN] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[FASTLOOKAHEADLIMITER_INGAIN] =
		 D_("Input gain (dB)");
		port_range_hints[FASTLOOKAHEADLIMITER_INGAIN].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[FASTLOOKAHEADLIMITER_INGAIN].LowerBound = -20;
		port_range_hints[FASTLOOKAHEADLIMITER_INGAIN].UpperBound = 20;

		/* Parameters for Limit (dB) */
		port_descriptors[FASTLOOKAHEADLIMITER_LIMIT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[FASTLOOKAHEADLIMITER_LIMIT] =
		 D_("Limit (dB)");
		port_range_hints[FASTLOOKAHEADLIMITER_LIMIT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[FASTLOOKAHEADLIMITER_LIMIT].LowerBound = -20;
		port_range_hints[FASTLOOKAHEADLIMITER_LIMIT].UpperBound = 0;

		/* Parameters for Release time (s) */
		port_descriptors[FASTLOOKAHEADLIMITER_RELEASE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[FASTLOOKAHEADLIMITER_RELEASE] =
		 D_("Release time (s)");
		port_range_hints[FASTLOOKAHEADLIMITER_RELEASE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[FASTLOOKAHEADLIMITER_RELEASE].LowerBound = 0.01;
		port_range_hints[FASTLOOKAHEADLIMITER_RELEASE].UpperBound = 2.0;

		/* Parameters for Attenuation (dB) */
		port_descriptors[FASTLOOKAHEADLIMITER_ATTENUATION] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL;
		port_names[FASTLOOKAHEADLIMITER_ATTENUATION] =
		 D_("Attenuation (dB)");
		port_range_hints[FASTLOOKAHEADLIMITER_ATTENUATION].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
		port_range_hints[FASTLOOKAHEADLIMITER_ATTENUATION].LowerBound = 0;
		port_range_hints[FASTLOOKAHEADLIMITER_ATTENUATION].UpperBound = 70;

		/* Parameters for Input 1 */
		port_descriptors[FASTLOOKAHEADLIMITER_IN_1] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[FASTLOOKAHEADLIMITER_IN_1] =
		 D_("Input 1");
		port_range_hints[FASTLOOKAHEADLIMITER_IN_1].HintDescriptor = 0;

		/* Parameters for Input 2 */
		port_descriptors[FASTLOOKAHEADLIMITER_IN_2] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[FASTLOOKAHEADLIMITER_IN_2] =
		 D_("Input 2");
		port_range_hints[FASTLOOKAHEADLIMITER_IN_2].HintDescriptor = 0;

		/* Parameters for Output 1 */
		port_descriptors[FASTLOOKAHEADLIMITER_OUT_1] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[FASTLOOKAHEADLIMITER_OUT_1] =
		 D_("Output 1");
		port_range_hints[FASTLOOKAHEADLIMITER_OUT_1].HintDescriptor = 0;

		/* Parameters for Output 2 */
		port_descriptors[FASTLOOKAHEADLIMITER_OUT_2] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[FASTLOOKAHEADLIMITER_OUT_2] =
		 D_("Output 2");
		port_range_hints[FASTLOOKAHEADLIMITER_OUT_2].HintDescriptor = 0;

		/* Parameters for latency */
		port_descriptors[FASTLOOKAHEADLIMITER_LATENCY] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL;
		port_names[FASTLOOKAHEADLIMITER_LATENCY] =
		 D_("latency");
		port_range_hints[FASTLOOKAHEADLIMITER_LATENCY].HintDescriptor = 0;

		fastLookaheadLimiterDescriptor->activate = activateFastLookaheadLimiter;
		fastLookaheadLimiterDescriptor->cleanup = cleanupFastLookaheadLimiter;
		fastLookaheadLimiterDescriptor->connect_port = connectPortFastLookaheadLimiter;
		fastLookaheadLimiterDescriptor->deactivate = NULL;
		fastLookaheadLimiterDescriptor->instantiate = instantiateFastLookaheadLimiter;
		fastLookaheadLimiterDescriptor->run = runFastLookaheadLimiter;
		fastLookaheadLimiterDescriptor->run_adding = runAddingFastLookaheadLimiter;
		fastLookaheadLimiterDescriptor->set_run_adding_gain = setRunAddingGainFastLookaheadLimiter;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (fastLookaheadLimiterDescriptor) {
		free((LADSPA_PortDescriptor *)fastLookaheadLimiterDescriptor->PortDescriptors);
		free((char **)fastLookaheadLimiterDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)fastLookaheadLimiterDescriptor->PortRangeHints);
		free(fastLookaheadLimiterDescriptor);
	}

}
