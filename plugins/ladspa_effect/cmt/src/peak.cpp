/* peak.cpp

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
#include <string.h>

/*****************************************************************************/

#include "cmt.h"
#include "utils.h"

/*****************************************************************************/

#define ET_INPUT  0
#define ET_OUTPUT 1

#define ET_FILTER 2

/** This class is used to provide plugins that perform envelope
    tracking. Peak and RMS are supported and smoothed or smoothed
    maximum approaches are available. */
class Tracker : public CMT_PluginInstance {
private:

  LADSPA_Data m_fState;
  LADSPA_Data m_fSampleRate;
  
public:

  Tracker(const LADSPA_Descriptor *,
	  unsigned long lSampleRate)
    : CMT_PluginInstance(3),
      m_fSampleRate(LADSPA_Data(lSampleRate)) {
  }
  
  friend void activateTracker(void * pvHandle);
  friend void runEnvelopeTracker_Peak(LADSPA_Handle Instance,
				      unsigned long SampleCount);
  friend void runEnvelopeTracker_RMS(LADSPA_Handle Instance,
				     unsigned long SampleCount);
  friend void runEnvelopeTracker_MaxPeak(LADSPA_Handle Instance,
					 unsigned long SampleCount);
  friend void runEnvelopeTracker_MaxRMS(LADSPA_Handle Instance,
					unsigned long SampleCount);
  
};

/** This class provides a simple peak monitor that records the highest
    signal peak present ever. It can be useful to identify clipping
    cases. */
class PeakMonitor : public CMT_PluginInstance {
private:

  LADSPA_Data m_fState;
  
public:

  PeakMonitor(const LADSPA_Descriptor *,
	      unsigned long lSampleRate)
    : CMT_PluginInstance(2) {
  }
  
  friend void activatePeakMonitor(void * pvHandle);
  friend void runPeakMonitor(LADSPA_Handle Instance,
			     unsigned long SampleCount);
  
};

/*****************************************************************************/

void 
activateTracker(void * pvHandle) {
  ((Tracker *)pvHandle)->m_fState = 0;
}

/*****************************************************************************/

void 
activatePeakMonitor(void * pvHandle) {
  ((PeakMonitor *)pvHandle)->m_fState = 0;
}

/*****************************************************************************/

void 
runEnvelopeTracker_Peak(LADSPA_Handle Instance,
			unsigned long SampleCount) {
  Tracker * poProcessor = (Tracker *)Instance;
  LADSPA_Data * pfInput = poProcessor->m_ppfPorts[ET_INPUT];
  LADSPA_Data fDrag = *(poProcessor->m_ppfPorts[ET_FILTER]);
  LADSPA_Data fOneMinusDrag = 1 - fDrag;
  LADSPA_Data &rfState = poProcessor->m_fState;
  for (unsigned long lSampleIndex = 0; 
       lSampleIndex < SampleCount; 
       lSampleIndex++) {
    LADSPA_Data fInput = *(pfInput++);
    LADSPA_Data fEnvelopeTarget = fabs(fInput);
    rfState = rfState * fDrag + fEnvelopeTarget * fOneMinusDrag;
  }
  *(poProcessor->m_ppfPorts[ET_OUTPUT]) = rfState;
}

/*****************************************************************************/

void 
runEnvelopeTracker_RMS(LADSPA_Handle Instance,
		       unsigned long SampleCount) {
  Tracker * poProcessor = (Tracker *)Instance;
  LADSPA_Data * pfInput = poProcessor->m_ppfPorts[ET_INPUT];
  LADSPA_Data fDrag = *(poProcessor->m_ppfPorts[ET_FILTER]);
  LADSPA_Data fOneMinusDrag = 1 - fDrag;
  LADSPA_Data &rfState = poProcessor->m_fState;
  for (unsigned long lSampleIndex = 0; 
       lSampleIndex < SampleCount; 
       lSampleIndex++) {
    LADSPA_Data fInput = *(pfInput++);
    LADSPA_Data fEnvelopeTarget = fInput * fInput;
    rfState = rfState * fDrag + fEnvelopeTarget * fOneMinusDrag;
  }
  *(poProcessor->m_ppfPorts[ET_OUTPUT]) = sqrt(rfState);
}

/*****************************************************************************/

void 
runEnvelopeTracker_MaxPeak(LADSPA_Handle Instance,
			   unsigned long SampleCount) {
  Tracker * poProcessor = (Tracker *)Instance;
  LADSPA_Data * pfInput = poProcessor->m_ppfPorts[ET_INPUT];
  LADSPA_Data fDrag = calculate60dBDrag(*(poProcessor->m_ppfPorts[ET_FILTER]),
					poProcessor->m_fSampleRate);
  LADSPA_Data &rfState = poProcessor->m_fState;
  for (unsigned long lSampleIndex = 0; 
       lSampleIndex < SampleCount; 
       lSampleIndex++) {
    LADSPA_Data fInput = *(pfInput++);
    LADSPA_Data fEnvelopeTarget = fabs(fInput);
    if (fEnvelopeTarget > rfState)
      rfState = fEnvelopeTarget;
    else {
      rfState *= fDrag;
      if (fEnvelopeTarget > rfState)
	rfState = fEnvelopeTarget;
    }
  }
  *(poProcessor->m_ppfPorts[ET_OUTPUT]) = rfState;
}

/*****************************************************************************/

void 
runEnvelopeTracker_MaxRMS(LADSPA_Handle Instance,
			  unsigned long SampleCount) {
  Tracker * poProcessor = (Tracker *)Instance;
  LADSPA_Data * pfInput = poProcessor->m_ppfPorts[ET_INPUT];
  LADSPA_Data fDrag = calculate60dBDrag(*(poProcessor->m_ppfPorts[ET_FILTER]),
					poProcessor->m_fSampleRate);
  LADSPA_Data &rfState = poProcessor->m_fState;
  for (unsigned long lSampleIndex = 0; 
       lSampleIndex < SampleCount; 
       lSampleIndex++) {
    LADSPA_Data fInput = *(pfInput++);
    LADSPA_Data fEnvelopeTarget = fInput * fInput;
    if (fEnvelopeTarget > rfState)
      rfState = fEnvelopeTarget;
    else {
      rfState *= fDrag;
      if (fEnvelopeTarget > rfState)
	rfState = fEnvelopeTarget;
    }
  }
  *(poProcessor->m_ppfPorts[ET_OUTPUT]) = sqrt(rfState);
}

/*****************************************************************************/

void 
runPeakMonitor(LADSPA_Handle Instance,
	       unsigned long SampleCount) {
  PeakMonitor * poProcessor = (PeakMonitor *)Instance;
  LADSPA_Data * pfInput = poProcessor->m_ppfPorts[ET_INPUT];
  LADSPA_Data &rfState = poProcessor->m_fState;
  for (unsigned long lSampleIndex = 0; 
       lSampleIndex < SampleCount; 
       lSampleIndex++) {
    LADSPA_Data fInput = *(pfInput++);
    LADSPA_Data fEnvelopeTarget = fabs(fInput);
    if (rfState < fEnvelopeTarget)
      rfState = fEnvelopeTarget;
  }
  *(poProcessor->m_ppfPorts[ET_OUTPUT]) = rfState;
}

/*****************************************************************************/

void
initialise_peak() {
  
  CMT_Descriptor * psDescriptor;

  psDescriptor = new CMT_Descriptor
    (1078,
     "track_peak",
     LADSPA_PROPERTY_HARD_RT_CAPABLE,
     "Envelope Tracker (Peak)",
     CMT_MAKER("Richard W.E. Furse"),
     CMT_COPYRIGHT("2000-2002", "Richard W.E. Furse"),
     NULL,
     CMT_Instantiate<Tracker>,
     activateTracker,
     runEnvelopeTracker_Peak,
     NULL,
     NULL,
     NULL);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL,
     "Output",
     LADSPA_HINT_BOUNDED_BELOW,
     0);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Smoothing Factor",
     LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE,
     0,
     1);
  registerNewPluginDescriptor(psDescriptor);
  
  psDescriptor = new CMT_Descriptor
    (1079,
     "track_rms",
     LADSPA_PROPERTY_HARD_RT_CAPABLE,
     "Envelope Tracker (RMS)",
     CMT_MAKER("Richard W.E. Furse"),
     CMT_COPYRIGHT("2000-2002", "Richard W.E. Furse"),
     NULL,
     CMT_Instantiate<Tracker>,
     activateTracker,
     runEnvelopeTracker_RMS,
     NULL,
     NULL,
     NULL);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL,
     "Output",
     LADSPA_HINT_BOUNDED_BELOW,
     0);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Smoothing Factor",
     LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE,
     0,
     1);
  registerNewPluginDescriptor(psDescriptor);
  
  psDescriptor = new CMT_Descriptor
    (1080,
     "track_max_peak",
     LADSPA_PROPERTY_HARD_RT_CAPABLE,
     "Envelope Tracker (Maximum Peak)",
     CMT_MAKER("Richard W.E. Furse"),
     CMT_COPYRIGHT("2000-2002", "Richard W.E. Furse"),
     NULL,
     CMT_Instantiate<Tracker>,
     activateTracker,
     runEnvelopeTracker_MaxPeak,
     NULL,
     NULL,
     NULL);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL,
     "Output",
     LADSPA_HINT_BOUNDED_BELOW,
     0);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Envelope Forgetting Factor (s/60dB)",
     (LADSPA_HINT_BOUNDED_BELOW
      | LADSPA_HINT_DEFAULT_MAXIMUM),
     0,
     10);
  registerNewPluginDescriptor(psDescriptor);
  
  psDescriptor = new CMT_Descriptor
    (1081,
     "track_max_rms",
     LADSPA_PROPERTY_HARD_RT_CAPABLE,
     "Envelope Tracker (Maximum RMS)",
     CMT_MAKER("Richard W.E. Furse"),
     CMT_COPYRIGHT("2000-2002", "Richard W.E. Furse"),
     NULL,
     CMT_Instantiate<Tracker>,
     activateTracker,
     runEnvelopeTracker_MaxRMS,
     NULL,
     NULL,
     NULL);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL,
     "Output",
     LADSPA_HINT_BOUNDED_BELOW,
     0);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Envelope Forgetting Factor (s/60dB)",
     (LADSPA_HINT_BOUNDED_BELOW
      | LADSPA_HINT_DEFAULT_MAXIMUM),
     0,
     10);
  registerNewPluginDescriptor(psDescriptor);
  
  psDescriptor = new CMT_Descriptor
    (1082,
     "peak",
     LADSPA_PROPERTY_HARD_RT_CAPABLE,
     "Peak Monitor",
     CMT_MAKER("Richard W.E. Furse"),
     CMT_COPYRIGHT("2000", "Richard W.E. Furse"),
     NULL,
     CMT_Instantiate<PeakMonitor>,
     activatePeakMonitor,
     runPeakMonitor,
     NULL,
     NULL,
     NULL);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL,
     "Peak",
     LADSPA_HINT_BOUNDED_BELOW,
     0);
  registerNewPluginDescriptor(psDescriptor);
}

/*****************************************************************************/

/* EOF */
