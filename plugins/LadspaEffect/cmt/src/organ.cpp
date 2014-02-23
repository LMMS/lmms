/* organ.cpp

   Organ - Additive Organ Synthesizer Voice
   Copyright (c) 1999, 2000 David A. Bartold

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

#include <math.h>
#include <stdlib.h>
#include "cmt.h"

#define PORT_OUT         0
#define PORT_GATE        1
#define PORT_VELOCITY    2
#define PORT_FREQ        3
#define PORT_BRASS       4
#define PORT_FLUTE       5
#define PORT_REED        6
#define PORT_HARM0       7
#define PORT_HARM1       8
#define PORT_HARM2       9
#define PORT_HARM3      10
#define PORT_HARM4      11
#define PORT_HARM5      12
#define PORT_ATTACK_LO  13
#define PORT_DECAY_LO   14
#define PORT_SUSTAIN_LO 15
#define PORT_RELEASE_LO 16
#define PORT_ATTACK_HI  17
#define PORT_DECAY_HI   18
#define PORT_SUSTAIN_HI 19
#define PORT_RELEASE_HI 20

#define NUM_PORTS       21

#define RESOLUTION   16384

#ifndef PI
#define PI 3.14159265358979
#endif

typedef struct Envelope
{
  int          envelope_decay;
  double       envelope;

  Envelope () : envelope_decay (0), envelope (0.0) {}
} Envelope;

static LADSPA_Data *g_sine_table;
static LADSPA_Data *g_triangle_table;
static LADSPA_Data *g_pulse_table;
static int          ref_count;

class Organ : CMT_PluginInstance
{
  LADSPA_Data sample_rate;

  Envelope env0;
  Envelope env1;

  unsigned long harm0_accum;
  unsigned long harm1_accum;
  unsigned long harm2_accum;
  unsigned long harm3_accum;
  unsigned long harm4_accum;
  unsigned long harm5_accum;

  public:

  Organ(const LADSPA_Descriptor * Descriptor,
        unsigned long             SampleRate)
    : CMT_PluginInstance(NUM_PORTS),
      sample_rate(SampleRate),
      harm0_accum(0), harm1_accum(0),
      harm2_accum(0), harm3_accum(0),
      harm4_accum(0), harm5_accum(0) {
    if (ref_count++ == 0)
      {
        int size = RESOLUTION;
        int half = size / 2;
        int slope = size / 10;
        int i;

        /* Initialize sine table. */
        g_sine_table = new LADSPA_Data[size];
        for (i = 0; i < size; i++)
          g_sine_table[i] = sin ((i * 2.0 * PI) / size) / 6.0;
  
        /* Initialize triangle table. */
        g_triangle_table = new LADSPA_Data[size];
        for (i = 0; i < half; i++)
          g_triangle_table[i] = (4.0 / size * i - 1.0) / 6.0;
        for (; i < size; i++)
          g_triangle_table[i] = (4.0 / size * (size - i) - 1.0) / 6.0;
  
        /* Initialize pulse table. */
        g_pulse_table = new LADSPA_Data[size];
        for (i = 0; i < slope; i++)
          g_pulse_table[i] = ((double) -i) / slope / 6.0;
        for (; i < half - slope; i++)
          g_pulse_table[i] = -1.0 / 6.0;
        for (; i < half + slope; i++)
          g_pulse_table[i] = ((double) i - half) / slope / 6.0;
        for (; i < size - slope; i++)
          g_pulse_table[i] = 1.0 / 6.0;
        for (; i < size; i++)
          g_pulse_table[i] = ((double) size - i) / slope / 6.0;
      }
  }

  ~Organ () {
    if (--ref_count == 0)
      {
        delete[] g_pulse_table;
        delete[] g_triangle_table;
        delete[] g_sine_table;
      }
  }

  static inline LADSPA_Data
  table_pos (LADSPA_Data   *table,
             unsigned long  freq_256,
             unsigned long *accum) {
    *accum += freq_256;
    while (*accum >= RESOLUTION * 256)
      *accum -= RESOLUTION * 256;
  
    return table[*accum >> 8];
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

  static inline LADSPA_Data
  multiplier(Organ       *organ,
             LADSPA_Data  value) {
    return 1.0 - pow (0.05, 1.0 / (organ->sample_rate * value));
  }

  static void
  activate(LADSPA_Handle Instance) {
    Organ *organ = (Organ*) Instance;

    organ->env0.envelope_decay = 0;
    organ->env0.envelope = 0.0;
    organ->env1.envelope_decay = 0;
    organ->env1.envelope = 0.0;
    organ->harm0_accum = 0;
    organ->harm1_accum = 0;
    organ->harm2_accum = 0;
    organ->harm3_accum = 0;
    organ->harm4_accum = 0;
    organ->harm5_accum = 0;
  }

  static void
  run(LADSPA_Handle Instance,
      unsigned long SampleCount) {
  Organ *organ = (Organ*) Instance;
  unsigned long i;
  LADSPA_Data **ports;
  LADSPA_Data *sine_table;
  LADSPA_Data *reed_table;
  LADSPA_Data *flute_table;
  unsigned long freq_256;
  unsigned long freq_256_harm0, freq_256_harm1;
  unsigned long freq_256_harm2, freq_256_harm3;
  unsigned long freq_256_harm4, freq_256_harm5;
  double attack0, decay0, release0;
  double attack1, decay1, release1;
  int gate;

  ports = organ->m_ppfPorts;

  gate = (*ports[PORT_GATE] > 0.0);
  if (gate == 0)
    {
      organ->env0.envelope_decay = 0;
      organ->env1.envelope_decay = 0;
    }

  sine_table = g_sine_table;
  reed_table = (*ports[PORT_REED] > 0.0) ? g_pulse_table : sine_table;
  flute_table = (*ports[PORT_FLUTE] > 0.0) ? g_triangle_table : sine_table;
  freq_256 = (int) (*ports[PORT_FREQ] *
                    ((double) RESOLUTION) /
                    organ->sample_rate * 256.0);

  freq_256_harm0 = freq_256 / 2;
  freq_256_harm1 = freq_256;

  attack0 = multiplier (organ, *ports[PORT_ATTACK_LO]);
  decay0 = multiplier (organ, *ports[PORT_DECAY_LO]);
  release0 = multiplier (organ, *ports[PORT_RELEASE_LO]);

  attack1 = multiplier (organ, *ports[PORT_ATTACK_HI]);
  decay1 = multiplier (organ, *ports[PORT_DECAY_HI]);
  release1 = multiplier (organ, *ports[PORT_RELEASE_HI]);

  if (*ports[PORT_BRASS] > 0.0)
    {
      freq_256_harm2 = freq_256 * 2;
      freq_256_harm3 = freq_256_harm2 * 2;
      freq_256_harm4 = freq_256_harm3 * 2;
      freq_256_harm5 = freq_256_harm4 * 2;

      for (i = 0; i < SampleCount; i++)
        ports[PORT_OUT][i] =
	  ((table_pos (sine_table, freq_256_harm0, &organ->harm0_accum) * *ports[PORT_HARM0]
	  + table_pos (sine_table, freq_256_harm1, &organ->harm1_accum) * *ports[PORT_HARM1]
	  + table_pos (reed_table, freq_256_harm2, &organ->harm2_accum) * *ports[PORT_HARM2])
          * envelope (&organ->env0, gate, attack0, decay0, *ports[PORT_SUSTAIN_LO], release0)
	  + (table_pos (sine_table, freq_256_harm3, &organ->harm3_accum) * *ports[PORT_HARM3]
	  + table_pos (flute_table, freq_256_harm4, &organ->harm4_accum) * *ports[PORT_HARM4]
	  + table_pos (flute_table, freq_256_harm5, &organ->harm5_accum) * *ports[PORT_HARM5])
          * envelope (&organ->env1, gate, attack1, decay1, *ports[PORT_SUSTAIN_HI], release1)) * *ports[PORT_VELOCITY];
    }
  else
    {
      freq_256_harm2 = freq_256 * 3 / 2;
      freq_256_harm3 = freq_256 * 2;
      freq_256_harm4 = freq_256 * 3;
      freq_256_harm5 = freq_256_harm3 * 2;

      for (i = 0; i < SampleCount; i++)
        ports[PORT_OUT][i] =
          ((table_pos (sine_table, freq_256_harm0, &organ->harm0_accum) * *ports[PORT_HARM0]
          + table_pos (sine_table, freq_256_harm1, &organ->harm1_accum) * *ports[PORT_HARM1]
          + table_pos (sine_table, freq_256_harm2, &organ->harm2_accum) * *ports[PORT_HARM2])
          * envelope (&organ->env0, gate, attack0, decay0, *ports[PORT_SUSTAIN_LO], release0)

          + (table_pos (reed_table, freq_256_harm3, &organ->harm3_accum) * *ports[PORT_HARM3]
          + table_pos (sine_table, freq_256_harm4, &organ->harm4_accum) * *ports[PORT_HARM4]
          + table_pos (flute_table, freq_256_harm5, &organ->harm5_accum) * *ports[PORT_HARM5])
          * envelope (&organ->env1, gate, attack1, decay1, *ports[PORT_SUSTAIN_HI], release1)) * *ports[PORT_VELOCITY];
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
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT
};

static const char * const g_psPortNames[] =
{
  "Out",
  "Gate",
  "Velocity",
  "Frequency (Hz)",
  "Brass", "Reed", "Flute",
  "16th Harmonic", "8th Harmonic",
  "5 1/3rd Harmonic", "4th Harmonic",
  "2 2/3rd Harmonic", "2nd Harmonic",
  "Attack Lo (Secs)", "Decay Lo (Secs)", "Sustain Lo (Level)", "Release Lo (Secs)",
  "Attack Hi (Secs)", "Decay Hi (Secs)", "Sustain Hi (Level)", "Release Hi (Secs)",
};

static LADSPA_PortRangeHint g_psPortRangeHints[] =
{
  /* Hints, Lower bound, Upper bound */
  { 0, 0.0, 0.0 },
  { LADSPA_HINT_TOGGLED, 0.0, 0.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 20000.0 },
  { LADSPA_HINT_TOGGLED, 0.0, 0.0 },
  { LADSPA_HINT_TOGGLED, 0.0, 0.0 },
  { LADSPA_HINT_TOGGLED, 0.0, 0.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.01, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.01, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.00, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.01, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.01, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.01, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.00, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.01, 1.0 }
};

void
initialise_organ() {
  CMT_Descriptor * psDescriptor;

  psDescriptor = new CMT_Descriptor
      (1222,
       "organ",
       LADSPA_PROPERTY_HARD_RT_CAPABLE,
       "Organ",
       CMT_MAKER("David A. Bartold"),
       CMT_COPYRIGHT("1999, 2000", "David A. Bartold"),
       NULL,
       CMT_Instantiate<Organ>,
       Organ::activate,
       Organ::run,
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
