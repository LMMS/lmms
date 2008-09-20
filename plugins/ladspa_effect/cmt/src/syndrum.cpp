/* syndrum.cpp

   SynDrum - Drum Synthesizer
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

#define PORT_OUT       0
#define PORT_TRIGGER   1
#define PORT_VELOCITY  2
#define PORT_FREQ      3
#define PORT_RESONANCE 4
#define PORT_RATIO     5

#define NUM_PORTS      6

#ifndef PI
#define PI 3.14159265358979
#endif

class SynDrum : public CMT_PluginInstance {
  LADSPA_Data sample_rate;

  LADSPA_Data spring_vel;
  LADSPA_Data spring_pos;
  LADSPA_Data env;

  int         last_trigger;

public:
  SynDrum(const LADSPA_Descriptor *,
          unsigned long s_rate)
    : CMT_PluginInstance(NUM_PORTS),
      sample_rate(s_rate),
      spring_vel(0.0F),
      spring_pos(0.0F),
      env(0.0F) {
  }

  ~SynDrum() {
  }

  static void
  activate(LADSPA_Handle Instance) {
    SynDrum *syndrum = (SynDrum*) Instance;
    syndrum->spring_vel = 0.0F;
    syndrum->spring_pos = 0.0F;
    syndrum->env = 0.0F;
    syndrum->last_trigger = 0;
  }

  static void
  run(LADSPA_Handle Instance,
      unsigned long SampleCount) {
    SynDrum *syndrum = (SynDrum*) Instance;
    unsigned long i;
    int trigger;
    LADSPA_Data freq_shift;
    LADSPA_Data factor;
    LADSPA_Data res;
  
    trigger = *syndrum->m_ppfPorts[PORT_TRIGGER] > 0.0;
    if (trigger == 1 && syndrum->last_trigger == 0)
      {
        syndrum->spring_vel = *syndrum->m_ppfPorts[PORT_VELOCITY];
        syndrum->env = *syndrum->m_ppfPorts[PORT_VELOCITY];
      }
    syndrum->last_trigger = trigger;

    factor = 2.0 * PI / syndrum->sample_rate;
    freq_shift = *syndrum->m_ppfPorts[PORT_FREQ] *
                 *syndrum->m_ppfPorts[PORT_RATIO];
    res = pow (0.05, 1.0 / (syndrum->sample_rate * *syndrum->m_ppfPorts[PORT_RESONANCE]));

    for (i = 0; i < SampleCount; i++)
      {
        LADSPA_Data cur_freq;
      
        cur_freq = *syndrum->m_ppfPorts[PORT_FREQ] +
                   (syndrum->env * freq_shift);
        cur_freq *= factor;
        syndrum->spring_vel -= syndrum->spring_pos * cur_freq;
        syndrum->spring_pos += syndrum->spring_vel * cur_freq;
        syndrum->spring_vel *= res;
        syndrum->env *= res;
      
        syndrum->m_ppfPorts[PORT_OUT][i] = syndrum->spring_pos;
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
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT
};

static const char * const g_psPortNames[] =
{
  "Out",
  "Trigger",
  "Velocity",
  "Frequency (Hz)",
  "Resonance",
  "Frequency Ratio"
};

static LADSPA_PortRangeHint g_psPortRangeHints[] =
{
  /* Hints, Lower bound, Upper bound */
  { 0, 0.0, 0.0 },
  { LADSPA_HINT_TOGGLED, 0.0, 0.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 10.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 20000.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.001, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 10.0 }
};

void
initialise_syndrum() {
  CMT_Descriptor * psDescriptor;

  psDescriptor = new CMT_Descriptor
      (1223,
       "syndrum",
       LADSPA_PROPERTY_HARD_RT_CAPABLE,
       "Syn Drum",
       CMT_MAKER("David A. Bartold"),
       CMT_COPYRIGHT("1999, 2000", "David A. Bartold"),
       NULL,
       CMT_Instantiate<SynDrum>,
       SynDrum::activate,
       SynDrum::run,
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
