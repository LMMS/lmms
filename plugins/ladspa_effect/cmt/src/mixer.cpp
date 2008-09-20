/* mixer.cpp

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

#include <stdlib.h>
#include <string.h>

/*****************************************************************************/

#include "cmt.h"

/*****************************************************************************/

/* The port numbers for the plugin: */

#define MIXER_INPUT1 0
#define MIXER_INPUT2 1
#define MIXER_OUTPUT 2

/** This plugin adds two signals together to produce a third. */
class SimpleMixer : public CMT_PluginInstance {
public:

  SimpleMixer(const LADSPA_Descriptor *,
	      unsigned long)
    : CMT_PluginInstance(3) {
  }

  friend void runSimpleMixer(LADSPA_Handle Instance,
			     unsigned long SampleCount);

};

/*****************************************************************************/

void 
runSimpleMixer(LADSPA_Handle Instance,
	       unsigned long SampleCount) {
  
  SimpleMixer * poMixer = (SimpleMixer *)Instance;

  LADSPA_Data * pfInput1 = poMixer->m_ppfPorts[MIXER_INPUT1];
  LADSPA_Data * pfInput2 = poMixer->m_ppfPorts[MIXER_INPUT2];
  LADSPA_Data * pfOutput = poMixer->m_ppfPorts[MIXER_OUTPUT];

  for (unsigned long lSampleIndex = 0; 
       lSampleIndex < SampleCount; 
       lSampleIndex++) 
    *(pfOutput++) = *(pfInput1++) + *(pfInput2++);
}

/*****************************************************************************/

void
initialise_mixer() {
  
  CMT_Descriptor * psDescriptor;

  psDescriptor = new CMT_Descriptor
    (1071,
     "mixer",
     LADSPA_PROPERTY_HARD_RT_CAPABLE,
     "Mixer (Stereo to Mono)",
     CMT_MAKER("Richard W.E. Furse"),
     CMT_COPYRIGHT("2000", "Richard W.E. Furse"),
     NULL,
     CMT_Instantiate<SimpleMixer>,
     NULL,
     runSimpleMixer,
     NULL,
     NULL,
     NULL);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input 1");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input 2");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output");
  registerNewPluginDescriptor(psDescriptor);
}

/*****************************************************************************/

/* EOF */
