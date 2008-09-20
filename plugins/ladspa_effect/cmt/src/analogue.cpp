/* analogue.cpp

   Analogue Voice - Analog synthesizer voice
   Copyright (c) 2000 David A. Bartold

   Computer Music Toolkit - a library of LADSPA plugins. Copyright (C)
   2000 Richard W.E. Furse. The author may be contacted at
   richard@muse.demon.co.uk.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public Licence as
   published by the Free Software Foundation; either version 2 of the
   Licence, or (at your option) any later version.

   This library is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307, USA. */

/*****************************************************************************/

#include <malloc.h>
#include <math.h>
#include <stdlib.h>
#include "cmt.h"

#define PORT_OUT            0
#define PORT_GATE           1
#define PORT_VELOCITY       2
#define PORT_FREQ           3
#define PORT_DCO1_OCTAVE    4
#define PORT_DCO1_WAVEFORM  5
#define PORT_DCO1_FM        6
#define PORT_DCO1_PWM       7
#define PORT_DCO1_ATTACK    8
#define PORT_DCO1_DECAY     9
#define PORT_DCO1_SUSTAIN  10
#define PORT_DCO1_RELEASE  11
#define PORT_DCO2_OCTAVE   12
#define PORT_DCO2_WAVEFORM 13
#define PORT_DCO2_FM       14
#define PORT_DCO2_PWM      15
#define PORT_DCO2_ATTACK   16
#define PORT_DCO2_DECAY    17
#define PORT_DCO2_SUSTAIN  18
#define PORT_DCO2_RELEASE  19
#define PORT_LFO_FREQ      20
#define PORT_LFO_FADEIN    21
#define PORT_FILT_ENV_MOD  22
#define PORT_FILT_LFO_MOD  23
#define PORT_FILT_RES      24
#define PORT_FILT_ATTACK   25
#define PORT_FILT_DECAY    26
#define PORT_FILT_SUSTAIN  27
#define PORT_FILT_RELEASE  28

#define NUM_PORTS          29

#ifndef PI
#define PI 3.14159265358979F
#endif

typedef struct Envelope
{
  int          envelope_decay;
  LADSPA_Data  envelope;

  Envelope () : envelope_decay (0), envelope (0.0) {}
} Envelope;

class Analogue : public CMT_PluginInstance
{
  LADSPA_Data sample_rate;

  int         trigger;
  Envelope    dco1_env;
  Envelope    dco2_env;
  Envelope    filt_env; 
  LADSPA_Data d1;
  LADSPA_Data d2;

  LADSPA_Data dco1_accum;
  LADSPA_Data dco2_accum;
  LADSPA_Data lfo_accum;

  LADSPA_Data lfo_vol;

public:
  Analogue(const LADSPA_Descriptor * Descriptor,
           unsigned long             SampleRate)
    : CMT_PluginInstance(NUM_PORTS),
      sample_rate (SampleRate),
      trigger (0),
      d1 (0.0), d2 (0.0),
      dco1_accum (0.0), dco2_accum (0.0), lfo_accum (0.0) {
  }

  ~Analogue () {
  }

  /* Third-order approximation of a sine wave. */
  static inline LADSPA_Data
  fast_sin(LADSPA_Data x) {
    if (x > PI)
      x = (x < PI * 1.5F) ? (PI - x) : (x - 2.0F * PI);
    else if (x > PI * 0.5F)
      x = PI - x;

    return x * (1.05F - x * x * 0.175F);
  }

  static inline LADSPA_Data
  tri(LADSPA_Data x) {
    if (x > 0.75F)
      x = x - 1.0F;
    else if (x > 0.25F)
      x = 0.5F - x;

    return x * 4.0F;
  }

  static inline LADSPA_Data
  envelope(Envelope    *env,
           int          gate,
           LADSPA_Data  attack,
           LADSPA_Data  decay,
           LADSPA_Data  sustain,
           LADSPA_Data  release)
  {
    if (gate)
      if (env->envelope_decay == 0)
        {
          env->envelope += (1.0F - env->envelope) * attack;
          if (env->envelope >= 0.95F)
            env->envelope_decay = 1;
        }
      else
        env->envelope += (sustain - env->envelope) * decay;
    else
      env->envelope += -env->envelope * release;

    return env->envelope;
  }

  static void
  activate(LADSPA_Handle Instance) {
    Analogue *analogue = (Analogue*) Instance;

    analogue->trigger = 0;
    analogue->dco1_env.envelope_decay = 0;
    analogue->dco1_env.envelope = 0.0;
    analogue->dco2_env.envelope_decay = 0;
    analogue->dco2_env.envelope = 0.0;
    analogue->filt_env.envelope_decay = 0;
    analogue->filt_env.envelope = 0.0;
    analogue->d1 = 0.0F;
    analogue->d2 = 0.0F;

    analogue->dco1_accum = 0.0F;
    analogue->dco2_accum = 0.0F;
    analogue->lfo_accum = 0.0F;
    analogue->lfo_vol = 0.0F;
  }

  static inline LADSPA_Data
  osc(int          waveform,
      LADSPA_Data  inc,
      LADSPA_Data  width,
      LADSPA_Data *accum) {
    *accum += inc;
    while (*accum >= 1.0F)
      *accum -= 1.0F;

    /* 0 = Sine wave */
    if (waveform == 0)
      if (*accum < width)
        return fast_sin (*accum / width * PI);
      else
        return fast_sin (PI + (*accum - width) / (1.0F - width) * PI);

    /* 1 = Triangle wave */
    else if (waveform == 1)
      if (*accum < width)
        return tri (*accum / width * 0.5);
      else
        return tri (0.5 + (*accum - width) * 0.5 / (1.0F - width));

    /* 2 = Square wave */
    else if (waveform == 2)
      return (*accum > width) ? 1.0F : -1.0F;

    /* 3 = Sawtooth wave */
    else if (waveform == 3)
      if (*accum < width)
        return *accum / width * 2.0F - 1.0F;
      else
        return (*accum - width) / (1.0F - width) * 2.0F - 1.0F;

    /* 4 = Fullwave Rectified Sine wave */
    else if (waveform == 4)
      if (*accum < width)
        return fast_sin (*accum / width * PI);
      else
        return fast_sin ((*accum - width) / (1.0F - width) * PI);

    /* 5 = Static */
    else
      return (rand () & 1) ? -1.0F : 1.0F;
  }

  static LADSPA_Data
  inc(LADSPA_Data oct,
      LADSPA_Data freq,
      LADSPA_Data sample_rate) {
    return pow (2.0, oct) * freq / sample_rate;
  }

  static void
  calc_a_b_c(Analogue    *analogue,
             LADSPA_Data  freq,
             LADSPA_Data *a,
             LADSPA_Data *b,
             LADSPA_Data *c) {
    LADSPA_Data top_freq, k, res;
  
    top_freq = freq;
    top_freq *= PI / analogue->sample_rate;
    res = exp (-1.20 + 3.455 * *analogue->m_ppfPorts[PORT_FILT_RES]);

    k = exp (-top_freq / res);
  
    *a = 2.0 * cos (2.0 * top_freq) * k;
    *b = -k * k;
    *c = (1.0 - *a - *b) * 0.2;
  }

  static inline LADSPA_Data
  multiplier(Analogue *analogue,
             LADSPA_Data value) {
    return 1.0 - pow (0.05, 1.0 / (analogue->sample_rate * value));
  }

  static void
  run(LADSPA_Handle Instance,
      unsigned long SampleCount) {
    Analogue *analogue = (Analogue*) Instance;
    unsigned long i;
    int waveform1, waveform2;
    int gate;
    LADSPA_Data lfo_inc, inc1, inc2;
    LADSPA_Data attack1, decay1, release1;
    LADSPA_Data attack2, decay2, release2;
    LADSPA_Data filt_attack, filt_decay, filt_release;
    LADSPA_Data lfo_fadein, a, b, c;
    LADSPA_Data dco1_pwm, dco2_pwm;
    LADSPA_Data dco1_fm, dco2_fm;
    LADSPA_Data filt_lfo_mod;
    LADSPA_Data **ports;

    ports = analogue->m_ppfPorts;
    gate = (*ports[PORT_GATE] > 0.0);
    if (gate == 1 && analogue->trigger == 0)
      {
        analogue->lfo_vol = 0.0F;
        analogue->dco1_env.envelope_decay = 0;
        analogue->dco1_env.envelope = 0.0;
        analogue->dco2_env.envelope_decay = 0;
        analogue->dco2_env.envelope = 0.0;
        analogue->filt_env.envelope_decay = 0;
        analogue->filt_env.envelope = 0.0;
      }

    analogue->trigger = gate;

    waveform1 = (int) *ports[PORT_DCO1_WAVEFORM];
    waveform2 = (int) *ports[PORT_DCO2_WAVEFORM];

    inc1 = inc (*ports[PORT_DCO1_OCTAVE],
                *ports[PORT_FREQ],
                analogue->sample_rate);
    inc2 = inc (*ports[PORT_DCO2_OCTAVE],
                *ports[PORT_FREQ],
                analogue->sample_rate);
    lfo_inc = 2.0F * PI * *ports[PORT_LFO_FREQ] / analogue->sample_rate;

    attack1 = multiplier (analogue, *ports[PORT_DCO1_ATTACK]);
    decay1 = multiplier (analogue, *ports[PORT_DCO1_DECAY]);
    release1 = multiplier (analogue, *ports[PORT_DCO1_RELEASE]);

    attack2 = multiplier (analogue, *ports[PORT_DCO2_ATTACK]);
    decay2 = multiplier (analogue, *ports[PORT_DCO2_DECAY]);
    release2 = multiplier (analogue, *ports[PORT_DCO2_RELEASE]);

    filt_attack = multiplier (analogue, *ports[PORT_FILT_ATTACK]);
    filt_decay = multiplier (analogue, *ports[PORT_FILT_DECAY]);
    filt_release = multiplier (analogue, *ports[PORT_FILT_RELEASE]);

    lfo_fadein = 1.0 / (*ports[PORT_LFO_FADEIN] * analogue->sample_rate);

    dco1_pwm = *analogue->m_ppfPorts[PORT_DCO1_PWM] * 0.225F;
    dco2_pwm = *analogue->m_ppfPorts[PORT_DCO2_PWM] * 0.225F;
    dco1_fm = *analogue->m_ppfPorts[PORT_DCO1_FM] * inc1 * 0.45F;
    dco2_fm = *analogue->m_ppfPorts[PORT_DCO2_FM] * inc2 * 0.45F;
    filt_lfo_mod = *analogue->m_ppfPorts[PORT_FILT_LFO_MOD] * 0.45F;

    for (i = 0; i < SampleCount; i++)
      {
        LADSPA_Data lfo, sample;

        analogue->lfo_accum += lfo_inc;
        while (analogue->lfo_accum >= 2.0F * PI)
          analogue->lfo_accum -= 2.0F * PI;

        lfo = fast_sin (analogue->lfo_accum) * analogue->lfo_vol;

        analogue->lfo_vol += lfo_fadein;
        if (analogue->lfo_vol >= 1.0F)
          analogue->lfo_vol = 1.0F;

        envelope (&analogue->filt_env,
                  gate, filt_attack, filt_decay,
                  *ports[PORT_FILT_SUSTAIN], filt_release);

        if ((i & 0x000f) == 0)
          calc_a_b_c (analogue,
           *ports[PORT_FREQ] * 0.25F + (analogue->filt_env.envelope *
            *ports[PORT_FILT_ENV_MOD] * *ports[PORT_VELOCITY] *
            (1.5 + filt_lfo_mod * lfo)) * *ports[PORT_FREQ] * 10.0F, &a, &b, &c);

        sample = osc (waveform1, inc1 * (1.0 + lfo * dco1_fm),
                      0.5F + lfo * dco1_pwm,
                      &analogue->dco1_accum)
                 * envelope (&analogue->dco1_env,
                             gate, attack1, decay1,
                             *ports[PORT_DCO1_SUSTAIN], release1)
               + osc (waveform2, inc2 * (1.0 + lfo * dco2_fm),
                       0.5F + lfo * dco2_pwm,
                       &analogue->dco2_accum)
                  * envelope (&analogue->dco2_env,
                              gate, attack2, decay2,
                              *ports[PORT_DCO2_SUSTAIN], release2);

        sample = a * analogue->d1 +
                 b * analogue->d2 +
                 c * *ports[PORT_VELOCITY] * sample;

        analogue->d2 = analogue->d1;
        analogue->d1 = sample;
        ports[PORT_OUT][i] = sample;
      }
  }
};

static LADSPA_PortDescriptor g_psPortDescriptors[] =
{
  LADSPA_PORT_AUDIO | LADSPA_PORT_OUTPUT,
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT
};

static const char * const g_psPortNames[] =
{
  "Out",
  "Gate",
  "Velocity",
  "Frequency (Hz)",

  "DCO1 Octave",
  "DCO1 Waveform",
  "DCO1 LFO Frequency Modulation",
  "DCO1 LFO Pulse Width Modulation",

  "DCO1 Attack",
  "DCO1 Decay",
  "DCO1 Sustain",
  "DCO1 Release",

  "DCO2 Octave",
  "DCO2 Waveform",
  "DCO2 LFO Frequency Modulation",
  "DCO2 LFO Pulse Width Modulation",

  "DCO2 Attack",
  "DCO2 Decay",
  "DCO2 Sustain",
  "DCO2 Release",

  "LFO Frequency (Hz)",
  "LFO Fadein",

  "Filter Envelope Modulation",
  "Filter LFO Modulation",
  "Filter Resonance",

  "Filter Attack",
  "Filter Decay",
  "Filter Sustain",
  "Filter Release"
};

static LADSPA_PortRangeHint g_psPortRangeHints[] =
{
  /* Hints, Lower bound, Upper bound */
  { 0, 0.0, 0.0 },
  { LADSPA_HINT_TOGGLED, 0.0, 0.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 20000.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.001, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 10.0 },

  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, -2.0, 2.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW |
    LADSPA_HINT_INTEGER, -0.1, 5.1 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 1.0 },

  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.01, 8.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.01, 8.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.00, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.01, 8.0 },

  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, -2.0, 2.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW |
    LADSPA_HINT_INTEGER, -0.1, 5.1 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 1.0 },

  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.01, 8.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.01, 8.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.00, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.01, 8.0 },

  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 20.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.01, 8.0 },

  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 1.0 },

  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.01, 8.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.01, 8.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.00, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.01, 8.0 }
};

void
initialise_analogue() {
  CMT_Descriptor * psDescriptor;

  psDescriptor = new CMT_Descriptor
      (1221,
       "analogue",
       LADSPA_PROPERTY_HARD_RT_CAPABLE,
       "Analogue Voice",
       CMT_MAKER("David A. Bartold"),
       CMT_COPYRIGHT("2000", "David A. Bartold"),
       NULL,
       CMT_Instantiate<Analogue>,
       Analogue::activate,
       Analogue::run,
       NULL,
       NULL,
       NULL);

  for (int i = 0; i < NUM_PORTS; i++)
    psDescriptor->addPort(
      g_psPortDescriptors[i],
      g_psPortNames[i],
      g_psPortRangeHints[i].HintDescriptor,
      g_psPortRangeHints[i].LowerBound,
      g_psPortRangeHints[i].UpperBound);

  registerNewPluginDescriptor(psDescriptor);
}
