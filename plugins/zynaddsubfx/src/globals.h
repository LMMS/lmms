/*
  ZynAddSubFX - a software synthesizer

  globals.h - it contains program settings and the program capabilities
              like number of parts, of effects
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2 or later) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

*/


#ifndef GLOBALS_H
#define GLOBALS_H

//What float type I use for internal sampledata
#define REALTYPE float

struct FFTFREQS {
    REALTYPE *s,*c;//sine and cosine components
};

extern void newFFTFREQS(FFTFREQS *f,int size);
extern void deleteFFTFREQS(FFTFREQS *f);

/**Sampling rate*/
extern int SAMPLE_RATE;

/**
 * The size of a sound buffer (or the granularity)
 * All internal transfer of sound data use buffer of this size
 * All parameters are constant during this period of time, exception
 * some parameters(like amplitudes) which are linear interpolated.
 * If you increase this you'll ecounter big latencies, but if you
 * decrease this the CPU requirements gets high.
 */
extern int SOUND_BUFFER_SIZE;


/**
 * The size of ADnote Oscillator
 * Decrease this => poor quality
 * Increase this => CPU requirements gets high (only at start of the note)
 */
extern int OSCIL_SIZE;

/**
 * The number of harmonics of additive synth
 * This must be smaller than OSCIL_SIZE/2
 */
#define MAX_AD_HARMONICS 128


/**
 * The number of harmonics of substractive
 */
#define MAX_SUB_HARMONICS 64


/*
 * The maximum number of samples that are used for 1 PADsynth instrument(or item)
 */
#define PAD_MAX_SAMPLES 64


/*
 * Number of parts
 */
#define NUM_MIDI_PARTS 16

/*
 * Number of Midi channes
 */
#define NUM_MIDI_CHANNELS 16

/*
 * The number of voices of additive synth for a single note
 */
#define NUM_VOICES 8

/*
 * The poliphony (notes)
 */
#define POLIPHONY 60

/*
 * Number of system effects
 */
#define NUM_SYS_EFX 4


/*
 * Number of insertion effects
 */
#define NUM_INS_EFX 8

/*
 * Number of part's insertion effects
 */
#define NUM_PART_EFX 3

/*
 * Maximum number of the instrument on a part
 */
#define NUM_KIT_ITEMS 16


/*
 * How is applied the velocity sensing
 */
#define VELOCITY_MAX_SCALE 8.0

/*
 * The maximum length of instrument's name
 */
#define PART_MAX_NAME_LEN 30

/*
 * The maximum number of bands of the equaliser
 */
#define MAX_EQ_BANDS 8
#if (MAX_EQ_BANDS>=20)
#error "Too many EQ bands in globals.h"
#endif


/*
 * Maximum filter stages
 */
#define MAX_FILTER_STAGES 5

/*
 * Formant filter (FF) limits
 */
#define FF_MAX_VOWELS 6
#define FF_MAX_FORMANTS 12
#define FF_MAX_SEQUENCE 8

#define LOG_2 0.693147181
#define PI 3.1415926536
#define LOG_10 2.302585093

/*
 * The threshold for the amplitude interpolation used if the amplitude
 * is changed (by LFO's or Envelope's). If the change of the amplitude
 * is below this, the amplitude is not interpolated
 */
#define AMPLITUDE_INTERPOLATION_THRESHOLD 0.0001

/*
 * How the amplitude threshold is computed
 */
#define ABOVE_AMPLITUDE_THRESHOLD(a,b) ( ( 2.0*fabs( (b) - (a) ) /  \
      ( fabs( (b) + (a) + 0.0000000001) ) ) > AMPLITUDE_INTERPOLATION_THRESHOLD )

/*
 * Interpolate Amplitude
 */
#define INTERPOLATE_AMPLITUDE(a,b,x,size) ( (a) + \
      ( (b) - (a) ) * (REALTYPE)(x) / (REALTYPE) (size) )


/*
 * dB
 */
#define dB2rap(dB) ((exp((dB)*LOG_10/20.0)))
#define rap2dB(rap) ((20*log(rap)/LOG_10))

/*
 * The random generator (0.0..1.0)
 */
#define RND (rand()/(RAND_MAX+1.0))

#define ZERO(data,size) {char *data_=(char *) data;for (int i=0;i<size;i++) data_[i]=0;};

enum ONOFFTYPE {OFF=0,ON=1};

enum MidiControllers {C_NULL=0,C_pitchwheel=1000,C_expression=11,C_panning=10,
                      C_filtercutoff=74,C_filterq=71,C_bandwidth=75,C_modwheel=1,C_fmamp=76,
                      C_volume=7,C_sustain=64,C_allnotesoff=123,C_allsoundsoff=120,C_resetallcontrollers=121,
                      C_portamento=65,C_resonance_center=77,C_resonance_bandwidth=78,

                      C_dataentryhi=0x06,C_dataentrylo=0x26,C_nrpnhi=99,C_nrpnlo=98
                     };

enum LegatoMsg {LM_Norm, LM_FadeIn, LM_FadeOut, LM_CatchUp, LM_ToNorm};

//is like i=(int)(floor(f))
#ifdef ASM_F2I_YES
#define F2I(f,i) __asm__ __volatile__ ("fistpl %0" : "=m" (i) : "t" (f-0.49999999) : "st") ;
#else
#define F2I(f,i) (i)=((f>0) ? ( (int)(f) ) :( (int)(f-1.0) ));
#endif



#ifndef  O_BINARY
#define O_BINARY 0
#endif

#endif

