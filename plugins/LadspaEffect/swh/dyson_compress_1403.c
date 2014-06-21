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

#line 10 "dyson_compress_1403.xml"

/*
 * Copyright (c) 1996, John S. Dyson
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * This code (easily) runs realtime on a P5-166 w/EDO, Triton-II on FreeBSD.
 *
 * More info/comments: dyson@freebsd.org
 *
 * This program provides compression of a stereo 16bit audio stream,
 * such as that contained by a 16Bit wav file.  Extreme measures have
 * been taken to make the compression as subtile as possible.  One
 * possible purpose for this code would be to master cassette tapes from
 * CD's for playback in automobiles where dynamic range needs to be
 * restricted.
 *
 * Suitably recoded for an embedded DSP, this would make a killer audio
 * compressor for broadcast or recording.  When writing this code, I
 * ignored the issues of roundoff error or trucation -- Pentiums have
 * really nice FP processors :-).
 */

      #include <ladspa-util.h>

      #define MAXLEVEL 0.9f
      #define NFILT 12
      #define NEFILT 17

      /* These filters should filter at least the lowest audio freq */
      #define RLEVELSQ0FILTER .001
      #define RLEVELSQ1FILTER .010
      /* These are the attack time for the rms measurement */
      #define RLEVELSQ0FFILTER .001
      #define RLEVELSQEFILTER .001
    
      #define RMASTERGAIN0FILTER .000003
    
      #define RPEAKGAINFILTER .001

      #define MAXFASTGAIN 3
      #define MAXSLOWGAIN 9

      #define FLOORLEVEL 0.06

      float hardlimit(float value, float knee, float limit) {
        float ab = fabs(value);
        if (ab >= limit) {
          value = value > 0 ? limit : -limit;
        }

        return value;
      }

#define DYSONCOMPRESS_PEAK_LIMIT       0
#define DYSONCOMPRESS_RELEASE_TIME     1
#define DYSONCOMPRESS_CFRATE           2
#define DYSONCOMPRESS_CRATE            3
#define DYSONCOMPRESS_INPUT            4
#define DYSONCOMPRESS_OUTPUT           5

static LADSPA_Descriptor *dysonCompressDescriptor = NULL;

typedef struct {
	LADSPA_Data *peak_limit;
	LADSPA_Data *release_time;
	LADSPA_Data *cfrate;
	LADSPA_Data *crate;
	LADSPA_Data *input;
	LADSPA_Data *output;
	LADSPA_Data *delay;
	float        extra_maxlevel;
	float        lastrgain;
	float        maxgain;
	float        mingain;
	float        ndelay;
	unsigned int ndelayptr;
	int          peaklimitdelay;
	float        rgain;
	float        rlevelsq0;
	float        rlevelsq1;
	LADSPA_Data *rlevelsqe;
	LADSPA_Data *rlevelsqn;
	float        rmastergain0;
	float        rpeakgain0;
	float        rpeakgain1;
	float        rpeaklimitdelay;
	float        sample_rate;
	LADSPA_Data run_adding_gain;
} DysonCompress;

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
		return dysonCompressDescriptor;
	default:
		return NULL;
	}
}

static void activateDysonCompress(LADSPA_Handle instance) {
	DysonCompress *plugin_data = (DysonCompress *)instance;
	LADSPA_Data *delay = plugin_data->delay;
	float extra_maxlevel = plugin_data->extra_maxlevel;
	float lastrgain = plugin_data->lastrgain;
	float maxgain = plugin_data->maxgain;
	float mingain = plugin_data->mingain;
	float ndelay = plugin_data->ndelay;
	unsigned int ndelayptr = plugin_data->ndelayptr;
	int peaklimitdelay = plugin_data->peaklimitdelay;
	float rgain = plugin_data->rgain;
	float rlevelsq0 = plugin_data->rlevelsq0;
	float rlevelsq1 = plugin_data->rlevelsq1;
	LADSPA_Data *rlevelsqe = plugin_data->rlevelsqe;
	LADSPA_Data *rlevelsqn = plugin_data->rlevelsqn;
	float rmastergain0 = plugin_data->rmastergain0;
	float rpeakgain0 = plugin_data->rpeakgain0;
	float rpeakgain1 = plugin_data->rpeakgain1;
	float rpeaklimitdelay = plugin_data->rpeaklimitdelay;
	float sample_rate = plugin_data->sample_rate;
#line 105 "dyson_compress_1403.xml"
	unsigned int i;

	for (i=0; i<ndelay; i++) {
	  delay[i] = 0;
	}
	for (i=0; i<NFILT + 1; i++) {
	  rlevelsqn[i] = 0;
	}
	for (i=0; i<NEFILT + 1; i++) {
	  rlevelsqe[i] = 0;
	}

	mingain = 10000;
	maxgain = 0;

	rpeaklimitdelay = 2500;
    
	rgain = rmastergain0 = 1.0;
	rlevelsq0 = 0;
	rlevelsq1 = 0;
    
	rpeakgain0 = 1.0;
	rpeakgain1 = 1.0;
	rpeaklimitdelay = 0;
	ndelayptr = 0;
	lastrgain = 1.0;

	extra_maxlevel = 0.0f;
	peaklimitdelay = 0;
	plugin_data->delay = delay;
	plugin_data->extra_maxlevel = extra_maxlevel;
	plugin_data->lastrgain = lastrgain;
	plugin_data->maxgain = maxgain;
	plugin_data->mingain = mingain;
	plugin_data->ndelay = ndelay;
	plugin_data->ndelayptr = ndelayptr;
	plugin_data->peaklimitdelay = peaklimitdelay;
	plugin_data->rgain = rgain;
	plugin_data->rlevelsq0 = rlevelsq0;
	plugin_data->rlevelsq1 = rlevelsq1;
	plugin_data->rlevelsqe = rlevelsqe;
	plugin_data->rlevelsqn = rlevelsqn;
	plugin_data->rmastergain0 = rmastergain0;
	plugin_data->rpeakgain0 = rpeakgain0;
	plugin_data->rpeakgain1 = rpeakgain1;
	plugin_data->rpeaklimitdelay = rpeaklimitdelay;
	plugin_data->sample_rate = sample_rate;

}

static void cleanupDysonCompress(LADSPA_Handle instance) {
#line 137 "dyson_compress_1403.xml"
	DysonCompress *plugin_data = (DysonCompress *)instance;
	free(plugin_data->delay);
	free(plugin_data->rlevelsqn);
	free(plugin_data->rlevelsqe);
	free(instance);
}

static void connectPortDysonCompress(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	DysonCompress *plugin;

	plugin = (DysonCompress *)instance;
	switch (port) {
	case DYSONCOMPRESS_PEAK_LIMIT:
		plugin->peak_limit = data;
		break;
	case DYSONCOMPRESS_RELEASE_TIME:
		plugin->release_time = data;
		break;
	case DYSONCOMPRESS_CFRATE:
		plugin->cfrate = data;
		break;
	case DYSONCOMPRESS_CRATE:
		plugin->crate = data;
		break;
	case DYSONCOMPRESS_INPUT:
		plugin->input = data;
		break;
	case DYSONCOMPRESS_OUTPUT:
		plugin->output = data;
		break;
	}
}

static LADSPA_Handle instantiateDysonCompress(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	DysonCompress *plugin_data = (DysonCompress *)malloc(sizeof(DysonCompress));
	LADSPA_Data *delay = NULL;
	float extra_maxlevel;
	float lastrgain;
	float maxgain;
	float mingain;
	float ndelay;
	unsigned int ndelayptr;
	int peaklimitdelay;
	float rgain;
	float rlevelsq0;
	float rlevelsq1;
	LADSPA_Data *rlevelsqe = NULL;
	LADSPA_Data *rlevelsqn = NULL;
	float rmastergain0;
	float rpeakgain0;
	float rpeakgain1;
	float rpeaklimitdelay;
	float sample_rate;

#line 78 "dyson_compress_1403.xml"
	sample_rate = (float)s_rate;

	mingain = 10000;
	maxgain = 0;

	rpeaklimitdelay = 2500;
    
	rgain = rmastergain0 = 1.0;
	rlevelsq0 = 0;
	rlevelsq1 = 0;
	ndelay = (int)(1.0 / RLEVELSQ0FFILTER);

	delay = calloc(ndelay, sizeof(LADSPA_Data));
	rlevelsqn = calloc(NFILT + 1, sizeof(float));
	rlevelsqe = calloc(NEFILT + 1, sizeof(float));
    
	rpeakgain0 = 1.0;
	rpeakgain1 = 1.0;
	rpeaklimitdelay = 0;
	ndelayptr = 0;
	lastrgain = 1.0;

	extra_maxlevel = 0.0f;
	peaklimitdelay = 0;

	plugin_data->delay = delay;
	plugin_data->extra_maxlevel = extra_maxlevel;
	plugin_data->lastrgain = lastrgain;
	plugin_data->maxgain = maxgain;
	plugin_data->mingain = mingain;
	plugin_data->ndelay = ndelay;
	plugin_data->ndelayptr = ndelayptr;
	plugin_data->peaklimitdelay = peaklimitdelay;
	plugin_data->rgain = rgain;
	plugin_data->rlevelsq0 = rlevelsq0;
	plugin_data->rlevelsq1 = rlevelsq1;
	plugin_data->rlevelsqe = rlevelsqe;
	plugin_data->rlevelsqn = rlevelsqn;
	plugin_data->rmastergain0 = rmastergain0;
	plugin_data->rpeakgain0 = rpeakgain0;
	plugin_data->rpeakgain1 = rpeakgain1;
	plugin_data->rpeaklimitdelay = rpeaklimitdelay;
	plugin_data->sample_rate = sample_rate;

	return (LADSPA_Handle)plugin_data;
}

#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b = v)
#define RUN_ADDING    0
#define RUN_REPLACING 1

static void runDysonCompress(LADSPA_Handle instance, unsigned long sample_count) {
	DysonCompress *plugin_data = (DysonCompress *)instance;

	/* Peak limit (dB) (float value) */
	const LADSPA_Data peak_limit = *(plugin_data->peak_limit);

	/* Release time (s) (float value) */
	const LADSPA_Data release_time = *(plugin_data->release_time);

	/* Fast compression ratio (float value) */
	const LADSPA_Data cfrate = *(plugin_data->cfrate);

	/* Compression ratio (float value) */
	const LADSPA_Data crate = *(plugin_data->crate);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	LADSPA_Data * delay = plugin_data->delay;
	float extra_maxlevel = plugin_data->extra_maxlevel;
	float lastrgain = plugin_data->lastrgain;
	float maxgain = plugin_data->maxgain;
	float mingain = plugin_data->mingain;
	float ndelay = plugin_data->ndelay;
	unsigned int ndelayptr = plugin_data->ndelayptr;
	int peaklimitdelay = plugin_data->peaklimitdelay;
	float rgain = plugin_data->rgain;
	float rlevelsq0 = plugin_data->rlevelsq0;
	float rlevelsq1 = plugin_data->rlevelsq1;
	LADSPA_Data * rlevelsqe = plugin_data->rlevelsqe;
	LADSPA_Data * rlevelsqn = plugin_data->rlevelsqn;
	float rmastergain0 = plugin_data->rmastergain0;
	float rpeakgain0 = plugin_data->rpeakgain0;
	float rpeakgain1 = plugin_data->rpeakgain1;
	float rpeaklimitdelay = plugin_data->rpeaklimitdelay;
	float sample_rate = plugin_data->sample_rate;

#line 143 "dyson_compress_1403.xml"
	unsigned long pos;
	float targetlevel = MAXLEVEL * DB_CO(peak_limit);
	float rgainfilter = 1.0f / (release_time * sample_rate);
	float fastgaincompressionratio = cfrate;
	float compressionratio = crate;
	float efilt;
	float levelsqe;
	float gain;
	float tgain;
	float d;
	float fastgain;
	float qgain;
	float tslowgain;
	float slowgain;
	float npeakgain;
	float new;
	float nrgain;
	float ngain;
	float ngsq;
	float tnrgain;
	float sqrtrpeakgain;
	float totalgain;
	unsigned int i;

	for (pos = 0; pos < sample_count; pos++) {
	  // Ergh! this was originally meant to track a stereo signal
	  float levelsq0 = 2.0f * (input[pos] * input[pos]);

	  delay[ndelayptr] = input[pos];
	  ndelayptr++;

	  if (ndelayptr >= ndelay) {
	    ndelayptr = 0;
	  }

	  if (levelsq0 > rlevelsq0) {
	    rlevelsq0 = (levelsq0 * RLEVELSQ0FFILTER) +
	     rlevelsq0 * (1 - RLEVELSQ0FFILTER);
	  } else {
	    rlevelsq0 = (levelsq0 * RLEVELSQ0FILTER) +
	     rlevelsq0 * (1 - RLEVELSQ0FILTER);
	  }

	  if (rlevelsq0 <= FLOORLEVEL * FLOORLEVEL) {
	    goto skipagc;
	  }

	  if (rlevelsq0 > rlevelsq1) {
	    rlevelsq1 = rlevelsq0;
	  } else {
	    rlevelsq1 = rlevelsq0 * RLEVELSQ1FILTER +
	      rlevelsq1 * (1 - RLEVELSQ1FILTER);
	  }

	  rlevelsqn[0] = rlevelsq1;
	  for(i = 0; i < NFILT-1; i++) {
	    if (rlevelsqn[i] > rlevelsqn[i+1])
	      rlevelsqn[i+1] = rlevelsqn[i];
	    else
	      rlevelsqn[i+1] = rlevelsqn[i] * RLEVELSQ1FILTER +
	        rlevelsqn[i+1] * (1 - RLEVELSQ1FILTER);
	  }

	  efilt = RLEVELSQEFILTER;
	  levelsqe = rlevelsqe[0] = rlevelsqn[NFILT-1];
	  for(i = 0; i < NEFILT-1; i++) {
	    rlevelsqe[i+1] = rlevelsqe[i] * efilt +
	      rlevelsqe[i+1] * (1.0 - efilt);
	    if (rlevelsqe[i+1] > levelsqe)
	      levelsqe = rlevelsqe[i+1];
	    efilt *= 1.0f / 1.5f;
	  }

	  gain = targetlevel / sqrt(levelsqe);
	  if (compressionratio < 0.99f) {
	    if (compressionratio == 0.50f)
	      gain = sqrt(gain);
	    else
	      gain = f_exp(log(gain) * compressionratio);
	  }

	  if (gain < rgain)
	    rgain = gain * RLEVELSQEFILTER/2 +
	      rgain * (1 - RLEVELSQEFILTER/2);
	  else
	    rgain = gain * rgainfilter +
	      rgain * (1 - rgainfilter);

	  lastrgain = rgain;
	  if ( gain < lastrgain)
	    lastrgain = gain;

	skipagc:;

	  tgain = lastrgain;
    
	  d = delay[ndelayptr];
    
	  fastgain = tgain;
	  if (fastgain > MAXFASTGAIN)
	    fastgain = MAXFASTGAIN;
    
	  if (fastgain < 0.0001)
	    fastgain = 0.0001;

	  qgain = f_exp(log(fastgain) * fastgaincompressionratio);

	  tslowgain = tgain / qgain;
	  if (tslowgain > MAXSLOWGAIN)
	    tslowgain = MAXSLOWGAIN;
	  if (tslowgain < rmastergain0)
	    rmastergain0 = tslowgain;
	  else
	    rmastergain0 = tslowgain * RMASTERGAIN0FILTER +
	      (1 - RMASTERGAIN0FILTER) * rmastergain0;
    
	  slowgain = rmastergain0;
	  npeakgain = slowgain * qgain;
    
	  new = d * npeakgain;
	  if (fabs(new) >= MAXLEVEL)
	    nrgain = MAXLEVEL / fabs(new);
	  else
	    nrgain = 1.0;
    
	  ngain = nrgain;
    
	  ngsq = ngain * ngain;
	  if (ngsq <= rpeakgain0) {
	    rpeakgain0 = ngsq /* * 0.50 + rpeakgain0 * 0.50 */;
	    rpeaklimitdelay = peaklimitdelay;
	  } else if (rpeaklimitdelay == 0) {
	    if (nrgain > 1.0)
	      tnrgain = 1.0;
	    else
	      tnrgain = nrgain;
	    rpeakgain0 = tnrgain * RPEAKGAINFILTER +
	      (1.0 - RPEAKGAINFILTER) * rpeakgain0;
	  }

	  if (rpeakgain0 <= rpeakgain1) {
	    rpeakgain1 = rpeakgain0;
	    rpeaklimitdelay = peaklimitdelay;
	  } else if (rpeaklimitdelay == 0) {
	    rpeakgain1 = RPEAKGAINFILTER * rpeakgain0 +
	      (1.0 - RPEAKGAINFILTER) * rpeakgain1;
	  } else {
	    --rpeaklimitdelay;
	  }

	  sqrtrpeakgain = sqrt(rpeakgain1);
	  totalgain = npeakgain * sqrtrpeakgain;
    
	  buffer_write(output[pos], new * sqrtrpeakgain);
    
	  if (totalgain > maxgain)
	    maxgain = totalgain;
	  if (totalgain < mingain)
	    mingain = totalgain;
	  if (output[pos] > extra_maxlevel)
	    extra_maxlevel = output[pos];
	}

	plugin_data->ndelayptr = ndelayptr;
	plugin_data->rlevelsq0 = rlevelsq0;
	plugin_data->rlevelsq1 = rlevelsq1;
	plugin_data->mingain = mingain;
	plugin_data->maxgain = maxgain;
	plugin_data->rpeaklimitdelay = rpeaklimitdelay;
	plugin_data->rgain = rgain;
	plugin_data->rmastergain0 = rmastergain0;
	plugin_data->rpeakgain0 = rpeakgain0;
	plugin_data->rpeakgain1 = rpeakgain1;
	plugin_data->lastrgain = lastrgain;
	plugin_data->extra_maxlevel = extra_maxlevel;
}
#undef buffer_write
#undef RUN_ADDING
#undef RUN_REPLACING

#define buffer_write(b, v) (b += (v) * run_adding_gain)
#define RUN_ADDING    1
#define RUN_REPLACING 0

static void setRunAddingGainDysonCompress(LADSPA_Handle instance, LADSPA_Data gain) {
	((DysonCompress *)instance)->run_adding_gain = gain;
}

static void runAddingDysonCompress(LADSPA_Handle instance, unsigned long sample_count) {
	DysonCompress *plugin_data = (DysonCompress *)instance;
	LADSPA_Data run_adding_gain = plugin_data->run_adding_gain;

	/* Peak limit (dB) (float value) */
	const LADSPA_Data peak_limit = *(plugin_data->peak_limit);

	/* Release time (s) (float value) */
	const LADSPA_Data release_time = *(plugin_data->release_time);

	/* Fast compression ratio (float value) */
	const LADSPA_Data cfrate = *(plugin_data->cfrate);

	/* Compression ratio (float value) */
	const LADSPA_Data crate = *(plugin_data->crate);

	/* Input (array of floats of length sample_count) */
	const LADSPA_Data * const input = plugin_data->input;

	/* Output (array of floats of length sample_count) */
	LADSPA_Data * const output = plugin_data->output;
	LADSPA_Data * delay = plugin_data->delay;
	float extra_maxlevel = plugin_data->extra_maxlevel;
	float lastrgain = plugin_data->lastrgain;
	float maxgain = plugin_data->maxgain;
	float mingain = plugin_data->mingain;
	float ndelay = plugin_data->ndelay;
	unsigned int ndelayptr = plugin_data->ndelayptr;
	int peaklimitdelay = plugin_data->peaklimitdelay;
	float rgain = plugin_data->rgain;
	float rlevelsq0 = plugin_data->rlevelsq0;
	float rlevelsq1 = plugin_data->rlevelsq1;
	LADSPA_Data * rlevelsqe = plugin_data->rlevelsqe;
	LADSPA_Data * rlevelsqn = plugin_data->rlevelsqn;
	float rmastergain0 = plugin_data->rmastergain0;
	float rpeakgain0 = plugin_data->rpeakgain0;
	float rpeakgain1 = plugin_data->rpeakgain1;
	float rpeaklimitdelay = plugin_data->rpeaklimitdelay;
	float sample_rate = plugin_data->sample_rate;

#line 143 "dyson_compress_1403.xml"
	unsigned long pos;
	float targetlevel = MAXLEVEL * DB_CO(peak_limit);
	float rgainfilter = 1.0f / (release_time * sample_rate);
	float fastgaincompressionratio = cfrate;
	float compressionratio = crate;
	float efilt;
	float levelsqe;
	float gain;
	float tgain;
	float d;
	float fastgain;
	float qgain;
	float tslowgain;
	float slowgain;
	float npeakgain;
	float new;
	float nrgain;
	float ngain;
	float ngsq;
	float tnrgain;
	float sqrtrpeakgain;
	float totalgain;
	unsigned int i;

	for (pos = 0; pos < sample_count; pos++) {
	  // Ergh! this was originally meant to track a stereo signal
	  float levelsq0 = 2.0f * (input[pos] * input[pos]);

	  delay[ndelayptr] = input[pos];
	  ndelayptr++;

	  if (ndelayptr >= ndelay) {
	    ndelayptr = 0;
	  }

	  if (levelsq0 > rlevelsq0) {
	    rlevelsq0 = (levelsq0 * RLEVELSQ0FFILTER) +
	     rlevelsq0 * (1 - RLEVELSQ0FFILTER);
	  } else {
	    rlevelsq0 = (levelsq0 * RLEVELSQ0FILTER) +
	     rlevelsq0 * (1 - RLEVELSQ0FILTER);
	  }

	  if (rlevelsq0 <= FLOORLEVEL * FLOORLEVEL) {
	    goto skipagc;
	  }

	  if (rlevelsq0 > rlevelsq1) {
	    rlevelsq1 = rlevelsq0;
	  } else {
	    rlevelsq1 = rlevelsq0 * RLEVELSQ1FILTER +
	      rlevelsq1 * (1 - RLEVELSQ1FILTER);
	  }

	  rlevelsqn[0] = rlevelsq1;
	  for(i = 0; i < NFILT-1; i++) {
	    if (rlevelsqn[i] > rlevelsqn[i+1])
	      rlevelsqn[i+1] = rlevelsqn[i];
	    else
	      rlevelsqn[i+1] = rlevelsqn[i] * RLEVELSQ1FILTER +
	        rlevelsqn[i+1] * (1 - RLEVELSQ1FILTER);
	  }

	  efilt = RLEVELSQEFILTER;
	  levelsqe = rlevelsqe[0] = rlevelsqn[NFILT-1];
	  for(i = 0; i < NEFILT-1; i++) {
	    rlevelsqe[i+1] = rlevelsqe[i] * efilt +
	      rlevelsqe[i+1] * (1.0 - efilt);
	    if (rlevelsqe[i+1] > levelsqe)
	      levelsqe = rlevelsqe[i+1];
	    efilt *= 1.0f / 1.5f;
	  }

	  gain = targetlevel / sqrt(levelsqe);
	  if (compressionratio < 0.99f) {
	    if (compressionratio == 0.50f)
	      gain = sqrt(gain);
	    else
	      gain = f_exp(log(gain) * compressionratio);
	  }

	  if (gain < rgain)
	    rgain = gain * RLEVELSQEFILTER/2 +
	      rgain * (1 - RLEVELSQEFILTER/2);
	  else
	    rgain = gain * rgainfilter +
	      rgain * (1 - rgainfilter);

	  lastrgain = rgain;
	  if ( gain < lastrgain)
	    lastrgain = gain;

	skipagc:;

	  tgain = lastrgain;
    
	  d = delay[ndelayptr];
    
	  fastgain = tgain;
	  if (fastgain > MAXFASTGAIN)
	    fastgain = MAXFASTGAIN;
    
	  if (fastgain < 0.0001)
	    fastgain = 0.0001;

	  qgain = f_exp(log(fastgain) * fastgaincompressionratio);

	  tslowgain = tgain / qgain;
	  if (tslowgain > MAXSLOWGAIN)
	    tslowgain = MAXSLOWGAIN;
	  if (tslowgain < rmastergain0)
	    rmastergain0 = tslowgain;
	  else
	    rmastergain0 = tslowgain * RMASTERGAIN0FILTER +
	      (1 - RMASTERGAIN0FILTER) * rmastergain0;
    
	  slowgain = rmastergain0;
	  npeakgain = slowgain * qgain;
    
	  new = d * npeakgain;
	  if (fabs(new) >= MAXLEVEL)
	    nrgain = MAXLEVEL / fabs(new);
	  else
	    nrgain = 1.0;
    
	  ngain = nrgain;
    
	  ngsq = ngain * ngain;
	  if (ngsq <= rpeakgain0) {
	    rpeakgain0 = ngsq /* * 0.50 + rpeakgain0 * 0.50 */;
	    rpeaklimitdelay = peaklimitdelay;
	  } else if (rpeaklimitdelay == 0) {
	    if (nrgain > 1.0)
	      tnrgain = 1.0;
	    else
	      tnrgain = nrgain;
	    rpeakgain0 = tnrgain * RPEAKGAINFILTER +
	      (1.0 - RPEAKGAINFILTER) * rpeakgain0;
	  }

	  if (rpeakgain0 <= rpeakgain1) {
	    rpeakgain1 = rpeakgain0;
	    rpeaklimitdelay = peaklimitdelay;
	  } else if (rpeaklimitdelay == 0) {
	    rpeakgain1 = RPEAKGAINFILTER * rpeakgain0 +
	      (1.0 - RPEAKGAINFILTER) * rpeakgain1;
	  } else {
	    --rpeaklimitdelay;
	  }

	  sqrtrpeakgain = sqrt(rpeakgain1);
	  totalgain = npeakgain * sqrtrpeakgain;
    
	  buffer_write(output[pos], new * sqrtrpeakgain);
    
	  if (totalgain > maxgain)
	    maxgain = totalgain;
	  if (totalgain < mingain)
	    mingain = totalgain;
	  if (output[pos] > extra_maxlevel)
	    extra_maxlevel = output[pos];
	}

	plugin_data->ndelayptr = ndelayptr;
	plugin_data->rlevelsq0 = rlevelsq0;
	plugin_data->rlevelsq1 = rlevelsq1;
	plugin_data->mingain = mingain;
	plugin_data->maxgain = maxgain;
	plugin_data->rpeaklimitdelay = rpeaklimitdelay;
	plugin_data->rgain = rgain;
	plugin_data->rmastergain0 = rmastergain0;
	plugin_data->rpeakgain0 = rpeakgain0;
	plugin_data->rpeakgain1 = rpeakgain1;
	plugin_data->lastrgain = lastrgain;
	plugin_data->extra_maxlevel = extra_maxlevel;
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


	dysonCompressDescriptor =
	 (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

	if (dysonCompressDescriptor) {
		dysonCompressDescriptor->UniqueID = 1403;
		dysonCompressDescriptor->Label = "dysonCompress";
		dysonCompressDescriptor->Properties =
		 LADSPA_PROPERTY_HARD_RT_CAPABLE;
		dysonCompressDescriptor->Name =
		 D_("Dyson compressor");
		dysonCompressDescriptor->Maker =
		 "Steve Harris <steve@plugin.org.uk>";
		dysonCompressDescriptor->Copyright =
		 "GPL";
		dysonCompressDescriptor->PortCount = 6;

		port_descriptors = (LADSPA_PortDescriptor *)calloc(6,
		 sizeof(LADSPA_PortDescriptor));
		dysonCompressDescriptor->PortDescriptors =
		 (const LADSPA_PortDescriptor *)port_descriptors;

		port_range_hints = (LADSPA_PortRangeHint *)calloc(6,
		 sizeof(LADSPA_PortRangeHint));
		dysonCompressDescriptor->PortRangeHints =
		 (const LADSPA_PortRangeHint *)port_range_hints;

		port_names = (char **)calloc(6, sizeof(char*));
		dysonCompressDescriptor->PortNames =
		 (const char **)port_names;

		/* Parameters for Peak limit (dB) */
		port_descriptors[DYSONCOMPRESS_PEAK_LIMIT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[DYSONCOMPRESS_PEAK_LIMIT] =
		 D_("Peak limit (dB)");
		port_range_hints[DYSONCOMPRESS_PEAK_LIMIT].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_0;
		port_range_hints[DYSONCOMPRESS_PEAK_LIMIT].LowerBound = -30;
		port_range_hints[DYSONCOMPRESS_PEAK_LIMIT].UpperBound = 0;

		/* Parameters for Release time (s) */
		port_descriptors[DYSONCOMPRESS_RELEASE_TIME] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[DYSONCOMPRESS_RELEASE_TIME] =
		 D_("Release time (s)");
		port_range_hints[DYSONCOMPRESS_RELEASE_TIME].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW;
		port_range_hints[DYSONCOMPRESS_RELEASE_TIME].LowerBound = 0;
		port_range_hints[DYSONCOMPRESS_RELEASE_TIME].UpperBound = 1;

		/* Parameters for Fast compression ratio */
		port_descriptors[DYSONCOMPRESS_CFRATE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[DYSONCOMPRESS_CFRATE] =
		 D_("Fast compression ratio");
		port_range_hints[DYSONCOMPRESS_CFRATE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[DYSONCOMPRESS_CFRATE].LowerBound = 0;
		port_range_hints[DYSONCOMPRESS_CFRATE].UpperBound = 1;

		/* Parameters for Compression ratio */
		port_descriptors[DYSONCOMPRESS_CRATE] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		port_names[DYSONCOMPRESS_CRATE] =
		 D_("Compression ratio");
		port_range_hints[DYSONCOMPRESS_CRATE].HintDescriptor =
		 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
		port_range_hints[DYSONCOMPRESS_CRATE].LowerBound = 0;
		port_range_hints[DYSONCOMPRESS_CRATE].UpperBound = 1;

		/* Parameters for Input */
		port_descriptors[DYSONCOMPRESS_INPUT] =
		 LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		port_names[DYSONCOMPRESS_INPUT] =
		 D_("Input");
		port_range_hints[DYSONCOMPRESS_INPUT].HintDescriptor = 0;

		/* Parameters for Output */
		port_descriptors[DYSONCOMPRESS_OUTPUT] =
		 LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		port_names[DYSONCOMPRESS_OUTPUT] =
		 D_("Output");
		port_range_hints[DYSONCOMPRESS_OUTPUT].HintDescriptor = 0;

		dysonCompressDescriptor->activate = activateDysonCompress;
		dysonCompressDescriptor->cleanup = cleanupDysonCompress;
		dysonCompressDescriptor->connect_port = connectPortDysonCompress;
		dysonCompressDescriptor->deactivate = NULL;
		dysonCompressDescriptor->instantiate = instantiateDysonCompress;
		dysonCompressDescriptor->run = runDysonCompress;
		dysonCompressDescriptor->run_adding = runAddingDysonCompress;
		dysonCompressDescriptor->set_run_adding_gain = setRunAddingGainDysonCompress;
	}
}

void  __attribute__((destructor)) swh_fini() {
	if (dysonCompressDescriptor) {
		free((LADSPA_PortDescriptor *)dysonCompressDescriptor->PortDescriptors);
		free((char **)dysonCompressDescriptor->PortNames);
		free((LADSPA_PortRangeHint *)dysonCompressDescriptor->PortRangeHints);
		free(dysonCompressDescriptor);
	}

}
