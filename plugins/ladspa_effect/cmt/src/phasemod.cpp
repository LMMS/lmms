/* phasemod.cpp

   Phase Modulated Voice - Phase Modulation synthesizer voice
   Copyright (c) 2001 David A. Bartold

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
#define PORT_DCO_MODULATION 4
#define PORT_DCO_OCTAVE     5
#define PORT_DCO_WAVEFORM   6
#define PORT_DCO_ATTACK     7
#define PORT_DCO_DECAY      8
#define PORT_DCO_SUSTAIN    9
#define PORT_DCO_RELEASE    10

#define DCO_MULTIPLIER      7

#define NUM_PORTS           46

#ifndef PI
#define PI 3.14159265358979F
#endif

typedef struct Envelope
{
  int          envelope_decay;
  LADSPA_Data  envelope;

  Envelope () : envelope_decay (0), envelope (0.0) {}
} Envelope;

class PhaseMod : public CMT_PluginInstance
{
  LADSPA_Data sample_rate;

  int         trigger;

  Envelope    dco_env[6];
  LADSPA_Data dco_accum[6];

public:
  PhaseMod(const LADSPA_Descriptor * Descriptor,
           unsigned long             SampleRate)
    : CMT_PluginInstance(NUM_PORTS),
      sample_rate (SampleRate),
      trigger (0) {
    int i;

    for (i = 0; i < 6; i++)
      dco_accum[i] = 0.0;
  }

  ~PhaseMod () {
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
    PhaseMod *phasemod = (PhaseMod*) Instance;
    int i;

    phasemod->trigger = 0;

    for (i = 0; i < 6; i++)
      {
        phasemod->dco_env[i].envelope_decay = 0;
        phasemod->dco_env[i].envelope = 0.0;
        phasemod->dco_accum[i] = 0.0;
      }
  }

  static inline LADSPA_Data
  osc(int          waveform,
      LADSPA_Data  inc,
      LADSPA_Data  phasemod,
      LADSPA_Data *accum) {
    LADSPA_Data pos;

    *accum += inc;
    while (*accum >= 1.0F)
      *accum -= 1.0F;

    pos = *accum + phasemod;
    while (pos < 0.0F) pos += 1.0F;
    while (pos > 1.0F) pos -= 1.0F;

    /* 0 = Sine wave */
    if (waveform == 0)
      return sin (pos * 2.0 * PI);

    /* 1 = Triangle wave */
    else if (waveform == 1)
      return tri (pos);

    /* 2 = Square wave */
    else if (waveform == 2)
      return (pos > 0.5) ? 1.0F : -1.0F;

    /* 3 = Sawtooth wave */
    else if (waveform == 3)
      return pos * 2.0F - 1.0F;

    /* 4 = Fullwave Rectified Sine wave */
    else if (waveform == 4)
      return fabs (pos * PI);

    /* 5 = Static */
    else
      return (rand () & 1) ? -1.0F : 1.0F;
  }

  static LADSPA_Data
  calc_inc(LADSPA_Data oct,
           LADSPA_Data freq,
           LADSPA_Data sample_rate) {
    return pow (2.0, oct) * freq / sample_rate;
  }

  static inline LADSPA_Data
  multiplier(PhaseMod *phasemod,
             LADSPA_Data value) {
    return 1.0 - pow (0.05, 1.0 / (phasemod->sample_rate * value));
  }

  static void
  run(LADSPA_Handle Instance,
      unsigned long SampleCount) {
    PhaseMod *phasemod = (PhaseMod*) Instance;

    unsigned long   i, j;
    int             gate;
    int             waveform[6];
    int             store[6];
    LADSPA_Data     inc[6];
    LADSPA_Data     attack[6];
    LADSPA_Data     decay[6];
    LADSPA_Data     release[6];
    LADSPA_Data   **ports;
    LADSPA_Data     vol;

    ports = phasemod->m_ppfPorts;
    gate = (*ports[PORT_GATE] > 0.0);

    if (gate == 1 && phasemod->trigger == 0)
      for (i = 0; i < 6; i++)
        phasemod->dco_env[i].envelope_decay = 0;

    phasemod->trigger = gate;

    for (i = 0; i < 6; i++)
      {
        int offset = DCO_MULTIPLIER * i;

        waveform[i] = (int) *ports[PORT_DCO_WAVEFORM + offset];
        inc[i] = calc_inc (*ports[PORT_DCO_OCTAVE + offset],
                           *ports[PORT_FREQ],
                           phasemod->sample_rate);
        attack[i] = multiplier (phasemod, *ports[PORT_DCO_ATTACK + offset]);
        decay[i] = multiplier (phasemod, *ports[PORT_DCO_DECAY + offset]);
        release[i] = multiplier (phasemod, *ports[PORT_DCO_RELEASE + offset]);
      }

    j = 1;
    for (i = 0; i < 5; i++)
      if (*ports[PORT_DCO_MODULATION + (i + 1) * DCO_MULTIPLIER] < 0.0001)
        store[i] = 1, j++;
      else
        store[i] = 0;
    store[5] = 1;
    vol = 1.0 / j;

    for (i = 0; i < SampleCount; i++)
      {
        LADSPA_Data sample;
        LADSPA_Data prev;

        sample = 0.0;
        prev = 1.0;
        for (j = 0; j < 6; j++)
          {
            int offset = DCO_MULTIPLIER * j;

            prev =
              envelope (&phasemod->dco_env[j],
                        gate, attack[j], decay[j],
                        *ports[PORT_DCO_SUSTAIN + offset], release[j]) *
              osc (waveform[j], inc[j],
                   prev * *ports[PORT_DCO_MODULATION + offset],
                   &phasemod->dco_accum[j]) *
              *ports[PORT_VELOCITY];

            if (store[j])
              sample += prev;
          }
        ports[PORT_OUT][i] = sample * vol;
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

  "DCO1 Modulation",
  "DCO1 Octave",
  "DCO1 Waveform",
  "DCO1 Attack",
  "DCO1 Decay",
  "DCO1 Sustain",
  "DCO1 Release",

  "DCO2 Modulation",
  "DCO2 Octave",
  "DCO2 Waveform",
  "DCO2 Attack",
  "DCO2 Decay",
  "DCO2 Sustain",
  "DCO2 Release",

  "DCO3 Modulation",
  "DCO3 Octave",
  "DCO3 Waveform",
  "DCO3 Attack",
  "DCO3 Decay",
  "DCO3 Sustain",
  "DCO3 Release",

  "DCO4 Modulation",
  "DCO4 Octave",
  "DCO4 Waveform",
  "DCO4 Attack",
  "DCO4 Decay",
  "DCO4 Sustain",
  "DCO4 Release",

  "DCO5 Modulation",
  "DCO5 Octave",
  "DCO5 Waveform",
  "DCO5 Attack",
  "DCO5 Decay",
  "DCO5 Sustain",
  "DCO5 Release",

  "DCO6 Modulation",
  "DCO6 Octave",
  "DCO6 Waveform",
  "DCO6 Attack",
  "DCO6 Decay",
  "DCO6 Sustain",
  "DCO6 Release"
};

static LADSPA_PortRangeHint g_psPortRangeHints[] =
{
  /* Hints, Lower bound, Upper bound */
  { 0, 0.0, 0.0 },

  { LADSPA_HINT_TOGGLED, 0.0, 0.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 20000.0 },

  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, -2.0, 2.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW |
    LADSPA_HINT_INTEGER, -0.1, 5.1 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.01, 8.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.01, 8.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.01, 8.0 },

  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, -2.0, 2.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW |
    LADSPA_HINT_INTEGER, -0.1, 5.1 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.01, 8.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.01, 8.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.01, 8.0 },

  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, -2.0, 2.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW |
    LADSPA_HINT_INTEGER, -0.1, 5.1 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.01, 8.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.01, 8.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.01, 8.0 },

  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, -2.0, 2.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW |
    LADSPA_HINT_INTEGER, -0.1, 5.1 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.01, 8.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.01, 8.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.01, 8.0 },

  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, -2.0, 2.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW |
    LADSPA_HINT_INTEGER, -0.1, 5.1 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.01, 8.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.01, 8.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.01, 8.0 },

  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, -2.0, 2.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW |
    LADSPA_HINT_INTEGER, -0.1, 5.1 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.01, 8.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.01, 8.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.01, 8.0 }
};

void
initialise_phasemod() {
  CMT_Descriptor * psDescriptor;

  psDescriptor = new CMT_Descriptor
      (1226,
       "phasemod",
       LADSPA_PROPERTY_HARD_RT_CAPABLE,
       "Phase Modulated Voice",
       CMT_MAKER("David A. Bartold"),
       CMT_COPYRIGHT("2001", "David A. Bartold"),
       NULL,
       CMT_Instantiate<PhaseMod>,
       PhaseMod::activate,
       PhaseMod::run,
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
