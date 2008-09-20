/* noise.cpp

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

/* The port numbers for the plugin: */

#define NOISE_AMPLITUDE 0
#define NOISE_OUTPUT    1

/** Plugin that provides white noise output. This is provided by
    calling rand() repeatedly. */
class WhiteNoise : public CMT_PluginInstance {
private:

  LADSPA_Data m_fRunAddingGain;

public:

  WhiteNoise(const LADSPA_Descriptor *,
	      unsigned long)
    : CMT_PluginInstance(2) {
  }

  friend void runWhiteNoise(LADSPA_Handle Instance,
			    unsigned long SampleCount);
  friend void runWhiteNoiseAdding(LADSPA_Handle Instance,
				  unsigned long SampleCount);
  friend void setWhiteNoiseRunAddingGain(LADSPA_Handle Instance,
					 LADSPA_Data   Gain);

};

/*****************************************************************************/

void 
runWhiteNoise(LADSPA_Handle Instance,
	      unsigned long SampleCount) {
  
  WhiteNoise * poNoise = (WhiteNoise *)Instance;

  LADSPA_Data fAmplitude = *(poNoise->m_ppfPorts[NOISE_AMPLITUDE]);
  LADSPA_Data fScalar = fAmplitude * LADSPA_Data(2.0 / RAND_MAX);

  LADSPA_Data * pfOutput = poNoise->m_ppfPorts[NOISE_OUTPUT];

  for (unsigned long lSampleIndex = 0; 
       lSampleIndex < SampleCount; 
       lSampleIndex++) 
    *(pfOutput++) = rand() * fScalar - fAmplitude;
}

void 
runWhiteNoiseAdding(LADSPA_Handle Instance,
		    unsigned long SampleCount) {
  
  WhiteNoise * poNoise = (WhiteNoise *)Instance;

  LADSPA_Data fAmplitude
    = *(poNoise->m_ppfPorts[NOISE_AMPLITUDE]);
  LADSPA_Data fScalar
    = poNoise->m_fRunAddingGain * fAmplitude * LADSPA_Data(2.0 / RAND_MAX);

  LADSPA_Data * pfOutput = poNoise->m_ppfPorts[NOISE_OUTPUT];

  for (unsigned long lSampleIndex = 0; 
       lSampleIndex < SampleCount; 
       lSampleIndex++) 
    *(pfOutput++) += rand() * fScalar - fAmplitude;

}

void 
setWhiteNoiseRunAddingGain(LADSPA_Handle Instance,
			   LADSPA_Data   Gain) {
}

/*****************************************************************************/

void
initialise_noise() {
  
  CMT_Descriptor * psDescriptor;

  psDescriptor = new CMT_Descriptor
    (1069,
     "noise_source_white",
     LADSPA_PROPERTY_HARD_RT_CAPABLE,
     "Noise Source (White)",
     CMT_MAKER("Richard W.E. Furse"),
     CMT_COPYRIGHT("2000-2002", "Richard W.E. Furse"),
     NULL,
     CMT_Instantiate<WhiteNoise>,
     NULL,
     runWhiteNoise,
     runWhiteNoiseAdding,
     setWhiteNoiseRunAddingGain,
     NULL);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Amplitude",
     (LADSPA_HINT_BOUNDED_BELOW 
      | LADSPA_HINT_LOGARITHMIC
      | LADSPA_HINT_DEFAULT_1),
     0,
     0);
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output");
  registerNewPluginDescriptor(psDescriptor);
}

/*****************************************************************************/

/* EOF */
