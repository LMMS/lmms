/* freeverb.cpp

   Computer Music Toolkit - a library of LADSPA plugins. Copyright (C)
   2000-2002 Richard W.E. Furse. Freeverb is also Copyright (C) 2000
   Jezar. Richard may be contacted at richard@muse.demon.co.uk. [V1
   Ported to LADSPA 15/7/2000 Richard W.E. Furse, V3 ported to CMT
   4/11/2000.]

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

#include "../cmt.h"
#include "Components/revmodel.h"

/*****************************************************************************/

enum {

  FV_Input1 = 0,
  FV_Input2,
  FV_Output1,
  FV_Output2,
  FV_Mode,
  FV_RoomSize,
  FV_Damping,
  FV_Wet,
  FV_Dry,
  FV_Width,

  FV_NumPorts

};

/*****************************************************************************/

/** This plugin wraps Jezar's Freeverb free reverberation module
    (version 3). */
class Freeverb3 : public CMT_PluginInstance, public revmodel {
public:

  Freeverb3(const LADSPA_Descriptor *, unsigned long lSampleRate)
    : CMT_PluginInstance(FV_NumPorts) {
    /* Richard's note 17/5/2000. Hmm - not sure I like the fact that
       lSampleRate isn't actually used in this function! */
  }
  friend void activateFreeverb3(LADSPA_Handle Instance);
  friend void runFreeverb3(LADSPA_Handle Instance, 
			   unsigned long SampleCount);

};

/*****************************************************************************/

void 
activateFreeverb3(LADSPA_Handle Instance) {
  Freeverb3 * poFreeverb = (Freeverb3 *)Instance;
  poFreeverb->mute();
}

/*****************************************************************************/

void
runFreeverb3(LADSPA_Handle Instance,
	     const unsigned long SampleCount) {

  Freeverb3 * poFreeverb = ((Freeverb3 *)Instance);

  /* Handle control ports. Note that this isn't especially efficient
     because of the way the update() code works in revmodel.cpp, but
     at least this approach allows Freeverb to work with almost no
     code changes. */

  if (*(poFreeverb->m_ppfPorts[FV_Mode]) > 0)
    poFreeverb->setmode(1);
  else
    poFreeverb->setmode(0);
  poFreeverb->setdamp(*(poFreeverb->m_ppfPorts[FV_Damping]));
  poFreeverb->setwet(*(poFreeverb->m_ppfPorts[FV_Wet]));
  poFreeverb->setdry(*(poFreeverb->m_ppfPorts[FV_Dry]));
  poFreeverb->setroomsize(*(poFreeverb->m_ppfPorts[FV_RoomSize]));
  poFreeverb->setwidth(*(poFreeverb->m_ppfPorts[FV_Width]));

  /* Connect to audio ports and run. */

  poFreeverb->processreplace(poFreeverb->m_ppfPorts[FV_Input1],
			     poFreeverb->m_ppfPorts[FV_Input2],
			     poFreeverb->m_ppfPorts[FV_Output1],
			     poFreeverb->m_ppfPorts[FV_Output2],
			     SampleCount,
			     1);
}

/*****************************************************************************/

void
initialise_freeverb3() {

  CMT_Descriptor * psDescriptor;

  psDescriptor = new CMT_Descriptor
    (1123,
     "freeverb3",
     LADSPA_PROPERTY_HARD_RT_CAPABLE,
     "Freeverb (Version 3)",
     CMT_MAKER("Jezar at Dreampoint, ported by Richard W.E. Furse"),
     CMT_COPYRIGHT("2000", "Jezar at Dreampoint"),
     NULL,
     CMT_Instantiate<Freeverb3>,
     activateFreeverb3, 
     runFreeverb3,
     NULL,
     NULL,
     NULL);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO, 
     "Input (Left)");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO, 
     "Input (Right)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO, 
     "Output (Left)");
  psDescriptor->addPort
    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO, 
     "Output (Right)");
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Freeze Mode",
     (LADSPA_HINT_TOGGLED 
      | LADSPA_HINT_DEFAULT_0),
     0,
     0);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Room Size", 
     (LADSPA_HINT_BOUNDED_BELOW
      | LADSPA_HINT_BOUNDED_ABOVE
      | LADSPA_HINT_DEFAULT_MIDDLE), 
     0,
     1);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Damping",
     (LADSPA_HINT_LOGARITHMIC 
      | LADSPA_HINT_BOUNDED_BELOW 
      | LADSPA_HINT_BOUNDED_ABOVE
      | LADSPA_HINT_DEFAULT_MIDDLE), 
     0,
     1);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Wet Level",
     (LADSPA_HINT_LOGARITHMIC 
      | LADSPA_HINT_BOUNDED_BELOW 
      | LADSPA_HINT_BOUNDED_ABOVE
      | LADSPA_HINT_DEFAULT_MIDDLE), 
     0,
     1);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Dry Level", 
     (LADSPA_HINT_LOGARITHMIC 
      | LADSPA_HINT_BOUNDED_BELOW 
      | LADSPA_HINT_BOUNDED_ABOVE
      | LADSPA_HINT_DEFAULT_MAXIMUM), 
     0,
     1);
  psDescriptor->addPort
    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
     "Width", 
     (LADSPA_HINT_BOUNDED_BELOW 
      | LADSPA_HINT_BOUNDED_ABOVE
      | LADSPA_HINT_DEFAULT_MIDDLE), 
     0,
     1);

  registerNewPluginDescriptor(psDescriptor);
}

/*****************************************************************************/

/* EOF */
