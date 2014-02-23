/* dynamic.cpp

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

/* This module provides unsophisticated implementations of compressor,
   expander and limiter plugins. Note that attack and decay times are
   applied at the LEVEL DETECTION stage rather than at gain processing
   (the reason no noise gate is provided). No delay is applied to the
   main signal. These are useful (and efficient) tools and extended
   compressors should probably allocate new IDs rather than break
   compatibility in parameter set and sound for this set. */

// Having said this, I'm not sure the attack/decay parameters are the
// right way around.

/*****************************************************************************/

#include <math.h>
#include <stdlib.h>
#include <string.h>

/*****************************************************************************/

#include "cmt.h"
#include "utils.h"

/*****************************************************************************/

class DynamicProcessor {
protected:

  /** This state variable is used to track the input envelope (peak or
      rms). The state is stored here so that the run function can
      perform low-pass filtering to produce a smoothed envelope. */
  LADSPA_Data m_fEnvelopeState;

  /** The sample rate in the world this instance exists in. */
  LADSPA_Data m_fSampleRate;

  DynamicProcessor(const LADSPA_Data fSampleRate)
    : m_fSampleRate(fSampleRate) {
  }

};

/*****************************************************************************/

#define CE_THRESHOLD  0
#define CE_RATIO      1
#define CE_ATTACK     2
#define CE_DECAY      3
#define CE_INPUT      4
#define CE_OUTPUT     5

/** This class is used to implement simple compressor and expander
    plugins. Attack and decay times are applied at the level detection
    stage rather than at gain processing. No delay is applied to the
    main signal. Both peak and RMS support is included. */
class CompressorExpander 
  : public CMT_PluginInstance, public DynamicProcessor {
public:

  CompressorExpander(const LADSPA_Descriptor *,
		     unsigned long lSampleRate)
    : CMT_PluginInstance(6),
    DynamicProcessor(lSampleRate) {
  }
  
  friend void activateCompressorExpander(void * pvHandle);
  friend void runCompressor_Peak(LADSPA_Handle Instance,
				 unsigned long SampleCount);
  friend void runCompressor_RMS(LADSPA_Handle Instance,
				unsigned long SampleCount);
  friend void runExpander_Peak(LADSPA_Handle Instance,
			       unsigned long SampleCount);
  friend void runExpander_RMS(LADSPA_Handle Instance,
			      unsigned long SampleCount);
  
};

/*****************************************************************************/

#define LN_THRESHOLD  0
#define LN_ATTACK     1
#define LN_DECAY      2
#define LN_INPUT      3
#define LN_OUTPUT     4

/** This class is used to implement simple limiter plugins. Attack and
    decay times are applied at the level detection stage rather than
    at gain processing. No delay is applied to the main signal. Both
    peak and RMS support is included. */
class Limiter 
  : public CMT_PluginInstance, public DynamicProcessor {
public:

  Limiter(const LADSPA_Descriptor *,
	  unsigned long lSampleRate)
    : CMT_PluginInstance(5),
    DynamicProcessor(lSampleRate) {
  }
  
  friend void activateLimiter(void * pvHandle);
  friend void runLimiter_Peak(LADSPA_Handle Instance,
			      unsigned long SampleCount);
  friend void runLimiter_RMS(LADSPA_Handle Instance,
			     unsigned long SampleCount);
  
};

/*****************************************************************************/

void 
activateCompressorExpander(void * pvHandle) {
  CompressorExpander * poProcessor = (CompressorExpander *)pvHandle;
  poProcessor->m_fEnvelopeState = 0;
}

/*****************************************************************************/

void 
activateLimiter(void * pvHandle) {
  Limiter * poProcessor = (Limiter *)pvHandle;
  poProcessor->m_fEnvelopeState = 0;
}

/*****************************************************************************/

void 
runCompressor_Peak(LADSPA_Handle Instance,
		   unsigned long SampleCount) {

  CompressorExpander * poProcessor = (CompressorExpander *)Instance;

  LADSPA_Data fThreshold 
    = BOUNDED_BELOW(*(poProcessor->m_ppfPorts[CE_THRESHOLD]), 
		    0);
  LADSPA_Data fOneOverThreshold
    = 1 / fThreshold;
  LADSPA_Data fRatioMinusOne
    = *(poProcessor->m_ppfPorts[CE_RATIO]) - 1;
  LADSPA_Data * pfInput
    = poProcessor->m_ppfPorts[CE_INPUT];
  LADSPA_Data * pfOutput
    = poProcessor->m_ppfPorts[CE_OUTPUT];

  LADSPA_Data fEnvelopeDrag_Attack 
    = calculate60dBDrag(*(poProcessor->m_ppfPorts[CE_ATTACK]),
			poProcessor->m_fSampleRate);
  LADSPA_Data fEnvelopeDrag_Decay   
    = calculate60dBDrag(*(poProcessor->m_ppfPorts[CE_DECAY]),
			poProcessor->m_fSampleRate);
 
  LADSPA_Data &rfEnvelopeState
    = poProcessor->m_fEnvelopeState;

  for (unsigned long lSampleIndex = 0; 
       lSampleIndex < SampleCount; 
       lSampleIndex++) {

    LADSPA_Data fInput = *(pfInput++);
    LADSPA_Data fEnvelopeTarget = fabs(fInput);
    if (fEnvelopeTarget > rfEnvelopeState)
      rfEnvelopeState = (rfEnvelopeState * fEnvelopeDrag_Attack
			 + fEnvelopeTarget * (1 - fEnvelopeDrag_Attack));
    else
      rfEnvelopeState = (rfEnvelopeState * fEnvelopeDrag_Decay
			 + fEnvelopeTarget * (1 - fEnvelopeDrag_Decay));
    
    /* Perform the mapping. This questions this plugin's claim of
       being `hard-realtime.' */
    LADSPA_Data fGain;
    if (rfEnvelopeState < fThreshold)
      fGain = 1;
    else {
      fGain = pow(rfEnvelopeState * fOneOverThreshold, fRatioMinusOne);
      if (isnan(fGain))
	fGain = 0;
    }

    /* Perform output. */
    *(pfOutput++) = fInput * fGain;
  }
}

/*****************************************************************************/

void 
runCompressor_RMS(LADSPA_Handle Instance,
		  unsigned long SampleCount) {
  
  CompressorExpander * poProcessor = (CompressorExpander *)Instance;

  LADSPA_Data fThreshold 
    = BOUNDED_BELOW(*(poProcessor->m_ppfPorts[CE_THRESHOLD]), 
		    0);
  LADSPA_Data fOneOverThreshold 
    = 1 / fThreshold;
  LADSPA_Data fRatioMinusOne
    = *(poProcessor->m_ppfPorts[CE_RATIO]) - 1;
  LADSPA_Data * pfInput
    = poProcessor->m_ppfPorts[CE_INPUT];
  LADSPA_Data * pfOutput
    = poProcessor->m_ppfPorts[CE_OUTPUT];

  LADSPA_Data fEnvelopeDrag_Attack 
    = calculate60dBDrag(*(poProcessor->m_ppfPorts[CE_ATTACK]),
			poProcessor->m_fSampleRate);
  LADSPA_Data fEnvelopeDrag_Decay   
    = calculate60dBDrag(*(poProcessor->m_ppfPorts[CE_DECAY]),
			poProcessor->m_fSampleRate);
 
  LADSPA_Data &rfEnvelopeState
    = poProcessor->m_fEnvelopeState;

  for (unsigned long lSampleIndex = 0; 
       lSampleIndex < SampleCount; 
       lSampleIndex++) {

    LADSPA_Data fInput = *(pfInput++);
    LADSPA_Data fEnvelopeTarget = fInput * fInput;
    if (fEnvelopeTarget > rfEnvelopeState)
      rfEnvelopeState = (rfEnvelopeState * fEnvelopeDrag_Attack
			 + fEnvelopeTarget * (1 - fEnvelopeDrag_Attack));
    else
      rfEnvelopeState = (rfEnvelopeState * fEnvelopeDrag_Decay
			 + fEnvelopeTarget * (1 - fEnvelopeDrag_Decay));
    
    LADSPA_Data fEnvelopeAmplitude = sqrt(rfEnvelopeState);
    
    /* Perform the mapping. This questions this plugin's claim of
       being `hard-realtime.' */
    LADSPA_Data fGain;
    if (fEnvelopeAmplitude < fThreshold)
      fGain = 1;
    else {
      fGain = pow(fEnvelopeAmplitude * fOneOverThreshold, fRatioMinusOne);
      if (isnan(fGain))
	fGain = 0;
    }

    /* Perform output. */
    *(pfOutput++) = fInput * fGain;
  }
}

/*****************************************************************************/

void 
runExpander_Peak(LADSPA_Handle Instance,
		 unsigned long SampleCount) {

  CompressorExpander * poProcessor = (CompressorExpander *)Instance;

  LADSPA_Data fThreshold 
    = BOUNDED_BELOW(*(poProcessor->m_ppfPorts[CE_THRESHOLD]), 
		    0);
  LADSPA_Data fOneOverThreshold 
    = 1 / fThreshold;
  LADSPA_Data fOneMinusRatio
    = 1 - *(poProcessor->m_ppfPorts[CE_RATIO]);
  LADSPA_Data * pfInput
    = poProcessor->m_ppfPorts[CE_INPUT];
  LADSPA_Data * pfOutput
    = poProcessor->m_ppfPorts[CE_OUTPUT];

  LADSPA_Data fEnvelopeDrag_Attack 
    = calculate60dBDrag(*(poProcessor->m_ppfPorts[CE_ATTACK]),
			poProcessor->m_fSampleRate);
  LADSPA_Data fEnvelopeDrag_Decay   
    = calculate60dBDrag(*(poProcessor->m_ppfPorts[CE_DECAY]),
			poProcessor->m_fSampleRate);
 
  LADSPA_Data &rfEnvelopeState
    = poProcessor->m_fEnvelopeState;

  for (unsigned long lSampleIndex = 0; 
       lSampleIndex < SampleCount; 
       lSampleIndex++) {

    LADSPA_Data fInput = *(pfInput++);
    LADSPA_Data fEnvelopeTarget = fabs(fInput);
    if (fEnvelopeTarget > rfEnvelopeState)
      rfEnvelopeState = (rfEnvelopeState * fEnvelopeDrag_Attack
			 + fEnvelopeTarget * (1 - fEnvelopeDrag_Attack));
    else
      rfEnvelopeState = (rfEnvelopeState * fEnvelopeDrag_Decay
			 + fEnvelopeTarget * (1 - fEnvelopeDrag_Decay));
    
    /* Perform the mapping. This questions this plugin's claim of
       being `hard-realtime.' */
    LADSPA_Data fGain;
    if (rfEnvelopeState > fThreshold)
      fGain = 1;
    else {
      fGain = pow(rfEnvelopeState * fOneOverThreshold, fOneMinusRatio);
      if (isnan(fGain))
	fGain = 0;
    }

    /* Perform output. */
    *(pfOutput++) = fInput * fGain;
  }
}

/*****************************************************************************/

void 
runExpander_RMS(LADSPA_Handle Instance,
		  unsigned long SampleCount) {
  
  CompressorExpander * poProcessor = (CompressorExpander *)Instance;

  LADSPA_Data fThreshold 
    = BOUNDED_BELOW(*(poProcessor->m_ppfPorts[CE_THRESHOLD]), 
		    0);
  LADSPA_Data fOneOverThreshold 
    = 1 / fThreshold;
  LADSPA_Data fOneMinusRatio
    = 1 - *(poProcessor->m_ppfPorts[CE_RATIO]);
  LADSPA_Data * pfInput
    = poProcessor->m_ppfPorts[CE_INPUT];
  LADSPA_Data * pfOutput
    = poProcessor->m_ppfPorts[CE_OUTPUT];

  LADSPA_Data fEnvelopeDrag_Attack 
    = calculate60dBDrag(*(poProcessor->m_ppfPorts[CE_ATTACK]),
			poProcessor->m_fSampleRate);
  LADSPA_Data fEnvelopeDrag_Decay   
    = calculate60dBDrag(*(poProcessor->m_ppfPorts[CE_DECAY]),
			poProcessor->m_fSampleRate);
 
  LADSPA_Data &rfEnvelopeState
    = poProcessor->m_fEnvelopeState;

  for (unsigned long lSampleIndex = 0; 
       lSampleIndex < SampleCount; 
       lSampleIndex++) {

    LADSPA_Data fInput = *(pfInput++);
    LADSPA_Data fEnvelopeTarget = fInput * fInput;
    if (fEnvelopeTarget > rfEnvelopeState)
      rfEnvelopeState = (rfEnvelopeState * fEnvelopeDrag_Attack
			 + fEnvelopeTarget * (1 - fEnvelopeDrag_Attack));
    else
      rfEnvelopeState = (rfEnvelopeState * fEnvelopeDrag_Decay
			 + fEnvelopeTarget * (1 - fEnvelopeDrag_Decay));
    
    LADSPA_Data fEnvelopeAmplitude = sqrt(rfEnvelopeState);
    
    /* Perform the mapping. This questions this plugin's claim of
       being `hard-realtime.' */
    LADSPA_Data fGain;
    if (fEnvelopeAmplitude > fThreshold) 
      fGain = 1; 
    else {
      fGain = pow(fEnvelopeAmplitude * fOneOverThreshold, fOneMinusRatio);
      if (isnan(fGain))
	fGain = 0;
    }

    /* Perform output. */
    *(pfOutput++) = fInput * fGain;
  }
}

/*****************************************************************************/

void 
runLimiter_Peak(LADSPA_Handle Instance,
		unsigned long SampleCount) {

  Limiter * poProcessor = (Limiter *)Instance;

  LADSPA_Data fThreshold 
    = BOUNDED_BELOW(*(poProcessor->m_ppfPorts[LN_THRESHOLD]), 
		    0);
  LADSPA_Data * pfInput
    = poProcessor->m_ppfPorts[LN_INPUT];
  LADSPA_Data * pfOutput
    = poProcessor->m_ppfPorts[LN_OUTPUT];

  LADSPA_Data fEnvelopeDrag_Attack 
    = calculate60dBDrag(*(poProcessor->m_ppfPorts[CE_ATTACK]),
			poProcessor->m_fSampleRate);
  LADSPA_Data fEnvelopeDrag_Decay   
    = calculate60dBDrag(*(poProcessor->m_ppfPorts[CE_DECAY]),
			poProcessor->m_fSampleRate);
 
  LADSPA_Data &rfEnvelopeState
    = poProcessor->m_fEnvelopeState;

  for (unsigned long lSampleIndex = 0; 
       lSampleIndex < SampleCount; 
       lSampleIndex++) {

    LADSPA_Data fInput = *(pfInput++);
    LADSPA_Data fEnvelopeTarget = fabs(fInput);
    if (fEnvelopeTarget > rfEnvelopeState)
      rfEnvelopeState = (rfEnvelopeState * fEnvelopeDrag_Attack
			 + fEnvelopeTarget * (1 - fEnvelopeDrag_Attack));
    else
      rfEnvelopeState = (rfEnvelopeState * fEnvelopeDrag_Decay
			 + fEnvelopeTarget * (1 - fEnvelopeDrag_Decay));
    
    /* Perform the mapping. This questions this plugin's claim of
       being `hard-realtime.' */
    LADSPA_Data fGain;
    if (rfEnvelopeState < fThreshold)
      fGain = 1;
    else {
      fGain = fThreshold / rfEnvelopeState;
      if (isnan(fGain))
	fGain = 0;
    }

    /* Perform output. */
    *(pfOutput++) = fInput * fGain;
  }
}

/*****************************************************************************/

void 
runLimiter_RMS(LADSPA_Handle Instance,
		unsigned long SampleCount) {

  Limiter * poProcessor = (Limiter *)Instance;

  LADSPA_Data fThreshold 
    = BOUNDED_BELOW(*(poProcessor->m_ppfPorts[LN_THRESHOLD]), 
		    0);
  LADSPA_Data * pfInput
    = poProcessor->m_ppfPorts[LN_INPUT];
  LADSPA_Data * pfOutput
    = poProcessor->m_ppfPorts[LN_OUTPUT];

  LADSPA_Data fEnvelopeDrag_Attack 
    = calculate60dBDrag(*(poProcessor->m_ppfPorts[CE_ATTACK]),
			poProcessor->m_fSampleRate);
  LADSPA_Data fEnvelopeDrag_Decay   
    = calculate60dBDrag(*(poProcessor->m_ppfPorts[CE_DECAY]),
			poProcessor->m_fSampleRate);
 
  LADSPA_Data &rfEnvelopeState
    = poProcessor->m_fEnvelopeState;

  for (unsigned long lSampleIndex = 0; 
       lSampleIndex < SampleCount; 
       lSampleIndex++) {

    LADSPA_Data fInput = *(pfInput++);
    LADSPA_Data fEnvelopeTarget = fInput * fInput;
    if (fEnvelopeTarget > rfEnvelopeState)
      rfEnvelopeState = (rfEnvelopeState * fEnvelopeDrag_Attack
			 + fEnvelopeTarget * (1 - fEnvelopeDrag_Attack));
    else
      rfEnvelopeState = (rfEnvelopeState * fEnvelopeDrag_Decay
			 + fEnvelopeTarget * (1 - fEnvelopeDrag_Decay));

    LADSPA_Data fEnvelopeAmplitude = sqrt(rfEnvelopeState);

    /* Perform the mapping. This questions this plugin's claim of
       being `hard-realtime.' */
    LADSPA_Data fGain;
    if (fEnvelopeAmplitude < fThreshold)
      fGain = 1;
    else {
      fGain = fThreshold / fEnvelopeAmplitude;
      if (isnan(fGain))
	fGain = 0;
    }

    /* Perform output. */
    *(pfOutput++) = fInput * fGain;
  }
}

/*****************************************************************************/

void 
initialise_dynamic() {
  
  CMT_Descriptor * psDescriptor;
  
  psDescriptor = new CMT_Descriptor
    (1072,
     "compress_peak",
     LADSPA_PROPERTY_HARD_RT_CAPABLE,
     "Simple Compressor (Peak Envelope Tracking)",
     CMT_MAKER("Richard W.E. Furse"),
     CMT_COPYRIGHT("2000-2002", "Richard W.E. Furse"),
     NULL,
     CMT_Instantiate<CompressorExpander>,
     activateCompressorExpander,
     runCompressor_Peak,
     NULL,
     NULL,
     NULL);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Threshold",
     (LADSPA_HINT_BOUNDED_BELOW 
      | LADSPA_HINT_LOGARITHMIC
      | LADSPA_HINT_DEFAULT_1),
     0,
     0);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Compression Ratio",
     (LADSPA_HINT_BOUNDED_ABOVE
      | LADSPA_HINT_DEFAULT_MIDDLE),
     0,
     1);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Output Envelope Attack (s)",
     (LADSPA_HINT_BOUNDED_BELOW
      | LADSPA_HINT_DEFAULT_MAXIMUM),
     0,
     0.1f);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Output Envelope Decay (s)",
     (LADSPA_HINT_BOUNDED_BELOW
      | LADSPA_HINT_DEFAULT_MAXIMUM),
     0,
     0.1f);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output");
  registerNewPluginDescriptor(psDescriptor);
  
  psDescriptor = new CMT_Descriptor
    (1073,
     "compress_rms",
     LADSPA_PROPERTY_HARD_RT_CAPABLE,
     "Simple Compressor (RMS Envelope Tracking)",
     CMT_MAKER("Richard W.E. Furse"),
     CMT_COPYRIGHT("2000-2002", "Richard W.E. Furse"),
     NULL,
     CMT_Instantiate<CompressorExpander>,
     activateCompressorExpander,
     runCompressor_RMS,
     NULL,
     NULL,
     NULL);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Threshold",
     (LADSPA_HINT_BOUNDED_BELOW 
      | LADSPA_HINT_LOGARITHMIC
      | LADSPA_HINT_DEFAULT_1),
     0,
     0);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Compression Ratio",
     (LADSPA_HINT_BOUNDED_ABOVE
      | LADSPA_HINT_DEFAULT_MIDDLE),
     0,
     1);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Output Envelope Attack (s)",
     (LADSPA_HINT_BOUNDED_BELOW
      | LADSPA_HINT_DEFAULT_MAXIMUM),
     0,
     0.1f);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Output Envelope Decay (s)",
     (LADSPA_HINT_BOUNDED_BELOW
      | LADSPA_HINT_DEFAULT_MAXIMUM),
     0,
     0.1f);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output");
  registerNewPluginDescriptor(psDescriptor);
  
  psDescriptor = new CMT_Descriptor
    (1074,
     "expand_peak",
     LADSPA_PROPERTY_HARD_RT_CAPABLE,
     "Simple Expander (Peak Envelope Tracking)",
     CMT_MAKER("Richard W.E. Furse"),
     CMT_COPYRIGHT("2000-2002", "Richard W.E. Furse"),
     NULL,
     CMT_Instantiate<CompressorExpander>,
     activateCompressorExpander,
     runExpander_Peak,
     NULL,
     NULL,
     NULL);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Threshold",
     (LADSPA_HINT_BOUNDED_BELOW 
      | LADSPA_HINT_LOGARITHMIC
      | LADSPA_HINT_DEFAULT_1),
     0,
     0);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Expansion Ratio",
     (LADSPA_HINT_BOUNDED_ABOVE
      | LADSPA_HINT_DEFAULT_MIDDLE),
     0,
     1);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Output Envelope Attack (s)",
     (LADSPA_HINT_BOUNDED_BELOW
      | LADSPA_HINT_DEFAULT_MAXIMUM),
     0,
     0.1f);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Output Envelope Decay (s)",
     (LADSPA_HINT_BOUNDED_BELOW
      | LADSPA_HINT_DEFAULT_MAXIMUM),
     0,
     0.1f);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output");
  registerNewPluginDescriptor(psDescriptor);
  
  psDescriptor = new CMT_Descriptor
    (1075,
     "expand_rms",
     LADSPA_PROPERTY_HARD_RT_CAPABLE,
     "Simple Expander (RMS Envelope Tracking)",
     CMT_MAKER("Richard W.E. Furse"),
     CMT_COPYRIGHT("2000-2002", "Richard W.E. Furse"),
     NULL,
     CMT_Instantiate<CompressorExpander>,
     activateCompressorExpander,
     runExpander_RMS,
     NULL,
     NULL,
     NULL);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Threshold",
     (LADSPA_HINT_BOUNDED_BELOW 
      | LADSPA_HINT_LOGARITHMIC
      | LADSPA_HINT_DEFAULT_1),
     0,
     0);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Expansion Ratio",
     (LADSPA_HINT_BOUNDED_ABOVE
      | LADSPA_HINT_DEFAULT_MIDDLE),
     0,
     1);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Output Envelope Attack (s)",
     (LADSPA_HINT_BOUNDED_BELOW
      | LADSPA_HINT_DEFAULT_MAXIMUM),
     0,
     0.1f);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Output Envelope Decay (s)",
     (LADSPA_HINT_BOUNDED_BELOW
      | LADSPA_HINT_DEFAULT_MAXIMUM),
     0,
     0.1f);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output");
  registerNewPluginDescriptor(psDescriptor);
  
  psDescriptor = new CMT_Descriptor
    (1076,
     "limit_peak",
     LADSPA_PROPERTY_HARD_RT_CAPABLE,
     "Simple Limiter (Peak Envelope Tracking)",
     CMT_MAKER("Richard W.E. Furse"),
     CMT_COPYRIGHT("2000-2002", "Richard W.E. Furse"),
     NULL,
     CMT_Instantiate<Limiter>,
     activateLimiter,
     runLimiter_Peak,
     NULL,
     NULL,
     NULL);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Threshold",
     (LADSPA_HINT_BOUNDED_BELOW 
      | LADSPA_HINT_LOGARITHMIC
      | LADSPA_HINT_DEFAULT_1),
     0,
     0);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Output Envelope Attack (s)",
     (LADSPA_HINT_BOUNDED_BELOW
      | LADSPA_HINT_DEFAULT_MAXIMUM),
     0,
     0.1f);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Output Envelope Decay (s)",
     (LADSPA_HINT_BOUNDED_BELOW
      | LADSPA_HINT_DEFAULT_MAXIMUM),
     0,
     0.1f);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output");
  registerNewPluginDescriptor(psDescriptor);
  
  psDescriptor = new CMT_Descriptor
    (1077,
     "limit_rms",
     LADSPA_PROPERTY_HARD_RT_CAPABLE,
     "Simple Limiter (RMS Envelope Tracking)",
     CMT_MAKER("Richard W.E. Furse"),
     CMT_COPYRIGHT("2000-2002", "Richard W.E. Furse"),
     NULL,
     CMT_Instantiate<Limiter>,
     activateLimiter,
     runLimiter_RMS,
     NULL,
     NULL,
     NULL);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Threshold",
     (LADSPA_HINT_BOUNDED_BELOW 
      | LADSPA_HINT_LOGARITHMIC
      | LADSPA_HINT_DEFAULT_1),
     0,
     0);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Output Envelope Attack (s)",
     (LADSPA_HINT_BOUNDED_BELOW
      | LADSPA_HINT_DEFAULT_MAXIMUM),
     0,
     0.1f);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Output Envelope Decay (s)",
     (LADSPA_HINT_BOUNDED_BELOW
      | LADSPA_HINT_DEFAULT_MAXIMUM),
     0,
     0.1f);
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
