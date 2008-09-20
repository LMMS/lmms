/* delay.cpp

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

/* This module provides delays and delays with feedback. A variety of
   maximum delay times are available. (The plugins reserve different
   amounts of memory space on this basis.) */

/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*****************************************************************************/

#include "cmt.h"

/*****************************************************************************/

#define DELAY_TYPE_COUNT 2

#define DELAY_LENGTH_COUNT 5

/*****************************************************************************/

#define LIMIT_BETWEEN(x, a, b)          \
(((x) < a) ? a : (((x) > b) ? b : (x)))

/*****************************************************************************/

#define DL_DELAY_LENGTH 0
#define DL_DRY_WET      1
#define DL_INPUT        2
#define DL_OUTPUT       3
/* Present only on feedback delays: */
#define DL_FEEDBACK     4

/** This class is used to implement delay line plugins. Different
    maximum delay times are supported as are both echo and feedback
    delays. */
class DelayLine : public CMT_PluginInstance {
private:

  LADSPA_Data m_fSampleRate;

  LADSPA_Data m_fMaximumDelay;

  LADSPA_Data * m_pfBuffer;

  /** Buffer size, a power of two. */
  unsigned long m_lBufferSize;

  /** Write pointer in buffer. */
  unsigned long m_lWritePointer;

  friend void activateDelayLine(LADSPA_Handle Instance);
  friend void runSimpleDelayLine(LADSPA_Handle Instance,
				 unsigned long SampleCount);
  friend void runFeedbackDelayLine(LADSPA_Handle Instance,
				   unsigned long SampleCount);

public:

  DelayLine(const unsigned long lSampleRate,
	    const LADSPA_Data fMaximumDelay) 
    : CMT_PluginInstance(4),
      m_fSampleRate(LADSPA_Data(lSampleRate)),
      m_fMaximumDelay(fMaximumDelay) {
    /* Buffer size is a power of two bigger than max delay time. */
    unsigned long lMinimumBufferSize 
      = (unsigned long)((LADSPA_Data)lSampleRate * m_fMaximumDelay);
    m_lBufferSize = 1;
    while (m_lBufferSize < lMinimumBufferSize)
      m_lBufferSize <<= 1;
    m_pfBuffer = new LADSPA_Data[m_lBufferSize];
  }
  
  ~DelayLine() {
    delete [] m_pfBuffer;
  }
};

/*****************************************************************************/

/* Initialise and activate a plugin instance. */
void
activateDelayLine(LADSPA_Handle Instance) {

  DelayLine * poDelayLine = (DelayLine *)Instance;

  /* Need to reset the delay history in this function rather than
     instantiate() in case deactivate() followed by activate() have
     been called to reinitialise a delay line. */
  memset(poDelayLine->m_pfBuffer, 
	 0, 
	 sizeof(LADSPA_Data) * poDelayLine->m_lBufferSize);

  poDelayLine->m_lWritePointer = 0;
}

/*****************************************************************************/

/* Run a delay line instance for a block of SampleCount samples. */
void 
runSimpleDelayLine(LADSPA_Handle Instance,
		   unsigned long SampleCount) {
  
  DelayLine * poDelayLine = (DelayLine *)Instance;

  unsigned long lBufferSizeMinusOne = poDelayLine->m_lBufferSize - 1;
  unsigned long lDelay = (unsigned long)
    (LIMIT_BETWEEN(*(poDelayLine->m_ppfPorts[DL_DELAY_LENGTH]),
		   0,
		   poDelayLine->m_fMaximumDelay)
     * poDelayLine->m_fSampleRate);

  LADSPA_Data * pfInput
    = poDelayLine->m_ppfPorts[DL_INPUT];
  LADSPA_Data * pfOutput
    = poDelayLine->m_ppfPorts[DL_OUTPUT];
  LADSPA_Data * pfBuffer
    = poDelayLine->m_pfBuffer;

  unsigned long lBufferWriteOffset
    = poDelayLine->m_lWritePointer;
  unsigned long lBufferReadOffset
    = lBufferWriteOffset + poDelayLine->m_lBufferSize - lDelay;
  LADSPA_Data fWet 
    = LIMIT_BETWEEN(*(poDelayLine->m_ppfPorts[DL_DRY_WET]),
		    0,
		    1);
  LADSPA_Data fDry
    = 1 - fWet;

  for (unsigned long lSampleIndex = 0;
       lSampleIndex < SampleCount;
       lSampleIndex++) {
    LADSPA_Data fInputSample = *(pfInput++);
    *(pfOutput++) = (fDry * fInputSample
		     + fWet * pfBuffer[((lSampleIndex + lBufferReadOffset)
					& lBufferSizeMinusOne)]);
    pfBuffer[((lSampleIndex + lBufferWriteOffset)
	      & lBufferSizeMinusOne)] = fInputSample;
  }

  poDelayLine->m_lWritePointer
    = ((poDelayLine->m_lWritePointer + SampleCount)
       & lBufferSizeMinusOne);
}

/*****************************************************************************/

/** Run a feedback delay line instance for a block of SampleCount samples. */
void 
runFeedbackDelayLine(LADSPA_Handle Instance,
		     unsigned long SampleCount) {
  
  DelayLine * poDelayLine = (DelayLine *)Instance;

  unsigned long lBufferSizeMinusOne = poDelayLine->m_lBufferSize - 1;
  unsigned long lDelay = (unsigned long)
    (LIMIT_BETWEEN(*(poDelayLine->m_ppfPorts[DL_DELAY_LENGTH]),
		   0,
		   poDelayLine->m_fMaximumDelay)
     * poDelayLine->m_fSampleRate);

  LADSPA_Data * pfInput
    = poDelayLine->m_ppfPorts[DL_INPUT];
  LADSPA_Data * pfOutput
    = poDelayLine->m_ppfPorts[DL_OUTPUT];
  LADSPA_Data * pfBuffer
    = poDelayLine->m_pfBuffer;

  unsigned long lBufferWriteOffset
    = poDelayLine->m_lWritePointer;
  unsigned long lBufferReadOffset
    = lBufferWriteOffset + poDelayLine->m_lBufferSize - lDelay;
  LADSPA_Data fWet 
    = LIMIT_BETWEEN(*(poDelayLine->m_ppfPorts[DL_DRY_WET]),
		    0,
		    1);
  LADSPA_Data fDry
    = 1 - fWet;
  LADSPA_Data fFeedback
    = LIMIT_BETWEEN(*(poDelayLine->m_ppfPorts[DL_FEEDBACK]),
		    -1,
		    1);

  for (unsigned long lSampleIndex = 0;
       lSampleIndex < SampleCount;
       lSampleIndex++) {

    LADSPA_Data fInputSample = *(pfInput++);
    LADSPA_Data &fDelayedSample = pfBuffer[((lSampleIndex + lBufferReadOffset)
					    & lBufferSizeMinusOne)];
    
    *(pfOutput++) = (fDry * fInputSample + fWet * fDelayedSample);

    pfBuffer[((lSampleIndex + lBufferWriteOffset)
	      & lBufferSizeMinusOne)]
      = fInputSample + fDelayedSample * fFeedback;
  }
  
  poDelayLine->m_lWritePointer
    = ((poDelayLine->m_lWritePointer + SampleCount)
       & lBufferSizeMinusOne);
}

/*****************************************************************************/

template <long lMaximumDelayMilliseconds> LADSPA_Handle 
CMT_Delay_Instantiate(const LADSPA_Descriptor * Descriptor,
		      unsigned long             SampleRate) {
  return new DelayLine(SampleRate,
			     LADSPA_Data(lMaximumDelayMilliseconds
					 * 0.001));
}

/*****************************************************************************/

void 
initialise_delay() {

  CMT_Descriptor * psDescriptor;

  const char * apcDelayTypeNames[DELAY_TYPE_COUNT] = {
    "Echo",
    "Feedback"
  };
  const char * apcDelayTypeLabels[DELAY_TYPE_COUNT] = {
    "delay",
    "fbdelay"
  };
  LADSPA_Run_Function afRunFunctions[DELAY_TYPE_COUNT] = {
    runSimpleDelayLine,
    runFeedbackDelayLine
  };

  LADSPA_Data afMaximumDelays[DELAY_LENGTH_COUNT] = {
    0.01,
    0.1,
    1,
    5,
    60
  };
  LADSPA_Instantiate_Function afInstantiateFunctions[DELAY_LENGTH_COUNT] = {
    CMT_Delay_Instantiate<10>,
    CMT_Delay_Instantiate<100>,
    CMT_Delay_Instantiate<1000>,
    CMT_Delay_Instantiate<5000>,
    CMT_Delay_Instantiate<60000>
  };
  
  for (long lDelayTypeIndex = 0;
       lDelayTypeIndex < DELAY_TYPE_COUNT; 
       lDelayTypeIndex++) {

    for (long lDelayLengthIndex = 0; 
	 lDelayLengthIndex < DELAY_LENGTH_COUNT;
	 lDelayLengthIndex++) {
      
      long lPluginIndex 
	= lDelayTypeIndex * DELAY_LENGTH_COUNT + lDelayLengthIndex;
      
      char acLabel[100];
      sprintf(acLabel,
	      "%s_%gs",
	      apcDelayTypeLabels[lDelayTypeIndex],
	      afMaximumDelays[lDelayLengthIndex]);
      char acName[100];
      sprintf(acName, 
	      "%s Delay Line (Maximum Delay %gs)",
	      apcDelayTypeNames[lDelayTypeIndex],
	      afMaximumDelays[lDelayLengthIndex]);
      
      psDescriptor = new CMT_Descriptor
	(1053 + lPluginIndex,
	 acLabel,
	 LADSPA_PROPERTY_HARD_RT_CAPABLE,
	 acName,
	 CMT_MAKER("Richard W.E. Furse"),
	 CMT_COPYRIGHT("2000-2002", "Richard W.E. Furse"),
	 NULL,
	 afInstantiateFunctions[lDelayLengthIndex],
	 activateDelayLine,
	 afRunFunctions[lDelayTypeIndex],
	 NULL,
	 NULL,
	 NULL);
      
      psDescriptor->addPort
	(LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
	 "Delay (Seconds)",
	 (LADSPA_HINT_BOUNDED_BELOW 
	  | LADSPA_HINT_BOUNDED_ABOVE
	  | LADSPA_HINT_DEFAULT_1),
	 0,
	 afMaximumDelays[lDelayLengthIndex]);
      psDescriptor->addPort
	(LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
	 "Dry/Wet Balance",
	 (LADSPA_HINT_BOUNDED_BELOW 
	  | LADSPA_HINT_BOUNDED_ABOVE
	  | LADSPA_HINT_DEFAULT_MIDDLE),
	 0,
	 1);
      psDescriptor->addPort
	(LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	 "Input");
      psDescriptor->addPort
	(LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
	 "Output");
      
      if (lDelayTypeIndex == 1)
	psDescriptor->addPort
	  (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
	   "Feedback",
	   (LADSPA_HINT_BOUNDED_BELOW 
	    | LADSPA_HINT_BOUNDED_ABOVE
	    | LADSPA_HINT_DEFAULT_HIGH),
	   -1,
	   1);

      registerNewPluginDescriptor(psDescriptor);
    }
  }
}

/*****************************************************************************/

/* EOF */
