/* canyondelay.cpp

   Canyon Delay - Deep Stereo Cross Delay
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

#define PORT_IN_LEFT      0
#define PORT_IN_RIGHT     1
#define PORT_OUT_LEFT     2
#define PORT_OUT_RIGHT    3
#define PORT_LTR_TIME     4
#define PORT_LTR_FEEDBACK 5
#define PORT_RTL_TIME     6
#define PORT_RTL_FEEDBACK 7
#define PORT_CUTOFF       8

#define NUM_PORTS         9

#ifndef PI
#define PI 3.14159265358979
#endif

class CanyonDelay : public CMT_PluginInstance {
  LADSPA_Data  sample_rate;

  long         datasize;
  LADSPA_Data *data_l;
  LADSPA_Data *data_r;
  LADSPA_Data  accum_l;
  LADSPA_Data  accum_r;

  int pos;

public:
  CanyonDelay(const LADSPA_Descriptor *,
              unsigned long s_rate)
    : CMT_PluginInstance(NUM_PORTS),
      sample_rate(s_rate),
      datasize(s_rate),
      data_l(new LADSPA_Data[datasize]),
      data_r(new LADSPA_Data[datasize]),
      accum_l(0.0),
      accum_r(0.0),
      pos(0) {
    for (long i = 0; i < datasize; i++)
      data_l[i] = data_r[i] = 0.0;
  }

  ~CanyonDelay() {
    delete[] data_l;
    delete[] data_r;
  }

  static void
  activate(LADSPA_Handle Instance) {
    CanyonDelay *delay = (CanyonDelay*) Instance;

    for (long i = 0; i < delay->datasize; i++)
      delay->data_l[i] = delay->data_r[i] = 0.0;

    delay->accum_l = 0.0;
    delay->accum_r = 0.0;
    delay->pos = 0;
  }

  static void
  run(LADSPA_Handle Instance,
      unsigned long SampleCount) {
    CanyonDelay *delay = (CanyonDelay*) Instance;
    LADSPA_Data **ports;
    unsigned long i;
    int l_to_r_offset, r_to_l_offset;
    LADSPA_Data ltr_invmag, rtl_invmag;
    LADSPA_Data filter_mag, filter_invmag;

    ports = delay->m_ppfPorts;

    l_to_r_offset = (int) (*ports[PORT_LTR_TIME] * delay->sample_rate);
    r_to_l_offset = (int) (*ports[PORT_RTL_TIME] * delay->sample_rate);

    ltr_invmag = 1.0 - fabs (*ports[PORT_LTR_FEEDBACK]);
    rtl_invmag = 1.0 - fabs (*ports[PORT_RTL_FEEDBACK]);

    filter_invmag = pow (0.5, (4.0 * PI * *ports[PORT_CUTOFF]) / delay->sample_rate);
    filter_mag = 1.0 - filter_invmag;

    for (i = 0; i < SampleCount; i++)
      {
        LADSPA_Data accum_l, accum_r;
        int pos1, pos2;

        accum_l = ports[PORT_IN_LEFT][i];
        accum_r = ports[PORT_IN_RIGHT][i];

        pos1 = delay->pos - r_to_l_offset + delay->datasize;
        while (pos1 >= delay->datasize)
          pos1 -= delay->datasize;
      
        pos2 = delay->pos - l_to_r_offset + delay->datasize;
        while (pos2 >= delay->datasize)
          pos2 -= delay->datasize;

        /* Mix channels with past samples. */
        accum_l = accum_l * rtl_invmag + delay->data_r[pos1] * *ports[PORT_RTL_FEEDBACK];
        accum_r = accum_r * ltr_invmag + delay->data_l[pos2] * *ports[PORT_LTR_FEEDBACK];

        /* Low-pass filter output. */
        accum_l = delay->accum_l * filter_invmag + accum_l * filter_mag;
        accum_r = delay->accum_r * filter_invmag + accum_r * filter_mag;

        /* Store IIR samples. */
        delay->accum_l = accum_l;
        delay->accum_r = accum_r;
      
        /* Store samples in arrays. */
        delay->data_l[delay->pos] = accum_l;
        delay->data_r[delay->pos] = accum_r;

        ports[PORT_OUT_LEFT][i] = accum_l;
        ports[PORT_OUT_RIGHT][i] = accum_r;

        delay->pos++;
        if (delay->pos >= delay->datasize)
          delay->pos -= delay->datasize;
      }
  }
};


static LADSPA_PortDescriptor g_psPortDescriptors[] =
{
  LADSPA_PORT_AUDIO | LADSPA_PORT_INPUT,
  LADSPA_PORT_AUDIO | LADSPA_PORT_INPUT,
  LADSPA_PORT_AUDIO | LADSPA_PORT_OUTPUT,
  LADSPA_PORT_AUDIO | LADSPA_PORT_OUTPUT,
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT
};

static const char * const g_psPortNames[] =
{
  "In (Left)",
  "In (Right)",
  "Out (Left)",
  "Out (Right)",
  "Left to Right Time (Seconds)",
  "Left to Right Feedback (Percent)",
  "Right to Left Time (Seconds)",
  "Right to Left Feedback (Percent)",
  "Low-Pass Cutoff (Hz)"
};

static LADSPA_PortRangeHint g_psPortRangeHints[] =
{
  /* Hints, Lower bound, Upper bound */
  { 0, 0.0, 0.0 },
  { 0, 0.0, 0.0 },
  { 0, 0.0, 0.0 },
  { 0, 0.0, 0.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.01, 0.99 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, -1.0, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.01, 0.99 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, -1.0, 1.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 1.0, 5000.0 }
};

void
initialise_canyondelay() {
  CMT_Descriptor * psDescriptor;

  psDescriptor = new CMT_Descriptor
      (1225,
       "canyon_delay",
       LADSPA_PROPERTY_HARD_RT_CAPABLE,
       "Canyon Delay",
       CMT_MAKER("David A. Bartold"),
       CMT_COPYRIGHT("1999, 2000", "David A. Bartold"),
       NULL,
       CMT_Instantiate<CanyonDelay>,
       CanyonDelay::activate,
       CanyonDelay::run,
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
