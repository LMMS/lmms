/* filter.cpp

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

#define SF_CUTOFF  0
#define SF_INPUT   1
#define SF_OUTPUT  2

/** Instance data for the OnePoll filter (one-poll, low or high
    pass). We can get away with using this structure for both low- and
    high-pass filters because the data stored is the same. Note that
    the actual run() calls differ however. */
class OnePollFilter : public CMT_PluginInstance {
private:

  LADSPA_Data m_fSampleRate;
  LADSPA_Data m_fTwoPiOverSampleRate;

  LADSPA_Data m_fLastOutput;
  LADSPA_Data m_fLastCutoff;
  LADSPA_Data m_fAmountOfCurrent;
  LADSPA_Data m_fAmountOfLast;

public:

  OnePollFilter(const LADSPA_Descriptor *,
	       unsigned long lSampleRate) 
    : CMT_PluginInstance(3),
      m_fSampleRate(LADSPA_Data(lSampleRate)),
      m_fTwoPiOverSampleRate(LADSPA_Data((2 * M_PI) / lSampleRate)),
      m_fLastCutoff(0),
      m_fAmountOfCurrent(0),
      m_fAmountOfLast(0) {
  }

  friend void activateOnePollFilter(LADSPA_Handle Instance);
  friend void runOnePollLowPassFilter(LADSPA_Handle Instance,
				     unsigned long SampleCount);
  friend void runOnePollHighPassFilter(LADSPA_Handle Instance,
				      unsigned long SampleCount);

};

/*****************************************************************************/

void 
activateOnePollFilter(LADSPA_Handle Instance) {
  ((OnePollFilter *)Instance)->m_fLastOutput = 0;
}

/*****************************************************************************/

/** Run the LPF algorithm for a block of SampleCount samples. */
void 
runOnePollLowPassFilter(LADSPA_Handle Instance,
			unsigned long SampleCount) {

  OnePollFilter * poFilter = (OnePollFilter *)Instance;

  LADSPA_Data * pfInput = poFilter->m_ppfPorts[SF_INPUT];
  LADSPA_Data * pfOutput = poFilter->m_ppfPorts[SF_OUTPUT];

  if (poFilter->m_fLastCutoff != *(poFilter->m_ppfPorts[SF_CUTOFF])) {
    poFilter->m_fLastCutoff = *(poFilter->m_ppfPorts[SF_CUTOFF]);
    if (poFilter->m_fLastCutoff <= 0) {
      /* Reject everything. */
      poFilter->m_fAmountOfCurrent = poFilter->m_fAmountOfLast = 0;
    }
    else if (poFilter->m_fLastCutoff > poFilter->m_fSampleRate * 0.5) {
      /* Above Nyquist frequency. Let everything through. */
      poFilter->m_fAmountOfCurrent = 1;
      poFilter->m_fAmountOfLast = 0;
    }
    else {
      poFilter->m_fAmountOfLast = 0;
      LADSPA_Data fComp = 2 - cos(poFilter->m_fTwoPiOverSampleRate
				  * poFilter->m_fLastCutoff);
      poFilter->m_fAmountOfLast	= fComp - (LADSPA_Data)sqrt(fComp * fComp - 1);
      poFilter->m_fAmountOfCurrent = 1 - poFilter->m_fAmountOfLast;
    }
  }

  LADSPA_Data fAmountOfCurrent = poFilter->m_fAmountOfCurrent;
  LADSPA_Data fAmountOfLast = poFilter->m_fAmountOfLast;
  LADSPA_Data fLastOutput = poFilter->m_fLastOutput;

  for (unsigned long lSampleIndex = 0;
       lSampleIndex < SampleCount;
       lSampleIndex++) {
    *(pfOutput++)
      = fLastOutput
      = (fAmountOfCurrent * *(pfInput++)
	 + fAmountOfLast * fLastOutput);
  }
  
  poFilter->m_fLastOutput = fLastOutput;
}

/*****************************************************************************/

/** Run the HPF algorithm for a block of SampleCount samples. */
void 
runOnePollHighPassFilter(LADSPA_Handle Instance,
		       unsigned long SampleCount) {

  OnePollFilter * poFilter = (OnePollFilter *)Instance;

  LADSPA_Data * pfInput = poFilter->m_ppfPorts[SF_INPUT];
  LADSPA_Data * pfOutput = poFilter->m_ppfPorts[SF_OUTPUT];

  if (poFilter->m_fLastCutoff != *(poFilter->m_ppfPorts[SF_CUTOFF])) {
    poFilter->m_fLastCutoff = *(poFilter->m_ppfPorts[SF_CUTOFF]);
    if (poFilter->m_fLastCutoff <= 0) {
      /* Let everything through. */
      poFilter->m_fAmountOfCurrent = 1;
      poFilter->m_fAmountOfLast = 0;
    }
    else if (poFilter->m_fLastCutoff > poFilter->m_fSampleRate * 0.5) {
      /* Above Nyquist frequency. Reject everything. */
      poFilter->m_fAmountOfCurrent = poFilter->m_fAmountOfLast = 0;
    }
    else {
      poFilter->m_fAmountOfLast = 0;
      LADSPA_Data fComp = 2 - cos(poFilter->m_fTwoPiOverSampleRate
				  * poFilter->m_fLastCutoff);
      poFilter->m_fAmountOfLast	= fComp - (LADSPA_Data)sqrt(fComp * fComp - 1);
      poFilter->m_fAmountOfCurrent = 1 - poFilter->m_fAmountOfLast;
    }

  }

  LADSPA_Data fAmountOfCurrent = poFilter->m_fAmountOfCurrent;
  LADSPA_Data fAmountOfLast = poFilter->m_fAmountOfLast;
  LADSPA_Data fLastOutput = poFilter->m_fLastOutput;

  for (unsigned long lSampleIndex = 0;
       lSampleIndex < SampleCount;
       lSampleIndex++) {
    fLastOutput
      = (fAmountOfCurrent * *pfInput
	 + fAmountOfLast * fLastOutput);
    *(pfOutput++) = *(pfInput++) - fLastOutput;
  }
  
  poFilter->m_fLastOutput = fLastOutput;
}

/*****************************************************************************/

void
initialise_filter() {
  
  CMT_Descriptor * psDescriptor;
  
  psDescriptor = new CMT_Descriptor
    (1051,
     "lpf",
     LADSPA_PROPERTY_HARD_RT_CAPABLE,
     "Low Pass Filter (One Pole)",
     CMT_MAKER("Richard W.E. Furse"),
     CMT_COPYRIGHT("2000-2002", "Richard W.E. Furse"),
     NULL,
     CMT_Instantiate<OnePollFilter>,
     activateOnePollFilter,
     runOnePollLowPassFilter,
     NULL,
     NULL,
     NULL);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Cutoff Frequency (Hz)",
     (LADSPA_HINT_BOUNDED_BELOW 
      | LADSPA_HINT_BOUNDED_ABOVE
      | LADSPA_HINT_SAMPLE_RATE
      | LADSPA_HINT_LOGARITHMIC
      | LADSPA_HINT_DEFAULT_440),
     0, 
     0.5f); /* Nyquist frequency (half the sample rate) */
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output");
  registerNewPluginDescriptor(psDescriptor);

  psDescriptor = new CMT_Descriptor
    (1052,
     "hpf",
     LADSPA_PROPERTY_HARD_RT_CAPABLE,
     "High Pass Filter (One Pole)",
     CMT_MAKER("Richard W.E. Furse"),
     CMT_COPYRIGHT("2000-2002", "Richard W.E. Furse"),
     NULL,
     CMT_Instantiate<OnePollFilter>,
     activateOnePollFilter,
     runOnePollHighPassFilter,
     NULL,
     NULL,
     NULL);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Cutoff Frequency (Hz)",
     (LADSPA_HINT_BOUNDED_BELOW 
      | LADSPA_HINT_BOUNDED_ABOVE
      | LADSPA_HINT_SAMPLE_RATE
      | LADSPA_HINT_LOGARITHMIC
      | LADSPA_HINT_DEFAULT_440),
     0, 
     0.5f); /* Nyquist frequency (half the sample rate) */
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
