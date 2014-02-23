/* sine.cpp

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

/* Sine table size is given by (1 << SINE_TABLE_BITS). */
#define SINE_TABLE_BITS 14
#define SINE_TABLE_SHIFT (8 * sizeof(unsigned long) - SINE_TABLE_BITS)

/*****************************************************************************/

LADSPA_Data * g_pfSineTable = NULL;
LADSPA_Data g_fPhaseStepBase = 0;

/*****************************************************************************/

void
initialise_sine_wavetable() {
  if (g_pfSineTable == NULL) {
    long lTableSize = (1 << SINE_TABLE_BITS);
    double dShift = (double(M_PI) * 2) / lTableSize;
    g_pfSineTable = new float[lTableSize];
    if (g_pfSineTable != NULL)
      for (long lIndex = 0; lIndex < lTableSize; lIndex++)
	g_pfSineTable[lIndex] = LADSPA_Data(sin(dShift * lIndex));
  }
  if (g_fPhaseStepBase == 0) {
    g_fPhaseStepBase = (LADSPA_Data)pow(2, sizeof(unsigned long) * 8);
  }
}

/*****************************************************************************/

#define OSC_FREQUENCY 0
#define OSC_AMPLITUDE 1
#define OSC_OUTPUT    2

/* This class provides sine wavetable oscillator
   plugins. Band-limiting to avoid aliasing is trivial because of the
   waveform in use. Four versions of the oscillator are provided,
   allowing the amplitude and frequency inputs of the oscillator to be
   audio signals rather than controls (for use in AM and FM
   synthesis). */
class SineOscillator : public CMT_PluginInstance{
private:

  /* Oscillator state: */

  unsigned long     m_lPhase;
  unsigned long     m_lPhaseStep;
  LADSPA_Data       m_fCachedFrequency;
  const LADSPA_Data m_fLimitFrequency;
  const LADSPA_Data m_fPhaseStepScalar;

  void setPhaseStepFromFrequency(const LADSPA_Data fFrequency) {
    if (fFrequency != m_fCachedFrequency) {
      if (fFrequency >= 0 && fFrequency < m_fLimitFrequency) 
	m_lPhaseStep = (unsigned long)(m_fPhaseStepScalar * fFrequency);
      else 
	m_lPhaseStep = 0;
      m_fCachedFrequency = fFrequency;
    }
  }

public:

  SineOscillator(const LADSPA_Descriptor *,
		 unsigned long lSampleRate) 
    : CMT_PluginInstance(3),
      m_lPhaseStep(0), 
      m_fCachedFrequency(0),
      m_fLimitFrequency(LADSPA_Data(lSampleRate * 0.5)),
      m_fPhaseStepScalar(LADSPA_Data(g_fPhaseStepBase / lSampleRate)) {
  }

  friend void activateSineOscillator(void * pvHandle);
  friend void runSineOscillator_FreqAudio_AmpAudio(LADSPA_Handle Instance,
						   unsigned long SampleCount);
  friend void runSineOscillator_FreqAudio_AmpCtrl(LADSPA_Handle Instance,
						  unsigned long SampleCount);
  friend void runSineOscillator_FreqCtrl_AmpAudio(LADSPA_Handle Instance,
						  unsigned long SampleCount);
  friend void runSineOscillator_FreqCtrl_AmpCtrl(LADSPA_Handle Instance,
						 unsigned long SampleCount);

};

/*****************************************************************************/

void 
activateSineOscillator(void * pvHandle) {
  ((SineOscillator *)pvHandle)->m_lPhase = 0;
}

/*****************************************************************************/

void 
runSineOscillator_FreqAudio_AmpAudio(LADSPA_Handle Instance,
				     unsigned long SampleCount) {
  SineOscillator * poSineOscillator = (SineOscillator *)Instance;
  LADSPA_Data * pfFrequency = poSineOscillator->m_ppfPorts[OSC_FREQUENCY];
  LADSPA_Data * pfAmplitude = poSineOscillator->m_ppfPorts[OSC_AMPLITUDE];
  LADSPA_Data * pfOutput = poSineOscillator->m_ppfPorts[OSC_OUTPUT];
  for (unsigned long lIndex = 0; lIndex < SampleCount; lIndex++) {
    /* Extract frequency at this point to guarantee inplace
       support. */
    LADSPA_Data fFrequency = *(pfFrequency++);
    *(pfOutput++)
      = (g_pfSineTable[poSineOscillator->m_lPhase >> SINE_TABLE_SHIFT]
	 * *(pfAmplitude++));
    poSineOscillator->setPhaseStepFromFrequency(fFrequency);
    poSineOscillator->m_lPhase 
      += poSineOscillator->m_lPhaseStep;
  }
}

/*****************************************************************************/

void 
runSineOscillator_FreqAudio_AmpCtrl(LADSPA_Handle Instance,
				    unsigned long SampleCount) {
  SineOscillator * poSineOscillator = (SineOscillator *)Instance;
  LADSPA_Data fAmplitude = *(poSineOscillator->m_ppfPorts[OSC_AMPLITUDE]);
  LADSPA_Data * pfFrequency = poSineOscillator->m_ppfPorts[OSC_FREQUENCY];
  LADSPA_Data * pfOutput = poSineOscillator->m_ppfPorts[OSC_OUTPUT];
  for (unsigned long lIndex = 0; lIndex < SampleCount; lIndex++) {
    /* Extract frequency at this point to guarantee inplace
       support. */
    LADSPA_Data fFrequency = *(pfFrequency++);
    *(pfOutput++)
      = (g_pfSineTable[poSineOscillator->m_lPhase >> SINE_TABLE_SHIFT]
	 * fAmplitude);
    poSineOscillator->setPhaseStepFromFrequency(fFrequency);
    poSineOscillator->m_lPhase 
      += poSineOscillator->m_lPhaseStep;
  }
}

/*****************************************************************************/

void
runSineOscillator_FreqCtrl_AmpAudio(LADSPA_Handle Instance,
				    unsigned long SampleCount) {
  SineOscillator * poSineOscillator = (SineOscillator *)Instance;
  poSineOscillator->setPhaseStepFromFrequency
    (*(poSineOscillator->m_ppfPorts[OSC_FREQUENCY]));
  LADSPA_Data * pfAmplitude = poSineOscillator->m_ppfPorts[OSC_AMPLITUDE];
  LADSPA_Data * pfOutput = poSineOscillator->m_ppfPorts[OSC_OUTPUT];
  for (unsigned long lIndex = 0; lIndex < SampleCount; lIndex++) {
    *(pfOutput++)
      = (g_pfSineTable[poSineOscillator->m_lPhase >> SINE_TABLE_SHIFT]
	 * *(pfAmplitude++));
    poSineOscillator->m_lPhase 
      += poSineOscillator->m_lPhaseStep;
  }
}

/*****************************************************************************/

void 
runSineOscillator_FreqCtrl_AmpCtrl(LADSPA_Handle Instance,
				   unsigned long SampleCount) {
  SineOscillator * poSineOscillator = (SineOscillator *)Instance;
  LADSPA_Data fAmplitude = *(poSineOscillator->m_ppfPorts[OSC_AMPLITUDE]);
  poSineOscillator->setPhaseStepFromFrequency
    (*(poSineOscillator->m_ppfPorts[OSC_FREQUENCY]));
  LADSPA_Data * pfOutput = poSineOscillator->m_ppfPorts[OSC_OUTPUT];
  for (unsigned long lIndex = 0; lIndex < SampleCount; lIndex++) {
    *(pfOutput++)
      = (g_pfSineTable[poSineOscillator->m_lPhase >> SINE_TABLE_SHIFT]
	 * fAmplitude);
    poSineOscillator->m_lPhase 
      += poSineOscillator->m_lPhaseStep;
  }
}

/*****************************************************************************/

void
initialise_sine() {

  initialise_sine_wavetable();

  const char * apcLabels[] = {
    "sine_faaa",
    "sine_faac",
    "sine_fcaa",
    "sine_fcac" 
  };
  const char * apcNames[] = { 
    "Sine Oscillator (Freq:audio, Amp:audio)",
    "Sine Oscillator (Freq:audio, Amp:control)",
    "Sine Oscillator (Freq:control, Amp:audio)",
    "Sine Oscillator (Freq:control, Amp:control)" 
  };
  LADSPA_Run_Function afRunFunction[] = {
    runSineOscillator_FreqAudio_AmpAudio,
    runSineOscillator_FreqAudio_AmpCtrl,
    runSineOscillator_FreqCtrl_AmpAudio,
    runSineOscillator_FreqCtrl_AmpCtrl 
  };
  LADSPA_PortDescriptor piFrequencyPortProperties[] = {
    LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,	
    LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,	
    LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
    LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL
  };
  LADSPA_PortDescriptor piAmplitudePortProperties[] = {
    LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,	
    LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
    LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,	
    LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL
  };
  
  for (long lPluginIndex = 0; lPluginIndex < 4; lPluginIndex++) {
    
    CMT_Descriptor * psDescriptor;
    
    psDescriptor = new CMT_Descriptor
      (1063 + lPluginIndex,
       apcLabels[lPluginIndex],
       LADSPA_PROPERTY_HARD_RT_CAPABLE,
       apcNames[lPluginIndex],
       CMT_MAKER("Richard W.E. Furse"),
       CMT_COPYRIGHT("2000-2002", "Richard W.E. Furse"),
       NULL,
       CMT_Instantiate<SineOscillator>,
       activateSineOscillator,
       afRunFunction[lPluginIndex],
       NULL,
       NULL,
       NULL);
    
    psDescriptor->addPort
      (piFrequencyPortProperties[lPluginIndex],
       "Frequency",
       (LADSPA_HINT_BOUNDED_BELOW
	| LADSPA_HINT_BOUNDED_ABOVE
	| LADSPA_HINT_SAMPLE_RATE 
	| LADSPA_HINT_LOGARITHMIC
	| LADSPA_HINT_DEFAULT_440),
       0, 
       0.5);
    psDescriptor->addPort
      (piAmplitudePortProperties[lPluginIndex],
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
}

/*****************************************************************************/

void 
finalise_sine() {
  delete [] g_pfSineTable;
}

/*****************************************************************************/

/* EOF */
