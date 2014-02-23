/* amp.cpp

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

#include <stdlib.h>

/*****************************************************************************/

#include "cmt.h"

/*****************************************************************************/

#define AMP_CONTROL 0
#define AMP_INPUT1  1
#define AMP_OUTPUT1 2

/** This plugin applies a gain to a mono signal. */
class MonoAmplifier : public CMT_PluginInstance {
public:

  MonoAmplifier(const LADSPA_Descriptor *,
		unsigned long)
    : CMT_PluginInstance(3) {
  }

  friend void runMonoAmplifier(LADSPA_Handle Instance,
			       unsigned long SampleCount);

};

/*****************************************************************************/

/* Ports as above, plus... */
#define AMP_INPUT2  3
#define AMP_OUTPUT2 4

/** This plugin applies a gain to a stereo signal. */
class StereoAmplifier : public CMT_PluginInstance {
public:

  StereoAmplifier(const LADSPA_Descriptor *,
		    unsigned long)
    : CMT_PluginInstance(5) {
  }

  friend void runStereoAmplifier(LADSPA_Handle Instance,
				   unsigned long SampleCount);
};

/*****************************************************************************/

void 
runMonoAmplifier(LADSPA_Handle Instance,
		   unsigned long SampleCount) {
  
  MonoAmplifier * poAmplifier = (MonoAmplifier *)Instance;

  LADSPA_Data * pfInput = poAmplifier->m_ppfPorts[AMP_INPUT1];
  LADSPA_Data * pfOutput = poAmplifier->m_ppfPorts[AMP_OUTPUT1];
  LADSPA_Data fGain = *(poAmplifier->m_ppfPorts[AMP_CONTROL]);

  for (unsigned long lSampleIndex = 0; 
       lSampleIndex < SampleCount; 
       lSampleIndex++) 
    *(pfOutput++) = *(pfInput++) * fGain;
}

/*****************************************************************************/

void 
runStereoAmplifier(LADSPA_Handle Instance,
		     unsigned long SampleCount) {

  unsigned long lSampleIndex;

  StereoAmplifier * poAmplifier = (StereoAmplifier *)Instance;

  LADSPA_Data fGain = *(poAmplifier->m_ppfPorts[AMP_CONTROL]);

  LADSPA_Data * pfInput = poAmplifier->m_ppfPorts[AMP_INPUT1];
  LADSPA_Data * pfOutput = poAmplifier->m_ppfPorts[AMP_OUTPUT1];
  for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) 
    *(pfOutput++) = *(pfInput++) * fGain;

  pfInput = poAmplifier->m_ppfPorts[AMP_INPUT2];
  pfOutput = poAmplifier->m_ppfPorts[AMP_OUTPUT2];
  for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) 
    *(pfOutput++) = *(pfInput++) * fGain;
}

/*****************************************************************************/

void
initialise_amp() {
  
  CMT_Descriptor * psDescriptor;

  psDescriptor = new CMT_Descriptor
    (1067,
     "amp_mono",
     LADSPA_PROPERTY_HARD_RT_CAPABLE,
     "Amplifier (Mono)",
     CMT_MAKER("Richard W.E. Furse"),
     CMT_COPYRIGHT("2000-2002", "Richard W.E. Furse"),
     NULL,
     CMT_Instantiate<MonoAmplifier>,
     NULL,
     runMonoAmplifier,
     NULL,
     NULL,
     NULL);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Gain",
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
  
  psDescriptor = new CMT_Descriptor
    (1068,
     "amp_stereo",
     LADSPA_PROPERTY_HARD_RT_CAPABLE,
     "Amplifier (Stereo)",
     CMT_MAKER("Richard W.E. Furse"),
     CMT_COPYRIGHT("2000-2002", "Richard W.E. Furse"),
     NULL,
     CMT_Instantiate<StereoAmplifier>,
     NULL,
     runStereoAmplifier,
     NULL,
     NULL,
     NULL);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Gain",
     (LADSPA_HINT_BOUNDED_BELOW 
      | LADSPA_HINT_LOGARITHMIC
      | LADSPA_HINT_DEFAULT_1),
     0,
     0);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (Left)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (Left)");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input (Right)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output (Right)");
  registerNewPluginDescriptor(psDescriptor);
}

/*****************************************************************************/

/* EOF */
