/* init.cpp

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

#include <ladspa.h>
#include <stdlib.h>
#include <string.h>

/*****************************************************************************/

#include "cmt.h"

/*****************************************************************************/

void initialise_modules();
void finalise_modules();

/*****************************************************************************/

int 
pluginNameComparator(const void * pvDescriptor1, const void * pvDescriptor2) {

  const CMT_Descriptor * psDescriptor1
    = *(const CMT_Descriptor **)pvDescriptor1;
  const CMT_Descriptor * psDescriptor2
    = *(const CMT_Descriptor **)pvDescriptor2;

  int iResult = strcmp(psDescriptor1->Name, psDescriptor2->Name);
  if (iResult < 0)
    return -1;
  else if (iResult > 0)
    return 1;
  else
    return 0;
}

/*****************************************************************************/

CMT_Descriptor ** g_ppsRegisteredDescriptors = NULL;
unsigned long g_lPluginCapacity = 0;
unsigned long g_lPluginCount = 0;

/*****************************************************************************/

#define CAPACITY_STEP 20

void 
registerNewPluginDescriptor(CMT_Descriptor * psDescriptor) {
  if (g_lPluginCapacity == g_lPluginCount) {
    /* Full. Enlarge capacity. */
    CMT_Descriptor ** ppsOldDescriptors
      = g_ppsRegisteredDescriptors;
    g_ppsRegisteredDescriptors
      = new CMT_Descriptor_ptr[g_lPluginCapacity + CAPACITY_STEP];
    if (g_lPluginCapacity > 0) {
      memcpy(g_ppsRegisteredDescriptors, 
	     ppsOldDescriptors,
	     g_lPluginCapacity * sizeof(CMT_Descriptor_ptr));
      delete [] ppsOldDescriptors;
    }
    g_lPluginCapacity += CAPACITY_STEP;
  }
  g_ppsRegisteredDescriptors[g_lPluginCount++] = psDescriptor;
}

/*****************************************************************************/

/** A global object of this class is used to perform initialisation
    and shutdown services for the entire library. The constructor is
    run when the library is loaded and the destructor when it is
    unloaded. */
class StartupShutdownHandler {
public:

  StartupShutdownHandler() {
    initialise_modules();
    qsort(g_ppsRegisteredDescriptors, 
	  g_lPluginCount,
	  sizeof(CMT_Descriptor_ptr),
	  pluginNameComparator);
  }

  ~StartupShutdownHandler() {
    if (g_ppsRegisteredDescriptors != NULL) {
      for (unsigned long lIndex = 0; lIndex < g_lPluginCount; lIndex++)
	delete g_ppsRegisteredDescriptors[lIndex];
      delete [] g_ppsRegisteredDescriptors;
    }
    finalise_modules();
  }

} g_oStartupShutdownHandler;
  
/*****************************************************************************/

const LADSPA_Descriptor * 
ladspa_descriptor(unsigned long Index) {
  if (Index < g_lPluginCount)
    return g_ppsRegisteredDescriptors[Index];
  else
    return NULL;
}

/*****************************************************************************/

/* EOF */
