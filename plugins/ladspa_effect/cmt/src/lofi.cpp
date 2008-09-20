/* lofi.cpp

   Lo Fi - Simulate low quality audio equipment
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

#define PORT_IN_LEFT     0
#define PORT_IN_RIGHT    1
#define PORT_OUT_LEFT    2
#define PORT_OUT_RIGHT   3
#define PORT_CRACKLING   4
#define PORT_OVERLOADING 5
#define PORT_BANDWIDTH   6

#define NUM_PORTS        7

#ifndef PI
#define PI 3.14159265358979
#endif

#ifndef MIN
#define MIN(x,y) ((x)<(y)?(x):(y))
#endif

#ifndef MAX
#define MAX(x,y) ((x)>(y)?(x):(y))
#endif

class Pop
{
public:
  float  x;
  float  dx;
  float  amp;
  float  pwr;
  Pop   *next;

  Pop (float dx, float amp, float pwr, Pop *next);
  ~Pop ();
};

Pop::Pop (float  _dx,
          float  _amp,
          float  _pwr,
          Pop   *_next)
  : x (0.0), dx (_dx), amp (_amp), pwr (_pwr), next (_next)
{
}

Pop::~Pop ()
{
  delete next;
}


class Record
{
public:
  int  rate;
  int  amount; /* 0 -> 100% */
  Pop *pops;

  LADSPA_Data process (LADSPA_Data sample);
  void        setAmount (int _amount);

  Record (int sample_rate);
  ~Record ();
};

Record::Record (int sample_rate)
  : rate (sample_rate),
    amount (0),
    pops (NULL)
{
}

Record::~Record ()
{
  delete pops;
}

static Pop *
record_pop_new (Record *record,
                Pop    *next)
{
  return new Pop ((rand () % 1500 + 500.0) / record->rate,
                  (rand () % 50) / 10000.0,
                  1.0,
                  next);
}

static Pop *
record_pop_loud_new (Record *record,
                     Pop    *next)
{
  return new Pop ((rand () % 500 + 2500.0) / record->rate,
                  (rand () % 100) / 400.0 + 0.5,
                  (rand () % 50) / 20.0,
                  next);
}

LADSPA_Data
Record::process (LADSPA_Data sample)
{
  Pop *pop;
  Pop **pop_prev;

  /* Add some crackle */
  if (rand () % rate < rate * amount / 4000)
    pops = record_pop_new (this, pops);

  /* Add some loud pops */
  if (rand () % (rate * 10) < rate * amount / 400000)
    pops = record_pop_loud_new (this, pops);

  /* Compute pops */
  pop_prev = &pops;
  pop = *pop_prev;
  while (pop != NULL)
    {
      if (pop->x >= 0.5)
        sample += (pow ((1.0 - pop->x) * 2.0, pop->pwr) - 0.5) * pop->amp;
      else
        sample += (pow (pop->x * 2.0, pop->pwr) - 0.5) * pop->amp;

      pop->x += pop->dx;
      if (pop->x > 1.0)
        {
          *pop_prev = pop->next;
          pop->next = NULL;
          delete pop;
        }
      else
        pop_prev = &pop->next;

      pop = *pop_prev;
    }

  return sample;
}

void
Record::setAmount (int _amount)
{
  amount = _amount;
}


class Compressor
{
public:
  int    rate;
  double amp;
  double up;
  double down; 
  float  vol;
  float  clamp_hi;
  float  clamp_lo;

  LADSPA_Data process (LADSPA_Data sample);
  void setClamp (float clamp);

  Compressor (int sample_rate, float clamp);
};

Compressor::Compressor (int sample_rate, float clamp)
  : rate (sample_rate), amp (0.5),
    up (1.0 / pow (0.5, 20.0 / sample_rate)),
    down (pow (0.5, 50.0 / sample_rate)),
    vol (0.5), clamp_hi (clamp), clamp_lo (1.0 / clamp)
{
}

LADSPA_Data
Compressor::process (LADSPA_Data sample)
{
  sample *= amp;

  if (fabs (sample) > vol)
    {
      amp *= down;
      if (amp < clamp_lo)
        amp = clamp_lo;
    }
  else
    {
      amp *= up;
      if (amp > clamp_hi)
        amp = clamp_hi;
    }

  return sample;
}

void
Compressor::setClamp (float clamp)
{
  clamp_hi = clamp;
  clamp_lo = 1.0 / clamp;
}


static inline LADSPA_Data
distort (LADSPA_Data in)
{
  if (in > 0.0F)
    return (in * 1.0F) / (in + 1.0F) * 2.0F;
  else
    return -(-in * 1.0F) / (-in + 1.0F) * 2.0F;
}


class BandwidthLimit
{
public:
  int   rate;
  float x;
  float dx;

  void setFreq (float freq);

  LADSPA_Data process (LADSPA_Data sample);
  BandwidthLimit (int _rate, float _freq);
};

BandwidthLimit::BandwidthLimit (int _rate, float _freq)
  : rate (_rate), x (0.0), dx (_freq / _rate)
{
}

LADSPA_Data
BandwidthLimit::process (LADSPA_Data sample)
{
  if (sample >= x)
    sample = MIN (x + dx, sample);
  else
    sample = MAX (x - dx, sample);
  x = sample;

  return sample;
}

void
BandwidthLimit::setFreq (float freq)
{
  dx = freq / rate;
}


class LoFi : public CMT_PluginInstance {
  Record         *record;
  Compressor     *compressor;
  BandwidthLimit *bandwidth_l;
  BandwidthLimit *bandwidth_r;

  int         last_trigger;

public:
  LoFi(const LADSPA_Descriptor *,
       unsigned long s_rate)
    : CMT_PluginInstance (NUM_PORTS),
      record (new Record (s_rate * 2)),
      compressor (new Compressor (s_rate * 2, 1.6)),
      bandwidth_l (new BandwidthLimit (s_rate, 8000.0)),
      bandwidth_r (new BandwidthLimit (s_rate, 8000.0)) {
  }

  ~LoFi() {
    delete bandwidth_l;
    delete bandwidth_r;
    delete compressor;
    delete record;
  }

  static void
  activate (LADSPA_Handle Instance) {
    LoFi *lofi = (LoFi*) Instance;

    lofi->bandwidth_l->setFreq (8000);
    lofi->bandwidth_r->setFreq (8000);
    lofi->compressor->setClamp (1.6);
    lofi->record->setAmount (0);
  }

  static void
  run(LADSPA_Handle Instance,
      unsigned long SampleCount) {
    LoFi *lofi = (LoFi*) Instance;
    unsigned long   i;
    LADSPA_Data   **ports = lofi->m_ppfPorts;
    LADSPA_Data     clamp;

    lofi->bandwidth_l->setFreq (ports[PORT_BANDWIDTH][0]);
    lofi->bandwidth_r->setFreq (ports[PORT_BANDWIDTH][0]);

    if (ports[PORT_OVERLOADING][0] > 99.0)
      clamp = 100.0;
    else
      clamp = 100.0 / (100.0 - ports[PORT_OVERLOADING][0]);

    lofi->compressor->setClamp (clamp);

    lofi->record->setAmount ((int) ports[PORT_CRACKLING][0]);

    for (i = 0; i < SampleCount; i++)
      {
        LADSPA_Data sample_l, sample_r;

        sample_l = ports[PORT_IN_LEFT][i];
        sample_r = ports[PORT_IN_RIGHT][i];

        sample_l = lofi->compressor->process (sample_l);
        sample_r = lofi->compressor->process (sample_r);
        sample_l = lofi->bandwidth_l->process (sample_l);
        sample_r = lofi->bandwidth_r->process (sample_r);
        sample_l = distort (sample_l);
        sample_r = distort (sample_r);
        sample_l = lofi->record->process (sample_l);
        sample_r = lofi->record->process (sample_r);

        ports[PORT_OUT_LEFT][i] = sample_l;
        ports[PORT_OUT_RIGHT][i] = sample_r;
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
  LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT
};

static const char * const g_psPortNames[] =
{
  "In (Left)",
  "In (Right)",

  "Out (Left)",
  "Out (Right)",

  "Crackling (%)",
  "Powersupply Overloading (%)",
  "Opamp Bandwidth Limiting (Hz)"
};

static LADSPA_PortRangeHint g_psPortRangeHints[] =
{
  /* Hints, Lower bound, Upper bound */
  { 0, 0.0, 0.0 },
  { 0, 0.0, 0.0 },
  { 0, 0.0, 0.0 },
  { 0, 0.0, 0.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW |
    LADSPA_HINT_INTEGER, -0.1, 100.1 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 0.0, 100.0 },
  { LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW, 1.0, 10000.0 }
};

void
initialise_lofi() {
  CMT_Descriptor * psDescriptor;

  psDescriptor = new CMT_Descriptor
      (1227,
       "lofi",
       0 /* Sorry, this module is not RT capable, run() calls malloc() */,
       "Lo Fi",
       CMT_MAKER("David A. Bartold"),
       CMT_COPYRIGHT("2001", "David A. Bartold"),
       NULL,
       CMT_Instantiate<LoFi>,
       LoFi::activate,
       LoFi::run,
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
