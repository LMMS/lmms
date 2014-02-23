/* null.cpp

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

#define NULL_PORT 0

/** This plugin can be used to take care of unwanted connections in a
    host's plugin network by generating zero data and audio or
    accepting (but ignoring) data and audio. */
class NullPlugin : public CMT_PluginInstance {
public:

  NullPlugin(const LADSPA_Descriptor *,
	     unsigned long)
    : CMT_PluginInstance(1) {
  }

  friend void runNull_Nop(LADSPA_Handle Instance,
			  unsigned long SampleCount);
  friend void runNull_OutputAudio(LADSPA_Handle Instance,
				  unsigned long SampleCount);
  friend void runNull_OutputControl(LADSPA_Handle Instance,
				    unsigned long SampleCount);

};

/*****************************************************************************/

#define IDENTITY_INPUT 0
#define IDENTITY_OUTPUT 1

/* This plugin passes its input to its output. There are audio and
   control varieties. */
class IdentityPlugin : public CMT_PluginInstance {
public:

  IdentityPlugin(const LADSPA_Descriptor *,
		 unsigned long)
    : CMT_PluginInstance(2) {
  }

  friend void runIdentity_Audio(LADSPA_Handle Instance,
				unsigned long SampleCount);
  friend void runIdentity_Control(LADSPA_Handle Instance,
				  unsigned long SampleCount);

};

/*****************************************************************************/

void 
runNull_Nop(LADSPA_Handle Instance,
	    unsigned long SampleCount) {
  /* Nothing to do. */
}

/*****************************************************************************/

void 
runNull_OutputAudio(LADSPA_Handle Instance,
		    unsigned long SampleCount) {
  NullPlugin * poPlugin = (NullPlugin *)Instance;
  memset(poPlugin->m_ppfPorts[NULL_PORT], 
	 0, 
	 sizeof(LADSPA_Data) * SampleCount);
}

/*****************************************************************************/

void 
runNull_OutputControl(LADSPA_Handle Instance,
		      unsigned long) {
  NullPlugin * poPlugin = (NullPlugin *)Instance;
  *(poPlugin->m_ppfPorts[NULL_PORT]) = 0;
}

/*****************************************************************************/

void 
runIdentity_Audio(LADSPA_Handle Instance,
		  unsigned long SampleCount) {
  IdentityPlugin * poPlugin = (IdentityPlugin *)Instance;
  if (poPlugin->m_ppfPorts[IDENTITY_OUTPUT] 
      != poPlugin->m_ppfPorts[IDENTITY_INPUT])
    memcpy(poPlugin->m_ppfPorts[IDENTITY_OUTPUT],
	   poPlugin->m_ppfPorts[IDENTITY_INPUT],
	   sizeof(LADSPA_Data) * SampleCount);
}

/*****************************************************************************/

void 
runIdentity_Control(LADSPA_Handle Instance,
		    unsigned long) {
  IdentityPlugin * poPlugin = (IdentityPlugin *)Instance;
  *(poPlugin->m_ppfPorts[IDENTITY_OUTPUT])
    = *(poPlugin->m_ppfPorts[IDENTITY_INPUT]);
}

/*****************************************************************************/

void
initialise_null() {
  
  CMT_Descriptor * psDescriptor;

  psDescriptor = new CMT_Descriptor
    (1083,
     "null_ci",
     LADSPA_PROPERTY_HARD_RT_CAPABLE,
     "Null (Control Input)",
     CMT_MAKER("Richard W.E. Furse"),
     CMT_COPYRIGHT("2000", "Richard W.E. Furse"),
     NULL,
     CMT_Instantiate<NullPlugin>,
     NULL,
     runNull_Nop,
     NULL,
     NULL,
     NULL);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Input");
  registerNewPluginDescriptor(psDescriptor);
  
  psDescriptor = new CMT_Descriptor
    (1084,
     "null_ai",
     LADSPA_PROPERTY_HARD_RT_CAPABLE,
     "Null (Audio Input)",
     CMT_MAKER("Richard W.E. Furse"),
     CMT_COPYRIGHT("2000", "Richard W.E. Furse"),
     NULL,
     CMT_Instantiate<NullPlugin>,
     NULL,
     runNull_Nop,
     NULL,
     NULL,
     NULL);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input");
  registerNewPluginDescriptor(psDescriptor);
  
  psDescriptor = new CMT_Descriptor
    (1085,
     "null_co",
     LADSPA_PROPERTY_HARD_RT_CAPABLE,
     "Null (Control Output)",
     CMT_MAKER("Richard W.E. Furse"),
     CMT_COPYRIGHT("2000", "Richard W.E. Furse"),
     NULL,
     CMT_Instantiate<NullPlugin>,
     NULL,
     runNull_OutputControl,
     NULL,
     NULL,
     NULL);
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL,
     "Output");
  registerNewPluginDescriptor(psDescriptor);
  
  psDescriptor = new CMT_Descriptor
    (1086,
     "null_ao",
     LADSPA_PROPERTY_HARD_RT_CAPABLE,
     "Null (Audio Output)",
     CMT_MAKER("Richard W.E. Furse"),
     CMT_COPYRIGHT("2000", "Richard W.E. Furse"),
     NULL,
     CMT_Instantiate<NullPlugin>,
     NULL,
     runNull_OutputAudio,
     NULL,
     NULL,
     NULL);
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output");
  registerNewPluginDescriptor(psDescriptor);

  psDescriptor = new CMT_Descriptor
    (1098,
     "identity_audio",
     LADSPA_PROPERTY_HARD_RT_CAPABLE,
     "Identity (Audio)",
     CMT_MAKER("Richard W.E. Furse"),
     CMT_COPYRIGHT("2000", "Richard W.E. Furse"),
     NULL,
     CMT_Instantiate<IdentityPlugin>,
     NULL,
     runIdentity_Audio,
     NULL,
     NULL,
     NULL);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
     "Input");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     "Output");
  registerNewPluginDescriptor(psDescriptor);

  psDescriptor = new CMT_Descriptor
    (1099,
     "identity_control",
     LADSPA_PROPERTY_HARD_RT_CAPABLE,
     "Identity (Control)",
     CMT_MAKER("Richard W.E. Furse"),
     CMT_COPYRIGHT("2000", "Richard W.E. Furse"),
     NULL,
     CMT_Instantiate<IdentityPlugin>,
     NULL,
     runIdentity_Control,
     NULL,
     NULL,
     NULL);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Input");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL,
     "Output");
  registerNewPluginDescriptor(psDescriptor);
}

/*****************************************************************************/

/* EOF */
