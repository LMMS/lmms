/* wshape_sine.cpp

   Computer Music Toolkit - a library of LADSPA plugins. Copyright (C)
   2000-2002 Richard W.E. Furse. The author may be contacted at
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

/*****************************************************************************/

#include "cmt.h"

/*****************************************************************************/

#define WSS_CONTROL 0
#define WSS_INPUT   1
#define WSS_OUTPUT  2

/** This plugin applies a gain to a mono signal. */
class SineWaveshaper : public CMT_PluginInstance {
public:

  SineWaveshaper(const LADSPA_Descriptor *,
		unsigned long)
    : CMT_PluginInstance(3) {
  }

  friend void runSineWaveshaper(LADSPA_Handle Instance,
			       unsigned long SampleCount);

};

/*****************************************************************************/

void 
runSineWaveshaper(LADSPA_Handle Instance,
		   unsigned long SampleCount) {
  
  SineWaveshaper * poProcessor = (SineWaveshaper *)Instance;

  LADSPA_Data * pfInput  = poProcessor->m_ppfPorts[WSS_INPUT];
  LADSPA_Data * pfOutput = poProcessor->m_ppfPorts[WSS_OUTPUT];
  LADSPA_Data   fLimit   = *(poProcessor->m_ppfPorts[WSS_CONTROL]);
  LADSPA_Data   fOneOverLimit = 1 / fLimit;

  for (unsigned long lSampleIndex = 0; 
       lSampleIndex < SampleCount; 
       lSampleIndex++) 
    *(pfOutput++) = fLimit * sin(*(pfInput++) * fOneOverLimit);
}

/*****************************************************************************/

void
initialise_wshape_sine() {
  
  CMT_Descriptor * psDescriptor;

  psDescriptor = new CMT_Descriptor
    (1097,
     "wshape_sine",
     LADSPA_PROPERTY_HARD_RT_CAPABLE,
     "Wave Shaper (Sine-Based)",
     CMT_MAKER("Richard W.E. Furse"),
     CMT_COPYRIGHT("2000-2002", "Richard W.E. Furse"),
     NULL,
     CMT_Instantiate<SineWaveshaper>,
     NULL,
     runSineWaveshaper,
     NULL,
     NULL,
     NULL);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Limiting Amplitude",
     (LADSPA_HINT_BOUNDED_BELOW 
      | LADSPA_HINT_LOGARITHMIC
      | LADSPA_HINT_DEFAULT_1),
     0,
     0);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output");
  registerNewPluginDescriptor(psDescriptor);
}

/*****************************************************************************/

/* EOF */
