/* descriptor.cpp

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

/* This module contains code providing and supporting the
   CMT_Descriptor() function that provides hosts with initial access
   to LADSPA plugins. ALL PLUGINS MUST BE REGISTERED IN THIS FILE (see
   below). */

/*****************************************************************************/

/* Module Initialisation:
   ---------------------- */

void initialise_am();
void initialise_ambisonic();
void initialise_amp();
void initialise_analogue();
void initialise_canyondelay();
void initialise_delay();
void initialise_dynamic();
void initialise_filter();
void initialise_freeverb3();
void initialise_grain();
void initialise_lofi();
void initialise_mixer();
void initialise_noise();
void initialise_null();
void initialise_organ();
void initialise_peak();
void initialise_phasemod();
void initialise_sine();
void initialise_syndrum();
void initialise_vcf303();
void initialise_wshape_sine();
namespace hardgate               { void initialise(); }
namespace disintegrator          { void initialise(); }
namespace pink                   { void initialise(); }
namespace pink_full              { void initialise(); }
namespace pink_sh                { void initialise(); }
namespace sledgehammer           { void initialise(); }
namespace logistic               { void initialise(); }

/** This function should initialise all modules in the library. This
    will lead to all plugin descriptors being registered. If you write
    a new plugin you should initialise it here. If the module has
    structures it wishes to remove also then these should be included
    in finalise_modules(). */
void 
initialise_modules() {
  initialise_am();
  initialise_ambisonic();
  initialise_amp();
  initialise_analogue();
  initialise_canyondelay();
  initialise_delay();
  initialise_dynamic();
  initialise_filter();
  initialise_freeverb3();
  initialise_grain();
  initialise_lofi();
  initialise_mixer();
  initialise_noise();
  initialise_null();
  initialise_organ();
  initialise_peak();
  initialise_phasemod();
  initialise_sine();
  initialise_syndrum();
  initialise_vcf303();
  initialise_wshape_sine();
  hardgate::initialise();
  disintegrator::initialise();
  pink::initialise();
  pink_full::initialise();
  pink_sh::initialise();
  sledgehammer::initialise();
  logistic::initialise();
}

/*****************************************************************************/

/* Module Finalisation:
   -------------------- */

void finalise_sine();

/** Finalise any structures allocated by the modules. This does not
    include descriptors passed to registerNewPluginDescriptor(). */
void
finalise_modules() {
  finalise_sine();
}

/*****************************************************************************/

/* EOF */
