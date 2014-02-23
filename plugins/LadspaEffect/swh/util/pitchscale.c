/****************************************************************************
*
* NAME: smsPitchScale.cp
* VERSION: 1.01
* HOME URL: http://www.dspdimension.com
* KNOWN BUGS: none
*
* SYNOPSIS: Routine for doing pitch scaling while maintaining
* duration using the Short Time Fourier Transform.
*
* DESCRIPTION: The routine takes a pitchScale factor value which is between 0.5
* (one octave down) and 2. (one octave up). A value of exactly 1 does not change
* the pitch. numSampsToProcess tells the routine how many samples in indata[0...
* numSampsToProcess-1] should be pitch scaled and moved to outdata[0 ...
* numSampsToProcess-1]. The two buffers can be identical (ie. it can process the
* data in-place). fftFrameLength defines the FFT frame size used for the
* processing. Typical values are 1024, 2048 and 4096. It may be any value <=
* MAX_FFT_FRAME_LENGTH but it MUST be a power of 2. osamp is the STFT
* oversampling factor which also determines the overlap between adjacent STFT
* frames. It should at least be 4 for moderate scaling ratios. A value of 32 is
* recommended for best quality. sampleRate takes the sample rate for the signal 
* in unit Hz, ie. 44100 for 44.1 kHz audio. The data passed to the routine in 
* indata[] should be in the range [-1.0, 1.0), which is also the output range 
* for the data. 
*
* COPYRIGHT 1999 Stephan M. Sprenger <sms@dspdimension.com>
*
* 						The Wide Open License (WOL)
*
* Permission to use, copy, modify, distribute and sell this software and its
* documentation for any purpose is hereby granted without fee, provided that
* the above copyright notice and this license appear in all source copies. 
* THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY OF
* ANY KIND. See http://www.dspguru.com/wol.htm for more information.
*
*****************************************************************************/

#include <string.h>
#include "../config.h"
#include <math.h>

#include "pitchscale.h"

static float ps_in[MAX_FRAME_LENGTH*2], ps_out[MAX_FRAME_LENGTH*2];
static fft_plan aplan = NULL, splan = NULL;

void pitch_scale(sbuffers *buffers, const double pitchScale, const long
		fftFrameLength, const long osamp, const long numSampsToProcess,
		const double sampleRate, const float *indata, float *outdata,
		const int adding, const float gain) {
/*
	Routine smsPitchScale(). See top of file for explanation Purpose: doing
	pitch scaling while maintaining duration using the Short Time Fourier
	Transform.  Author: (c)1999 Stephan M. Sprenger <sms@dspdimension.com>
*/
	double magn, phase, tmp;
	double freqPerBin, expct;
        long i,k, qpd, index, inFifoLatency, stepSize,
	  fftFrameSize2;
	double phaseArr[MAX_FRAME_LENGTH];
	float ri[16];

	float *gInFIFO = buffers->gInFIFO;
	float *gOutFIFO = buffers->gOutFIFO;
	float *gLastPhase = buffers->gLastPhase;
	float *gSumPhase = buffers->gSumPhase;
	float *gOutputAccum = buffers->gOutputAccum;
	float *gAnaFreq = buffers->gAnaFreq;
	float *gAnaMagn = buffers->gAnaMagn;
	float *gSynFreq = buffers->gSynFreq;
	float *gSynMagn = buffers->gSynMagn;
	float *gWindow = buffers->gWindow;
	long gRover = buffers->gRover;

	if (aplan == NULL) {
		int i;

		for (i=0; i<fftFrameLength; i++) {
			ps_in[i + fftFrameLength] = 0.0f;
		}
#ifdef FFTW3
		aplan = fftwf_plan_r2r_1d(fftFrameLength, ps_in, ps_out, FFTW_R2HC, FFTW_MEASURE);
		splan = fftwf_plan_r2r_1d(fftFrameLength, ps_in, ps_out, FFTW_HC2R, FFTW_MEASURE);
#else
		aplan = rfftw_create_plan(fftFrameLength, FFTW_REAL_TO_COMPLEX, FFTW_ESTIMATE);
		splan = rfftw_create_plan(fftFrameLength, FFTW_COMPLEX_TO_REAL, FFTW_ESTIMATE);
#endif
	}

	/* set up some handy variables */
	fftFrameSize2 = fftFrameLength/2;
	stepSize = fftFrameLength/osamp;
	freqPerBin = sampleRate*2.0/(double)fftFrameLength;
	expct = 2.0*M_PI*(double)stepSize/(double)fftFrameLength;
	inFifoLatency = fftFrameLength-stepSize;
	if (gRover == false) gRover = inFifoLatency;

	/* main processing loop */
	for (i = 0; i < numSampsToProcess; i++){

		/* As long as we have not yet collected enough data just read in */
		gInFIFO[gRover] = indata[i];
		if (adding) {
			outdata[i] += (gOutFIFO[gRover-inFifoLatency] * gain);
		} else {
			outdata[i] = gOutFIFO[gRover-inFifoLatency];
		}
		gRover++;

		/* As long as we have not yet collected enough data just read in */
		/* now we have enough data for processing */
		if (gRover >= fftFrameLength) {
			gRover = inFifoLatency;

			/* do windowing and store */
			for (k = 0; k < fftFrameLength; k++) {
				ps_in[k] = gInFIFO[k] * gWindow[k];
			}

		/* As long as we have not yet collected enough data just read in */

			/* ***************** ANALYSIS ******************* */
			/* do transform */
#ifdef FFTW3
			fftwf_execute(aplan);
#else
			rfftw_one(aplan, ps_in, ps_out);
#endif

			/* this is the analysis step */

			/* Hard math first, we can 3dnow this */
			for (k = 1; k <= fftFrameSize2; k+=8) {
				float *mb = &gAnaMagn[k];
			
				ri[0] = ps_out[k];
				ri[2] = ps_out[k+1];
				ri[4] = ps_out[k+2];
				ri[6] = ps_out[k+3];
				ri[8] = ps_out[k+4];
				ri[10] = ps_out[k+5];
				ri[12] = ps_out[k+6];
				ri[14] = ps_out[k+7];
			
				ri[1] = ps_out[fftFrameLength - k];
				ri[3] = ps_out[fftFrameLength - (k + 1)];
				ri[5] = ps_out[fftFrameLength - (k + 2)];
				ri[7] = ps_out[fftFrameLength - (k + 3)];
				ri[9] = ps_out[fftFrameLength - (k + 4)];
				ri[11] = ps_out[fftFrameLength - (k + 5)];
				ri[13] = ps_out[fftFrameLength - (k + 6)];
				ri[15] = ps_out[fftFrameLength - (k + 7)];

				/* compute magnitude and phase. */
#ifdef ACCEL_3DNOW
#warning Using processor specific 3DNow! accelerations
				__asm__ __volatile__ (
				" \n\
				femms  \n\
				movq (%%eax), %%mm0  \n\
				movq 8(%%eax), %%mm1  \n\
				movq 16(%%eax), %%mm2  \n\
				movq 24(%%eax), %%mm3  \n\
				movq 32(%%eax), %%mm4  \n\
				movq 40(%%eax), %%mm5 \n\
				movq 48(%%eax), %%mm6 \n\
				movq 56(%%eax), %%mm7 \n\
				# do the squares and add \n\
				pfmul	%%mm0, %%mm0 \n\
				pfacc   %%mm0, %%mm0 \n\
				pfmul	%%mm1, %%mm1 \n\
				pfacc   %%mm1, %%mm1 \n\
				pfmul	%%mm2, %%mm2 \n\
				pfacc	%%mm2, %%mm2 \n\
				pfmul	%%mm3, %%mm3 \n\
				pfacc   %%mm3, %%mm3 \n\
				pfmul	%%mm4, %%mm4 \n\
				pfacc	%%mm4, %%mm4 \n\
				pfmul	%%mm5, %%mm5 \n\
				pfacc   %%mm5, %%mm5 \n\
				pfmul	%%mm6, %%mm6 \n\
				pfacc	%%mm6, %%mm6 \n\
				pfmul	%%mm7, %%mm7 \n\
				pfacc   %%mm7, %%mm7 \n\
				# Recip square roots. \n\
				pfrsqrt %%mm0, %%mm0 \n\
				pfrsqrt %%mm1, %%mm1 \n\
				pfrsqrt %%mm2, %%mm2 \n\
				pfrsqrt %%mm3, %%mm3 \n\
				pfrsqrt %%mm4, %%mm4 \n\
				pfrsqrt %%mm5, %%mm5 \n\
				pfrsqrt %%mm6, %%mm6 \n\
				pfrsqrt %%mm7, %%mm7 \n\
				pfrcp %%mm0, %%mm0 \n\
				pfrcp %%mm1, %%mm1 \n\
				pfrcp %%mm2, %%mm2 \n\
				pfrcp %%mm3, %%mm3 \n\
				pfrcp %%mm4, %%mm4 \n\
				pfrcp %%mm5, %%mm5 \n\
				pfrcp %%mm6, %%mm6 \n\
				pfrcp %%mm7, %%mm7 \n\
				# ship em out			 \n\
				movd	%%mm0, (%%edx) \n\
				movd	%%mm1, 4(%%edx) \n\
				movd	%%mm2, 8(%%edx) \n\
				movd	%%mm3, 12(%%edx) \n\
				movd	%%mm4, 16(%%edx) \n\
				movd	%%mm5, 20(%%edx) \n\
				movd	%%mm6, 24(%%edx) \n\
				movd	%%mm7, 28(%%edx) \n\
				femms \n\
				"
				:
				: "a" (ri), "d" (mb)
				: "memory");
									

#else
				mb[0] = sqrt(ri[0]*ri[0]+ ri[1]*ri[1]);
				mb[1] = sqrt(ri[2]*ri[2] + ri[3]*ri[3]);
				mb[2] = sqrt(ri[4]*ri[4] + ri[5]*ri[5]);
				mb[3] = sqrt(ri[6]*ri[6] + ri[7]*ri[7]);
#endif

				phaseArr[k] = atan2(ri[1], ri[0]);
				phaseArr[k+1] = atan2(ri[3], ri[2]);
				phaseArr[k+2] = atan2(ri[5], ri[4]);
				phaseArr[k+3] = atan2(ri[7], ri[6]);
				phaseArr[k+4] = atan2(ri[9], ri[8]);
				phaseArr[k+5] = atan2(ri[11], ri[10]);
				phaseArr[k+6] = atan2(ri[13], ri[12]);
				phaseArr[k+7] = atan2(ri[15], ri[14]);
			}
			
			for (k = 1; k <= fftFrameSize2; k++) {

				/* compute phase difference */
				tmp = phaseArr[k] - gLastPhase[k];
				gLastPhase[k] = phaseArr[k];

				/* subtract expected phase difference */
				tmp -= (double)k*expct;

				/* map delta phase into +/- Pi interval */
				qpd = tmp/M_PI;
				if (qpd >= 0) qpd += qpd&1;
				else qpd -= qpd&1;
				tmp -= M_PI*(double)qpd;

				/* get deviation from bin frequency from the +/- Pi interval */
				tmp = osamp*tmp/(2.0f*M_PI);

				/* compute the k-th partials' true frequency */
				tmp = (double)k*freqPerBin + tmp*freqPerBin;

				/* store magnitude and true frequency in analysis arrays */
				gAnaFreq[k] = tmp;

			}

			/* ***************** PROCESSING ******************* */
			/* this does the actual pitch scaling */
			memset(gSynMagn, 0, fftFrameLength*sizeof(float));
			memset(gSynFreq, 0, fftFrameLength*sizeof(float));
			for (k = 0; k <= fftFrameSize2; k++) {
				index = k/pitchScale;
				if (index <= fftFrameSize2) {
					/* new bin overrides existing if magnitude is higher */ 
					if (gAnaMagn[index] > gSynMagn[k]) {
						gSynMagn[k] = gAnaMagn[index];
						gSynFreq[k] = gAnaFreq[index] * pitchScale;
					}

					/* fill empty bins with nearest neighbour */

					if ((gSynFreq[k] == 0.) && (k > 0)) {
						gSynFreq[k] = gSynFreq[k-1];
						gSynMagn[k] = gSynMagn[k-1];
					}
				}
			}


			/* ***************** SYNTHESIS ******************* */
			/* this is the synthesis step */
			for (k = 1; k <= fftFrameSize2; k++) {

				/* get magnitude and true frequency from synthesis arrays */
				magn = gSynMagn[k];
				tmp = gSynFreq[k];

				/* subtract bin mid frequency */
				tmp -= (double)k*freqPerBin;

				/* get bin deviation from freq deviation */
				tmp /= freqPerBin;

				/* take osamp into account */
				tmp = 2.*M_PI*tmp/osamp;

				/* add the overlap phase advance back in */
				tmp += (double)k*expct;

				/* accumulate delta phase to get bin phase */
				gSumPhase[k] += tmp;
				phase = gSumPhase[k];

				ps_in[k] = magn*cosf(phase);
				ps_in[fftFrameLength - k] = magn*sinf(phase);
			} 

			/* do inverse transform */
#ifdef FFTW3
			fftwf_execute(splan);
#else
			rfftw_one(splan, ps_in, ps_out);
#endif

			/* do windowing and add to output accumulator */ 
			for(k=0; k < fftFrameLength; k++) {
				gOutputAccum[k] += 2.0f*gWindow[k]*ps_out[k]/(fftFrameSize2*osamp);
			}
			for (k = 0; k < stepSize; k++) gOutFIFO[k] = gOutputAccum[k];

			/* shift accumulator */
			memmove(gOutputAccum, gOutputAccum+stepSize, fftFrameLength*sizeof(float));

			/* move input FIFO */
			for (k = 0; k < inFifoLatency; k++) gInFIFO[k] = gInFIFO[k+stepSize];
		}
	}

	buffers->gRover = gRover;
}
