/* am.cpp

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

#define AM_INPUT1 0
#define AM_INPUT2 1
#define AM_OUTPUT 2

/** This plugin multiplies two signals together to produce a third. */
class AmplitudeModulator : public CMT_PluginInstance {
public:

  AmplitudeModulator(const LADSPA_Descriptor *,
		     unsigned long)
    : CMT_PluginInstance(3) {
  }

  friend void runAmplitudeModulator(LADSPA_Handle Instance,
			     unsigned long SAmplitudeModulatorpleCount);

};

/*****************************************************************************/

void 
runAmplitudeModulator(LADSPA_Handle Instance,
		      unsigned long SAmplitudeModulatorpleCount) {
  
  AmplitudeModulator * poAmplitudeModulator = (AmplitudeModulator *)Instance;

  LADSPA_Data * pfInput1 = poAmplitudeModulator->m_ppfPorts[AM_INPUT1];
  LADSPA_Data * pfInput2 = poAmplitudeModulator->m_ppfPorts[AM_INPUT2];
  LADSPA_Data * pfOutput = poAmplitudeModulator->m_ppfPorts[AM_OUTPUT];

  for (unsigned long lSAmplitudeModulatorpleIndex = 0; 
       lSAmplitudeModulatorpleIndex < SAmplitudeModulatorpleCount; 
       lSAmplitudeModulatorpleIndex++) 
    *(pfOutput++) = *(pfInput1++) * *(pfInput2++);
}

/*****************************************************************************/

void
initialise_am() {
  
  CMT_Descriptor * psDescriptor = new CMT_Descriptor
    (1070,
     "am",
     LADSPA_PROPERTY_HARD_RT_CAPABLE,
     "Amplitude Modulator",
     CMT_MAKER("Richard W.E. Furse"),
     CMT_COPYRIGHT("2000", "Richard W.E. Furse"),
     NULL,
     CMT_Instantiate<AmplitudeModulator>,
     NULL,
     runAmplitudeModulator,
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
