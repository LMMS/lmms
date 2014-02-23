/* cmt.cpp

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

#include <string.h>

/*****************************************************************************/

#include "cmt.h"

/*****************************************************************************/

inline char * 
localStrdup(const char * input) {
  char * output = new char[strlen(input) + 1];
  strcpy(output, input);
  return output;
}

/*****************************************************************************/

CMT_Descriptor::
~CMT_Descriptor() {
  if (Label)
    delete [] Label;
  if (Name)
    delete [] Name;
  if (Maker)
    delete [] Maker;
  if (Copyright)
    delete [] Copyright;
  if (ImplementationData)
    delete (CMT_ImplementationData *)ImplementationData;
  if (PortDescriptors)
    delete [] PortDescriptors;
  if (PortNames) {
    for (unsigned long lIndex = 0; lIndex < PortCount; lIndex++)
      if (PortNames[lIndex])
	delete [] PortNames[lIndex];
    delete [] PortNames;
  }
  if (PortRangeHints)
    delete [] PortRangeHints;
}

/*****************************************************************************/

void 
CMT_ConnectPort(LADSPA_Handle Instance,
		unsigned long Port,
		LADSPA_Data * DataLocation) {
  CMT_PluginInstance * poInstance = (CMT_PluginInstance *)Instance;
  poInstance->m_ppfPorts[Port] = DataLocation;
}

/*****************************************************************************/

void 
CMT_Cleanup(LADSPA_Handle Instance) {
  CMT_PluginInstance * poInstance = (CMT_PluginInstance *)Instance;
  delete poInstance;
}

/*****************************************************************************/

CMT_Descriptor::
CMT_Descriptor(unsigned long                       lUniqueID,
	       const char *                        pcLabel,
	       LADSPA_Properties                   iProperties,
	       const char *                        pcName,
	       const char *                        pcMaker,
	       const char *                        pcCopyright,
	       CMT_ImplementationData *            poImplementationData,
	       LADSPA_Instantiate_Function         fInstantiate,
	       LADSPA_Activate_Function            fActivate,
	       LADSPA_Run_Function                 fRun,
	       LADSPA_Run_Adding_Function          fRunAdding,
	       LADSPA_Set_Run_Adding_Gain_Function fSetRunAddingGain,
	       LADSPA_Deactivate_Function          fDeactivate) {

  UniqueID = lUniqueID;
  Label = localStrdup(pcLabel);
  Properties = iProperties;
  Name = localStrdup(pcName);
  Maker = localStrdup(pcMaker);
  Copyright = localStrdup(pcCopyright);
  PortCount = 0;
  ImplementationData = poImplementationData;

  instantiate = fInstantiate;
  connect_port = CMT_ConnectPort;
  activate = fActivate;
  run = fRun;
  run_adding = fRunAdding;
  set_run_adding_gain = fSetRunAddingGain;
  deactivate = fDeactivate;
  cleanup = CMT_Cleanup;

};

/*****************************************************************************/

typedef char * char_ptr;

void CMT_Descriptor::
addPort(LADSPA_PortDescriptor          iPortDescriptor,
	const char *                   pcPortName,
	LADSPA_PortRangeHintDescriptor iHintDescriptor,
	LADSPA_Data                    fLowerBound,
	LADSPA_Data                    fUpperBound) {

  unsigned long lOldPortCount = PortCount;
  unsigned long lNewPortCount = PortCount + 1;

  LADSPA_PortDescriptor * piOldPortDescriptors 
    = (LADSPA_PortDescriptor *)PortDescriptors;
  char ** ppcOldPortNames
    = (char **)PortNames;
  LADSPA_PortRangeHint * psOldPortRangeHints
    = (LADSPA_PortRangeHint *)PortRangeHints;

  LADSPA_PortDescriptor * piNewPortDescriptors 
    = new LADSPA_PortDescriptor[lNewPortCount];
  char ** ppcNewPortNames
    = new char_ptr[lNewPortCount];
  LADSPA_PortRangeHint * psNewPortRangeHints
    = new LADSPA_PortRangeHint[lNewPortCount];

  if (piNewPortDescriptors == NULL
      || ppcNewPortNames == NULL 
      || psNewPortRangeHints == NULL) {
    /* Memory allocation failure that we cannot handle. Best option is
       probably just to get out while the going is reasonably good. */
    return;
  }

  for (unsigned long lPortIndex = 0;
       lPortIndex < lOldPortCount; 
       lPortIndex++) {
    piNewPortDescriptors[lPortIndex] = piOldPortDescriptors[lPortIndex];
    ppcNewPortNames[lPortIndex] = ppcOldPortNames[lPortIndex];
    psNewPortRangeHints[lPortIndex] = psOldPortRangeHints[lPortIndex];
  }
  if (lOldPortCount > 0) {
    delete [] piOldPortDescriptors;
    delete [] ppcOldPortNames;
    delete [] psOldPortRangeHints;
  }

  piNewPortDescriptors[lOldPortCount] = iPortDescriptor;
  ppcNewPortNames[lOldPortCount] = localStrdup(pcPortName);
  psNewPortRangeHints[lOldPortCount].HintDescriptor = iHintDescriptor;
  psNewPortRangeHints[lOldPortCount].LowerBound = fLowerBound;
  psNewPortRangeHints[lOldPortCount].UpperBound = fUpperBound;

  PortDescriptors = piNewPortDescriptors;
  PortNames = ppcNewPortNames;
  PortRangeHints = psNewPortRangeHints;

  PortCount++;
}

/*****************************************************************************/

/* EOF */
