/* ladspa_types.h

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

#ifndef CMT_LADSPA_TYPES_INCLUDED
#define CMT_LADSPA_TYPES_INCLUDED

/*****************************************************************************/

#include <ladspa.h>

/* Compatibility hack for version 1.0. */
#ifndef LADSPA_VERSION_MAJOR
#define LADSPA_HINT_DEFAULT_MINIMUM 0x40
#define LADSPA_HINT_DEFAULT_LOW     0x80
#define LADSPA_HINT_DEFAULT_MIDDLE  0xC0
#define LADSPA_HINT_DEFAULT_HIGH    0x100
#define LADSPA_HINT_DEFAULT_MAXIMUM 0x140
#define LADSPA_HINT_DEFAULT_0       0x200
#define LADSPA_HINT_DEFAULT_1       0x240
#define LADSPA_HINT_DEFAULT_100     0x280
#define LADSPA_HINT_DEFAULT_440     0x2C0
#endif

/*****************************************************************************/

typedef LADSPA_Handle (*LADSPA_Instantiate_Function)
  (const struct _LADSPA_Descriptor * Descriptor,
   unsigned long                     SampleRate);

typedef void (*LADSPA_Connect_Port_Function)
  (LADSPA_Handle Instance,
   unsigned long Port,
   LADSPA_Data * DataLocation);

typedef void (*LADSPA_Activate_Function)    
  (LADSPA_Handle Instance);

typedef void (*LADSPA_Run_Function)         
  (LADSPA_Handle Instance,
   unsigned long SampleCount);

typedef void (*LADSPA_Run_Adding_Function)  
  (LADSPA_Handle Instance,
   unsigned long SampleCount);

typedef void (*LADSPA_Set_Run_Adding_Gain_Function)
  (LADSPA_Handle Instance,
   LADSPA_Data   Gain);

typedef void (*LADSPA_Deactivate_Function)  
  (LADSPA_Handle Instance);

typedef void (*LADSPA_Cleanup_Function)     
  (LADSPA_Handle Instance);

typedef LADSPA_Data * LADSPA_Data_ptr;

/*****************************************************************************/

#endif

/* EOF */
