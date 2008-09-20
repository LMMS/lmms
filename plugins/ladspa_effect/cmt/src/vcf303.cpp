/* vcf303.cpp

   VCF 303 - TB-303 Resonant Filter
   Copyright (c) 1998 Andy Sloane
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


#include <malloc.h>
#include <math.h>
#include "cmt.h"

#define PORT_IN        0
#define PORT_OUT       1
#define PORT_TRIGGER   2
#define PORT_CUTOFF    3
#define PORT_RESONANCE 4
#define PORT_ENV_MOD   5
#define PORT_DECAY     6

#define NUM_PORTS      7

#ifndef PI
#define PI 3.14159265358979
#endif

class Vcf303 : public CMT_PluginInstance {
  LADSPA_Data sample_rate;

  LADSPA_Data d1, d2, c0;
  int         last_trigger;
  int         envpos;

public:
  Vcf303(const LADSPA_Descriptor *,
         unsigned long s_rate)
    : CMT_PluginInstance(NUM_PORTS),
      sample_rate(s_rate),
      d1(0.0), d2(0.0), c0(0.0),
      last_trigger(0),
      envpos(0) {
  }

  ~Vcf303() {
  }

  static void
  activate(LADSPA_Handle Instance) {
    Vcf303 *vcf303 = (Vcf303*) Instance;

    vcf303->d1 = 0.0;
    vcf303->d2 = 0.0;
    vcf303->c0 = 0.0;
    vcf303->last_trigger = 0;
    vcf303->envpos = 0;
  }

  static inline void
  recalc_a_b_c (Vcf303      *filter,
                LADSPA_Data  e0,
                LADSPA_Data  c0,
                LADSPA_Data  resonance,
                LADSPA_Data *a,
                LADSPA_Data *b,
                LADSPA_Data *c) {
    LADSPA_Data whopping, k;
  
    whopping = e0 + c0;
    k = exp (-whopping / resonance);
  
    *a = 2.0 * cos (2.0 * whopping) * k;
    *b = -k * k;
    *c = (1.0 - *a - *b) * 0.2;
  }

  static void
  run(LADSPA_Handle Instance,
      unsigned long SampleCount) {
    Vcf303 *vcf303 = (Vcf303*) Instance;
    unsigned long i;
    LADSPA_Data e0, d, a, b, c;
    LADSPA_Data decay, resonance;
    LADSPA_Data **ports;
    int trigger;

    /* Update vars given envmod, cutoff, and reso. */
    ports = vcf303->m_ppfPorts;
    e0 = exp (5.613 - 0.8 * *ports[PORT_ENV_MOD] + 2.1553 *
              *ports[PORT_CUTOFF] - 0.7696 * (1.0 - *ports[PORT_RESONANCE]));
    e0 *= PI / vcf303->sample_rate;

    trigger = (*ports[PORT_TRIGGER] > 0.0);
    if (trigger == 1 && vcf303->last_trigger == 0)
      {
        LADSPA_Data e1;

        e1 = exp (6.109 + 1.5876 * *ports[PORT_ENV_MOD] + 2.1553 *
                  *ports[PORT_CUTOFF] - 1.2 * (1.0 - *ports[PORT_RESONANCE]));
        e1 *= PI / vcf303->sample_rate;
        vcf303->c0 = e1 - e0;
      }
    vcf303->last_trigger = trigger;
  
    /* Update decay given envdecay. */
    d = 0.2 + (2.3 * *ports[PORT_DECAY]);
    d *= vcf303->sample_rate;
    d = pow (0.1, 1.0 / d);
    decay = pow (d, 64);
  
    /* Update resonance. */
    resonance = exp (-1.20 + 3.455 * *ports[PORT_RESONANCE]);
  
    recalc_a_b_c (vcf303, e0, vcf303->c0, resonance, &a, &b, &c);

    for (i = 0; i < SampleCount; i++)
      {
        LADSPA_Data sample;

        sample = a * vcf303->d1 + b * vcf303->d2 + c * ports[PORT_IN][i];
        ports[PORT_OUT][i] = sample;

        vcf303->d2 = vcf303->d1;
        vcf303->d1 = sample;

        vcf303->envpos++;
        if (vcf303->envpos >= 64)
          {
            vcf303->envpos = 0;
            vcf303->c0 *= decay;
            recalc_a_b_c (vcf303, e0, vcf303->c0, resonance, &a, &b, &c);
          }
      }
  }
};


static LADSPA_PortDescriptor g_psPortDescriptors[] =
{
  LADSPA_PORT_AUDIO | LADSPA_PORT_INPUT,
  LADSPA_PORT_AUDIO | LADSPA_PORT_OUTPUT,
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT
};

static const char * const g_psPortNames[] =
{
  "In",
  "Out",
  "Trigger",
  "Cutoff",
  "Resonance",
  "Envelope Modulation",
  "Decay"
};

static LADSPA_PortRangeHint g_psPortRangeHints[] =
{
  /* Hints, Lower bound, Upper bound */
  { 0, 0.0, 0.0 },
  { 0, 0.0, 0.0 },
  { LADSPA_HINT_TOGGLED, 0.0, 0.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 1.0 }
};

void
initialise_vcf303() {
  CMT_Descriptor * psDescriptor;

  psDescriptor = new CMT_Descriptor
      (1224,
       "vcf303",
       LADSPA_PROPERTY_HARD_RT_CAPABLE,
       "VCF 303",
       CMT_MAKER("David A. Bartold"),
       CMT_COPYRIGHT("1998-2000", "Andy Sloane, David A. Bartold"),
       NULL,
       CMT_Instantiate<Vcf303>,
       Vcf303::activate,
       Vcf303::run,
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
