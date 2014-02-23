/* grain.cpp

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
#include <string.h>

/*****************************************************************************/

#include "cmt.h"
#include "utils.h"

/*****************************************************************************/

/** Period (in seconds) from which grains are selected. */
#define GRAIN_MAXIMUM_HISTORY  6
#define GRAIN_MAXIMUM_BLOCK    1 /* (seconds) */

#define GRAIN_MAXIMUM_SCATTER (GRAIN_MAXIMUM_HISTORY - GRAIN_MAXIMUM_BLOCK)
#define GRAIN_MAXIMUM_LENGTH  (GRAIN_MAXIMUM_HISTORY - GRAIN_MAXIMUM_BLOCK)

/** What quality should we require when sampling the normal
    distribution to generate grain counts? */
#define GRAIN_NORMAL_RV_QUALITY 16

/*****************************************************************************/

/** Pointers to this can be used as linked list of grains. */
class Grain {
private:

  long m_lReadPointer;
  long m_lGrainLength;
  long m_lAttackTime;

  long m_lRunTime;

  bool m_bFinished;

  LADSPA_Data m_fAttackSlope;
  LADSPA_Data m_fDecaySlope;

public:

  Grain(const long        lReadPointer,
	const long        lGrainLength,
	const long        lAttackTime)
    : m_lReadPointer(lReadPointer),
      m_lGrainLength(lGrainLength),
      m_lAttackTime(lAttackTime),
      m_lRunTime(0),
      m_bFinished(false) {
    if (lAttackTime <= 0) {
      m_fAttackSlope = 0;
      m_fDecaySlope = LADSPA_Data(1.0 / lGrainLength);
    }
    else {
      m_fAttackSlope = LADSPA_Data(1.0 / lAttackTime);
      if (lAttackTime >= lGrainLength) 
	m_fDecaySlope = 0;
      else 
	m_fDecaySlope = LADSPA_Data(1.0 / (lGrainLength - lAttackTime));
    }
  }

  bool isFinished() const {
    return m_bFinished;
  }

  /** NULL if end of grain list. */
  Grain * m_poNextGrain;

  void run(const unsigned long lSampleCount,
	   float * pfOutput,
	   const float * pfHistoryBuffer,
	   const unsigned long lHistoryBufferSize) {

    LADSPA_Data fAmp;
    if (m_lRunTime < m_lAttackTime)
      fAmp = m_fAttackSlope * m_lRunTime;
    else
      fAmp = m_fDecaySlope * (m_lGrainLength - m_lRunTime);

    for (unsigned long lSampleIndex = 0;
	 lSampleIndex < lSampleCount;
	 lSampleIndex++) {

      if (fAmp < 0) {
	m_bFinished = true;
	break;
      }

      *(pfOutput++) += fAmp * pfHistoryBuffer[m_lReadPointer];
      
      m_lReadPointer = (m_lReadPointer + 1) & (lHistoryBufferSize - 1);

      if (m_lRunTime < m_lAttackTime)
	fAmp += m_fAttackSlope;
      else 
	fAmp -= m_fDecaySlope;

      m_lRunTime++;
    }
  }
};

/*****************************************************************************/

#define GRN_INPUT        0
#define GRN_OUTPUT       1
#define GRN_DENSITY      2
#define GRN_SCATTER      3
#define GRN_GRAIN_LENGTH 4
#define GRN_GRAIN_ATTACK 5

/** This plugin cuts an audio stream up and uses it to generate a
    granular texture. */
class GrainScatter : public CMT_PluginInstance {
private:

  Grain * m_poCurrentGrains;

  long m_lSampleRate;

  LADSPA_Data * m_pfBuffer;

  /** Buffer size, a power of two. */
  unsigned long m_lBufferSize;

  /** Write pointer in buffer. */
  unsigned long m_lWritePointer;
  
public:

  GrainScatter(const LADSPA_Descriptor *,
	       unsigned long lSampleRate)
    : CMT_PluginInstance(6),
      m_poCurrentGrains(NULL),
      m_lSampleRate(lSampleRate) {
    /* Buffer size is a power of two bigger than max delay time. */
    unsigned long lMinimumBufferSize 
      = (unsigned long)((LADSPA_Data)lSampleRate * GRAIN_MAXIMUM_HISTORY);
    m_lBufferSize = 1;
    while (m_lBufferSize < lMinimumBufferSize)
      m_lBufferSize <<= 1;
    m_pfBuffer = new LADSPA_Data[m_lBufferSize];
  }

  ~GrainScatter() {
    delete [] m_pfBuffer;
  }

  friend void activateGrainScatter(LADSPA_Handle Instance);
  friend void runGrainScatter(LADSPA_Handle Instance,
			      unsigned long SampleCount);

};

/*****************************************************************************/

/** Initialise and activate a plugin instance. */
void
activateGrainScatter(LADSPA_Handle Instance) {

  GrainScatter * poGrainScatter = (GrainScatter *)Instance;

  /* Need to reset the delay history in this function rather than
     instantiate() in case deactivate() followed by activate() have
     been called to reinitialise a delay line. */
  memset(poGrainScatter->m_pfBuffer, 
	 0, 
	 sizeof(LADSPA_Data) * poGrainScatter->m_lBufferSize);

  poGrainScatter->m_lWritePointer = 0;
}

/*****************************************************************************/

void 
runGrainScatter(LADSPA_Handle Instance,
		unsigned long SampleCount) {

  GrainScatter * poGrainScatter = (GrainScatter *)Instance;

  LADSPA_Data * pfInput  = poGrainScatter->m_ppfPorts[GRN_INPUT];
  LADSPA_Data * pfOutput = poGrainScatter->m_ppfPorts[GRN_OUTPUT];

  unsigned long lMaximumSampleCount 
    = (unsigned long)(poGrainScatter->m_lSampleRate 
		      * GRAIN_MAXIMUM_BLOCK);

  if (SampleCount > lMaximumSampleCount) {

    /* We're beyond our capabilities. We're going to run out of delay
       line for a large grain. Divide and conquer. */

    runGrainScatter(Instance, lMaximumSampleCount);

    poGrainScatter->m_ppfPorts[GRN_INPUT]  += lMaximumSampleCount;
    poGrainScatter->m_ppfPorts[GRN_OUTPUT] += lMaximumSampleCount;
    runGrainScatter(Instance, SampleCount - lMaximumSampleCount);
    poGrainScatter->m_ppfPorts[GRN_INPUT]  = pfInput;
    poGrainScatter->m_ppfPorts[GRN_OUTPUT] = pfOutput;

  }
  else {
    
    /* Move the delay line along. */
    if (poGrainScatter->m_lWritePointer 
	+ SampleCount
	> poGrainScatter->m_lBufferSize) {
      memcpy(poGrainScatter->m_pfBuffer + poGrainScatter->m_lWritePointer, 
	     pfInput,
	     sizeof(LADSPA_Data) * (poGrainScatter->m_lBufferSize
				    - poGrainScatter->m_lWritePointer));
      memcpy(poGrainScatter->m_pfBuffer,
	     pfInput + (poGrainScatter->m_lBufferSize
			- poGrainScatter->m_lWritePointer),
	     sizeof(LADSPA_Data) * (SampleCount 
				    - (poGrainScatter->m_lBufferSize
				       - poGrainScatter->m_lWritePointer)));
    }
    else {
      memcpy(poGrainScatter->m_pfBuffer + poGrainScatter->m_lWritePointer, 
	     pfInput,
	     sizeof(LADSPA_Data) * SampleCount);
    }
    poGrainScatter->m_lWritePointer 
      = ((poGrainScatter->m_lWritePointer + SampleCount)
	 & (poGrainScatter->m_lBufferSize - 1));

    /* Empty the output buffer. */
    memset(pfOutput, 0, SampleCount * sizeof(LADSPA_Data));
    
    /* Process current grains. */
    Grain ** ppoGrainReference = &(poGrainScatter->m_poCurrentGrains);
    while (*ppoGrainReference != NULL) {
      (*ppoGrainReference)->run(SampleCount,
				pfOutput,
				poGrainScatter->m_pfBuffer,
				poGrainScatter->m_lBufferSize);
      if ((*ppoGrainReference)->isFinished()) {
	Grain *poNextGrain = (*ppoGrainReference)->m_poNextGrain;
	delete *ppoGrainReference;
	*ppoGrainReference = poNextGrain;
      }
      else {
	ppoGrainReference = &((*ppoGrainReference)->m_poNextGrain);
      }
    }

    LADSPA_Data fSampleRate = LADSPA_Data(poGrainScatter->m_lSampleRate);
    LADSPA_Data fDensity 
      = BOUNDED_BELOW(*(poGrainScatter->m_ppfPorts[GRN_DENSITY]),
		      0);

    /* We want to average fDensity new grains per second. We need to
       use a RNG to generate a new grain count from the fraction of a
       second we are dealing with. Use a normal distribution and
       choose standard deviation also to be fDensity. This could be
       separately parameterised but any guarantees could be confusing
       given that individual grains are uniformly distributed within
       the block. Note that fDensity isn't quite grains/sec as we
       discard negative samples from the RV. */
    double dGrainCountRV_Mean = fDensity * SampleCount / fSampleRate;
    double dGrainCountRV_SD   = dGrainCountRV_Mean;
    double dGrainCountRV = sampleNormalDistribution(dGrainCountRV_Mean,
						    dGrainCountRV_SD,
						    GRAIN_NORMAL_RV_QUALITY);
    unsigned long lNewGrainCount = 0;
    if (dGrainCountRV > 0)
      lNewGrainCount = (unsigned long)(0.5 + dGrainCountRV);
    if (lNewGrainCount > 0) {

      LADSPA_Data fScatter 
	= BOUNDED(*(poGrainScatter->m_ppfPorts[GRN_SCATTER]),
		  0,
		  GRAIN_MAXIMUM_SCATTER);
      LADSPA_Data fGrainLength
	= BOUNDED_BELOW(*(poGrainScatter->m_ppfPorts[GRN_GRAIN_LENGTH]),
			0);
      LADSPA_Data fAttack
	= BOUNDED_BELOW(*(poGrainScatter->m_ppfPorts[GRN_GRAIN_ATTACK]),
			0);

      long lScatterSampleWidth
	= long(fSampleRate * fScatter) + 1;
      long lGrainLength   
	= long(fSampleRate * fGrainLength);
      long lAttackTime
	= long(fSampleRate * fAttack);

      for (unsigned long lIndex = 0; lIndex < lNewGrainCount; lIndex++) {

	long lOffset = rand() % SampleCount;

	long lGrainReadPointer 
	  = (poGrainScatter->m_lWritePointer 
	     - SampleCount 
	     + lOffset
	     - (rand() % lScatterSampleWidth));
	while (lGrainReadPointer < 0)
	  lGrainReadPointer += poGrainScatter->m_lBufferSize;
	lGrainReadPointer &= (poGrainScatter->m_lBufferSize - 1);

	Grain * poNewGrain = new Grain(lGrainReadPointer,
				       lGrainLength,
				       lAttackTime);

	poNewGrain->m_poNextGrain = poGrainScatter->m_poCurrentGrains;
	poGrainScatter->m_poCurrentGrains = poNewGrain;

	poNewGrain->run(SampleCount - lOffset,
			pfOutput + lOffset,
			poGrainScatter->m_pfBuffer,
			poGrainScatter->m_lBufferSize);
      }
    }
  }
}

/*****************************************************************************/

void
initialise_grain() {
  
  CMT_Descriptor * psDescriptor = new CMT_Descriptor
    (1096,
     "grain_scatter",
     0,
     "Granular Scatter Processor",
     CMT_MAKER("Richard W.E. Furse"),
     CMT_COPYRIGHT("2000-2002", "Richard W.E. Furse"),
     NULL,
     CMT_Instantiate<GrainScatter>,
     activateGrainScatter,
     runGrainScatter,
     NULL,
     NULL,
     NULL);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Density (Grains/s)",
     (LADSPA_HINT_BOUNDED_BELOW 
      | LADSPA_HINT_DEFAULT_MAXIMUM),
     0,
     10);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Scatter (s)",
     (LADSPA_HINT_BOUNDED_BELOW 
      | LADSPA_HINT_BOUNDED_ABOVE
      | LADSPA_HINT_DEFAULT_MIDDLE),
     0,
     GRAIN_MAXIMUM_SCATTER);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Grain Length (s)",
     (LADSPA_HINT_BOUNDED_BELOW
      | LADSPA_HINT_DEFAULT_MAXIMUM),
     0,
     0.2);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Grain Attack (s)",
     (LADSPA_HINT_BOUNDED_BELOW
      | LADSPA_HINT_DEFAULT_MAXIMUM),
     0,
     0.05);

  registerNewPluginDescriptor(psDescriptor);
}

/*****************************************************************************/

/* EOF */
